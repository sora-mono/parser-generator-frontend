/// @file syntax_generate.h
/// @brief 定义表示产生式的宏以简化配置生成过程
/// @details
/// 1.将每个规约函数通过虚函数包装到类中，每个包装类实例化一个对象用于序列化到文件中，
/// 利用虚函数机制运行时调用正确的规约函数
/// 2.为了尽量简化使用方法和实现难度，作者选择通过宏在配置文件中描述产生式和相关信息来定义产生式，
/// 只需编译一次，运行一次就能生成适用于解析器的配置；
/// 相比于yacc和lex，减少了一次解析的步骤
/// 3.该文件内定义多个表示产生式的宏，这些宏均用于表示产生式，并在需要的文件内
/// 转化为相应代码，避免每个需要生成代码的文件中都手动定义产生式
/// 4.该文件必须且仅被Config/ProductionConfig/production_config-inc.h包含，
/// 5.该文件不应使用标准头文件保护，下面有特化的的宏保护
/// 6.用户定义头文件请在user_defined_functions.h中添加，
/// 禁止在production_config-inc.h中添加用户定义的头文件
/// 7.关键字（2）词法分析优先级高于运算符（1），运算符词法分析优先级高于普通单词（0）

// 这部分声明用户定义产生式用的宏的原型
// 同时还原这些宏为无实现，防止不同文件对宏的特化冲突或忘记#undef等问题
// 强烈建议调用宏时使用最简单的语法，避免运算
// 从而避免宏生成时表达式计算结果错误等问题
// 当前实现中使用3个单词优先级：0~2，普通单词优先级为0，运算符优先级为1，
// 关键字优先级为2；当一个单词对应多个正则解析为优先级高的正则

/// @brief 修饰非终结节点产生式名
/// @param[in] node_symbol ：产生式名
/// @param[in] production_body_seq ：区分产生式体的字符
/// @return 返回裸的包装Reduct函数用的类名
/// @note
/// 例：
/// NONTERMINAL_NODE_SYMBOL_MODIFY(example1,2)
/// 展开为 example1_2_
#define NONTERMINAL_NODE_SYMBOL_MODIFY(node_symbol, production_body_seq) \
  node_symbol##_##production_body_seq##_
/// @brief 修饰非终结节点产生式名
/// @param[in] node_symbol ：产生式名
/// @param[in] production_body_seq ：区分产生式体的字符
/// @return 返回包装Reduct函数用的类名的字符串形式
/// @note
/// 例：
/// NONTERMINAL_NODE_SYMBOL_MODIFY_STR(example1,2)
/// 展开为 "example1_2_"
#define NONTERMINAL_NODE_SYMBOL_MODIFY_STR(node_symbol, production_body_seq) \
#node_symbol##"_"## #production_body_seq##"_"

#undef GENERATOR_DEFINE_KEY_WORD
/// @brief 定义关键字
/// @param[in] node_symbol ：产生式名（关键字是终结产生式）
/// @param[in] key_word ：待定义的关键字字符串（不支持正则）
/// @details
/// 例：GENERATOR_DEFINE_KEY_WORD(KeyWordExample, "example_key_word")
/// @note key_word不支持正则表达式
/// 关键字词法分析优先级高于运算符和普通单词
#define GENERATOR_DEFINE_KEY_WORD(node_symbol, key_word)

#undef GENERATOR_DEFINE_BINARY_OPERATOR
/// @brief 定义双目运算符
/// @param[in] node_symbol ：产生式名（运算符也是一种终结产生式）
/// @param[in] operator_symbol ：运算符字符串（不支持正则）
/// @param[in] binary_operator_associatity
/// ：双目运算符结合性（枚举OperatorAssociatityType）
/// @param[in] binary_operator_priority ：双目运算符优先级
/// @details
/// 例：GENERATOR_DEFINE_BINARY_OPERATOR(PlusOperator, "+",
///                                      OperatorAssociatityType::kLeftToRight,
///                                      12)
/// @note 运算符优先级数值越大优先级越高
/// @attention 相同operator_symbol的运算符只能定义一次
#define GENERATOR_DEFINE_BINARY_OPERATOR(node_symbol, operator_symbol, \
                                         binary_operator_associatity,  \
                                         binary_operator_priority)

