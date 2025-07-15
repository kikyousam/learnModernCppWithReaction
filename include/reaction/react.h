#pragma once

#include "reaction/expression.h"
#include <atomic>

namespace reaction
{
    template<typename Type, typename... Args>
    class ReactImpl : public Expression<Type, Args...>  //用来和用户交互, 采用继承的方式表示is a的关系
    {  //实现类
    public:
        using ExprType = Expression<Type, Args...>::ExprType;
        using ValueType = Expression<Type, Args...>::ValueType;
        ReactImpl(const ReactImpl& d) {}
        using Expression<Type, Args...>::Expression;
        decltype(auto) get() const {        //完全保留返回值的类型
            return this->getValue();
        }

        template <typename T>
        void operator=(T &&t) {
            value(std::forward<T>(t));
        }

        template <typename T>
            requires (Convertable<T, ValueType> && IsVarExpr<ExprType> && !ConstType<ValueType>)
        void value(T &&t) {
            this->updateValue(std::forward<T>(t));
            this->notify();
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
        std::atomic<int> m_weakRefCount{0};  //引用计数，线程安全
    };

    template <typename ReactType>
    class React  //管理类
    {
    public:
        ReactType &operator*() {
            return *getPtr();
        }

        explicit React(std::shared_ptr<ReactType> ptr = nullptr) : m_weakPtr(ptr) {
            if(auto p = m_weakPtr.lock()) {
                p->addWeakRef();
            }
        }

        ~React() {
            if(auto p = m_weakPtr.lock()) {
                p->releaseWeakRef();
            }
        }

        React(const React& other) : m_weakPtr(other.m_weakPtr) {
            if(auto p = m_weakPtr.lock()) {
                p->addWeakRef();
            }
        }

        React& operator=(const React& other) {
            if (this != &other) {
                if(auto p = m_weakPtr.lock()) {
                    p->releaseWeakRef();
                }
                m_weakPtr = other.m_weakPtr;
                if(auto p = m_weakPtr.lock()) {
                    p->addWeakRef();
                }
            }
            return *this;
        }

        React(React&& other) noexcept : m_weakPtr(std::move(other.m_weakPtr)) {
            other.m_weakPtr.reset();
        }

        React& operator=(React&& other) noexcept {
            if (this != &other) {
                if(auto p = m_weakPtr.lock()) {
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

        decltype(auto) get() const 
            requires(IsDataReact<ReactType>)  //如果是数据react
        {
            return getPtr()->get();
        }

        template <typename T>
        void value(T &&t) {
            return getPtr()->value(std::forward<T>(t));
        }

        auto getPtr() const {
            if(auto p = m_weakPtr.lock()) {
                return p;
            }
            throw std::runtime_error("Weak pointer expired");
        }

        // operator decltype(std::declval<ReactType>().get())() const {
        //     return get();
        // }
    private:
        std::weak_ptr<ReactType> m_weakPtr;
    };

    template <typename SrcType>
    using Field = React<ReactImpl<std::decay_t<SrcType>>>;  //Field是一个React类型的别名，表示一个字段
    class FieldBase
    {
    public:
        template <typename T>
        auto field(T &&t)
        {
            auto ptr = std::make_shared<ReactImpl<std::decay_t<T>>>(std::forward<T>(t));
            FieldGraph::getInstance().addObj(m_id, ptr->shared_from_this());
            return React(ptr);
        }

        uint64_t getID() const {
            return m_id;
        }
    private:
        UniqueID m_id;  //每个FieldBase都有一个唯一的ID
    };

    template <typename SrcType>
    auto var(SrcType &&t) {
        auto ptr = std::make_shared<ReactImpl<std::decay_t<SrcType>>>(std::forward<SrcType>(t));
        if constexpr(HasField<std::decay_t<SrcType>>) {
            FieldGraph::getInstance().bindField(ptr->getValue().getID(), ptr->shared_from_this());
        } 
        ObserverGraph::getInstance().addNode(ptr);
        return React(ptr);
    }

    template <typename SrcType>
    auto constVar(SrcType &&t) {
        auto ptr = std::make_shared<ReactImpl<const std::decay_t<SrcType>>>(std::forward<SrcType>(t));
        ObserverGraph::getInstance().addNode(ptr);
        return React(ptr);
    }

    template <typename Func, typename... Args>
    auto calc(Func &&fun, Args &&...args) {
        auto ptr = std::make_shared<ReactImpl<std::decay_t<Func>, std::decay_t<Args>...>>(std::forward<Func>(fun), std::forward<Args>(args)...);
        ObserverGraph::getInstance().addNode(ptr);
        return React(ptr);
    }

    template <typename Func, typename... Args>
    auto action(Func &&fun, Args &&...args) {
        return calc(std::forward<Func>(fun), std::forward<Args>(args)...);
    }
}