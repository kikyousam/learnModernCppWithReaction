#pragma once

#include "reaction/expression.h"

namespace reaction
{
    template<typename Type, typename... Args>
    class ReactImpl : public Expression<Type, Args...>  //用来和用户交互, 采用继承的方式表示is a的关系
    {  //实现类
    public:
        using ExprType = Expression<Type, Args...>::ExprType;
        using valueType = Expression<Type, Args...>::valueType;
        ReactImpl(const ReactImpl& d) {}
        using Expression<Type, Args...>::Expression;
        auto get() const {
            return this->getValue();
        }

        template <typename T>
            requires Convertable<T, valueType> && IsVarExpr<ExprType>
        void value(T &&t) {
            this->updateValue(std::forward<T>(t));
            this->notify();
        }
    };

    template <typename ReactType>
    class React  //管理类
    {
    public:
        explicit React(std::shared_ptr<ReactType> ptr = nullptr) : m_weakPtr(ptr) {}
    private:
        std::weak_ptr<ReactImpl> m_observer;
    };

    template <typename SrcType>
    auto var(SrcType &&t) {
        auto ptr = std::make_shared<ReactImpl<std::decay_t<SrcType>>>(std::forward<SrcType>(t));
        ObserverGraph::getInstance().addNode(ptr);
        return React(ptr);

    template <typename Func, typename... Args>
    auto calc(Func &&fun, Args &&...args) {
    }   auto ptr = std::make_shared<ReactImpl<std::decay_t<Func>, std::decay_t<Args>...>>(std::forward<Func>(fun), std::forward<Args>(args)...);
        ObserverGraph::getInstance().addNode(ptr);
        return React(ptr);
}