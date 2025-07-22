#pragma once

#include "reaction/resource.h"
#include "reaction/triggerMode.h"
namespace reaction {
template <typename Op, typename L, typename R>
class BinaryOpExpr {
public:
    using ValueType = typename std::common_type_t<typename L::ValueType, typename R::ValueType>;

    template <typename Left, typename Right>
    BinaryOpExpr(Op op, Left &&left, Right &&right)
        : m_op(op), m_left(std::forward<Left>(left)), m_right(std::forward<Right>(right)) {}

    auto operator()() const {
        return calculate();
    }

private:
    auto calculate() const {
        return m_op(m_left(), m_right());
    }

    [[no_unique_address]] Op m_op;
    L m_left;
    R m_right;
};

struct addOp {
    auto operator()(auto &&lhs, auto &&rhs) const {
        return lhs + rhs;
    }
};

struct subOp {
    auto operator()(auto &&lhs, auto &&rhs) const {
        return lhs - rhs;
    }
};

struct mulOp {
    auto operator()(auto &&lhs, auto &&rhs) const {
        return lhs * rhs;
    }
};

struct divOp {
    auto operator()(auto &&lhs, auto &&rhs) const {
        return lhs / rhs;
    }
};

template <typename Type>
struct ValueWrapper {
    using ValueType = Type;
    Type value;

    template <typename T>
    ValueWrapper(T &&v) : value(std::forward<T>(v)) {}

    const Type &operator()() const {
        return value;
    }
};

template <typename Op, typename L, typename R>
auto makeBinaryOpExpr(L &&lhs, R &&rhs) {
    return BinaryOpExpr<Op, ExprWarper<std::decay_t<L>>, ExprWarper<std::decay_t<R>>>(Op{}, std::forward<L>(lhs), std::forward<R>(rhs));
}

template <typename L, typename R>
requires IsValidExprOperand<L, R>
auto operator+(L &&lhs, R &&rhs) {
    return makeBinaryOpExpr<addOp>(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <typename L, typename R>
requires IsValidExprOperand<L, R>
auto operator-(L &&lhs, R &&rhs) {
    return makeBinaryOpExpr<subOp>(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <typename L, typename R>
requires IsValidExprOperand<L, R>
auto operator*(L &&lhs, R &&rhs) {
    return makeBinaryOpExpr<mulOp>(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <typename L, typename R>
requires IsValidExprOperand<L, R>
auto operator/(L &&lhs, R &&rhs) {
    return makeBinaryOpExpr<divOp>(std::forward<L>(lhs), std::forward<R>(rhs));
}

// 主模板声明（带参数包）
template <typename... Ts>
class Expression;

// 特化2：复杂表达式（多个参数）
template <IsTrigMode TrigMode, typename Fun, typename... Args>
class Expression<TrigMode, Fun, Args...> : public Resource<ReturnType<TrigMode, Fun, Args...>>, public TrigMode {
public:
    using ExprType = CalcExpr;
    using ValueType = ReturnType<TrigMode, Fun, Args...>;

    template <typename F, typename... A>
    void setSource(F &&fun, A &&...args) {
        if constexpr (std::convertible_to<ReturnType<TrigMode, std::decay_t<F>, std::decay_t<A>...>, ValueType>) {
            this->updateObserver(args.getPtr()...);
            setFunctor(createFun(std::forward<F>(fun), std::forward<A>(args)...));

            if constexpr (!VoidType<ValueType>) {
                this->notify(this->updateValue(evaluate()));
            } else {
                evaluate();
                this->notify();
            }
        }
    }

    void addObjCb(NodePtr obj) {
        this->updateObserver(obj);
    }

private:
    void valueChanged(bool Changed = true) override {
        if constexpr (std::is_same_v<TrigMode, ChangeTrig>) {
            TrigMode::setChanged(Changed); // 如果是ChangeTrig模式，设置changed状态
        }

        if (TrigMode::checkTrigger()) {
            if constexpr (!VoidType<ValueType>) {
                auto oldValue = this->getValue();
                auto newValue = evaluate();
                this->updateValue(newValue);
                if constexpr (ComparableType<ValueType>) {
                    this->notify(oldValue != newValue); // 如果值真的改变了，通知观察者
                } else {
                    this->notify(); // 如果不可比较类型，直接通知
                }
            } else {
                evaluate();
                this->notify();
            }
        }
    }

    template <typename F, typename... A>
    auto createFun(F &&fun, A &&...args) {
        return [fun = std::forward<F>(fun), ... args = args.getPtr()]() {
            if constexpr (VoidType<ValueType>) {
                std::invoke(fun, args->get()...);
                return VoidWrapper{}; // 如果返回值是void，则返回一个VoidWrapper
            } else
                // 如果返回值不是void，则返回计算结果
                return std::invoke(fun, args->get()...);
        };
    }

    auto evaluate() const {
        if constexpr (VoidType<ValueType>) {
            std::invoke(m_fun);
        } else {
            return std::invoke(m_fun);
        }
    }

    void setFunctor(const std::function<ValueType()> &fun) {
        m_fun = fun;
    }

    std::function<ValueType()> m_fun;
};

// 特化1：简单表达式（单一参数）
template <IsTrigMode TrigMode, NonInvocableType Type>
requires(!IsBinaryOpExpr<Type>) class Expression<TrigMode, Type> : public Resource<Type> {
public:
    // Expression(Type &&t) : Resource<Type>(std::forward<Type>(t));  // 在派生类中委托构造基类的构造函数。 CPP11可以用下面替代
    using ExprType = VarExpr;
    using Resource<Type>::Resource;
    using ValueType = Type;
};

template <IsTrigMode TrigMode, typename Op, typename L, typename R>
class Expression<TrigMode, BinaryOpExpr<Op, L, R>>
    : public Expression<TrigMode, std::function<typename std::common_type_t<typename L::ValueType, typename R::ValueType>()>> {
public:
    template <typename T>
    Expression(T &&t) : m_expr(std::forward<T>(t)) {}

protected:
    void setOpExpr() {
        this->setSource([this]() {
            return m_expr();
        });
    }

private:
    BinaryOpExpr<Op, L, R> m_expr;
};
} // namespace reaction