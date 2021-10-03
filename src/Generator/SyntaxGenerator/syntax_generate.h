// 该文件的目的是为了去除执行generator生成编译所需代码这一步
// 通过宏在定义产生式时生成代码，定义产生式后就可以编译运行generator生成配置
// 该文件需要被三个文件包含，并在三个文件中相同的宏生成不同代码

// 下面的宏保证该宏定义最终仅被process_functions_classes.h
// user_defined_function_and_data_register.h和config_construct.cpp包含
// 且相同名字的宏在三种文件中定义不同内容
// 该文件不应使用标准头文件保护，应该使用下面的宏保护
// 这么写为了使用户在编辑器里可以看到宏声明
// 并且可以仅写一次产生式就能在多个文件中生成所需代码
// 该头文件必须在使用宏之前被包含
// （最简单的方法是放在production_config-inc.h第一行）

// 这部分声明用户定义产生式用的宏的原型
// 同时还原这些宏为无实现，防止不同文件对宏的特化冲突或忘记#undef等问题
// 强烈建议使用最简单的语法，避免运算
// 使用字符串时直接使用双引号类型（可以使用生肉字）
// 从而避免宏生成时表达式计算结果错误等问题

// 修饰非终结节点产生式名以获取包装Reduct函数用的类名
#define NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(production_symbol,   \
                                             production_body_seq) \
  production_symbol##production_body_seq##_

// 定义关键字
// 例：GENERATOR_DEFINE_KEY_WORD("example_key_word")
#ifdef GENERATOR_DEFINE_KEY_WORD
#undef GENERATOR_DEFINE_KEY_WORD
#endif  // !GENERATOR_DEFINE_KEY_WORD
#define GENERATOR_DEFINE_KEY_WORD(key_word)

// 添加双目运算符
// 输入参数从左到右：运算符符号，结合性（枚举OperatorAssociatityType），优先级
// 例：GENERATOR_DEFINE_BINARY_OPERATOR("example_symbol",
//                                      OperatorAssociatityType::kLeftToRight,
//                                      2)
#ifdef GENERATOR_DEFINE_BINARY_OPERATOR
#undef GENERATOR_DEFINE_BINARY_OPERATOR
#endif  // !GENERATOR_DEFINE_BINARY_OPERATOR
#define GENERATOR_DEFINE_BINARY_OPERATOR( \
    operator_symbol, binary_operator_associatity, binary_operator_priority)
// 添加左侧单目运算符
// 输入参数从左到右：运算符符号，结合性（枚举OperatorAssociatityType），优先级
// 例：GENERATOR_DEFINE_UNARY_OPERATOR("example_symbol",
//                                      OperatorAssociatityType::kLeftToRight,
//                                      2)
#ifdef GENERATOR_DEFINE_UNARY_OPERATOR
#undef GENERATOR_DEFINE_UNARY_OPERATOR
#endif  // !GENERATOR_DEFINE_UNARY_OPERATOR
#define GENERATOR_DEFINE_UNARY_OPERATOR( \
    operator_symbol, unary_operator_associatity, unary_operator_priority)
// 添加左侧单目和双目运算符
// 不可以使用GENERATOR_DEFINE_UNARY_OPERATOR和GENERATOR_DEFINE_BINARY_OPERATOR
// 组合声明同时支持左侧单目和双目的运算符，必须使用该宏一次性声明两种
// 输入参数从左到右：运算符符号，结合性（枚举OperatorAssociatityType），优先级
// 例：GENERATOR_DEFINE_BINARY_UNARY_OPERATOR("example_symbol",
//                                      OperatorAssociatityType::kLeftToRight,
//                                      2)
#ifdef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#undef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#endif  // !GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#define GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(                             \
    operator_symbol, binary_operator_associatity, binary_operator_priority, \
    unary_operator_associatity, unary_operator_priority)

// 定义终结节点，production_body为正则表达式
// 例：GENERATOR_DEFINE_TERMINAL_PRODUCTION(example_symbol,
//                                          "[a-zA-Z_][a-zA-Z_0-9]*")
#ifdef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_TERMINAL_PRODUCTION
#define GENERATOR_DEFINE_TERMINAL_PRODUCTION(production_symbol, production_body)

// 非终结节点定义，产生式体部分请使用初始化列表书写法
// 例：{"production_body1","production_body2"}
// 五个参数都不在两侧加双引号
// production_symbol为产生式名
// reduct_function是规约用函数
// production_body_seq是产生式体编号
// ...部分为产生式体
// 每一个相同的产生式名下各产生式体编号必须不同，可使用[a-zA-Z0-9_]+
// 该编号用于区分不同的产生式体并为其构建相应的包装函数
// 例：GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(example_symbol,
//                ExampleFunctionName, 0, false, {"Id", "=", "Assignable"})
#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION( \
    production_symbol, reduct_function_name, production_body_seq, ...)

// 设置非终结节点可以空规约
// 未调用此方法的非终结节点默认不可以空规约
// 在调用时应保证给定的非终结节点已经完整定义（所有依赖均已定义）
// 所以强烈建议在定义所有产生式后调用该方法
// 例：GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(example_symbol)
#ifdef GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT
#undef GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT
#endif
#define GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT( \
    production_symbol)

