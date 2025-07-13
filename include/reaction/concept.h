#include <concepts>

namespace reaction 
{
    // ------------------------------------------forward declaration-----------------------------------
    struct VarExpr{};
    struct CalcExpr{};

    template <typename Type, typename ... Args>
    class ReactImpl;

    template <typename ReactType>
    class React; 

    // ------------------------------------------concepts----------------------------------------------
    template<typename T, typename U>
    concept Convertable = std::is_convertible_v<std::decay_t<T>, std::decay_t<U>>;

    struct VarExpr;
    template<typename T>
    concept IsVarExpr = std::is_same_v<T, VarExpr>; //concept本质是变量模板

    // ------------------------------------------traits------------------------------------------------
    template <typename T>
    struct ExpressionTraits
    {
        using type = T;
    };

    template <typename T>
    struct ExpressionTraits<React<ReactImpl<T>>>
    {
        using type = T;
    };

    template <typename Fun, typename... Args>
    struct ExpressionTraits<React<ReactImpl<Fun, Args...>>>
    {
        using type = std::invoke_result_t<Fun, typename ExpressionTraits<Args>::type...>;  //为了避免歧义，递归萃取Args中的类型
    };

    template <typename Fun, typename... Args>
    using ReturnType = typename ExpressionTraits<React<ReactImpl<Fun, Args...>>>::type;
}