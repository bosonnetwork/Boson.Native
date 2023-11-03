/*
 * Copyright (c) 2022 trinity-tech.io
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

#include <uv.h>

#include <memory>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <cassert>
#include <fstream>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <nlohmann/json.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "activeproxy.h"
#include "connection.h"
#include "exceptions/exceptions.h"
#include "utils/addr.h"
#include "crypto/hex.h"

namespace carrier {
namespace activeproxy {

static const uint32_t IDLE_CHECK_INTERVAL = 60 * 1000;          // 60 seconds
static const uint32_t MAX_IDLE_TIME = 5 * 60 * 1000;            // 5 minutes
static const uint32_t RE_ANNOUNCE_INTERVAL = 60 * 60 * 1000;    // 1 hour
static const uint32_t HEALTH_CHECK_INTERVAL = 10 * 1000;        // 10 seconds
static const uint32_t PERSISTENCE_INTERVAL = 60 * 60 * 1000;    // 1 hour

static const size_t MAX_DATA_PACKET_SIZE = 0x7FFF;      // 32767

std::future<void> ActiveProxy::initialize(Sp<Node> node, const std::map<std::string, std::any>& configure) {
    log = Logger::get("AcriveProxy");

    if (configure.count("logLevel")) {
        logLevel = std::any_cast<std::string>(configure.at("logLevel"));
        log->setLevel(logLevel);
    }

    if (configure.count("persistPath")) {
        auto dirPath = std::any_cast<std::string>(configure.at("persistPath"));
        persistPath = dirPath + "/activeProxy.cache";
    }

    if (!configure.count("upstreamHost"))
        throw std::invalid_argument("Addon ActiveProxy's configure item has error: missing upstreamHost!");

    if (!configure.count("upstreamPort"))
        throw std::invalid_argument("Addon ActiveProxy's configure item has error: missing upstreamPort!");

    upstreamHost = std::any_cast<std::string>(configure.at("upstreamHost"));
    upstreamPort = (uint16_t)std::any_cast<int64_t>(configure.at("upstreamPort"));
    if (upstreamHost.empty() || upstreamPort == 0)
        throw std::invalid_argument("Addon ActiveProxy's configure item has error: empty upstreamHost or upstreamPort is not allowed");

    if (configure.count("serverPeerId")) {
        serverPeerId = std::any_cast<std::string>(configure.at("serverPeerId"));
        auto found = loadServicePeer();
        if (!found)
            found = lookupServicePeer(node);
        if (!found)
            throw std::invalid_argument("Addon ActiveProxy can't find available service for peer: " + serverPeerId + "!");
    } else if (configure.count("serverId") && configure.count("serverHost") && configure.count("serverPort")) {
        // TODO: to be remove
        std::string id = std::any_cast<std::string>(configure.at("serverId"));
        serverId = Id(id);
        serverHost = std::any_cast<std::string>(configure.at("serverHost"));
        serverPort = (uint16_t)std::any_cast<int64_t>(configure.at("serverPort"));
    } else {
        throw std::invalid_argument("Addon ActiveProxy's configure item has error: missing serverPeerId!");
    }

    if (serverHost.empty() || serverPort == 0)
        throw std::runtime_error("Addon ActiveProxy's configure item has error: empty serverHost or serverPort is not allowed");

    if (configure.count("peerPrivateKey")) {
        std::string sk = std::any_cast<std::string>(configure.at("peerPrivateKey"));
        peerKeypair = Signature::KeyPair::fromPrivateKey(Hex::decode(sk));
    }

    if (configure.count("domainName"))
        domainName = std::any_cast<std::string>(configure.at("domainName"));

    if (configure.count("maxConnections"))
        maxConnections = (uint32_t)std::any_cast<int64_t>(configure.at("maxConnections"));

    //init data
    this->node = node;

    auto addrs = SocketAddress::resolve(serverHost, serverPort);
    serverAddr = addrs[0];

    addrs = SocketAddress::resolve(upstreamHost, upstreamPort);
    upstreamAddr = addrs[0];

    serverName.reserve(serverHost.length() + 8);
    upstreamName.reserve(upstreamHost.length() + 8);

    serverName.append(serverHost).append(":").append(std::to_string(serverPort));
    upstreamName.append(upstreamHost).append(":").append(std::to_string(upstreamPort));

    readBuffer.resize(MAX_DATA_PACKET_SIZE);

    //start
    startPromise = std::promise<void>();
    start();
    startCheckServicePeer();
    return startPromise.get_future();
}

std::future<void> ActiveProxy::deinitialize() {
    stopPromise = std::promise<void>();
    stopCheckServicePeer();
    stop();
    return stopPromise.get_future();
}

void ActiveProxy::onStop() noexcept
{
    log->info("Addon ActiveProxy is on-stopping...");
    running = false;

    uv_check_stop(&checkHandle);
    uv_timer_stop(&timerHandle);
    uv_close((uv_handle_t*)&checkHandle, nullptr);
    uv_close((uv_handle_t*)&timerHandle, nullptr);
    uv_close((uv_handle_t*)&stopHandle, nullptr);

    // close all connections
    for (const auto& c : connections) {
        c->onClosed(nullptr);
        c->close();
        c->unref();
    }

    connections.clear();
    stopPromise.set_value();
}

bool ActiveProxy::needsNewConnection() noexcept
{
    if (connections.size() >= maxConnections)
        return false;

    // reconnect delay after connect to server failed
    if (reconnectDelay && uv_now(&loop) - lastConnectTimestamp < reconnectDelay)
        return false;

    if (connections.empty()) {
        if (serverPk.has_value())
            reset();
        return true;
    }

    if (inFlights == connections.size())
        return true;

    // TODO: other conditions?
    return false;
}

void ActiveProxy::onIteration() noexcept
{
    if (first) {
        startPromise.set_value();
        first = false;
    }

    if (needsNewConnection())
        connect();

    auto now = uv_now(&loop);
    if (now - lastIdleCheckTimestamp >= IDLE_CHECK_INTERVAL) {
        lastIdleCheckTimestamp = now;
        idleCheck();
    }

    if (now - lastHealthCheckTimestamp >= HEALTH_CHECK_INTERVAL) {
        lastHealthCheckTimestamp = now;
        healthCheck();
    }

    if (peer.has_value() && now - lastAnnouncePeerTimestamp >= RE_ANNOUNCE_INTERVAL) {
        lastAnnouncePeerTimestamp = now;
        announcePeer();
    }
}

void ActiveProxy::idleCheck() noexcept
{
    auto now = uv_now(&loop);

    // Dump the current status: should change the log level to debug later
    log->info("Addon ActiveProxy STATUS dump: Connections = {}, inFlights = {}, idle = {}",
            connections.size(), inFlights,
            idleTimestamp == UINT64_MAX ? 0 : (now - idleTimestamp) / 1000);

    for (const auto& c: connections)
        log->info("Addon ActiveProxy STATUS dump: {}", c->status());

    if (idleTimestamp == UINT64_MAX || (now - idleTimestamp) < MAX_IDLE_TIME)
        return;

    if (inFlights != 0 || connections.size() <= 1)
        return;

    log->info("Addon ActiveProxy is closing the redundant connections due to long time idle...");
    for (auto c = connections.end() - 1; c > connections.begin(); --c) {
        (*c)->onClosed(nullptr);
        (*c)->close();
        (*c)->unref();
    }

    connections.resize(1);
}

void ActiveProxy::healthCheck() noexcept
{
    for (const auto& c: connections)
        c->periodicCheck();
}

void ActiveProxy::start()
{
    log->info("Addon ActiveProxy is starting...");

    int rc = uv_loop_init(&loop);
    if (rc < 0) {
        log->error("Addon ActiveProxy failed to initialize the event loop({}): {}", rc, uv_strerror(rc));
        throw networking_error(uv_strerror(rc));
    }

    // init the stop handle
    rc = uv_async_init(&loop, &stopHandle, [](uv_async_t* handle) {
        ActiveProxy* ap = (ActiveProxy*)handle->data;
        ap->onStop();
    });
    if (rc < 0) {
        log->error("Addon ActiveProxy failed ot initialize the stop handle({}): {}", rc, uv_strerror(rc));
        uv_loop_close(&loop);
        throw networking_error(uv_strerror(rc));
    }
    stopHandle.data = this;

    // init the idle/iteration handle
    uv_check_init(&loop, &checkHandle); // always success
    checkHandle.data = this;
    rc = uv_check_start(&checkHandle, [](uv_check_t* handle) {
        ActiveProxy* ap = (ActiveProxy*)handle->data;
        ap->onIteration();
    });
    if (rc < 0) {
        log->error("Addon ActiveProxy failed to start the iteration handle({}): {}", rc, uv_strerror(rc));
        uv_close((uv_handle_t*)&checkHandle, nullptr);
        uv_close((uv_handle_t*)&stopHandle, nullptr);
        uv_loop_close(&loop);
        throw networking_error(uv_strerror(rc));
    }

    auto now = uv_now(&loop);
    lastIdleCheckTimestamp = now;
    lastHealthCheckTimestamp = now;

    uv_timer_init(&loop, &timerHandle);
    timerHandle.data = this;
    rc = uv_timer_start(&timerHandle, [](uv_timer_t* handle) {
        ActiveProxy* ap = (ActiveProxy*)handle->data;
        ap->onIteration();
    }, HEALTH_CHECK_INTERVAL, HEALTH_CHECK_INTERVAL);
    if (rc < 0) {
        log->error("Addon ActiveProxy failed to start timer ({}): {}", rc, uv_strerror(rc));
        uv_check_stop(&checkHandle);
        uv_close((uv_handle_t*)&checkHandle, nullptr);
        uv_close((uv_handle_t*)&stopHandle, nullptr);
        uv_loop_close(&loop);
        throw networking_error(uv_strerror(rc));
    }

    // Start the loop in thread
    runner = std::thread([&]() {
        log->info("Addon ActiveProxy is running.");
        running = true;
        first = true;
        int rc = uv_run(&loop, UV_RUN_DEFAULT);
        if (rc < 0) {
            log->error("Addon ActiveProxy failed to start the event loop({}): {}", rc, uv_strerror(rc));
            running = false;
            uv_check_stop(&checkHandle);
            uv_timer_stop(&timerHandle);
            uv_close((uv_handle_t*)&checkHandle, nullptr);
            uv_close((uv_handle_t*)&timerHandle, nullptr);
            uv_close((uv_handle_t*)&stopHandle, nullptr);
            uv_loop_close(&loop);

            auto exp = std::make_exception_ptr(networking_error(uv_strerror(rc)));
            startPromise.set_exception(exp);
        }

        running = false;
        uv_loop_close(&loop);
        log->info("Addon ActiveProxy is stopped.");
    });
}

void ActiveProxy::stop() noexcept
{
    if (!running) {
        stopPromise.set_value();
        return;
    }

    log->info("Addon ActiveProxy is stopping...");
    uv_async_send(&stopHandle);
    try {
        runner.join();
    } catch(...) {
    }
}

void ActiveProxy::connect() noexcept
{
    assert(running);

    log->debug("Addon ActiveProxy tried to create a new connectoin.");

    ProxyConnection* connection = new ProxyConnection {*this};
    connections.push_back(connection);

    connection->onAuthorized([this](ProxyConnection* c, const CryptoBox::PublicKey& serverPk, uint16_t port, bool domainEnabled) {
        this->serverPk = serverPk;
        this->relayPort = port;
        this->box = CryptoBox{serverPk, this->sessionKey.privateKey() };

        std::string domain = domainEnabled ? domainName : "";
        if (peerKeypair.has_value()) {
            peer = PeerInfo::create(peerKeypair.value(), serverId, node->getId(), port, domain);
            // will announce the peer in the next libuv iteration
        }

        if (!domain.empty())
            log->info("-**- ActiveProxy: server: {}:{}, domain: {} -**-",
                serverHost, relayPort, domain);
        else
            log->info("-**- ActiveProxy: server: {}:{} -**-",
                serverHost, relayPort);

    });

    connection->onOpened([this](ProxyConnection* c) {
        serverFails = 0;
        reconnectDelay = 0;
    });

    connection->onOpenFailed([this](ProxyConnection* c) {
        serverFails++;
        if (reconnectDelay < 64)
            reconnectDelay = (1 << serverFails) * 1000;
    });

    connection->onClosed([this](ProxyConnection* c) {
        auto it = connections.begin();

        for (; it != connections.end(); ++it) {
            if (*it == c)
                break;
        }

        if (it != connections.end())
            connections.erase(it);

        c->unref();
    });

    connection->onBusy([this](ProxyConnection* c) {
        ++inFlights;
        idleTimestamp = UINT64_MAX;
    });

    connection->onIdle([this](ProxyConnection* c) {
        if (--inFlights == 0)
            idleTimestamp = uv_now(&loop);
    });

    lastConnectTimestamp = uv_now(&loop);
    connection->connectServer();
}

void ActiveProxy::announcePeer() noexcept
{
    if (!peer.has_value())
        return;

    log->info("Announce peer {} : {}", peer.value().getId().toBase58String(),
        peer.value().toString());

    if (peer.value().hasAlternativeURL())
        log->info("-**- ActiveProxy: server: {}:{}, domain: {} -**-",
            serverHost, peer.value().getPort(), peer.value().getAlternativeURL());
    else
        log->info("-**- ActiveProxy: server: {}:{} -**-",
            serverHost, peer.value().getPort());

    node->announcePeer(peer.value());
}

bool ActiveProxy::loadServicePeer() {
    if (persistPath.empty() || serverPeerId.empty())
        return false;

    std::ifstream file(persistPath, std::ios::binary);
    if (!file.is_open())
        return false;

    std::vector<uint8_t> data{};
    if (!file.bad()) {
        auto length = file.rdbuf()->pubseekoff(0, std::ios_base::end);
        if (length == 0) {
            file.close();
            return false;
        }

        data.resize(length);
        file.rdbuf()->pubseekoff(0, std::ios_base::beg);
        file.read(reinterpret_cast<char*>(data.data()), length);
    }

    try {
        nlohmann::json root = nlohmann::json::from_cbor(data);

        auto getPeerId = root.at("peerId").get<std::string>();
        if (serverPeerId != getPeerId) {
            log->warn("The cached peerId {} is different from the config peerId {}, discarded cached peer.",
                serverPeerId, getPeerId);
            file.close();
            return false;
        }

        serverHost = root.at("serverHost").get<std::string>();
        serverPort = root.at("serverPort").get<int>();
        auto idstr = root.at("serverId").get<std::string>();
        if (serverHost.empty() || serverPort == 0 || idstr.empty()) {
            log->warn("The cached peer {} information is invalid, discorded cached data", serverPeerId);
            return false;
        }
        serverId = Id(idstr);

        log->info("Load peer {} with server {}:{} from persistence file.", getPeerId, serverHost, serverPort);
    } catch (const std::exception& e) {
        log->warn("read persistence file '{}' error: {}", persistPath, e.what());
        file.close();
        return false;
    }

    file.close();
    return true;
}

void ActiveProxy::saveServicePeer() {
    if (persistPath.empty())
        return;

    std::ofstream file(persistPath, std::ios::binary);
    if (!file.is_open())
        return;

    if (serverHost.empty() || serverPort == 0) {
        log->trace("Skip to save server information");
        return;
    }

    nlohmann::json root = nlohmann::json::object();
    root["peerId"] = serverPeerId;
    root["serverHost"] = serverHost;
    root["serverPort"] = serverPort;
    root["serverId"] = serverId.toString();

    auto data = nlohmann::json::to_cbor(root);
    file.write(reinterpret_cast<char*>(data.data()), data.size());
    file.close();

    log->info("-**- Saved the service peer: peerId {}, nodeId: {}, server address: {}:{}.",
        serverPeerId, serverId.toString(), serverHost, serverPort);
}

bool ActiveProxy::lookupServicePeer(Sp<Node> node) {
    auto peerId = Id(serverPeerId);

    log->info("Addon ActiveProxy is trying to find peer {} ...", serverPeerId);
    auto future = node->findPeer(peerId, 8);
    auto peers = future.get();
    if (peers.empty()) {
        log->warn("Cannot find a server peer {} at this moment, please try it later!!!", serverPeerId);
        return false;
    }
    log->info("Addon ActiveProxy found {} peers.", peers.size());

    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);
    std::shuffle(peers.begin(), peers.end(), e);

    auto found {false};
    for (const auto& peer: peers) {
        serverPort = peer.getPort();
        serverId = peer.getNodeId();

        log->info("Trying to locate node {} hosting service peer {} ...", serverId.toString(), peer.toString());
        auto nis = node->findNode(serverId).get();
        if (!nis.hasValue()) {
            log->warn("Addon ActiveProxy can't locate node: {}! Go on next ...", serverId.toString());
            continue;
        }

        auto ni = (nis.getV4() != nullptr) ? nis.getV4() : nis.getV6();
        serverHost = ni->getAddress().host();
        log->info("A server node {} hosting address: {} found", serverId.toString(), ni->getAddress().toString());

        found = true;
        break;
    }
    return found;
}

void ActiveProxy::startCheckServicePeer() {
    if (persistPath.empty())
        return;

    uv_loop_init(&assstLoop);
    uv_async_init(&assstLoop, &assistAsync, [](uv_async_t* handle) {
        ActiveProxy* ap = (ActiveProxy*)handle->data;
        uv_close((uv_handle_t*)&ap->assistTimer, nullptr);
        uv_close((uv_handle_t*)&ap->assistAsync, nullptr);
    });
    assistAsync.data = this;

    uv_timer_init(&assstLoop, &assistTimer);
    assistTimer.data = this;
    auto rc = uv_timer_start(&assistTimer, [](uv_timer_t* handle) {
        ActiveProxy* ap = (ActiveProxy*)handle->data;
        if (ap->lookupServicePeer(ap->node))
            ap->saveServicePeer();
    }, PERSISTENCE_INTERVAL, PERSISTENCE_INTERVAL);
    if (rc < 0) {
        uv_close((uv_handle_t*)&assistTimer, nullptr);
        uv_close((uv_handle_t*)&assistAsync, nullptr);
        uv_loop_close(&assstLoop);
        return;
    }

    assistRunner = std::thread([&]() {
        int rc = uv_run(&assstLoop, UV_RUN_DEFAULT);
        if (rc < 0) {
            uv_timer_stop(&assistTimer);
            uv_close((uv_handle_t*)&assistTimer, nullptr);
            uv_close((uv_handle_t*)&assistAsync, nullptr);
        }
        uv_loop_close(&assstLoop);
    });
}

void ActiveProxy::stopCheckServicePeer() noexcept {
    uv_timer_stop(&assistTimer);
    uv_async_send(&assistAsync);

    try {
        assistRunner.join();
    } catch(...) {
    }
}

} // namespace activeproxy
} // namespace carrier
