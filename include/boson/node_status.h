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

#include <cstdint>
#include <string>
#include "def.h"

namespace boson {

class BOSON_PUBLIC NodeStatus {
public:
    enum Enum : uint8_t {
        Stopped = 0,
        Initializing,
        Running
    };

    constexpr NodeStatus() = delete;
    constexpr NodeStatus(Enum e) : e(e) {};

    // Allows comparisons with Enum constants.
    constexpr operator Enum() const noexcept {
        return e;
    }

    // Needed to prevent if(e)
    explicit operator bool() const = delete;

    std::string toString() const noexcept {
        switch (e) {
            case Stopped: return "Stopped";
            case Initializing: return "Initializing";
            case Running: return "Running";
            default:
                return "Invalid value";
        }
    }

private:
    Enum e {};
};

class BOSON_PUBLIC NodeStatusListener {
public:
    NodeStatusListener() :
        statusChanged([](NodeStatus, NodeStatus){}),
        started([](){}),
        stopped([](){}) {};

    std::function<void(NodeStatus, NodeStatus)> statusChanged;
    std::function<void()> started;
    std::function<void()> stopped;
};

} // namespace boson
