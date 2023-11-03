/*
 * Copyright (c) 2022 - 2023 trinity-tech.io
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#endif

#include "dht.h"
#include "error_code.h"
#include "constants.h"
#include "rpcserver.h"
#include "rpcstatistics.h"
#include "carrier/node.h"
#include "utils/time.h"
#include "utils/random_generator.h"
#include "exceptions/dht_error.h"
#include "exceptions/exceptions.h"
#include "messages/message.h"
#include "messages/error_message.h"

namespace carrier {

static const uint32_t PERIODIC_INTERVAL = 100;        // 100 milliseconds
static const size_t MAX_DATA_PACKET_SIZE = 0x7FFF;      // 32767

struct SendRequest {
    uv_udp_send_t request;
    uv_buf_t buf;
    Sp<Message> msg {};
    RPCServer* rpcServer;

    SendRequest(RPCServer* svr, size_t size, Sp<Message>& _msg) {
        request.data = svr;
        rpcServer = svr;
        msg = _msg;
        buf.base = new char[size];
        buf.len = buf.base ? size : 0;
    }

    ~SendRequest() {
        delete[] buf.base;
    };
};

RPCServer::RPCServer(Node& _node, const Sp<DHT> _dht4, const Sp<DHT> _dht6): node(_node),
    dht4(_dht4 ? std::optional<std::reference_wrapper<DHT>>(*_dht4) : std::nullopt),
    dht6(_dht6 ? std::optional<std::reference_wrapper<DHT>>(*_dht6) : std::nullopt) {

    nextTxid = RandomGenerator<int>(1,32768)();

    log = Logger::get("RpcServer");

    if (_dht4 != nullptr)
        bind4 = _dht4->getOrigin();
    if (_dht6 != nullptr)
        bind6 = _dht6->getOrigin();

    readBuffer.resize(MAX_DATA_PACKET_SIZE);
}

RPCServer::~RPCServer() {
    stop();
}

void RPCServer::readStart(uv_udp_t* handle, const SocketAddress& bind) {
    auto rc = uv_udp_init(&loop, handle);
    if (rc < 0) {
        failHandler(rc, "initialize the udp");
    }

    handle->data = this;
    rc = uv_udp_bind(handle, (const struct sockaddr*) bind.addr(), 0);
    if (rc < 0) {
        failHandler(rc, "bind the udp");
    }

    rc = uv_udp_recv_start(handle, [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        RPCServer* svr = (RPCServer*)handle->data;
        buf->base = (char *)svr->readBuffer.data(); //need to use two buf? one for upd4 and udp5
        buf->len  = svr->readBuffer.size();
    }, [](uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
        RPCServer* svr = (RPCServer*)handle->data;
        if (nread > 0) {
            svr->handlePacket((uint8_t*)buf->base, nread, addr);
        }
    });
    if (rc < 0) {
        failHandler(rc, "receive the udp start");
    }
}

void RPCServer::sendData(Sp<Message>& msg) {

    const auto& remoteAddr = msg->getRemoteAddress();
    uv_udp_t* handle;
    switch (remoteAddr.family()) {
        case AF_INET:
            handle = &udp4Handle;
            break;
        case AF_INET6:
            handle = &udp6Handle;
            break;
        default:
            throw std::runtime_error("Unsupported address family!");
    }

    auto buffer = msg->serialize();
    auto encrypted = node.encrypt(msg->getRemoteId(), {buffer});
    size_t size = ID_BYTES + encrypted.size();
    SendRequest* request = new SendRequest{this, size, msg};
    uint8_t* ptr = (uint8_t*)request->buf.base;
    std::memcpy(ptr, msg->getId().data(), ID_BYTES);
    std::memcpy(ptr + ID_BYTES, encrypted.data(), encrypted.size());

    log->debug("Send {} to server {}.", msg->toString(), remoteAddr.toString());
    auto rc = uv_udp_send((uv_udp_send_t*)request, handle, &request->buf, 1, remoteAddr.addr(), [](uv_udp_send_t *req, int status) {
        SendRequest* request = (SendRequest*)req;
        RPCServer* svr = request->rpcServer;
        Sp<Message>& msg = request->msg;

        if (status < 0) {
            svr->messageQueue.push(msg);
            svr->log->error("Send {} to {} failed({}): {}",
                    msg->toString(), msg->getRemoteAddress().toString(), status, uv_strerror(status));
        }
        else {
            svr->stats.onSentBytes(request->buf.len);
            svr->stats.onSentMessage(*msg);

            svr->log->debug("Sent {}/{} to {}: [{}] {}", msg->getMethodString(), msg->getTypeString(),
                    msg->getRemoteAddress().toString(), request->buf.len, msg->toString());
        }

        delete request;
    });
    if (rc < 0) {
        messageQueue.push(msg);
        log->error("Send {} to {} failed({}): {}",
                msg->toString(), msg->getRemoteAddress().toString(), rc, uv_strerror(rc));
        delete request;
    }
}

void RPCServer::onStop() noexcept
{
    running = false;
    log->info("RPCServer is on-stopping...");

    if (udp6Started) {
        udp6Started = false;
        uv_udp_recv_stop(&udp6Handle);
        uv_close((uv_handle_t*)&udp6Handle, NULL);
    }

    if (udp4Started) {
        udp4Started = false;
        uv_udp_recv_stop(&udp4Handle);
        uv_close((uv_handle_t*)&udp4Handle, NULL);
    }

    if (timerStarted) {
        timerStarted = false;
        uv_timer_stop(&timerHandle);
        uv_close((uv_handle_t*)&timerHandle, nullptr);
    }

    if (checkStarted) {
        checkStarted = false;
        uv_check_stop(&checkHandle);
        uv_close((uv_handle_t*)&checkHandle, nullptr);
    }

    if (asyncInited) {
        asyncInited = false;
        uv_close((uv_handle_t*)&stopHandle, nullptr);
    }
}

void RPCServer::failHandler(int rc, std::string errType) {
    log->error("RPCServer failed to {}({}): {}", errType, rc, uv_strerror(rc));
    onStop();
    if (loopInited) {
        loopInited = false;
        uv_loop_close(&loop);
    }
    throw networking_error(uv_strerror(rc));
}

//--------------------------------------------------------------

void RPCServer::start() {
    if (state != State::INITIAL)
        return;

    state = State::RUNNING;
    startTime = currentTimeMillis();

    int rc = uv_loop_init(&loop);
    if (rc < 0) {
        failHandler(rc, "initialize the event loop");
    }
    loopInited = true;

    // init the stop handle
    rc = uv_async_init(&loop, &stopHandle, [](uv_async_t* handle) {
        RPCServer* svr = (RPCServer*)handle->data;
        svr->onStop();
    });
    if (rc < 0) {
        failHandler(rc, "initialize the stop handle");
    }
    stopHandle.data = this;
    asyncInited = true;

    // init the idle/iteration handle
    uv_check_init(&loop, &checkHandle); // always success
    checkHandle.data = this;
    uv_check_start(&checkHandle, [](uv_check_t* handle) { // always success
        RPCServer* svr = (RPCServer*)handle->data;
        svr->periodic();
    });
    checkStarted = true;

    // init the timer
    uv_timer_init(&loop, &timerHandle);
    timerHandle.data = this;
    rc = uv_timer_start(&timerHandle, [](uv_timer_t* handle) {
        RPCServer* svr = (RPCServer*)handle->data;
        svr->periodic();
    }, PERIODIC_INTERVAL, PERIODIC_INTERVAL);
    if (rc < 0) {
        failHandler(rc, "start timer");
    }
    timerStarted = true;

    // udp4 read start
    if (hasIPv4()) {
        log->trace("RPCServer start reading udp4 packet.");
        readStart(&udp4Handle, bind4);
        udp4Started = true;
    }

    // udp6 read start
    if (hasIPv6()) {
        log->trace("RPCServer start reading udp6 packet.");
        readStart(&udp6Handle, bind6);
        udp4Started = true;
    }

    // start the loop in thread
    dht_thread = std::thread([&]() {
        log->info("RPCServer is running.");
        running = true;

        int rc = uv_run(&loop, UV_RUN_DEFAULT);
        if (rc < 0) {
            log->error("RPCServer failed to start the event loop({}): {}", rc, uv_strerror(rc));
            onStop();
        }

        uv_loop_close(&loop);
        log->info("RPCServer is stopped.");
    });

}

void RPCServer::stop() {
    if(state == State::STOPPED)
        return;

    state = State::STOPPED;
    if (!running.exchange(false))
        return;

    uv_async_send(&stopHandle);
    try {
        dht_thread.join();
    } catch(...) {
    }

    if (hasIPv4())
        log->info("Stopped RPC Server ipv4: {}", bind4.toString());
    if (hasIPv6())
        log->info("Stopped RPC Server ipv6: {}", bind6.toString());
}

void RPCServer::updateReachability(uint64_t now) {
    // don't do pings too often if we're not receiving anything
    // (connection might be dead)
    if (receivedMessages != messagesAtLastReachableCheck) {
        _isReachable = true;
        lastReachableCheck = now;
        messagesAtLastReachableCheck = receivedMessages;
        return;
    }

    if (now - lastReachableCheck > Constants::RPC_SERVER_REACHABILITY_TIMEOUT)
        _isReachable = false;
}

void RPCServer::sendCall(Sp<RPCCall>& call) {
    int txid = nextTxid++;
    if (txid == 0) // 0 is invalid txid, skip
        txid = nextTxid++;

    if (calls.find(txid) != calls.end())
        throw std::runtime_error("Transaction ID already exists");

    call->getRequest()->setTxid(txid);
    calls[txid] = call;
    dispatchCall(call);
}

void RPCServer::dispatchCall(Sp<RPCCall>& call) {
    auto request = call->getRequest();
    assert(request != nullptr);

    auto responseHandler = [](RPCCall*, Sp<Message>&) {};
    auto timeoutHandler = [=](RPCCall* _call) {
        stats.onTimeoutMessage(*_call->getRequest());
        auto it = calls.find(_call->getRequest()->getTxid());
        if (it != calls.end()) {
            it->second->getDHT().onTimeout(_call);
            calls.erase(it);
        }
    };


    call->addTimeoutHandler(timeoutHandler);
    call->addResponseHandler(responseHandler);

    request->setAssociatedCall(call.get());
    sendMessage(request);
}

void RPCServer::sendMessage(Sp<Message> msg) {
    msg->setId(node.getId());
    auto short_name = Constants::NODE_SHORT_NAME;
    msg->setVersion(Version::build(short_name, Constants::NODE_VERSION));

    auto call = msg->getAssociatedCall();
    if (call != nullptr) {
        call->getDHT().onSend(call->getTargetId());
        call->sent(this);
    }

    sendData(msg);
}

void RPCServer::sendError(Sp<Message> msg, int code, const std::string& err) {
    auto em = std::make_shared<ErrorMessage>(msg->getMethod(), msg->getTxid(), code, err);
    em->setRemote(msg->getId(), msg->getOrigin());
    sendMessage(em);
}

void RPCServer::handlePacket(const uint8_t *buf, size_t buflen, const SocketAddress& from) {
    Sp<Message> msg = nullptr;
    std::vector<uint8_t> buffer;

    Id sender({buf, ID_BYTES});

    try {
        buffer = node.decrypt(sender, {buf + ID_BYTES, buflen - ID_BYTES});
    } catch(std::exception &e) {
        stats.onDroppedPacket(buflen);
        log->warn("Decrypt packet error from {}, ignored: len {}, {}", from.toString(), buflen, e.what());
        return;
    }

    try {
        msg = Message::parse(buffer.data(), buffer.size());
    } catch(std::exception& e) {
        stats.onDroppedPacket(buflen);
        log->warn("Got a wrong packet from {}, ignored.", from.toString());
        return;
    }

    receivedMessages++;
    stats.onReceivedBytes(buflen);
    stats.onReceivedMessage(*msg);
    msg->setId(sender);
    msg->setOrigin(from);

    log->debug("Received {}/{} from {}: [{}] {}", msg->getMethodString(), msg->getTypeString(),
            from.toString(), buflen, msg->toString());

    // transaction id should be a non-zero integer
    if (msg->getType() != Message::Type::ERR && msg->getTxid() == 0) {
        log->warn("Received a message with invalid transaction id.");

        sendError(msg, ErrorCode::ProtocolError,
                "Received a message with an invalid transaction id, expected a non-zero transaction id");
    }

    // just respond to incoming requests, no need to match them to pending requests
    if(msg->getType() == Message::Type::REQUEST) {
        handleMessage(msg);
        return;
    }

    // check if this is a response to an outstanding request
    auto it = calls.find(msg->getTxid());
    if (it != calls.end()) {
        Sp<RPCCall> call = it->second;
        // message matches transaction ID and origin == destination
        // we only check the IP address here. the routing table applies more strict checks to also verify a stable port
        if (call->getRequest()->getRemoteAddress() == msg->getOrigin()) {
            // remove call first in case of exception
            calls.erase(it);
            msg->setAssociatedCall(call.get());
            call->responsed(msg);

            // processCallQueue();
            // apply after checking for a proper response
            handleMessage(msg);

            return;
        }

        // 1. the message is not a request
        // 2. transaction ID matched
        // 3. request destination did not match response source!!
        // this happening by chance is exceedingly unlikely
        // indicates either port-mangling NAT, a multhomed host listening on any-local address or some kind of attack
        // ignore response
        log->warn("Transaction id matched, socket address did not, ignoring message, request: {} -> response: {}, version: {}",
                call->getRequest()->getRemoteAddress().toString(), msg->getOrigin().toString(), msg->getReadableVersion());

        if(msg->getType() == Message::Type::RESPONSE && dht6) {
            // this is more likely due to incorrect binding implementation in ipv6. notify peers about that
            // don't bother with ipv4, there are too many complications
            auto err = std::make_shared<ErrorMessage>(msg->getMethod(), msg->getTxid(), ErrorCode::ProtocolError,
                    "A request was sent to " + call->getRequest()->getRemoteAddress().toString() +
                    " and a response with matching transaction id was received from " + msg->getOrigin().toString() +
                    " . Multihomed nodes should ensure that sockets are properly bound and responses are sent with the correct source socket address. See BEPs 32 and 45.");
            err->setRemote(msg->getId(), call->getRequest()->getRemoteAddress());
            sendMessage(err);
        }

        // but expect an upcoming timeout if it's really just a misbehaving node
        call->responseSocketMismatch();
        call->stall();
        return;
    }

    // a) it's not a request
    // b) didn't find a call
    // c) up-time is high enough that it's not a stray from a restart
    // did not expect this response
    if (msg->getType() == Message::Type::RESPONSE && (currentTimeMillis() - startTime) > 2 * 60 * 1000) {
        log->warn("Cannot find RPC call for {}", msg->getType() == Message::Type::RESPONSE
                ? "response" : "error", msg->getTxid());

        sendError(msg, ErrorCode::ProtocolError,
                "Received a response message whose transaction ID did not match a pending request or transaction expired");
        return;
    }

    if (msg->getType() == Message::Type::ERR) {
        handleMessage(msg);
        return;
    }

    log->debug("Ignored message: {}", msg->toString());
}

void RPCServer::handleMessage(Sp<Message> msg) {
    if (msg->getOrigin().family() == AF_INET)
        dht4->get().onMessage(msg);
    else
        dht6->get().onMessage(msg);
}

void RPCServer::periodic() {
    while(!messageQueue.empty()) {
        auto msg = messageQueue.front();
        messageQueue.pop();
        sendData(msg);
    }

    scheduler.syncTime();
    scheduler.run();
}

} // namespace carrier