// 设置根产生式
// 根产生式定义，需要在给定产生式被正式添加后使用
// （即该产生式所依赖的全部节点都能够添加后）
// 例：GENERATOR_DEFINE_ROOT_PRODUCTION(example_symbol)
#ifdef GENERATOR_DEFINE_ROOT_PRODUCTION
#undef GENERATOR_DEFINE_ROOT_PRODUCTION
#endif  // GENERATOR_DEFINE_BINARY_OPERATOR
#define GENERATOR_DEFINE_ROOT_PRODUCTION(production_symbol)

// 这部分对三种需要产生式信息的文件特化
#ifdef GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_
// 定义该宏以屏蔽用来允许使用IntelliSense而包含的头文件
// production_config-inc.h在config_construct.cpp中生成函数内内容
// 在函数定义内包含头文件会导致大量奇怪错误
#define SHIELD_HEADERS_FOR_INTELLISENSE
// 防止重复包含
#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_END

#ifdef GENERATOR_DEFINE_KEY_WORD
#undef GENERATOR_DEFINE_KEY_WORD
#endif  // GENERATOR_DEFINE_KEY_WORD
#define GENERATOR_DEFINE_KEY_WORD(key_word) AddKeyWord(key_word);

#ifdef GENERATOR_DEFINE_BINARY_OPERATOR
#undef GENERATOR_DEFINE_BINARY_OPERATOR
#endif  // GENERATOR_DEFINE_BINARY_OPERATOR
#define GENERATOR_DEFINE_BINARY_OPERATOR(                                   \
    operator_symbol, binary_operator_associatity, binary_operator_priority) \
  AddBinaryOperatorNode(operator_symbol,                                    \
                        frontend::common::binary_operator_associatity,      \
                        OperatorPriority(binary_operator_priority));
#ifdef GENERATOR_DEFINE_UNARY_OPERATOR
#undef GENERATOR_DEFINE_UNARY_OPERATOR
#endif  // GENERATOR_DEFINE_UNARY_OPERATOR
#define GENERATOR_DEFINE_UNARY_OPERATOR(                                  \
    operator_symbol, unary_operator_associatity, unary_operator_priority) \
  AddUnaryOperatorNode(operator_symbol,                                   \
                       frontend::common::unary_operator_associatity,      \
                       OperatorPriority(unary_operator_priority));
#ifdef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#undef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#endif  // !GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#define GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(                             \
    operator_symbol, binary_operator_associatity, binary_operator_priority, \
    unary_operator_associatity, unary_operator_priority)                    \
  AddBinaryUnaryOperatorNode(operator_symbol,                               \
                             frontend::common::binary_operator_associatity, \
                             OperatorPriority(binary_operator_priority),    \
                             frontend::common::unary_operator_associatity,  \
                             OperatorPriority(unary_operator_priority))

#ifdef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_TERMINAL_PRODUCTION
#define GENERATOR_DEFINE_TERMINAL_PRODUCTION(production_symbol, \
                                             production_body)   \
  AddTerminalNode(#production_symbol, production_body);

#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(                   \
    production_symbol, reduct_function, production_body_seq, ...)  \
  AddNonTerminalNode<NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(         \
      production_symbol, production_body_seq)>(#production_symbol, \
                                               __VA_ARGS__);

#ifdef GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT
#undef GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT
#endif
#define GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT( \
    production_symbol)                                           \
  SetNonTerminalNodeCouldEmptyReduct(#production_symbol);

#ifdef GENERATOR_DEFINE_ROOT_PRODUCTION
#undef GENERATOR_DEFINE_ROOT_PRODUCTION
#endif  // GENERATOR_DEFINE_ROOT_PRODUCTION
#define GENERATOR_DEFINE_ROOT_PRODUCTION(production_symbol) \
  SetRootProduction(#production_symbol);

#endif

#elif defined GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_
// 防止被重复包含
#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_END

#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION

#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(                           \
    production_symbol, reduct_function, production_body_seq, ...)          \
  class NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(production_symbol,            \
                                             production_body_seq)          \
      : public frontend::generator::syntaxgenerator::                      \
            ProcessFunctionInterface {                                     \
   public:                                                                 \
    virtual ProcessFunctionInterface::UserData Reduct(                     \
        std::vector<ProcessFunctionInterface::WordDataToUser>&& word_data) \
        override {                                                         \
      return reduct_function(std::move(word_data));                        \
    }                                                                      \
  };
#endif

#elif defined GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER
// 防止重复包含
#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_END

#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION

#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(                              \
    production_symbol, production_body, reduct_function, production_body_seq, \
    ...)                                                                      \
  BOOST_CLASS_EXPORT_GUID(                                                    \
      frontend::generator::syntaxgenerator::                                  \
          NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(production_symbol,             \
                                               production_body_seq),          \
      "frontend::generator::"                                                 \
      "syntaxgenerator::" #NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(              \
          production_symbol, production_body_seq))
#endif

#else

#error 该文件仅且必须最终被Generator/SyntaxGenerator下的\
process_functions_classes.h \
process_functions_classes_register.h \
config_construct.cpp所包含

#endif

// 为了使用Intellisense在这个宏里包含需要的头文件
// 此处的头文件在config_construct.cpp中会被忽略
// 用户请在user_defined_functions.h内添加自己定义的头文件
#ifndef SHIELD_HEADERS_FOR_INTELLISENSE
#include "Common/common.h"
#include "Config/ProductionConfig/user_defined_functions.h"
// 提供运算符结合性的枚举
using OperatorAssociatityType = frontend::common::OperatorAssociatityType;
#endif  // !SHIELD_HEADERS_FOR_INTELLIGENCE