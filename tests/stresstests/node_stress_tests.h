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

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <carrier.h>

#define TEST_ALL 0

namespace test {

class NodeStressTests : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(NodeStressTests);
#if TEST_ALL
    CPPUNIT_TEST(testAll);
#else
    CPPUNIT_TEST(testFindNode);
    CPPUNIT_TEST(testAnnounceAndFindPeer);
    CPPUNIT_TEST(testStoreAndFindValue);
    CPPUNIT_TEST(testUpdateAndFindSignedValue);
    CPPUNIT_TEST(testUpdateAndFindEncryptedValue);
#endif
    CPPUNIT_TEST_SUITE_END();

public:
    NodeStressTests();

    void setUp();
    void tearDown();

    void testAll();
    void testFindNode();
    void testAnnounceAndFindPeer();
    void testStoreAndFindValue();
    void testUpdateAndFindSignedValue();
    void testUpdateAndFindEncryptedValue();

private:
    void prepareWorkingDirectory();
    void startBootstraps();
    void stopBootstraps();
    void startTestNodes();
    void stopTestNodes();
    void dumpRoutingTables();
    std::vector<uint8_t> stringToData(const std::string& str);

    std::shared_ptr<Node> node1 {};
    std::shared_ptr<Node> node2 {};
    std::shared_ptr<Node> node3 {};

    std::vector<Sp<Node>> bootstrapNodes {};
    std::vector<NodeInfo> bootstraps {};

    std::vector<Sp<Node>> testNodes {};

    DefaultConfiguration::Builder dcb {};

    std::string workingDir {};
};

}  // namespace test
