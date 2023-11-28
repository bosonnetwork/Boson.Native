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

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

#include <iostream>
#include <string>
#include <cctype>
//#include <algorithm>

#include <boson.h>
#include "utils.h"
#include "node_stress_tests.h"

using namespace boson;

namespace test {

static const int BOOTSTRAP_NODES_PORT_START = 39100;
static const int TEST_NODES_PORT_START = 39200;

static const int BOOTSTRAP_NODES = 8;
static const int TEST_NODES = 1024;

static const int BOOTSTRAP_INTERVAL = 5; //seconds
static const int NODES_STARTED_WAIT = 20; //seconds

static const std::string TEST_DIR = "stress_tests" + Utils::PATH_SEP;
static int test_num = 0;
static std::vector<std::string> dir_lists {
    "testFindNode",
    "testAnnounceAndFindPeer",
    "testStoreAndFindValue",
    "testUpdateAndFindSignedValue",
    "testUpdateAndFindEncryptedValue"};

CPPUNIT_TEST_SUITE_REGISTRATION(NodeStressTests);

NodeStressTests::NodeStressTests() {
    auto path = Utils::getPwdStorage(TEST_DIR);
    Utils::removeStorage(path);

    workingDir = "stressTest" + Utils::PATH_SEP;

    std::string localAddr = Utils::getLocalIpAddresses();
    dcb.setIPv4Address(localAddr);
}

void NodeStressTests::prepareWorkingDirectory() {
#if TEST_ALL
    workingDir = TEST_DIR;
#else
    workingDir = TEST_DIR + dir_lists[test_num++] + Utils::PATH_SEP;
#endif
    auto path = Utils::getPwdStorage(workingDir);
    Utils::removeStorage(path);
}

void NodeStressTests::startBootstraps() {
    bootstrapNodes.clear();
    bootstraps.clear();
    for (int i = 0; i < BOOTSTRAP_NODES; i++) {
        printf("\n\n\007🟢 Starting the bootstrap node %d ...\n", i);

        std::string dir = workingDir +  "bootstraps" + Utils::PATH_SEP + "node-" + std::to_string(i);
        auto path = Utils::getPwdStorage(dir);

        dcb.setListeningPort(BOOTSTRAP_NODES_PORT_START + i);
        dcb.setStoragePath(path);

        auto config = dcb.build();
        auto bootstrap = std::make_shared<Node>(config);
        bootstrap->start();

        bootstrapNodes.emplace_back(bootstrap);
        bootstraps.emplace_back(*(bootstrap->getNodeInfo().getV4()));
    }

    int i = 0;
    for (auto& node : bootstrapNodes) {
        printf("\n\n\007⌛ Bootstraping the bootstrap node %d - %s ...\n", i, node->getId().toBase58String().c_str());
        node->bootstrap(bootstraps);
        sleep(BOOTSTRAP_INTERVAL);
        printf("\007🟢 The bootstrap node %d - %s is ready ...\n", i++, node->getId().toBase58String().c_str());
    }
}


void NodeStressTests::stopBootstraps() {
    puts("\n\n\007🟢 Stopping all the bootstrap nodes ...\n");

    for (auto& node : bootstrapNodes)
        node->stop();
}

void NodeStressTests::startTestNodes() {
    dcb.setBootstrap(bootstraps);

    for (int i = 0; i < TEST_NODES; i++) {
        printf("\007🟢 Starting the test node %d ...\n", i);

        std::string dir = workingDir +  "nodes" + Utils::PATH_SEP + "node-" + std::to_string(i);
        auto path = Utils::getPwdStorage(dir);
        Utils::removeStorage(path);

        dcb.setListeningPort(TEST_NODES_PORT_START + i);
        dcb.setStoragePath(dir);

        auto config = dcb.build();
        auto node = std::make_shared<Node>(config);
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();
        auto listener = std::make_shared<ConnectionStatusListener>();
        listener->profound = [=](Network network) {
            promise->set_value();
        };
		node->addConnectionStatusListener(listener);
        node->start();

        testNodes.emplace_back(node);
        printf("\007⌛ Wainting for the test node %d - %s ready ...\n", i, node->getId().toBase58String().c_str());
        future.get();
    }

    puts("\n\n\007⌛ Wainting for all the test nodes ready ...");
    sleep(NODES_STARTED_WAIT);
}

void NodeStressTests::stopTestNodes() {
    puts("\n\n\007🟢 Stopping all the test nodes...\n");

    for (auto& node : testNodes)
        node->stop();
}

void NodeStressTests::dumpRoutingTables() {
    int i = 0;
    for (auto& node : bootstrapNodes) {
        printf("\007🟢 Dumping the routing table of nodes %s ...\n", node->getId().toBase58String().c_str());
        auto routingtable = node->toString();
        std::string dir = workingDir +  "bootstraps" + Utils::PATH_SEP + "node-" + std::to_string(i) + Utils::PATH_SEP + "routingtable";
        auto path = Utils::getPwdStorage(dir);
        std::ofstream out(path);
        out << routingtable;
        i++;
    }

    i = 0;
    for (auto& node : testNodes) {
        printf("\007🟢 Dumping the routing table of nodes %s ...\n", node->getId().toBase58String().c_str());
        auto routingtable = node->toString();
        std::string dir = workingDir +  "nodes" + Utils::PATH_SEP + "node-" + std::to_string(i) + Utils::PATH_SEP + "routingtable";
        auto path = Utils::getPwdStorage(dir);
        std::ofstream out(path);
        out << routingtable;
        i++;
    }
}



void NodeStressTests::setUp() {
    prepareWorkingDirectory();
    startBootstraps();
    startTestNodes();

    puts("\n\n\007🟢 All the nodes are ready!!! starting to run the test cases");
}

void NodeStressTests::tearDown() {
    dumpRoutingTables();
    stopTestNodes();
    stopBootstraps();
}

void NodeStressTests::testFindNode() {
    for (int i = 0; i < TEST_NODES; i++) {
        auto target = testNodes[i];
        printf("\n\n\007🟢 Looking up node %s ...\n", target->getId().toBase58String().c_str());

        for (int j = 0; j < TEST_NODES; j++) {
            auto node = testNodes[j];
            printf("\n\n\007⌛ %s looking up node %s ...\n", node->getId().toBase58String().c_str(), target->getId().toBase58String().c_str());
            auto nis = node->findNode(target->getId()).get();
            printf("\007🟢 %s lookup node %s finished\n", node->getId().toBase58String().c_str(), target->getId().toBase58String().c_str());

            CPPUNIT_ASSERT(nis.hasValue());
            CPPUNIT_ASSERT_EQUAL(*(target->getNodeInfo().getV4()), *(nis.getV4()));
        }
    }
}

void NodeStressTests::testAnnounceAndFindPeer() {
    for (int i = 0; i < TEST_NODES; i++) {
        auto announcer = testNodes[i];
        auto p = PeerInfo::create(announcer->getId(), 8888);

        printf("\n\n\007🟢 %s announce peer %s ...\n", announcer->getId().toBase58String().c_str(), p.getId().toBase58String().c_str());
        announcer->announcePeer(p).get();

        printf("\n\n\007🟢 Looking up peer %s ...\n", p.getId().toBase58String().c_str());
        for (int j = 0; j < TEST_NODES; j++) {
            auto node = testNodes[j];
            printf("\n\n\007⌛ %s looking up peer %s ...\n", node->getId().toBase58String().c_str(), p.getId().toBase58String().c_str());
            auto result = node->findPeer(p.getId(), 0).get();
            printf("\007🟢 %s lookup peer %s finished\n", node->getId().toBase58String().c_str(), p.getId().toBase58String().c_str());

            CPPUNIT_ASSERT(!result.empty());
            CPPUNIT_ASSERT_EQUAL(1, (int)result.size());
            CPPUNIT_ASSERT_EQUAL(p, result[0]);
        }
    }
}

std::vector<uint8_t> NodeStressTests::stringToData(const std::string& str) {
    std::vector<uint8_t> data(str.size());
    data.assign(str.begin(), str.end());
    return data;
}

void NodeStressTests::testStoreAndFindValue() {
    for (int i = 0; i < TEST_NODES; i++) {
        auto announcer = testNodes[i];
        auto data = stringToData("Hello from " + announcer->getId().toBase58String());
        auto v = Value::createValue(data);

        printf("\n\n\007🟢 %s store value %s ...\n", announcer->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());
        announcer->storeValue(v).get();

        printf("\n\n\007🟢 Looking up value %s ...\n", v.getId().toBase58String().c_str());
        for (int j = 0; j < TEST_NODES; j++) {
            auto node = testNodes[j];
            printf("\n\n\007⌛ %s looking up value %s ...\n", node->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());
            auto result = node->findValue(v.getId()).get();
            printf("\007🟢 %s lookup value %s finished\n", node->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());

            CPPUNIT_ASSERT(result);
            CPPUNIT_ASSERT(v == *result);
        }
    }
}

void NodeStressTests::testUpdateAndFindSignedValue() {
    std::vector<Value> values(TEST_NODES);

    // initial announcement
    for (int i = 0; i < TEST_NODES; i++) {
        auto announcer = testNodes[i];
        auto peerKeyPair = Signature::KeyPair::random();
        auto nonce = CryptoBox::Nonce::random();
        auto data = stringToData("Hello from " + announcer->getId().toBase58String());
        auto v = Value::createSignedValue(peerKeyPair, nonce, data);
        values[i] = v;

        printf("\n\n\007🟢 %s store value %s ...\n", announcer->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());
        announcer->storeValue(v).get();

        printf("\n\n\007🟢 Looking up value %s ...\n", v.getId().toBase58String().c_str());
        for (int j = 0; j < TEST_NODES; j++) {
            auto node = testNodes[j];
            printf("\n\n\007⌛ %s looking up value %s ...\n", node->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());
            auto result = node->findValue(v.getId()).get();
            printf("\007🟢 %s lookup value %s finished\n", node->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());

            CPPUNIT_ASSERT(result);
            CPPUNIT_ASSERT(nonce == v.getNonce());
            CPPUNIT_ASSERT(Id(peerKeyPair.publicKey()) == v.getPublicKey());
            CPPUNIT_ASSERT(v.isMutable());
            CPPUNIT_ASSERT(v.isValid());
            CPPUNIT_ASSERT(v == *result);
        }
    }

    // update announcement
    for (int i = 0; i < TEST_NODES; i++) {
        auto announcer = testNodes[i];
        auto v = values[i];
        auto data = stringToData("Updated value from " + announcer->getId().toBase58String());
        v = v.update(data);
        values[i] = v;

        printf("\n\n\007🟢 %s update value %s ...\n", announcer->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());
        announcer->storeValue(v).get();

        printf("\n\n\007🟢 Looking up value %s ...\n", v.getId().toBase58String().c_str());
        for (int j = 0; j < TEST_NODES; j++) {
            auto node = testNodes[j];
            printf("\n\n\007⌛ %s looking up value %s ...\n", node->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());
            auto result = node->findValue(v.getId()).get();
            printf("\007🟢 %s lookup value %s finished\n", node->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());

            CPPUNIT_ASSERT(result);
            CPPUNIT_ASSERT(v.isMutable());
            CPPUNIT_ASSERT(v.isValid());
            CPPUNIT_ASSERT(v == *result);
        }
    }
}

void NodeStressTests::testUpdateAndFindEncryptedValue() {
    std::vector<Value> values(TEST_NODES);
    std::vector<Signature::KeyPair> recipients(TEST_NODES);

    // initial announcement
    for (int i = 0; i < TEST_NODES; i++) {
        auto announcer = testNodes[i];
        auto recipient =  Signature::KeyPair::random();
        recipients[i] = recipient;

        auto peerKeyPair =  Signature::KeyPair::random();
        auto nonce = CryptoBox::Nonce::random();
        auto data = stringToData("Hello from " + announcer->getId().toBase58String());
        auto v = Value::createEncryptedValue(peerKeyPair, Id(recipient.publicKey()), nonce, data);
        values[i] = v;

        printf("\n\n\007🟢 %s store value %s ...\n", announcer->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());
        announcer->storeValue(v).get();

        printf("\n\n\007🟢 Looking up value %s ...\n", v.getId().toBase58String().c_str());
        for (int j = 0; j < TEST_NODES; j++) {
            auto node = testNodes[j];
            printf("\n\n\007⌛ %s looking up value %s ...\n", node->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());
            auto result = node->findValue(v.getId()).get();
            printf("\007🟢 %s lookup value %s finished\n", node->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());

            CPPUNIT_ASSERT(result);
            CPPUNIT_ASSERT(nonce == v.getNonce());
            CPPUNIT_ASSERT(Id(peerKeyPair.publicKey()) == v.getPublicKey());
            CPPUNIT_ASSERT(v.isMutable());
            CPPUNIT_ASSERT(v.isEncrypted());
            CPPUNIT_ASSERT(v.isValid());
            CPPUNIT_ASSERT(v == *result);

            auto d = v.decryptData(recipient.privateKey());
            CPPUNIT_ASSERT(data == d);
        }
    }

    // update announcement
    for (int i = 0; i < TEST_NODES; i++) {
        auto announcer = testNodes[i];
        auto recipient = recipients[i];

        auto v = values[i];
        auto data = stringToData("Updated value  from " + announcer->getId().toBase58String());
        v = v.update(data);
        values[i] = v;

        printf("\n\n\007🟢 %s update value %s ...\n", announcer->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());
        announcer->storeValue(v).get();

        printf("\n\n\007🟢 Looking up value %s ...\n", v.getId().toBase58String().c_str());
        for (int j = 0; j < TEST_NODES; j++) {
            auto node = testNodes[j];
            printf("\n\n\007⌛ %s looking up value %s ...\n", node->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());
            auto result = node->findValue(v.getId()).get();
            printf("\007🟢 %s lookup value %s finished\n", node->getId().toBase58String().c_str(), v.getId().toBase58String().c_str());

            CPPUNIT_ASSERT(result);
            CPPUNIT_ASSERT(v.isMutable());
            CPPUNIT_ASSERT(v.isEncrypted());
            CPPUNIT_ASSERT(v.isValid());
            CPPUNIT_ASSERT(v == *result);

            auto d = v.decryptData(recipient.privateKey());
            CPPUNIT_ASSERT(data == d);
        }
    }
}

void NodeStressTests::testAll() {
    testFindNode();
    testAnnounceAndFindPeer();
    testStoreAndFindValue();
    testUpdateAndFindSignedValue();
    testUpdateAndFindEncryptedValue();
}

}  // namespace test
