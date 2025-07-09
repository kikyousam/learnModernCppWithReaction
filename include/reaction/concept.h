#include <concepts>

namespace reaction 
{
    template<typename T, typename U>
    concept ConvertCC = std::is_convertible_v<std::decay_t<T>, std::decay_t<U>>;

    struct VarExpr;
    template<typename T>
    concept VarExprCC = std::is_same_v<T, VarExpr>; //concept本质是变量模板
}