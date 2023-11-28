/*
 * Copyright (c) 2023 trinity-tech.io
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

#include "network.h"

namespace boson {

template <class T>
class BOSON_PUBLIC Result {

public:
    Result(Sp<T> v4, Sp<T> v6): v4(v4), v6(v6) {};

    Sp<T> getV4() {
        return v4;
    }

    Sp<T> getV6() {
        return v6;
    }

    Sp<T> getValue(Network network) {
        switch (network) {
        case Network::IPv4:
            return v4;

        case Network::IPv6:
            return v6;
        }

        return nullptr;
    }

    bool isEmpty() {
        return v4 == nullptr && v6 == nullptr;
    }

    bool hasValue() {
        return v4 != nullptr || v6 != nullptr;
    }

    bool isComplete() {
        return v4 != nullptr && v6 != nullptr;
    }

    void setValue(Network network, Sp<T> value) {
        switch (network) {
        case Network::IPv4:
            v4 = value;
            break;

        case Network::IPv6:
            v6 = value;
            break;
        }
    }

private:
    Sp<T> v4 {nullptr};
    Sp<T> v6 {nullptr};
};

} // namespace boson
