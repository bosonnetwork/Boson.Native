/*
 * Copyright (c) 2022 - 2023 trinity-tech.io
 * Copyright (c) 2023 -  ~   bosonnetwork.io
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

#pragma once

#include <map>
#include <vector>
#include <memory>
#include <sstream>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <nlohmann/json.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "constants.h"
#include "boson/socket_address.h"
#include "boson/id.h"
#include "boson/version.h"
#include "message_key.h"

namespace boson {

class RPCCall;
class RPCServer;

#define METHOD_TOTAL 7
#define TYPE_TOTAL 3

class Message: protected MessageKey {
public:
    static const int MSG_VERSION = 0x01;
    static const int BASE_SIZE = 56;

public:
    class Method {
    public:
        enum Enum : uint8_t {
            UNKNOWN     = 0x00,
            PING        = 0x01,
            FIND_NODE   = 0x02,
            ANNOUNCE_PEER   = 0x03,
            FIND_PEER   = 0x04,
            STORE_VALUE = 0x05,
            FIND_VALUE  = 0x06
        };

        constexpr Method() = delete;
        constexpr Method(Enum _e) : e(_e) {};

        constexpr operator Enum() const noexcept {
            return e;
        }

        explicit operator bool() const = delete;

        int ordinal() const noexcept {
            return static_cast<int>(e);
        }

        static int total() noexcept {
            return METHOD_TOTAL;
        }

        Sp<Message> createRequest() const;
        Sp<Message> createResponse() const;

        std::string toString() const noexcept {
            switch (e) {
                case UNKNOWN: default: return "unknown";
                case PING: return "ping";
                case FIND_NODE: return "find_node";
                case ANNOUNCE_PEER: return "announce_peer";
                case FIND_PEER: return "find_peer";
                case STORE_VALUE: return "store_value";
                case FIND_VALUE: return "find_value";
            }
        }

        static Method valueOf(int value) {
            auto method = value & MSG_METHOD_MASK;
            switch(method) {
                case 0x00: return UNKNOWN;
                case 0x01: return PING;
                case 0x02: return FIND_NODE;
                case 0x03: return ANNOUNCE_PEER;
                case 0x04: return FIND_PEER;
                case 0x05: return STORE_VALUE;
                case 0x06: return FIND_VALUE;
                default:
                    throw std::invalid_argument("Invalid message method: " + std::to_string(method));
            }
        }

    private:
        Enum e {};
    };

    class Type {
    public:
        enum Enum : uint8_t {
            ERR         = 0x00,
            REQUEST     = 0x20,
            RESPONSE    = 0x40
        };

        constexpr Type() = delete;
        constexpr Type(Enum _e) : e(_e) {};

        explicit operator bool() const = delete;
        constexpr operator Enum() const noexcept {
            return e;
        }

        int ordinal() const noexcept {
            switch(e) {
                default: case ERR: return 0;
                case REQUEST: return 1;
                case RESPONSE: return 2;
            }
        }

        static int total() noexcept {
            return TYPE_TOTAL;
        }

        std::string toString() const noexcept {
            switch (e) {
                case ERR: default: return "e";
                case REQUEST: return "q";
                case RESPONSE: return "r";
            }
        }

        static int ordinalOf(Type type) {
            return type.ordinal();
        }

        static Type valueOf(int value) {
            auto type = value & MSG_TYPE_MASK;
            switch(type) {
                case 0x00: return ERR;
                case 0x20: return REQUEST;
                case 0x40: return RESPONSE;
                default:
                    throw std::invalid_argument("Invalid message type: " + std::to_string(type));
            }
        }
    private:
        Enum e {};
    };

    Message() = delete;
    Message(const Message&) = delete;

    Method getMethod() const {
        return Method::valueOf(type & MSG_METHOD_MASK);
    }

    Type getType() const {
        return Type::valueOf(type & MSG_TYPE_MASK);
    }

    std::string getMethodString() const {
        return getMethod().toString();
    }

    std::string getTypeString() const {
        return getType().toString();
    }

    std::string getKeyString() const {
        return getType().toString();
    }

    static std::string getMethodString(Method method) {
        return method.toString();
    }

    static std::string getTypeString(Type type) {
        return type.toString();
    }

    void setId(const Id& id) noexcept {
        this->id = id;
    }

    const Id& getId() const noexcept {
        return id;
    }

    void setTxid(int txid) noexcept {
        this->txid = txid;
    }

    int getTxid() const noexcept {
        return txid;
    }

    void setVersion(int version) noexcept {
        this->version = version;
    }

    int getVersion() const noexcept {
        return version;
    }

    const SocketAddress& getOrigin() const noexcept {
        return origin;
    }

    void setOrigin(const SocketAddress& origin) noexcept {
        this->origin = origin;
    }

    const SocketAddress& getRemoteAddress() noexcept {
        return remoteAddr;
    }

    const Id& getRemoteId() noexcept {
        return remoteId;
    }

    void setRemote(const Id& id, const SocketAddress& address) noexcept {
        this->remoteId = id;
        this->remoteAddr = address;
    }

    std::string getReadableVersion() const noexcept {
        return Version::toString(version);
    }

    void setAssociatedCall(RPCCall* call) noexcept {
        associatedCall = call;
    }

    RPCCall* getAssociatedCall() const noexcept {
        return associatedCall;
    }

    static Sp<Message> parse(const uint8_t* buf, size_t buflen);

    std::string toString() const;
    std::vector<uint8_t> serialize() const;

    virtual int estimateSize() const {
        return BASE_SIZE;
    }

protected:
    explicit Message(Type _type, Method _method, int _txid = 0)
        : type(static_cast<int>(_type) | static_cast<int>(_method)), txid(_txid), version(0) {}

    virtual void parse(const std::string& fieldName, nlohmann::json& object) {}
    virtual void toString(std::stringstream& ss) const {}
    virtual void serializeInternal(nlohmann::json& root) const;

private:
    static Sp<Message> createMessage(int type);

    static const int MSG_TYPE_MASK;
    static const int MSG_METHOD_MASK;

    SocketAddress origin;
    SocketAddress remoteAddr;

    Id id;
    Id remoteId;

    RPCCall* associatedCall {};

    int type {0};
    int txid {0};
    int version {0};
};

} // namespace boson
