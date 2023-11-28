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

#include "message_key.h"

namespace boson {

const std::string MessageKey::KEY_REQ_NODES4     = "n4";
const std::string MessageKey::KEY_REQ_NODES6     = "n6";

const std::string MessageKey::KEY_TYPE           = "y";
const std::string MessageKey::KEY_ID             = "i";
const std::string MessageKey::KEY_TXID           = "t";
const std::string MessageKey::KEY_VERSION        = "v";

const std::string MessageKey::KEY_REQUEST        = "q";
const std::string MessageKey::KEY_REQ_TARGET     = "t";
const std::string MessageKey::KEY_REQ_WANT       = "w";
const std::string MessageKey::KEY_REQ_PORT       = "p";
const std::string MessageKey::KEY_REQ_TOKEN      = "tok";
const std::string MessageKey::KEY_REQ_PUBLICKEY  = "k";
const std::string MessageKey::KEY_REQ_RECIPIENT  = "rec";
const std::string MessageKey::KEY_REQ_NONCE      = "n";
const std::string MessageKey::KEY_REQ_SIGNATURE  = "sig";
const std::string MessageKey::KEY_REQ_VALUE      = "v";
const std::string MessageKey::KEY_REQ_CAS        = "cas";
const std::string MessageKey::KEY_REQ_SEQ        = "seq";
const std::string MessageKey::KEY_REQ_PROXY_ID   = "x";
const std::string MessageKey::KEY_REQ_ALT        = "alt";

const std::string MessageKey::KEY_RESPONSE       = "r";
const std::string MessageKey::KEY_RES_NODES4     = "n4";
const std::string MessageKey::KEY_RES_NODES6     = "n6";
const std::string MessageKey::KEY_RES_TOKEN      = "tok";
const std::string MessageKey::KEY_RES_PEERS      = "p";
const std::string MessageKey::KEY_RES_PEERS4     = "p4";
const std::string MessageKey::KEY_RES_PEERS6     = "p6";
const std::string MessageKey::KEY_RES_PUBLICKEY  = "k";
const std::string MessageKey::KEY_RES_RECIPIENT  = "rec";
const std::string MessageKey::KEY_RES_NONCE      = "n";
const std::string MessageKey::KEY_RES_SIGNATURE  = "sig";
const std::string MessageKey::KEY_RES_VALUE      = "v";
const std::string MessageKey::KEY_RES_SEQ        = "seq";

const std::string MessageKey::KEY_ERROR          = "e";
const std::string MessageKey::KEY_ERR_CODE       = "c";
const std::string MessageKey::KEY_ERR_MESSAGE    = "m";

} // namespace boson
