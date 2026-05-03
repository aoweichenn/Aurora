#ifndef AURORA_ADT_GRAPH_H
#define AURORA_ADT_GRAPH_H

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

    unsigned addNode(NodeTy data) {
        unsigned id = numNodes();
        nodes_.push_back({std::move(data), {}, {}});
        return id;
    }

    NodeTy& getNode(unsigned id) { return nodes_[id].data; }
    const NodeTy& getNode(unsigned id) const { return nodes_[id].data; }
    unsigned numNodes() const noexcept { return static_cast<unsigned>(nodes_.size()); }

    void addEdge(unsigned from, unsigned to) {
        nodes_[from].succs.push_back({to});
        nodes_[to].preds.push_back({from});
    }

    const SmallVector<EdgeType, 4>& successors(unsigned nodeId) const { return nodes_[nodeId].succs; }
    const SmallVector<EdgeType, 4>& predecessors(unsigned nodeId) const { return nodes_[nodeId].preds; }
    unsigned numSuccessors(unsigned nodeId) const { return static_cast<unsigned>(nodes_[nodeId].succs.size()); }
    unsigned numPredecessors(unsigned nodeId) const { return static_cast<unsigned>(nodes_[nodeId].preds.size()); }

    SmallVector<unsigned, 16> reversePostOrder(unsigned entry) const;
    SmallVector<unsigned, 16> postOrder(unsigned entry) const;

private:
    struct GraphNode {
        NodeTy data;
        SmallVector<EdgeType, 4> succs;
        SmallVector<EdgeType, 4> preds;
    };
    SmallVector<GraphNode, 32> nodes_;
};

template <typename NodeTy>
SmallVector<unsigned, 16> DirectedGraph<NodeTy>::postOrder(unsigned entry) const {
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

template <typename NodeTy>
SmallVector<unsigned, 16> DirectedGraph<NodeTy>::reversePostOrder(unsigned entry) const {
    auto po = postOrder(entry);
    SmallVector<unsigned, 16> rpo;
    rpo.reserve(po.size());
    for (unsigned i = po.size(); i > 0; --i) rpo.push_back(po[i - 1]);
    return rpo;
}

} // namespace aurora

#endif // AURORA_ADT_GRAPH_H