#undef GENERATOR_DEFINE_UNARY_OPERATOR
/// @brief 定义左侧单目运算符
/// @param[in] node_symbol ：产生式名（运算符也是一种终结产生式）
/// @param[in] operator_symbol ：运算符字符串（不支持正则）
/// @param[in] unary_operator_associatity
/// ：左侧单目运算符结合性（枚举OperatorAssociatityType）
/// @param[in] unary_operator_priority ：左侧单目运算符优先级
/// @details
/// 例：GENERATOR_DEFINE_UNARY_OPERATOR(SizeofOperator, "sizeof",
///                                     OperatorAssociatityType::kLeftToRight,
///                                     2)
/// @note 运算符优先级数值越大优先级越高
/// @attention 相同operator_symbol的运算符只能定义一次
#define GENERATOR_DEFINE_UNARY_OPERATOR(node_symbol, operator_symbol, \
                                        unary_operator_associatity,   \
                                        unary_operator_priority)

#undef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
/// @brief 定义同时具有双目和左侧单目语义的运算符
/// @param[in] node_symbol ：产生式名（运算符也是一种终结产生式）
/// @param[in] operator_symbol ：双语义运算符字符串（不支持正则）
/// @param[in] binary_operator_associatity
/// ：双目运算符结合性（枚举OperatorAssociatityType）
/// @param[in] binary_operator_priority ：双目运算符优先级
/// @param[in] unary_operator_associatity
/// ：左侧单目运算符结合性（枚举OperatorAssociatityType）
/// @param[in] unary_operator_priority ：左侧单目运算符优先级
/// @details
/// 例：GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(kMultiResolveRefOperator, "*",
///                                      OperatorAssociatityType::kLeftToRight,
///                                      13
///                                      OperatorAssociatityType::kRightToLeft,
///                                      14)
/// @attention 相同operator_symbol的运算符只能定义一次
/// 不允许通过组合GENERATOR_DEFINE_UNARY_OPERATOR和
/// GENERATOR_DEFINE_BINARY_OPERATOR来声明同时支持双目和左侧单目语义的运算符，
/// 必须使用该宏一次性声明这种运算符
#define GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(                \
    node_symbol, operator_symbol, binary_operator_associatity, \
    binary_operator_priority, unary_operator_associatity,      \
    unary_operator_priority)

#undef GENERATOR_DEFINE_TERMINAL_PRODUCTION
/// @brief 定义终结产生式
/// @param[in] node_symbol ：终结节点名字符串
/// @param[in] production_body ：终结节点体字符串（正则表达式形式）
/// @details
/// 例：GENERATOR_DEFINE_TERMINAL_PRODUCTION("Id", "[a-zA-Z_][a-zA-Z_0-9]*")
#define GENERATOR_DEFINE_TERMINAL_PRODUCTION(node_symbol, production_body)

#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
/// @brief 定义非终结产生式
/// @param[in] node_symbol ：非终结产生式名
/// @param[in] reduct_function ：规约非终结产生式的函数名
/// @param[in] ... ：产生式体
/// @details
/// 例：GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(example_symbol,
///                ReductFunctionName, Id, AssignOperator, Assignable)
/// @attention 每行只能使用一次该宏
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(node_symbol, reduct_function, \
                                                ...)

#undef GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT
/// @brief 设置非终结产生式可以空规约
/// @param[in] node_symbol ：待设置可以空规约的非终结产生式名
/// @note
/// 未调用此方法的非终结节点默认不可以空规约
/// 调用时仅需保证给定的非终结产生式已经定义，
/// 不要求已定义非终结产生式依赖的产生式
/// 例：GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(example_symbol)
#define GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(node_symbol)

#undef GENERATOR_DEFINE_ROOT_PRODUCTION
/// @brief 设置根产生式
/// @param[in] node_symbol ：待设置的根非终结产生式名
/// @note
/// 调用时仅需已定义该非终结节点，不要求已定义非终结产生式依赖的产生式
/// 例：GENERATOR_DEFINE_ROOT_PRODUCTION(example_symbol)
#define GENERATOR_DEFINE_ROOT_PRODUCTION(node_symbol)

// 在config_construct.cpp中转化为对SyntaxGenerator成员函数的调用代码
// 存放在ConfigConstruct函数中，向SyntaxGenerator传达产生式信息
#ifdef GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_

