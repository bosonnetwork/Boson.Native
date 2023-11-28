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
#include "messages/announce_peer_request.h"
#include "messages/announce_peer_response.h"

#include "utils.h"
#include "announce_peer_tests.h"

namespace test {
CPPUNIT_TEST_SUITE_REGISTRATION(AnnouncePeerTests);

void AnnouncePeerTests::testAnnouncePeerRequestSize() {
    auto origin = Id::random();
    std::vector<uint8_t> sig(64);
    auto peer = PeerInfo::of(Id::random().blob(), {}, Id::random().blob(), {}, 65535, {}, sig);

    auto msg = AnnouncePeerRequest();
    msg.setId(origin);
    msg.setTxid(0x87654321);
    msg.setToken(0x88888888);
    msg.setVersion(VERSION);
    msg.setPeer(peer);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::REQUEST);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::ANNOUNCE_PEER);
    CPPUNIT_ASSERT(msg.getId() == origin);
    CPPUNIT_ASSERT(msg.getTxid() == 0x87654321);
    CPPUNIT_ASSERT(msg.getVersion() == VERSION);
    CPPUNIT_ASSERT(msg.getToken() == 0x88888888);
    CPPUNIT_ASSERT(msg.getPeer() == peer);

    auto serialized = msg.serialize();
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());
}

void AnnouncePeerTests::testAnnouncePeerRequestSize2() {
    auto origin = Id::random();
    std::vector<uint8_t> sig(64);
    Random::buffer(sig.data(), sig.size());
    auto peer = PeerInfo::of(Id::random().blob(), {}, Id::random().blob(), {}, 65535, "https://abc.pc2.net", sig);

    auto msg = AnnouncePeerRequest();
    msg.setId(origin);
    msg.setTxid(0x87654321);
    msg.setToken(0x88888888);
    msg.setVersion(VERSION);
    msg.setPeer(peer);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::REQUEST);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::ANNOUNCE_PEER);
    CPPUNIT_ASSERT(msg.getId() == origin);
    CPPUNIT_ASSERT(msg.getTxid() == 0x87654321);
    CPPUNIT_ASSERT(msg.getVersion() == VERSION);
    CPPUNIT_ASSERT(msg.getToken() == 0x88888888);
    CPPUNIT_ASSERT(msg.getPeer() == peer);

    auto bin = msg.serialize();
    CPPUNIT_ASSERT(bin.size() <= msg.estimateSize());
}

void AnnouncePeerTests::testAnnouncePeerRequest() {
    auto nodeId = Id::random();
    auto peerId = Id::random();
    int txid  = Utils::getRandomValue();
    int token = Utils::getRandomValue();

    uint16_t port  = Utils::getRandom(1, 65535);
    std::vector<uint8_t> sig(64);
    Random::buffer(sig.data(), sig.size());
    auto peer = PeerInfo::of(peerId.blob(), {}, nodeId.blob(), {}, port, {}, sig);

    auto msg = AnnouncePeerRequest();
    msg.setId(nodeId);
    msg.setTxid(txid);
    msg.setToken(token);
    msg.setVersion(VERSION);
    msg.setPeer(peer);

    auto serialized = msg.serialize();
    auto parsed = Message::parse(serialized.data(), serialized.size());
    parsed->setId(nodeId);
    auto _msg = std::static_pointer_cast<AnnouncePeerRequest>(parsed);

    CPPUNIT_ASSERT(_msg->getType() == Message::Type::REQUEST);
    CPPUNIT_ASSERT(_msg->getMethod() == Message::Method::ANNOUNCE_PEER);
    CPPUNIT_ASSERT(_msg->getId() == nodeId);
    CPPUNIT_ASSERT(_msg->getTxid() == txid);
    CPPUNIT_ASSERT(_msg->getReadableVersion() == VERSION_STR);
    CPPUNIT_ASSERT(_msg->getToken() == token);
    CPPUNIT_ASSERT(_msg->getPeer() == peer);
}

void AnnouncePeerTests::testAnnouncePeerRequest2() {
    auto nodeId = Id::random();
    auto origin = Id::random();
    auto peerId = Id::random();
    auto txid  = Utils::getRandomValue();
    auto port  = Utils::getRandom(1, 65535);
    auto token = Utils::getRandomValue();

    std::vector<uint8_t> sig(64);
    Random::buffer(sig.data(), sig.size());
    auto peer = PeerInfo::of(peerId.blob(), {}, nodeId.blob(), origin.blob(), port, "http://abc.pc2.net/", sig);

    auto msg = AnnouncePeerRequest();
    msg.setId(origin);
    msg.setTxid(txid);
    msg.setToken(token);
    msg.setVersion(VERSION);
    msg.setPeer(peer);

    auto serialized = msg.serialize();
    auto parsed = Message::parse(serialized.data(), serialized.size());
    parsed->setId(origin);
    auto _msg = std::static_pointer_cast<AnnouncePeerRequest>(parsed);

    CPPUNIT_ASSERT(_msg->getType() == Message::Type::REQUEST);
    CPPUNIT_ASSERT(_msg->getMethod() == Message::Method::ANNOUNCE_PEER);
    CPPUNIT_ASSERT(_msg->getId() == origin);
    CPPUNIT_ASSERT(_msg->getTxid() == txid);
    CPPUNIT_ASSERT(_msg->getReadableVersion() == VERSION_STR);
    CPPUNIT_ASSERT(_msg->getToken() == token);
    CPPUNIT_ASSERT(_msg->getPeer() == peer);
}

void AnnouncePeerTests::testAnnouncePeerResponseSize() {
    auto origin = Id::random();
    auto msg = AnnouncePeerResponse(0xf7654321);
    msg.setId(origin);
    msg.setTxid(0x87654321);
    msg.setVersion(VERSION);

    CPPUNIT_ASSERT(msg.getType() == Message::Type::RESPONSE);
    CPPUNIT_ASSERT(msg.getMethod() == Message::Method::ANNOUNCE_PEER);
    CPPUNIT_ASSERT(msg.getId() == origin);
    CPPUNIT_ASSERT(msg.getTxid() == 0x87654321);
    CPPUNIT_ASSERT(msg.getVersion() == VERSION);

    auto serialized = msg.serialize();
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());
}

void AnnouncePeerTests::testAnnouncePeerResponse() {
    auto id = Id::random();
    int txid = Utils::getRandomValue();

    auto msg = AnnouncePeerResponse(txid);
    msg.setId(id);
    msg.setVersion(VERSION);

    auto serialized = msg.serialize();
    CPPUNIT_ASSERT(serialized.size() <= msg.estimateSize());

    auto parsed = Message::parse(serialized.data(), serialized.size());
    parsed->setId(id);
    auto _msg = std::static_pointer_cast<AnnouncePeerResponse>(parsed);

    CPPUNIT_ASSERT(_msg->getType() == Message::Type::RESPONSE);
    CPPUNIT_ASSERT(_msg->getMethod() == Message::Method::ANNOUNCE_PEER);
    CPPUNIT_ASSERT(_msg->getId() == id);
    CPPUNIT_ASSERT(_msg->getTxid() == txid);
    CPPUNIT_ASSERT(_msg->getReadableVersion() == VERSION_STR);
}

}
