#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_TYPE_TRAITS_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_TYPE_TRAITS_H_

#include <tuple>
#include <type_traits>

namespace frontend::generator::syntax_generator {

#define GENERATOR_REGIST_TYPE_BY_NAME(registed_type, registed_name) \
  namespace frontend::generator::syntax_generator::type_register {  \
  using registed_name = registed_type                               \
  }

#define GENERATOR_GET_TYPE_BY_NAME(name) \
  frontend::generator::syntax_generator::type_register::name

template <class T>
struct FunctionTraits;

template <class ReturnType, class... FunctionArgs>
struct FunctionTraits<ReturnType(FunctionArgs...)> {
  using return_type = ReturnType;
  using arg_types = std::tuple<FunctionArgs...>;

  static constexpr auto arg_size = sizeof...(FunctionArgs);

  template <size_t N, class... args>
    requires(N < sizeof...(args))
  struct NthArgTypeImpl;

  template <size_t N, class T, class... args>
    requires(N < sizeof...(args) + 1)
  struct NthArgTypeImpl<N, T, args...> {
    using type = typename NthArgTypeImpl<N - 1, args...>::type;
  };

  template <class T, class... args>
  struct NthArgTypeImpl<0, T, args...> {
    using type = T;
  };

  template <size_t N>
  using NthArgType = typename NthArgTypeImpl<N, FunctionArgs...>::type;
};

template <class TargetFunction, class... ArgsTypes>
concept FunctionCallableWithArgs =
    requires(TargetFunction func) { func(std::declval<ArgsTypes>()...); };

template <class... T>
constexpr size_t CountTypeSize() {
  return sizeof...(T);
}

}  // namespace frontend::generator::syntax_generator
#endif  // !GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_TYPE_TRAITS_H_