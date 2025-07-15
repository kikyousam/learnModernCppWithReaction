#pragma once

#include <vector>
#include "reaction/concept.h"
#include <unordered_set>
#include "reaction/utility.h"

namespace reaction
{
    class ObserverNode : public std::enable_shared_from_this<ObserverNode>  //使用enable_shared_from_this来支持shared_ptr
    {
    public:
        ~ObserverNode() = default; //虚函数需要一个虚析构

        virtual void valueChanged() {
            this->notify();
        }

        void addObserver(ObserverNode* observer) {
            m_observers.emplace_back(observer);
        }

        template <typename... Args>
        void updateObserver(Args&&... args) {
            (void)(..., args.getPtr()->addObserver(this));
        }

        void notify() {
            for(auto observer: m_observers) {
                observer->valueChanged();
            }
        }
    private:
        std::vector<ObserverNode *> m_observers;
    };

    class ObserverGraph {  //管理类，全局单例
    public:
        static ObserverGraph& getInstance()
        {
            static ObserverGraph instance;
            return instance;
        }

        void addNode(NodePtr node)
        {
            m_nodes.insert(node);
        }

        void removeNode(NodePtr node)
        {
            m_nodes.erase(node);
        }
    private:
        ObserverGraph() = default;
        std::unordered_set<NodePtr> m_nodes;
    };

    class FieldGraph
    {
    public:
        static FieldGraph& getInstance()
        {
            static FieldGraph instance;
            return instance;
        }

        void addObj(const uint64_t &id, NodePtr node)
        {
            m_fieldMap[id].insert(node);
        }

        void deleteObj(const uint64_t &id)
        {
            m_fieldMap.erase(id);
        }

        void bindField(const uint64_t &id, NodePtr node)
        {
            if(!m_fieldMap.contains(id)) {
                return;
            }
            for(auto &n : m_fieldMap[id]) {
                n->addObserver(node.get());
            }
        }
    private:
        FieldGraph() = default;
        std::unordered_map<uint64_t, std::unordered_set<NodePtr>> m_fieldMap;
    };
}