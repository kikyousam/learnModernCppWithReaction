#pragma once

#include <vector>
#include "reaction/concept.h"
#include <unordered_set>

namespace reaction
{
    class ObserverNode
    {
    public:
        ~ObserverNode() = default; //虚函数需要一个虚析构

        virtual void valueChanged() {}

        void addObserver(ObserverNode* observer) {
            m_observers.emplace_back(observer);
        }

        template <typename... Args>
        void updateObserver(Args&&... args) {
            (void)(..., args.addObserver(this));
        }

        void notify() {
            for(auto observer: m_observers) {
                observer->valueChanged();
            }
        }
    private:
        std::vector<ObserverNode *> m_observers;
    };

    using NodePtr = std::shared_ptr<ObserverNode>;
    class ObserverGraph {  //管理类，全局单例
    public:
        static ObserverGraph& getINstance()
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
    }
}