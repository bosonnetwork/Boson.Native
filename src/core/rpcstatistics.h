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

#pragma once

#include <atomic>
#include <vector>
#include "messages/message.h"

namespace boson {

class RPCStatistics {
public:
    RPCStatistics() {};

    uint32_t getReceivedBytes() const noexcept{
        return receivedBytes.load();
    }

    uint32_t getSentBytes() const noexcept {
        return sentBytes.load();
    }

    uint32_t getReceivedBytesPerSec() noexcept;
    uint32_t getSentBytesPerSec() noexcept;

    uint32_t getReceivedMessages(Message::Method method, Message::Type type) const noexcept;
    uint32_t getTotalReceivedMessages() const noexcept;

    uint32_t getSentMessages(Message::Method method, Message::Type type) const noexcept;
    uint32_t getTotalSentMessages() const noexcept;

    uint32_t getTimeoutMessages(Message::Method method) const noexcept;
    uint32_t getTotalTimeoutMessages() const noexcept;

    uint32_t getDropedPackets() const noexcept {
        return droppedPackets.load();
    }

    uint32_t getDroppedBytes() const noexcept {
        return droppedBytes.load();
    }

    void onReceivedBytes(uint32_t receivedBytes) noexcept {
        lastReceivedBytes.fetch_add(receivedBytes);
        this->receivedBytes.fetch_add(receivedBytes);
    }

    void onSentBytes(uint32_t sentBytes) noexcept  {
        lastSentBytes.fetch_add(sentBytes);
        this->sentBytes.fetch_add(sentBytes);
    }

    void onReceivedMessage(const Message&) noexcept;
    void onSentMessage(const Message& message) noexcept;
    void onTimeoutMessage(const Message& message) noexcept;

    void onDroppedPacket(int bytes) noexcept {
        droppedPackets++;
        droppedBytes.fetch_add(bytes);
    }

    std::string toString() const;

private:
    std::atomic_uint32_t receivedBytes {0};
    std::atomic_uint32_t sentBytes {0};

    std::atomic_uint32_t lastReceivedBytes {0};
    std::atomic_uint32_t lastSentBytes {0};

    volatile uint64_t lastReceivedTimestamp {0};
    volatile uint64_t lastSentTimestamp {0};

    volatile uint32_t receivedBytesPerSec {0};
    volatile uint32_t sentBytesPerSec {0};

    std::array<std::array<std::atomic_uint32_t, TYPE_TOTAL>, METHOD_TOTAL> receivedMessages {};
    std::array<std::array<std::atomic_uint32_t, TYPE_TOTAL>, METHOD_TOTAL> sentMessages {};
    std::array<std::atomic_uint32_t, TYPE_TOTAL> timeoutMessages {};

    std::atomic_uint32_t droppedPackets {0};
    std::atomic_uint32_t droppedBytes {0};
};

} // namespace boson
