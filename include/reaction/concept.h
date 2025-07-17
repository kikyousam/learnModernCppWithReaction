#include <concepts>
#include <memory>

namespace reaction 
{
    // ------------------------------------------forward declaration-----------------------------------
    struct VarExpr{};
    struct CalcExpr{};

    template <typename Type, typename ... Args>
    class ReactImpl;

    template <typename ReactType>
    class React; 

    class ObserverNode;

    using NodePtr = std::shared_ptr<ObserverNode>;

    class FieldBase;

    struct VoidWrapper; //用于void类型的特殊处理

    template <typename T>
    struct ValueWrapper;

    template <typename Op, typename L, typename R>
    class BinaryOpExpr;
    // ------------------------------------------concepts----------------------------------------------
    template<typename T, typename U>
    concept Convertable = std::is_convertible_v<std::decay_t<T>, std::decay_t<U>>;

    struct VarExpr;
    template<typename T>
    concept IsVarExpr = std::is_same_v<T, VarExpr>; //concept本质是变量模板

    template<typename T>
    concept HasField = requires(T t) {
        { t.getID() } -> std::same_as<uint64_t>;
        requires std::is_base_of_v<FieldBase, T>;
    };

    template<typename T>
    concept ConstType = std::is_const_v<std::remove_reference_t<T>>;

    template<typename T>
    concept VoidType = std::is_void_v<std::remove_reference_t<T>> || std::is_same_v<T, VoidWrapper>;

    template<typename T>
    concept InvocableType = std::is_invocable_v<std::decay_t<T>>;

    template<typename T>
    concept NonInvocableType = !InvocableType<T>;

    template<typename... Args>
    concept HasArguments = sizeof...(Args) > 0;

    template<typename T>
    concept IsReactNode = requires(T t) {
        { t.shared_from_this() } -> std::same_as<NodePtr>;
    };

    template<typename T>
    concept IsDataReact = requires(T t) {
        typename T::ValueType;
        requires IsReactNode<T> && !VoidType<typename T::ValueType>;
    };

    // ------------------------------------------traits------------------------------------------------
    template <typename T>
    struct IsReact : std::false_type
    {
    };

    template <typename T>
    struct IsReact<React<T>> : std::true_type
    {
        using type = T;
    };

    template <typename T>
    struct ExpressionTraits
    {
        using type = T;
    };

    template <NonInvocableType T>
    struct ExpressionTraits<React<ReactImpl<T>>>
    {
        using type = T;
    };

    template <typename Fun, typename... Args>
    struct ExpressionTraits<React<ReactImpl<Fun, Args...>>>
    {
        using rawType = std::invoke_result_t<Fun, typename ExpressionTraits<Args>::type...>;  //为了避免歧义，递归萃取Args中的类型
        using type = std::conditional_t<VoidType<rawType>, VoidWrapper, rawType>; //如果是void类型，使用VoidWrapper
    };
    
    template <typename Fun, typename... Args>
    using ReturnType = typename ExpressionTraits<React<ReactImpl<Fun, Args...>>>::type;

    template <typename T>
    struct BinaryOpExprTraits : std::false_type
    {
    };

    template <typename Op, typename L, typename R>
    struct BinaryOpExprTraits<BinaryOpExpr<Op, L, R>> : std::true_type
    {
    };

    template <typename T>
    concept IsBinaryOpExpr = BinaryOpExprTraits<std::decay_t<T>>::value;

    template <typename T>
    using ExprWarper = std::conditional_t<
        IsReact<T>::value || IsBinaryOpExpr<T>,
        T,
        ValueWrapper<std::decay_t<T>>
    >;

    template <typename L, typename R>
    concept IsValidExprOperand = 
        IsReact<std::decay_t<L>>::value || IsReact<std::decay_t<R>>::value || 
        IsBinaryOpExpr<L> || IsBinaryOpExpr<R>;
}