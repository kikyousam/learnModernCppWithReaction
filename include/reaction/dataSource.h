#pragma once

#include "reaction/expression.h"

namespace reaction
{
    template<typename Type, typename... Args>
    class DataSource : public Expression<Type, Args...>  //用来和用户交互, 采用继承的方式表示is a的关系
    {
    public:
        using ExprType = Expression<Type, Args...>::ExprType;
        using valueType = Expression<Type, Args...>::valueType;
        DataSource(const DataSource& d) {}
        using Expression<Type, Args...>::Expression;
        auto get() const {
            return this->getValue();
        }

        template <typename T>
            requires ConvertCC<T, valueType> && VarExprCC<ExprType>
        void value(T &&t) {
            this->updateValue(std::forward<T>(t));
            this->notify();
        }
    };

    template <typename T>
    auto var(T &&t) {
        return DataSource<std::decay_t<T>>(std::forward<T>(t)); //参数是万能引用，根据左值还是右值引用执行相应的构造函数
    }

    template <typename Func, typename... Args>
    auto calc(Func &&fun, Args &&...args) {
        return DataSource<std::decay_t<Func>, std::decay_t<Args>...>(std::forward<Func>(fun), std::forward<Args>(args)...); //参数是万能引用，根据左值还是右值引用执行相应的构造函数, 类的类型参数需要去引用
    }
}