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

#include "messages/message.h"
#include "messages/store_value_request.h"
#include "messages/store_value_response.h"

#include "utils.h"
#include "store_value_tests.h"

namespace test {
CPPUNIT_TEST_SUITE_REGISTRATION(StoreValueTests);

void StoreValueTests::testStoreValueRequestSize() {
    std::vector<uint8_t> data(1025, 'D');
    auto value = Value::of({}, {}, {}, {}, -1, {}, data);

    auto origin = Id::random();
    auto msg = StoreValueRequest();
    msg.setId(origin);
    msg.setTxid(0x87654321);
    msg.setVersion(VERSION);
    msg.setToken(0x88888888);
    msg.setValue(value);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::REQUEST);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::STORE_VALUE);
    CPPUNIT_ASSERT(msg.getId() == origin);
    CPPUNIT_ASSERT(msg.getTxid() == 0x87654321);
    CPPUNIT_ASSERT(msg.getVersion() == VERSION);
    CPPUNIT_ASSERT(msg.getToken() == 0x88888888);
    CPPUNIT_ASSERT(msg.getValue() == value);
    CPPUNIT_ASSERT(msg.getExpectedSequenceNumber() == -1);

    auto serialized = msg.serialize();
    printMessage(msg, serialized);
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());
}

void StoreValueTests::testStoreSignedValueRequestSize() {
    std::vector<uint8_t> nonce(24, 'N');
    std::vector<uint8_t> sig(64, 'S');
    std::vector<uint8_t> data(1025, 'D');
    Id pk = Id::random();
    int seq = 0x77654321;
    Value value = Value::of(pk.blob(), {}, {}, nonce, seq, sig, data);

    auto origin = Id::random();
    StoreValueRequest msg = StoreValueRequest();
    msg.setId(origin);
    msg.setTxid(0x87654321);
    msg.setVersion(VERSION);
    msg.setToken(0x88888888);
    msg.setExpectedSequenceNumber(seq - 1);
    msg.setValue(value);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::REQUEST);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::STORE_VALUE);
    CPPUNIT_ASSERT(msg.getId() == origin);
    CPPUNIT_ASSERT(msg.getTxid() == 0x87654321);
    CPPUNIT_ASSERT(msg.getVersion() == VERSION);
    CPPUNIT_ASSERT(msg.getToken() == 0x88888888);
    CPPUNIT_ASSERT(msg.getValue() == value);
    CPPUNIT_ASSERT(msg.getExpectedSequenceNumber() == seq-1);

    auto serialized = msg.serialize();
    printMessage(msg, serialized);
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());
}

void StoreValueTests::testStoreEncryptedValueRequestSize() {
    std::vector<uint8_t> nonce(24, 'N');
    std::vector<uint8_t> sig(64, 'S');
    std::vector<uint8_t> data(1025, 'D');
    int seq = 0x77654321;
    Value value = Value::of(Id::random().blob(), {}, Id::random().blob(), nonce, seq, sig, data);

    auto origin = Id::random();
    StoreValueRequest msg = StoreValueRequest();
    msg.setId(origin);
    msg.setTxid(0x87654321);
    msg.setVersion(VERSION);
    msg.setToken(0x88888888);
    msg.setExpectedSequenceNumber(seq - 1);
    msg.setValue(value);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::REQUEST);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::STORE_VALUE);
    CPPUNIT_ASSERT(msg.getId() == origin);
    CPPUNIT_ASSERT(msg.getTxid() == 0x87654321);
    CPPUNIT_ASSERT(msg.getVersion() == VERSION);
    CPPUNIT_ASSERT(msg.getToken() == 0x88888888);
    CPPUNIT_ASSERT(msg.getValue() == value);
    CPPUNIT_ASSERT(msg.getExpectedSequenceNumber() == seq-1);

    auto serialized = msg.serialize();
    printMessage(msg, serialized);
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());
}

void StoreValueTests::testStoreValueRequest() {
    auto nodeId = Id::random();
    int txid = Utils::getRandomInteger(62);
    int token = Utils::getRandomValue();
    std::vector<uint8_t> data(1025);
    Random::buffer(data);

    Value value = Value::of({}, {}, {}, {}, 0, {}, data);

    auto msg = StoreValueRequest();
    msg.setId(nodeId);
    msg.setTxid(txid);
    msg.setVersion(VERSION);
    msg.setToken(token);
    msg.setValue(value);

    auto serialized = msg.serialize();
    printMessage(msg, serialized);

    auto parsed = Message::parse(serialized.data(), serialized.size());
    parsed->setId(nodeId);
    auto _msg = std::static_pointer_cast<StoreValueRequest>(parsed);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::REQUEST);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::STORE_VALUE);
    CPPUNIT_ASSERT(msg.getId() == nodeId);
    CPPUNIT_ASSERT(msg.getTxid() == txid);
    CPPUNIT_ASSERT(msg.getReadableVersion() == VERSION_STR);
    CPPUNIT_ASSERT(msg.getToken() == token);
    CPPUNIT_ASSERT(msg.getValue() == value);
    CPPUNIT_ASSERT(msg.getExpectedSequenceNumber() == -1);
}

