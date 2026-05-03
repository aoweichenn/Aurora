#include <gtest/gtest.h>
#include "Aurora/ADT/Graph.h"

using namespace aurora;

TEST(GraphTest, AddNode) {
    DirectedGraph<int> g;
    unsigned id = g.addNode(42);
    EXPECT_EQ(g.numNodes(), 1u);
    EXPECT_EQ(g.getNode(id), 42);
}

TEST(GraphTest, AddEdge) {
    DirectedGraph<int> g;
    unsigned a = g.addNode(10);
    unsigned b = g.addNode(20);
    g.addEdge(a, b);
    EXPECT_EQ(g.numSuccessors(a), 1u);
    EXPECT_EQ(g.numPredecessors(b), 1u);
}

TEST(GraphTest, SuccessorsAndPredecessors) {
    DirectedGraph<int> g;
    unsigned a = g.addNode(1);
    unsigned b = g.addNode(2);
    unsigned c = g.addNode(3);
    g.addEdge(a, b);
    g.addEdge(b, c);
    g.addEdge(a, c);

    EXPECT_EQ(g.numSuccessors(a), 2u);
    EXPECT_EQ(g.numPredecessors(a), 0u);
    EXPECT_EQ(g.numPredecessors(c), 2u);
}

TEST(GraphTest, PostOrder) {
    DirectedGraph<int> g;
    g.addNode(0);
    g.addNode(1);
    g.addNode(2);
    g.addNode(3);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 3);

    auto po = g.postOrder(0);
    EXPECT_EQ(po.size(), 4u);
    // Post order should have children before parents
    EXPECT_EQ(po[0], 3u);
    EXPECT_EQ(po[1], 2u);
    EXPECT_EQ(po[2], 1u);
    EXPECT_EQ(po[3], 0u);
}

TEST(GraphTest, ReversePostOrder) {
    DirectedGraph<int> g;
    g.addNode(0);
    g.addNode(1);
    g.addNode(2);
    g.addNode(3);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 3);

    auto rpo = g.reversePostOrder(0);
    EXPECT_EQ(rpo.size(), 4u);
    EXPECT_EQ(rpo[0], 0u);
    EXPECT_EQ(rpo[1], 1u);
    EXPECT_EQ(rpo[2], 2u);
    EXPECT_EQ(rpo[3], 3u);
}

TEST(GraphTest, DagPostOrder) {
    DirectedGraph<int> g;
    // Create a small DAG
    g.addNode(0); g.addNode(1); g.addNode(2); g.addNode(3); g.addNode(4);
    g.addEdge(0, 1); g.addEdge(0, 2);
    g.addEdge(1, 3); g.addEdge(2, 3);
    g.addEdge(3, 4);

    auto rpo = g.reversePostOrder(0);
    EXPECT_EQ(rpo.size(), 5u);
    EXPECT_EQ(rpo[0], 0u);
    EXPECT_EQ(rpo[4], 4u);
}
