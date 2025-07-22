#pragma once

#include "reaction/utility.h"
#include <functional>

namespace reaction {

inline thread_local NodeSet g_delay_list; // 延迟注册的观察者列表
class ObserverGraph {                     // 管理类，全局单例
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

        hasRepeatDependency(source, target);
        m_observerList.at(target).get().insert(source);
        m_dependentList.at(source).insert(target);
    }

    void addNode(NodePtr node);

    void removeNode(NodePtr node) {
        m_observerList.erase(node);
        m_dependentList.erase(node);
    }

    void setName(NodePtr node, const std::string &name) {
        m_nameList[node] = name;
    }

    std::string getName(NodePtr node) {
        if (m_nameList.contains(node)) {
            return m_nameList[node];
        } else {
            return "";
        }
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

    void hasRepeatDependency(NodePtr source, NodePtr target) {
        NodeSet dependencies;
        collectDependencies(target, dependencies);

        NodeSet visited;
        for (auto dependency : m_dependentList.at(source)) {
            checkDependency(source, dependency.lock(), dependencies, visited);
        }
    }

    void checkDependency(NodePtr source, NodePtr node, NodeSet targetDependencies, NodeSet &visited) {
        if (visited.contains(node)) {
            return; // 已经访问过，避免重复检查
        }
        visited.insert(node);

        if (targetDependencies.contains(node)) {
            if (m_repeatList.at(node).get().contains(source)) {
                m_repeatList.at(node).get()[source]++;
            } else {
                m_repeatList.at(node).get()[source] = 2; // 初始计数为2，表示重复依赖
            }
        }

        for (auto &neighbor : m_dependentList.at(node)) {
            checkDependency(source, neighbor.lock(), targetDependencies, visited);
        }
    }
    void collectDependencies(NodePtr node, NodeSet &dependencies) {
        NodeMap dependenciesMap;
        collectDependencies(node, dependenciesMap);

        for (auto &[depNode, count] : dependenciesMap) {
            if (count == 1) {
                dependencies.insert(depNode);
            }
        }
    }

    void collectDependencies(NodePtr node, NodeMap &dependencies) {
        if (!node) return;

        if (dependencies.contains(node)) {
            dependencies[node]++;
        } else {
            dependencies[node] = 1;
        }

        for (auto &neighbor : m_dependentList.at(node)) {
            collectDependencies(neighbor.lock(), dependencies);
        }
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
            if (dfs(neighbor.lock(), visited, stack)) {
                return true;
            }
        }

        stack.erase(node);
        return false;
    }

    ObserverGraph() = default;
    std::unordered_map<NodePtr, NodeSetRef> m_observerList;
    std::unordered_map<NodePtr, NodeSet> m_dependentList;
    std::unordered_map<NodePtr, NodeMapRef> m_repeatList; // 用于存储重复的观察者
    std::unordered_map<NodePtr, std::string> m_nameList;
};

class ObserverNode : public std::enable_shared_from_this<ObserverNode> // 使用enable_shared_from_this来支持shared_ptr
{
public:
    ~ObserverNode() = default; // 虚函数需要一个虚析构

    virtual void valueChanged(bool changed = true) {
        this->notify(changed);
    }

    template <typename... Args>
    void updateObserver(Args &&...args) {
        auto self = this->shared_from_this();
        (ObserverGraph::getInstance().addObserver(self, args), ...);
    }

    void notify(bool changed = true) {
        for (auto &[repeat, _] : m_repeats) {
            g_delay_list.insert(repeat.lock());
        }

        for (auto observer : m_observers) {
            if (!g_delay_list.contains(observer)) {
                if (auto obsPtr = observer.lock()) {
                    obsPtr->valueChanged(changed);
                }
            }
        }

        if (!g_delay_list.empty()) {
            for (auto &[repeat, _] : m_repeats) {
                g_delay_list.erase(repeat.lock());
            }

            for (auto &[repeat, _] : m_repeats) {
                if (auto obsPtr = repeat.lock()) {
                    obsPtr->valueChanged(changed);
                }
            }
        }
    }

private:
    NodeSet m_observers;
    NodeMap m_repeats; // 用于存储重复的观察者

    friend class ObserverGraph; // 允许ObserverGraph访问私有成员
};

inline void ObserverGraph::addNode(NodePtr node) {
    m_observerList.insert({node, std::ref(node->m_observers)});
    m_dependentList.insert({node, NodeSet{}});
    m_repeatList.insert({node, std::ref(node->m_repeats)});
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
            ObserverGraph::getInstance().addObserver(node, n.lock());
        }
    }

private:
    FieldGraph() = default;
    std::unordered_map<uint64_t, NodeSet> m_fieldMap;
};
} // namespace reaction