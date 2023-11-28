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

#include "message.h"
#include "ping_request.h"
#include "ping_response.h"
#include "error_message.h"
#include "find_node_request.h"
#include "announce_peer_request.h"
#include "find_peer_request.h"
#include "store_value_request.h"
#include "find_value_request.h"
#include "announce_peer_response.h"
#include "find_node_response.h"
#include "find_peer_response.h"
#include "find_value_response.h"
#include "store_value_response.h"
#include "message_error.h"

namespace boson {

const int Message::MSG_TYPE_MASK = 0xE0;
const int Message::MSG_METHOD_MASK = 0x1F;

Sp<Message> Message::Method::createRequest() const {
    switch(e) {
    case Method::PING:
        return std::make_shared<PingRequest>();
    case Method::FIND_NODE:
        return std::make_shared<FindNodeRequest>();
    case Method::ANNOUNCE_PEER:
        return std::make_shared<AnnouncePeerRequest>();
    case Method::FIND_PEER:
        return std::make_shared<FindPeerRequest>();
    case Method::STORE_VALUE:
        return std::make_shared<StoreValueRequest>();
    case Method::FIND_VALUE:
        return std::make_shared<FindValueRequest>();
    case Method::UNKNOWN:
    default:
        throw MessageError("Invalid request method: " + std::to_string(e));
    }
}

Sp<Message> Message::Method::createResponse() const {
    switch(e) {
    case Method::PING:
        return std::make_shared<PingResponse>();
    case Method::FIND_NODE:
        return std::make_shared<FindNodeResponse>();
    case Method::ANNOUNCE_PEER:
        return std::make_shared<AnnouncePeerResponse>();
    case Method::FIND_PEER:
        return std::make_shared<FindPeerResponse>();
    case Method::STORE_VALUE:
        return std::make_shared<StoreValueResponse>();
    case Method::FIND_VALUE:
        return std::make_shared<FindValueResponse>();
    case Method::UNKNOWN:
    default:
        throw MessageError("Invalid response method: " + std::to_string(e));
    }
}

Sp<Message> Message::parse(const uint8_t* buf, size_t buflen) {
    auto root = nlohmann::json::from_cbor(std::vector<uint8_t>{buf, buf + buflen});
    if (!root.is_object())
        throw MessageError("Invalid message: not a CBOR object");

    auto type = root.find(KEY_TYPE);
    if (type == root.end()) {
        throw MessageError("Invalid message: missing type field");
    }

    auto message = Message::createMessage(type->get<uint8_t>());
    for (const auto& [key, value]: root.items()) {
        if (key == KEY_TXID) {
            value.get_to(message->txid);
        } else if (key == KEY_VERSION) {
            value.get_to(message->version);
        } else if (key == KEY_REQUEST || key == KEY_RESPONSE || key == KEY_ERROR) {
            message->parse(key, value);
        }
    }
    return message;
}

Sp<Message> Message::createMessage(int messageType) {
    auto type = Type::valueOf(messageType);
    auto method = Method::valueOf(messageType);

    switch (type) {
    case Type::REQUEST:
        return method.createRequest();

    case Type::RESPONSE:
        return method.createResponse();

    case Type::ERR:
        return std::make_shared<ErrorMessage>(method);

    default:
        throw MessageError("INTERNAL ERROR: should never happen.");
    }
}

std::string Message::toString() const {
    std::stringstream ss;
    ss.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    ss.str().reserve(1500);

    ss << "y:" << getTypeString()
        << ",m:" << getMethodString()
        << ",t:" << std::to_string(txid);
    toString(ss);
    if (version != 0)
        ss << ",v:" << getReadableVersion();
    return ss.str();
}

void Message::serializeInternal(nlohmann::json& root) const {
    root[KEY_TYPE] = type;
    root[KEY_TXID] = txid;
    root[KEY_VERSION] = version;
}

std::vector<uint8_t> Message::serialize() const {
    nlohmann::json root = nlohmann::json::object();
    serializeInternal(root);
    return nlohmann::json::to_cbor(root);
}

} // namespace boson
