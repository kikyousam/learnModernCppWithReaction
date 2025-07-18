#pragma once

#include "reaction/concept.h"
#include "reaction/utility.h"
#include <functional>

namespace reaction {

using NodeSet = std::unordered_set<NodePtr>;
using NodeSetRef = std::reference_wrapper<NodeSet>;

class ObserverGraph { // 管理类，全局单例
public:
    static ObserverGraph &getInstance() {
        static ObserverGraph instance;
        return instance;
    }

    void addObserver(NodePtr source, NodePtr target) {
        if (source == target) {
            throw std::runtime_error("Source and target cannot be the same node.");
        }

        if (hasCycle(source, target)) {
            throw std::runtime_error("Adding this observer would create a cycle in the graph.");
        }

        m_observerList.at(target).get().insert(source);
        m_dependentList.at(source).insert(target);
    }

    void addNode(NodePtr node);

    void removeNode(NodePtr node) {
        m_observerList.erase(node);
        m_dependentList.erase(node);
    }

private:
    bool hasCycle(NodePtr source, NodePtr target) {
        m_observerList.at(target).get().insert(source);
        m_dependentList.at(source).insert(target);
        NodeSet visited;
        NodeSet stack;

        bool isCycle = dfs(source, visited, stack);
        m_observerList.at(target).get().erase(source);
        m_dependentList.at(source).erase(target);
        return isCycle;
    }

    bool dfs(NodePtr node, NodeSet &visited, NodeSet &stack) {
        if (stack.contains(node)) {
            return true; // Cycle detected
        }
        if (visited.contains(node)) {
            return false; // Already visited
        }

        visited.insert(node);
        stack.insert(node);

        for (const auto &neighbor : m_observerList.at(node).get()) {
            if (dfs(neighbor, visited, stack)) {
                return true;
            }
        }

        stack.erase(node);
        return false;
    }

    ObserverGraph() = default;
    std::unordered_map<NodePtr, NodeSetRef> m_observerList;
    std::unordered_map<NodePtr, NodeSet> m_dependentList;
};

class ObserverNode : public std::enable_shared_from_this<ObserverNode> // 使用enable_shared_from_this来支持shared_ptr
{
public:
    ~ObserverNode() = default; // 虚函数需要一个虚析构

    virtual void valueChanged() {
        this->notify();
    }

    template <typename... Args>
    void updateObserver(Args &&...args) {
        auto self = this->shared_from_this();
        (ObserverGraph::getInstance().addObserver(self, args), ...);
    }

    void notify() {
        for (auto observer : m_observers) {
            observer->valueChanged();
        }
    }

private:
    NodeSet m_observers;

    friend class ObserverGraph; // 允许ObserverGraph访问私有成员
};

inline void ObserverGraph::addNode(NodePtr node) {
    m_observerList.insert({node, std::ref(node->m_observers)});
    m_dependentList.insert({node, NodeSet{}});
}
class FieldGraph {
public:
    static FieldGraph &getInstance() {
        static FieldGraph instance;
        return instance;
    }

    void addObj(const uint64_t &id, NodePtr node) {
        m_fieldMap[id].insert(node);
    }

    void deleteObj(const uint64_t &id) {
        m_fieldMap.erase(id);
    }

    void bindField(const uint64_t &id, NodePtr node) {
        if (!m_fieldMap.contains(id)) {
            return;
        }
        for (auto &n : m_fieldMap[id]) {
            ObserverGraph::getInstance().addObserver(node, n);
        }
    }

private:
    FieldGraph() = default;
    std::unordered_map<uint64_t, NodeSet> m_fieldMap;
};
} // namespace reaction