#undef GENERATOR_DEFINE_KEY_WORD
#define GENERATOR_DEFINE_KEY_WORD(node_symbol, key_word) \
  AddKeyWord(#node_symbol, key_word);

#undef GENERATOR_DEFINE_BINARY_OPERATOR
#define GENERATOR_DEFINE_BINARY_OPERATOR(node_symbol, operator_symbol, \
                                         binary_operator_associatity,  \
                                         binary_operator_priority)     \
  AddBinaryOperator(#node_symbol, operator_symbol,                     \
                    binary_operator_associatity,                       \
                    OperatorPriority(binary_operator_priority));

#undef GENERATOR_DEFINE_UNARY_OPERATOR
#define GENERATOR_DEFINE_UNARY_OPERATOR(node_symbol, operator_symbol, \
                                        unary_operator_associatity,   \
                                        unary_operator_priority)      \
  AddLeftUnaryOperator(#node_symbol, operator_symbol,                 \
                       unary_operator_associatity,                    \
                       OperatorPriority(unary_operator_priority));

#undef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#define GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(                               \
    node_symbol, operator_symbol, binary_operator_associatity,                \
    binary_operator_priority, unary_operator_associatity,                     \
    unary_operator_priority)                                                  \
  AddBinaryLeftUnaryOperator(                                                 \
      #node_symbol, operator_symbol, binary_operator_associatity,             \
      OperatorPriority(binary_operator_priority), unary_operator_associatity, \
      OperatorPriority(unary_operator_priority));

#undef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#define GENERATOR_DEFINE_TERMINAL_PRODUCTION(node_symbol, production_body) \
  AddSimpleTerminalProduction(#node_symbol, production_body);

#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(node_symbol, reduct_function, \
                                                ...)                          \
  GENERATOR_DEFINE_NONTERMINAL_PRODUCTION_IMPL(node_symbol, reduct_function,  \
                                               __LINE__, __VA_ARGS__)

