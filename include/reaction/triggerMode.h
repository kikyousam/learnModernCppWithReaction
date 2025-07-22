#include <functional>

namespace reaction {
struct AlwaysTrig {
    bool checkTrigger() const {
        return true; // Always trigger
    }
};

struct ChangeTrig {
    bool checkTrigger() const {
        return isChanged; // Trigger only on change
    }

    void setChanged(bool changed) {
        isChanged = changed; // Set the change state
    }

private:
    bool isChanged = true;
};

struct FilterTrig {
    bool checkTrigger() const {
        // Implement filter logic here
        return std::invoke(m_filterFunc);
    }

    template <typename F, typename... A>
    void setFilterFunc(F &&f, A &&...args) {
        m_filterFunc = createFun(std::forward<F>(f), std::forward<A>(args)...);
    }

private:
    template <typename F, typename... A>
    auto createFun(F &&fun, A &&...args) {
        return [fun = std::forward<F>(fun), ... args = args.getPtr()]() {
            return std::invoke(fun, args->get()...);
        };
    }
    std::function<bool()> m_filterFunc = []() { return true; }; // Default filter function
};
} // namespace reaction