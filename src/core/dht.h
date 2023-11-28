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

#include <memory>
#include <list>
#include <cstdio>
#include <atomic>
#include <map>
#include <mutex>

#include "boson/id.h"
#include "boson/value.h"
#include "boson/node_info.h"
#include "boson/peer_info.h"
#include "boson/lookup_option.h"
#include "boson/types.h"
#include "boson/connection_status.h"

#include "task/task_manager.h"
#include "rpcserver.h"
#include "routing_table.h"
#include "token_manager.h"

namespace boson {

class Message;
class PingRequest;
class RoutingTable;
class LookupResponse;
class Node;
class DHT;


class DHT {
    class CompletionStatus {
    public:
        enum Enum : uint8_t {
            Pending = 0,
            Canceled,
            Completed
        };

        constexpr CompletionStatus() = delete;
        constexpr CompletionStatus(Enum e) : e(e) {};

        // Allows comparisons with Enum constants.
        constexpr operator Enum() const noexcept {
            return e;
        }

        // Needed to prevent if(e)
        explicit operator bool() const = delete;

        std::string toString() const noexcept {
            switch (e) {
                case Pending: return "Pending";
                case Canceled: return "Canceled";
                case Completed: return "Completed";
                default:
                    return "Invalid value";
            }
        }

    private:
        Enum e {};
    };
    class BootstrapStage {
    public:
        BootstrapStage(DHT* dht): dht(dht) {};

        void fillHomeBucket(CompletionStatus status) {
            if (_fillHomeBucket == status)
                return;

            _fillHomeBucket = status;
            updateConnectionStatus();
        }

        void fillAllBuckets(CompletionStatus status) {
            if (_fillAllBuckets == status)
                return;
            _fillAllBuckets = status;
            updateConnectionStatus();
        }

        void clearBootstrapStatus() {
            _fillHomeBucket = CompletionStatus::Pending;
            _fillAllBuckets = CompletionStatus::Pending;
        }

    private:
        bool completed(CompletionStatus status) {
            return status > CompletionStatus::Pending;
        }

        void updateConnectionStatus();

        CompletionStatus _fillHomeBucket {CompletionStatus::Pending};
        CompletionStatus _fillAllBuckets {CompletionStatus::Pending};

        DHT* dht;
        mutable std::mutex mtx {};
    };

public:
    explicit DHT(Network _type, const Node& _node, const SocketAddress& _addr);

    Network getType() const noexcept {
        return type;
    }

    const Node& getNode() const noexcept {
        return node;
    }

    Sp<NodeInfo> getNode(const Id&) const;

    RPCServer& getServer() const noexcept {
        assert(rpcServer);
        return *rpcServer;
    };

    void setServer(const Sp<RPCServer> server) noexcept {
        this->rpcServer = server;
    }

    void setTokenManager(const Sp<TokenManager> manager) noexcept {
        this->tokenManager = manager;
    }

    const SocketAddress& getOrigin() const noexcept {
        return addr;
    }

    RoutingTable& getRoutingTable() noexcept {
        return routingTable;
    }

    TaskManager& getTaskManager() noexcept {
        return taskMan;
    }

    void enablePersistence(const std::string& path) noexcept {
        persistFile = path;
    }

    const std::vector<Sp<NodeInfo>>& getBootstraps() const noexcept {
        return bootstrapNodes;
    }

    std::vector<Id> getBootstrapIds() const {
        std::vector<Id> ids {};
        for (const auto& node: bootstrapNodes) {
            ids.push_back(node->getId());
        }
        return ids;
    }

    void bootstrap();
    void bootstrap(const std::vector<NodeInfo>&);

    void fillHomeBucket(const std::list<Sp<NodeInfo>>&);

    void start(std::vector<Sp<NodeInfo>>& bootstrapNodes);
    void stop();

    bool isRunning() const noexcept {
        return running;
    }

#ifdef BOSON_CRAWLER
    void ping(Sp<NodeInfo> node, std::function<void(Sp<NodeInfo>)> completeHandler);
    void getNodes(const Id& id, Sp<NodeInfo> node, std::function<void(std::list<Sp<NodeInfo>>)> completeHandler);
#endif

    Sp<Task> findNode(const Id& id, std::function<void(Sp<NodeInfo>)> completeHandler) {
        return findNode(id, LookupOption::CONSERVATIVE, completeHandler);
    };

    Sp<Task> findNode(const Id& id, LookupOption option, std::function<void(Sp<NodeInfo>)> completeHandler);
    Sp<Task> findValue(const Id& id, LookupOption option, std::function<void(Sp<Value>)> completeHandler);
    Sp<Task> storeValue(const Value& value, std::function<void(std::list<Sp<NodeInfo>>)> completeHandler);
    Sp<Task> findPeer(const Id& id, int expected, LookupOption option, std::function<void(std::vector<PeerInfo>)> completeHandler);
    Sp<Task> announcePeer(const PeerInfo& peer, std::function<void(std::list<Sp<NodeInfo>>)> completeHandler);

    void onTimeout(RPCCall* call);
    void onSend(const Id& id);

    void onMessage(Sp<Message>);
    std::string toString() const;

    bool isSelfAddress(const SocketAddress& addr) const {
        return this->addr == addr;
    }

private:
    void received(Sp<Message>);
    void update();
    void updateBootstrap();
    void sendError(Sp<Message> q, int code, const std::string& msg);

    void onRequest(Sp<Message>);
    void onResponse(Sp<Message>);
    void onError(Sp<Message>);

    void onPing(const Sp<Message>&);
    void onFindNode(const Sp<Message>&);
    void onFindValue(const Sp<Message>&);
    void onStoreValue(const Sp<Message>&);
    void onFindPeers(const Sp<Message>&);
    void onAnnouncePeer(const Sp<Message>&);

    void populateClosestNodes(Sp<LookupResponse> r, const Id& target, int v4, int v6);

    void setStatus(ConnectionStatus expected, ConnectionStatus newStatus);

private:
    Network type {Network::IPv4};
    ConnectionStatus status {ConnectionStatus::Disconnected};

    const Node& node;
    Sp<RPCServer> rpcServer {};
    Sp<TokenManager> tokenManager {};

    SocketAddress addr;

    RoutingTable routingTable {*this};
    TaskManager taskMan {};

    std::vector<Sp<NodeInfo>> bootstrapNodes = {};
    std::map<SocketAddress, Id> knownNodes = {};
    std::atomic<bool> bootstrapping {false};
    std::atomic<bool> needUpdateBootstrap {false};
    uint64_t lastBootstrap {0};
    BootstrapStage bootstrapStage {this};

    uint64_t lastSave {0};
    bool running = false;

    std::string persistFile;

    Sp<Logger> log;
};

} // namespace boson