#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION_IMPL
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION_IMPL(      \
    node_symbol, reduct_function, node_symbol_seq, ...)    \
  AddNonTerminalProduction<NONTERMINAL_NODE_SYMBOL_MODIFY( \
      node_symbol, node_symbol_seq)>(#node_symbol, #__VA_ARGS__);

#undef GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT
#define GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(node_symbol) \
  SetNonTerminalNodeCouldEmptyReduct(#node_symbol);

#undef GENERATOR_DEFINE_ROOT_PRODUCTION
#define GENERATOR_DEFINE_ROOT_PRODUCTION(node_symbol) \
  SetRootProduction(#node_symbol);

// 在process_function_classes.h中转化为包装规约函数的类
// 类名修饰方法见宏NONTERMINAL_NODE_SYMBOL_MODIFY和
// NONTERMINAL_NODE_SYMBOL_MODIFY_STR
#elif defined GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_
using namespace frontend::generator::syntax_generator::type_register;

#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(node_symbol, reduct_function, \
                                                ...)                          \
  GENERATOR_DEFINE_NONTERMINAL_PRODUCTION_IMPL(node_symbol, reduct_function,  \
                                               __LINE__, __VA_ARGS__)

#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION_IMPL
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION_IMPL(                         \
    node_symbol, reduct_function, node_symbol_seq, ...)                       \
  class NONTERMINAL_NODE_SYMBOL_MODIFY(node_symbol, node_symbol_seq)          \
      : public ProcessFunctionInterface {                                     \
   public:                                                                    \
    template <class TargetType>                                               \
    static TargetType&& GetArgument(std::any* container,                      \
                                    std::list<std::any>* temp_obj) {          \
      if (!container->has_value()) {                                          \
        temp_obj->emplace_back(TargetType());                                 \
        container = &temp_obj->back();                                        \
      }                                                                       \
      return std::move(*std::any_cast<TargetType>(container));                \
    }                                                                         \
    template <class TupleTypes, class IntegerSequence>                        \
    struct CallReductFunctionImpl;                                            \
                                                                              \
    template <class... Types, class SeqType, size_t... seq>                   \
    struct CallReductFunctionImpl<std::tuple<Types...>,                       \
                                  std::integer_sequence<SeqType, seq...>> {   \
      static std::any DoCall(std::vector<std::any>&& args) {                  \
        LOG_INFO("Parser", "Reduct Function Called: "## #reduct_function)     \
        std::list<std::any> temp_obj;                                         \
        return reduct_function(GetArgument<Types>(&args[seq], &temp_obj)...); \
      }                                                                       \
    };                                                                        \
                                                                              \
    template <class... T>                                                     \
    struct CallReductFunction                                                 \
        : public CallReductFunctionImpl<                                      \
              std::tuple<T...>,                                               \
              std::make_index_sequence<CountTypeSize<__VA_ARGS__>()>> {};     \
                                                                              \
    virtual std::any Reduct(                                                  \
        std::vector<std::any>&& word_data) const override {                   \
      static_assert(CountTypeSize<__VA_ARGS__>() ==                           \
                        FunctionTraits<decltype(reduct_function)>::arg_size,  \
                    "Arguments number required by reduct function doesn't "   \
                    "match subproduction number");                            \
      static_assert(                                                          \
          FunctionCallableWithArgs<decltype(reduct_function), __VA_ARGS__>,   \
          "Argument types required by reduct function can't be converted "    \
          "from one or more subproduction reduct result type");               \
      return CallReductFunction<__VA_ARGS__>::DoCall(std::move(word_data));   \
    }                                                                         \
                                                                              \
    virtual std::string GetReductFunctionName() const override {              \
      return #reduct_function;                                                \
    }                                                                         \
                                                                              \
   private:                                                                   \
    friend class boost::serialization::access;                                \
    template <class Archive>                                                  \
    void serialize(Archive& ar, const unsigned int version) {                 \
      ar& boost::serialization::base_object<ProcessFunctionInterface>(*this); \
    }                                                                         \
  };

// 在process_functions_classes_register.h中转化为注册
// process_function_classes.h中派生类的宏，配合boost-serialization使用
#elif defined GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER

#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION

#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(node_symbol, reduct_function, \
                                                ...)                          \
  GENERATOR_DEFINE_NONTERMINAL_PRODUCTION_IMPL(node_symbol, reduct_function,  \
                                               __LINE__, __VA_ARGS__)

#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION_IMPL
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION_IMPL(                        \
    node_symbol, reduct_function, node_symbol_seq, ...)                      \
  BOOST_CLASS_EXPORT_GUID(                                                   \
      frontend::generator::syntax_generator::NONTERMINAL_NODE_SYMBOL_MODIFY( \
          node_symbol, node_symbol_seq),                                     \
      "frontend::generator::"                                                \
      "syntax_generator::" NONTERMINAL_NODE_SYMBOL_MODIFY_STR(               \
          node_symbol, node_symbol_seq))

// 在reduct_type_register.h中注册规约函数的返回值类型，用于检查规约函数的返回值类型和实现编译期自动类型转换
#elif defined GENERATOR_SYNTAXGENERATOR_REDUCT_TYPE_REGISTER

#undef GENERATOR_DEFINE_KEY_WORD
#define GENERATOR_DEFINE_KEY_WORD(node_symbol, key_word)           \
  namespace frontend::generator::syntax_generator::type_register { \
  using node_symbol = std::string;                                 \
  }

#undef GENERATOR_DEFINE_BINARY_OPERATOR
#define GENERATOR_DEFINE_BINARY_OPERATOR(node_symbol, operator_symbol, \
                                         binary_operator_associatity,  \
                                         binary_operator_priority)     \
  namespace frontend::generator::syntax_generator::type_register {     \
  using node_symbol = std::string;                                     \
  }

#undef GENERATOR_DEFINE_UNARY_OPERATOR
#define GENERATOR_DEFINE_UNARY_OPERATOR(node_symbol, operator_symbol, \
                                        unary_operator_associatity,   \
                                        unary_operator_priority)      \
  namespace frontend::generator::syntax_generator::type_register {    \
  using node_symbol = std::string;                                    \
  }

#undef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#define GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(                    \
    node_symbol, operator_symbol, binary_operator_associatity,     \
    binary_operator_priority, unary_operator_associatity,          \
    unary_operator_priority)                                       \
  namespace frontend::generator::syntax_generator::type_register { \
  using node_symbol = std::string;                                 \
  }

#undef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#define GENERATOR_DEFINE_TERMINAL_PRODUCTION(node_symbol, production_body) \
  namespace frontend::generator::syntax_generator::type_register {         \
  using node_symbol = std::string;                                         \
  }

#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(node_symbol, reduct_function, \
                                                ...)                          \
  namespace frontend::generator::syntax_generator::type_register {            \
  using node_symbol =                                                         \
      std::decay_t<FunctionTraits<decltype(reduct_function)>::return_type>;   \
  }

#else
#error 请勿在process_functions_classes_register.h、process_function_classes.h、config_construct.cpp以外包含production_config-inc.h或重复包含
#endif