#pragma once

#include <vector>
#include "reaction/concept.h"

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
}