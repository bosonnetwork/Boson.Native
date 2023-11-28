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

#include <algorithm>
#include "rpcstatistics.h"
#include "utils/time.h"

namespace boson {

uint32_t RPCStatistics::getReceivedBytesPerSec() noexcept {
    uint64_t now = currentTimeMillis();
    uint64_t d = now - lastReceivedTimestamp;
    if (d > 950) {
        uint32_t lrb = lastReceivedBytes.exchange(0);
        receivedBytesPerSec = static_cast<uint32_t>(lrb * 1000 / d);
        lastReceivedTimestamp = now;
    }
    return receivedBytesPerSec;
}

uint32_t RPCStatistics::getSentBytesPerSec() noexcept {
    uint64_t now = currentTimeMillis();
    uint64_t d = now - lastSentTimestamp;
    if (d > 950) {
        long lrb = lastSentBytes.exchange(0);
        sentBytesPerSec = static_cast<uint32_t>(lrb * 1000 / d);
        lastSentTimestamp = now;
    }
    return sentBytesPerSec;
}

uint32_t RPCStatistics::getReceivedMessages(Message::Method method, Message::Type type) const noexcept {
    return receivedMessages[method.ordinal()][type.ordinal()].load();
}

uint32_t RPCStatistics::getTotalReceivedMessages() const noexcept {
    uint32_t total {0};
    std::for_each(receivedMessages.begin(), receivedMessages.end(), [&total](auto& item) {
        uint32_t sum {0};
        std::for_each(item.begin(), item.end(), [&sum](auto& n) {
            sum += n.load();
        });
        total += sum;
    });
    return total;
}

uint32_t RPCStatistics::getSentMessages(Message::Method method, Message::Type type) const noexcept {
    return sentMessages[method.ordinal()][type.ordinal()].load();
}

uint32_t RPCStatistics::getTotalSentMessages() const noexcept {
    uint32_t total {0};
    std::for_each(sentMessages.begin(), sentMessages.end(), [&total](auto& item) {
        uint32_t sum {0};
        std::for_each(item.begin(), item.end(), [&sum](auto& n) {
            sum += n.load();
        });
        total += sum;
    });
    return total;
}

uint32_t RPCStatistics::getTimeoutMessages(Message::Method method) const noexcept {
    return timeoutMessages[method.ordinal()].load();
}

uint32_t RPCStatistics::getTotalTimeoutMessages() const noexcept {
    uint32_t total {0};
    std::for_each(timeoutMessages.begin(), timeoutMessages.end(), [&total](auto& item) {
        total += item.load();
    });
    return total;
}

void RPCStatistics::onReceivedMessage(const Message& msg) noexcept {
    receivedMessages[msg.getMethod().ordinal()][msg.getType().ordinal()]++;
}

void RPCStatistics::onSentMessage(const Message& msg) noexcept {
    sentMessages[msg.getMethod().ordinal()][msg.getType().ordinal()]++;
}

void RPCStatistics::onTimeoutMessage(const Message& msg) noexcept {
    timeoutMessages[msg.getMethod().ordinal()]++;
}

std::string RPCStatistics::toString() const {
    std::stringstream ss;
    ss << "### local RPCs" << std::endl;
    ss << std::setw(18) << std::left << "Method"
        << std::setw(19) << std::left << "REQ" << " | "
        << std::setw(19) << std::left << "RSP"
        << std::setw(19) << std::left << "Error"
        << std::setw(19) << std::left << "Timeout"
        << std::endl;

    std::vector<Message::Method> methods {
        Message::Method::FIND_NODE,
        Message::Method::ANNOUNCE_PEER,
        Message::Method::FIND_PEER,
        Message::Method::STORE_VALUE,
        Message::Method::FIND_VALUE
    };

    for (auto& method : methods) {
        auto ordinal = method.ordinal();
        auto sent = sentMessages[ordinal][Message::Type::ordinalOf(Message::Type::REQUEST)].load();
        auto received = receivedMessages[ordinal][Message::Type::ordinalOf(Message::Type::RESPONSE)].load();
        auto error = receivedMessages[ordinal][Message::Type::ordinalOf(Message::Type::ERR)].load();
        auto timeout = timeoutMessages[ordinal].load();

        ss << std::setw(18) << std::left << Message::getMethodString(method)
            << std::setw(19) << std::left << sent << " | "
            << std::setw(19) << std::left << received
            << std::setw(19) << std::left << error
            << std::setw(19) << std::left << timeout
            << std::endl;
    }

    ss << std::endl << "### remote RPCs" << std::endl;
    ss << std::setw(18) << std::left << "Method"
        << std::setw(19) << std::left << "REQ" << " | "
        << std::setw(19) << std::left << "RSP"
        << std::setw(19) << std::left << "Errors"
        << std::endl;

    for (auto& method : methods) {
        auto ordinal = method.ordinal();
        auto sent = sentMessages[ordinal][Message::Type::ordinalOf(Message::Type::RESPONSE)].load();
        auto received = receivedMessages[ordinal][Message::Type::ordinalOf(Message::Type::REQUEST)].load();
        auto error = sentMessages[ordinal][Message::Type::ordinalOf(Message::Type::ERR)].load();

        ss << std::setw(18) << std::left << Message::getMethodString(method)
            << std::setw(19) << std::left << sent << " | "
            << std::setw(19) << std::left << received
            << std::setw(19) << std::left << error
            << std::endl;
    }

    ss << std::endl << "### Total[messages/bytes]" << std::endl;
    ss << "    sent " << getTotalSentMessages() << "/" << sentBytes.load()
        << ", received " << getTotalReceivedMessages() << "/" << receivedBytes.load()
        << ", timeout " << getTotalTimeoutMessages() << "/-"
        << ", dropped " << droppedPackets.load() << "/" << droppedBytes.load()
        << std::endl;

    return ss.str();
}

} // namespace boson
