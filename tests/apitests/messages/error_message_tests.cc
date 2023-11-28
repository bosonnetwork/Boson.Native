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

#include "messages/message.h"
#include "messages/error_message.h"

#include "utils.h"
#include "error_message_tests.h"

namespace test {
CPPUNIT_TEST_SUITE_REGISTRATION(ErrorMessageTests);

void ErrorMessageTests::testErrorMessageSize() {
    auto nodeId =Id::random();
    std::string error;
    error.assign(1025, 'E');

    auto msg = ErrorMessage(Message::Method::PING, 0xF7654321, 0x87654321, error);
    msg.setId(nodeId);
    msg.setVersion(VERSION);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::ERR);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::PING);
    CPPUNIT_ASSERT(msg.getId() == nodeId);
    CPPUNIT_ASSERT(msg.getVersion() == VERSION);

    auto serialized = msg.serialize();
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());
}

void ErrorMessageTests::testErrorMessage() {
    auto nodeId = Id::random();
    int txid = Utils::getRandomValue();
    int code = Utils::getRandomValue();
    auto error = "Test error message";

    auto msg = ErrorMessage(Message::Method::PING, txid, code, error);
    msg.setId(nodeId);
    msg.setVersion(VERSION);

    auto serialized = msg.serialize();
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());

    auto parsed = Message::parse(serialized.data(), serialized.size());
    auto _msg = std::static_pointer_cast<ErrorMessage>(parsed);

    CPPUNIT_ASSERT(_msg->getType() == Message::Type::ERR);
    CPPUNIT_ASSERT(_msg->getMethod() == Message::Method::PING);
    CPPUNIT_ASSERT(_msg->getTxid() == txid);
    CPPUNIT_ASSERT(_msg->getCode() == code);
    CPPUNIT_ASSERT(_msg->getMessage() == error);
    CPPUNIT_ASSERT(_msg->getReadableVersion() == VERSION_STR);
}

void ErrorMessageTests::testErrorMessagei18n() {
    int txid = Utils::getRandomValue();
    int code = Utils::getRandomValue();
    std::string error = "错误信息；エラーメッセージ；에러 메시지；Message d'erreur";

    auto msg = ErrorMessage(Message::Method::UNKNOWN, txid, code, error);
    msg.setId(Id::random());
    msg.setVersion(VERSION);

    auto serialized = msg.serialize();
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());

    auto parsed = Message::parse(serialized.data(), serialized.size());
    auto _msg = std::static_pointer_cast<ErrorMessage>(parsed);

    CPPUNIT_ASSERT(_msg->getType() == Message::Type::ERR);
    CPPUNIT_ASSERT(_msg->getMethod() == Message::Method::UNKNOWN);
    CPPUNIT_ASSERT(_msg->getTxid() == txid);
    CPPUNIT_ASSERT(_msg->getCode() == code);
    CPPUNIT_ASSERT(_msg->getMessage() == error);
    CPPUNIT_ASSERT(_msg->getReadableVersion() == VERSION_STR);
}

}