void StoreValueTests::testStoreSignedValueRequest() {
    auto nodeId = Id::random();
    int txid = Utils::getRandomInteger(62);
    auto pk = Id::random();
    auto nonce = CryptoBox::Nonce::random();
    int cas = Utils::getRandomInteger(62);
    int seq = cas + 1;
    std::vector<uint8_t> sig(64);
    Utils::setRandomBytes(sig.data(), sig.size());
    int token = Utils::getRandomValue();
    std::vector<uint8_t> data(1025);
    Random::buffer(data);

    Value value = Value::of(pk.blob(), {}, {}, nonce.blob(), seq, sig, data);
    auto msg = StoreValueRequest();
    msg.setId(nodeId);
    msg.setTxid(txid);
    msg.setVersion(VERSION);
    msg.setToken(token);
    msg.setExpectedSequenceNumber(cas);
    msg.setValue(value);

    auto serialized = msg.serialize();
    auto parsed = Message::parse(serialized.data(), serialized.size());
    parsed->setId(nodeId);
    auto _msg = std::static_pointer_cast<StoreValueRequest>(parsed);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::REQUEST);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::STORE_VALUE);
    CPPUNIT_ASSERT(msg.getId() == nodeId);
    CPPUNIT_ASSERT(msg.getTxid() == txid);
    CPPUNIT_ASSERT(msg.getReadableVersion() == VERSION_STR);
    CPPUNIT_ASSERT(msg.getToken() == token);
    CPPUNIT_ASSERT(msg.getValue() == value);
    CPPUNIT_ASSERT(msg.getExpectedSequenceNumber() == cas);
}

void StoreValueTests::testStoreEncryptedValueRequest() {
    auto nodeId = Id::random();
    int txid = Utils::getRandomInteger(62);
    auto pk = Id::random();
    auto recipient = Id::random();
    auto nonce = CryptoBox::Nonce::random();
    int cas = Utils::getRandomInteger(62);
    int seq = cas + 1;
    std::vector<uint8_t> sig(64);
    Utils::setRandomBytes(sig.data(), sig.size());
    int token = Utils::getRandomValue();
    std::vector<uint8_t> data(1025);
    Random::buffer(data);

    Value value = Value::of(pk.blob(), {}, recipient.blob(), nonce.blob(), seq, sig, data);
    auto msg = StoreValueRequest();
    msg.setId(nodeId);
    msg.setTxid(txid);
    msg.setVersion(VERSION);
    msg.setToken(token);
    msg.setExpectedSequenceNumber(cas);
    msg.setValue(value);

    auto serialized = msg.serialize();
    auto parsed = Message::parse(serialized.data(), serialized.size());
    parsed->setId(nodeId);
    auto _msg = std::static_pointer_cast<StoreValueRequest>(parsed);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::REQUEST);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::STORE_VALUE);
    CPPUNIT_ASSERT(msg.getId() == nodeId);
    CPPUNIT_ASSERT(msg.getTxid() == txid);
    CPPUNIT_ASSERT(msg.getReadableVersion() == VERSION_STR);
    CPPUNIT_ASSERT(msg.getToken() == token);
    CPPUNIT_ASSERT(msg.getValue() == value);
    CPPUNIT_ASSERT(msg.getExpectedSequenceNumber() == cas);
}

void StoreValueTests::testStoreValueResponseSize() {
    auto id = Id::random();
    auto msg = StoreValueResponse(0xf7654321);
    msg.setId(id);
    msg.setTxid(0x87654321);
    msg.setVersion(VERSION);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::RESPONSE);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::STORE_VALUE);
    CPPUNIT_ASSERT(msg.getId() == id);
    CPPUNIT_ASSERT(msg.getTxid() == 0x87654321);
    CPPUNIT_ASSERT(msg.getVersion() == VERSION);

    auto serialized = msg.serialize();
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());
}

void StoreValueTests::testStoreValueResponse() {
    auto id = Id::random();
    int txid = Utils::getRandomInteger(62);
    auto msg = StoreValueResponse(txid);
    msg.setId(id);

    auto serialized = msg.serialize();
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());

    auto parsed = Message::parse(serialized.data(), serialized.size());
    parsed->setId(id);
    auto _msg = std::static_pointer_cast<StoreValueResponse>(parsed);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::RESPONSE);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::STORE_VALUE);
    CPPUNIT_ASSERT(msg.getId() == id);
    CPPUNIT_ASSERT(msg.getTxid() == txid);
    CPPUNIT_ASSERT(msg.getVersion() == 0);
}

}
