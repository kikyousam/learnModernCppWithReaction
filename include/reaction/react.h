#pragma once

#include "reaction/expression.h"
#include <atomic>

namespace reaction {
inline thread_local std::function<void(NodePtr)> g_reg_fun = nullptr; // 全局注册函数，用于注册回调

struct RegGuard {
public:
    RegGuard(const std::function<void(NodePtr)> &reg_fun) {
        g_reg_fun = reg_fun; // 设置全局注册函数
    }

    ~RegGuard() {
        g_reg_fun = nullptr; // 清除注册函数
    }
};

template <IsTrigMode TrigMode, typename Type, typename... Args>
class ReactImpl : public Expression<TrigMode, Type, Args...> // 用来和用户交互, 采用继承的方式表示is a的关系
{                                                            // 实现类
public:
    using ExprType = Expression<TrigMode, Type, Args...>::ExprType;
    using ValueType = Expression<TrigMode, Type, Args...>::ValueType;
    ReactImpl(const ReactImpl &d) {}
    using Expression<TrigMode, Type, Args...>::Expression;
    decltype(auto) get() const { // 完全保留返回值的类型
        return this->getValue();
    }

    template <typename F, HasArguments... A>
    void set(F &&fun, A &&...args) {
        this->setSource(std::forward<F>(fun), std::forward<A>(args)...);
    }

    template <typename F>
    void set(F &&fun) {
        RegGuard guard([this](NodePtr obj) {
            this->addObjCb(obj);
        }); // 注册函数
        this->setSource(std::forward<F>(fun));
    }

    void set() {
        RegGuard guard([this](NodePtr obj) {
            this->addObjCb(obj);
        }); // 注册函数
        this->setOpExpr();
    }

    auto getRaw() const {
        return this->getRawPtr();
    }

    template <typename T>
    void operator=(T &&t) {
        value(std::forward<T>(t));
    }

    template <typename T>
    requires(Convertable<T, ValueType> &&IsVarExpr<ExprType> && !ConstType<ValueType>) void value(T &&t) {
        this->notify(this->updateValue(std::forward<T>(t)));
    }

    void addWeakRef() {
        m_weakRefCount++;
    }

    void releaseWeakRef() {
        if (--m_weakRefCount == 0) {
            ObserverGraph::getInstance().removeNode(std::static_pointer_cast<ObserverNode>(this->shared_from_this()));
            if constexpr (HasField<ValueType>) {
                FieldGraph::getInstance().deleteObj(this->getValue().getID());
            }
        }
    }

private:
    std::atomic<int> m_weakRefCount{0}; // 引用计数，线程安全
};

template <typename ReactType>
class React // 管理类
{
public:
    using ValueType = typename ReactType::ValueType;
    ReactType &operator*() {
        return *getPtr();
    }

    explicit React(std::shared_ptr<ReactType> ptr = nullptr) : m_weakPtr(ptr) {
        if (auto p = m_weakPtr.lock()) {
            p->addWeakRef();
        }
    }

    ~React() {
        if (auto p = m_weakPtr.lock()) {
            p->releaseWeakRef();
        }
    }

    React(const React &other) : m_weakPtr(other.m_weakPtr) {
        if (auto p = m_weakPtr.lock()) {
            p->addWeakRef();
        }
    }

    React &operator=(const React &other) {
        if (this != &other) {
            if (auto p = m_weakPtr.lock()) {
                p->releaseWeakRef();
            }
            m_weakPtr = other.m_weakPtr;
            if (auto p = m_weakPtr.lock()) {
                p->addWeakRef();
            }
        }
        return *this;
    }

    React(React &&other) noexcept : m_weakPtr(std::move(other.m_weakPtr)) {
        other.m_weakPtr.reset();
    }

    React &operator=(React &&other) noexcept {
        if (this != &other) {
            if (auto p = m_weakPtr.lock()) {
                p->releaseWeakRef();
            }
            m_weakPtr = std::move(other.m_weakPtr);
            other.m_weakPtr.reset();
        }
        return *this;
    }

    operator bool() const {
        return !m_weakPtr.expired();
    }

    decltype(auto) get() const {
        return getPtr()->get();
    }

    decltype(auto) operator()() const {
        if (g_reg_fun) {
            std::invoke(g_reg_fun, getPtr()); // 调用全局注册函数
        }
        return get();
    }

    template <typename F, typename... A>
    React &reset(F &&fun, A &&...args) {
        getPtr()->set(std::forward<F>(fun), std::forward<A>(args)...);
        return *this;
    }

    template <typename T>
    React &value(T &&t) {
        getPtr()->value(std::forward<T>(t));
        return *this;
    }

    template <typename F, typename... A>
    React &filter(F &&fun, A &&...args) {
        getPtr()->setFilterFunc(std::forward<F>(fun), std::forward<A>(args)...);
        return *this;
    }

    React &setName(const std::string &name) {
        ObserverGraph::getInstance().setName(getPtr(), name);
        return *this;
    }

    std::string getName() const {
        return ObserverGraph::getInstance().getName(getPtr());
    }

    auto getPtr() const {
        if (auto p = m_weakPtr.lock()) {
            return p;
        }
        throw std::runtime_error("Weak pointer expired");
    }

    auto operator->() const {
        return getPtr()->getRaw();
    }

private:
    std::weak_ptr<ReactType> m_weakPtr;
};

template <typename SrcType, IsTrigMode TrigMode = ChangeTrig>
using Field = React<ReactImpl<TrigMode, std::decay_t<SrcType>>>; // Field是一个React类型的别名，表示一个字段
class FieldBase {
public:
    template <IsTrigMode TrigMode = ChangeTrig, typename T>
    auto field(T &&t) {
        auto ptr = std::make_shared<ReactImpl<TrigMode, std::decay_t<T>>>(std::forward<T>(t));
        ObserverGraph::getInstance().addNode(ptr);
        FieldGraph::getInstance().addObj(m_id, ptr->shared_from_this());
        return React(ptr);
    }

    uint64_t getID() const {
        return m_id;
    }

private:
    UniqueID m_id; // 每个FieldBase都有一个唯一的ID
};

template <IsTrigMode TrigMode = ChangeTrig, typename SrcType>
auto var(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<TrigMode, std::decay_t<SrcType>>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    if constexpr (HasField<std::decay_t<SrcType>>) {
        FieldGraph::getInstance().bindField(ptr->getValue().getID(), ptr->shared_from_this());
    }
    return React(ptr);
}

template <IsTrigMode TrigMode = ChangeTrig, typename SrcType>
auto constVar(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<TrigMode, const std::decay_t<SrcType>>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    return React(ptr);
}

template <IsTrigMode TrigMode = ChangeTrig, typename OpExpr>
auto expr(OpExpr &&opExpr) {
    auto ptr = std::make_shared<ReactImpl<TrigMode, std::decay_t<OpExpr>>>(std::forward<OpExpr>(opExpr));
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set();
    return React(ptr);
}

template <IsTrigMode TrigMode = ChangeTrig, typename Func, typename... Args>
auto calc(Func &&fun, Args &&...args) {
    auto ptr = std::make_shared<ReactImpl<TrigMode, std::decay_t<Func>, std::decay_t<Args>...>>();
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set(std::forward<Func>(fun), std::forward<Args>(args)...);
    return React(ptr);
}

template <IsTrigMode TrigMode = ChangeTrig, typename Func, typename... Args>
auto action(Func &&fun, Args &&...args) {
    return calc<TrigMode>(std::forward<Func>(fun), std::forward<Args>(args)...);
}
} // namespace reaction