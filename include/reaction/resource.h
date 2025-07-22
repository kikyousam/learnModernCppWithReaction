#pragma once

#include "reaction/oberverNode.h"
#include <exception>
#include <memory>

namespace reaction {
template <typename Type>
class Resource : public ObserverNode // 一个值就对应一个观察者结点
{
public:
    // Resource(Type &&t) :m_ptr<std::make_unique<Type>(std::forward<Type>(t)) {}// 引用折叠不能作用于长生命周期的对象，可以用于函数
    Resource() : m_ptr(nullptr) {}
    template <typename T>
    Resource(T &&t) : m_ptr(std::make_unique<Type>(std::forward<T>(t))) {}

    Resource(const Resource &) = delete;
    Resource &operator=(const Resource &) = delete;

    Type &getValue() const {
        if (!m_ptr) {
            throw std::runtime_error("Resource is not initialized");
        }
        return *m_ptr;
    }

    Type *getRawPtr() const {
        if (!m_ptr) {
            throw std::runtime_error("Resource is not initialized");
        }
        return m_ptr.get();
    }

    template <typename T>
    bool updateValue(T &&t) {
        bool changed = true;
        if (!m_ptr) {
            m_ptr = std::make_unique<Type>(std::forward<T>(t));
        } else {
            if constexpr (ComparableType<Type>) {
                changed = (*m_ptr != std::forward<T>(t)); // 如果是可比较类型，检查是否真的改变了
            }
            *m_ptr = std::forward<T>(t);
        }

        return changed;
    }

private:
    std::unique_ptr<Type> m_ptr;
};

struct VoidWrapper {}; // 用于void类型的特殊处理

template <>
class Resource<VoidWrapper> : public ObserverNode {
public:
    auto getValue() const {
        return VoidWrapper{};
    }
};
} // namespace reaction