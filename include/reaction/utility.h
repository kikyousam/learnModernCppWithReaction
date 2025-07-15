#include <cstdint>
#include <atomic>
#include <unordered_map>
#include <unordered_set>

namespace reaction
{
class UniqueID
{
public:
    UniqueID() : m_id(generate()) {}

    bool operator==(const UniqueID &other) const {
        return m_id == other.m_id;
    }

    bool operator!=(const UniqueID &other) const {
        return !(*this == other);
    }

    operator uint64_t() const {
        return m_id;
    }
private:
    uint64_t generate(){
        static std::atomic<uint64_t> idCounter{0};
        return idCounter.fetch_add(1, std::memory_order_relaxed);
    }
    uint64_t m_id;

    friend struct std::hash<UniqueID>;  // Allow std::hash to access private members
};

}

namespace std
{
    template <>
    struct hash<reaction::UniqueID>
    {
        size_t operator()(const reaction::UniqueID &id) const noexcept {
            return std::hash<uint64_t>()(id);
        }
    };
}