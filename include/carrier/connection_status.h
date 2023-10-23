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

#include <cstdint>
#include <string>
#include <functional>
#include "def.h"
#include "network.h"

namespace carrier {

class CARRIER_PUBLIC ConnectionStatus {
public:
    enum Enum : uint8_t {
        Disconnected = 0,
        Connecting,
        Connected,
        Profound
    };

    constexpr ConnectionStatus() = delete;
    constexpr ConnectionStatus(Enum e) : e(e) {};

    // Allows comparisons with Enum constants.
    constexpr operator Enum() const noexcept {
        return e;
    }

    // Needed to prevent if(e)
    explicit operator bool() const = delete;

    std::string toString() const noexcept {
        switch (e) {
            case Disconnected: return "Disconnected";
            case Connecting: return "Connecting";
            case Connected: return "Connected";
            case Profound: return "Profound";
            default:
                return "Invalid value";
        }
    }

private:
    Enum e {};
};

class CARRIER_PUBLIC ConnectionStatusListener {
public:
    ConnectionStatusListener() :
        statusChanged([](Network, ConnectionStatus, ConnectionStatus){}),
        connected([](Network){}),
        profound([](Network){}),
        disconnected([](Network){}) {};

    std::function<void(Network, ConnectionStatus, ConnectionStatus)> statusChanged;
    std::function<void(Network)> connected;
    std::function<void(Network)> profound;
    std::function<void(Network)> disconnected;
};

} // namespace carrier
