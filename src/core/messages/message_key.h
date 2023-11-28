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

#include <string>

namespace boson {

struct MessageKey {
public:
    static const std::string KEY_REQ_NODES4;
    static const std::string KEY_REQ_NODES6;

    static const std::string KEY_TYPE;
    static const std::string KEY_ID;
    static const std::string KEY_TXID ;
    static const std::string KEY_VERSION;

    static const std::string KEY_REQUEST;
    static const std::string KEY_REQ_TARGET;
    static const std::string KEY_REQ_WANT;
    static const std::string KEY_REQ_PORT;
    static const std::string KEY_REQ_TOKEN;
    static const std::string KEY_REQ_PUBLICKEY;
    static const std::string KEY_REQ_RECIPIENT;
    static const std::string KEY_REQ_NONCE;
    static const std::string KEY_REQ_SIGNATURE;
    static const std::string KEY_REQ_VALUE;
    static const std::string KEY_REQ_CAS;
    static const std::string KEY_REQ_SEQ;
    static const std::string KEY_REQ_PROXY_ID;
    static const std::string KEY_REQ_ALT;

    static const std::string KEY_RESPONSE;
    static const std::string KEY_RES_NODES4;
    static const std::string KEY_RES_NODES6;
    static const std::string KEY_RES_TOKEN;
    static const std::string KEY_RES_PEERS;
    static const std::string KEY_RES_PEERS4;
    static const std::string KEY_RES_PEERS6;
    static const std::string KEY_RES_PUBLICKEY;
    static const std::string KEY_RES_RECIPIENT;
    static const std::string KEY_RES_NONCE;
    static const std::string KEY_RES_SIGNATURE;
    static const std::string KEY_RES_VALUE;
    static const std::string KEY_RES_SEQ;

    static const std::string KEY_ERROR;
    static const std::string KEY_ERR_CODE;
    static const std::string KEY_ERR_MESSAGE;
};

} // namespace boson
