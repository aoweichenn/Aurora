#pragma once

#include <utility>
#include "Aurora/ADT/SmallVector.h"

namespace aurora {

struct GraphEdge {
    unsigned target;
};

template <typename NodeTy>
class DirectedGraph {
public:
    using NodeType = NodeTy;
    using EdgeType = GraphEdge;

    DirectedGraph() = default;
    ~DirectedGraph() = default;

    [[nodiscard]] unsigned addNode(NodeTy data) {
        const unsigned id = numNodes();
        nodes_.push_back({std::move(data), {}, {}});
        return id;
    }

    [[nodiscard]] NodeTy& getNode(unsigned id) { return nodes_[id].data; }
    [[nodiscard]] const NodeTy& getNode(unsigned id) const { return nodes_[id].data; }
    [[nodiscard]] unsigned numNodes() const noexcept { return static_cast<unsigned>(nodes_.size()); }

    void addEdge(unsigned from, unsigned to) {
        nodes_[from].succs.push_back({to});
        nodes_[to].preds.push_back({from});
    }

    [[nodiscard]] const SmallVector<EdgeType, 4>& successors(unsigned nodeId) const { return nodes_[nodeId].succs; }
    [[nodiscard]] const SmallVector<EdgeType, 4>& predecessors(unsigned nodeId) const { return nodes_[nodeId].preds; }
    [[nodiscard]] unsigned numSuccessors(unsigned nodeId) const { return static_cast<unsigned>(nodes_[nodeId].succs.size()); }
    [[nodiscard]] unsigned numPredecessors(unsigned nodeId) const { return static_cast<unsigned>(nodes_[nodeId].preds.size()); }

    [[nodiscard]] SmallVector<unsigned, 16> reversePostOrder(unsigned entry) const;
    [[nodiscard]] SmallVector<unsigned, 16> postOrder(unsigned entry) const;

private:
    struct GraphNode {
        NodeTy data;
        SmallVector<EdgeType, 4> succs;
        SmallVector<EdgeType, 4> preds;
    };
    SmallVector<GraphNode, 32> nodes_;
};

template <typename NodeTy>
SmallVector<unsigned, 16> DirectedGraph<NodeTy>::reversePostOrder(const unsigned entry) const {
    auto po = postOrder(entry);
    SmallVector<unsigned, 16> rpo;
    rpo.reserve(po.size());
    for (auto i = static_cast<unsigned>(po.size()); i > 0; --i) rpo.push_back(po[i - 1]);
    return rpo;
}

template <typename NodeTy>
SmallVector<unsigned, 16> DirectedGraph<NodeTy>::postOrder(const unsigned entry) const {
    SmallVector<unsigned, 16> result;
    SmallVector<unsigned, 32> stack;
    SmallVector<unsigned char, 32> visited(numNodes(), 0);
    stack.push_back(entry);
    while (!stack.empty()) {
        unsigned node = stack.back();
        if (visited[node] == 0) {
            visited[node] = 1;
            for (auto& edge : nodes_[node].succs)
                if (visited[edge.target] == 0) stack.push_back(edge.target);
        } else if (visited[node] == 1) {
            visited[node] = 2; stack.pop_back(); result.push_back(node);
        } else {
            stack.pop_back();
        }
    }
    return result;
}

} // namespace aurora

