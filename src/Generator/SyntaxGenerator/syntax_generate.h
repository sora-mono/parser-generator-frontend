/// @file syntax_generate.h
/// @brief 定义表示产生式的宏以简化配置生成过程
/// @details
/// 1.作者能力所限，无法在文件中存储规约用函数，只能曲线救国，将每个规约函数
/// 包装到类中，每个包装类实例化一个对象，对象中通过虚函数包装规约函数，
/// 利用虚函数机制运行时调用正确的规约函数，创建的对象可通过boost-serialization
/// 序列化，从而在配置文件中保存规约函数
/// 2.作者能力有限，假如通过读取配置文件解析，然后生成配置则步骤如下：
///   1)运行Generator根据配置文件生成生成配置所需要追加的源代码
///   2)再次编译
///   3)运行Generator生成配置
/// 3)这步作者没有方法去除，存在用户定义的规约函数如何保存到配置文件的问题，在
/// 生成配置时这些函数都只是函数名，如果通过宏将函数名绑定到具体函数，那么用户
/// 需要在配置文件中写两遍函数，其中一遍还是定义EXPORT(func,"func")这样的宏，
/// 浪费有效时间而且维护不便，还需要运行两次Generator，严重影响使用体验。
/// 3.由于2的原因作者选择通过宏在配置文件中描述产生式和相关信息来定义产生式，
/// 只需定义配置一次，编译一次，运行一次，宏自动在需要的文件中生成相关代码
/// 4.该文件内定义多个表示产生式的宏，这些宏均用于表示产生式，并在需要的文件内
/// 转化为相应代码，避免每个需要生成代码的文件中都手动定义产生式
/// 5.该文件必须且仅被Config/ProductionConfig/production_config-inc.h包含，
/// production_config-inc.h需要且仅被process_functions_classes.h
/// process_functions_classes_register.h和config_construct.cpp三个文件包含，
/// 并在三个文件中生成不同代码
/// 6.该文件不应使用标准头文件保护，下面有特化的的宏保护
/// 7.用户定义头文件请在user_defined_functions.h中添加，
/// 禁止在production_config-inc.h中添加用户定义的头文件
/// 8.关键字词法分析优先级高于运算符，运算符词法分析优先级高于普通单词

// 这部分声明用户定义产生式用的宏的原型
// 同时还原这些宏为无实现，防止不同文件对宏的特化冲突或忘记#undef等问题
// 强烈建议调用宏时使用最简单的语法，避免运算
// 使用字符串时直接使用双引号类型（可以使用生肉字）
// 从而避免宏生成时表达式计算结果错误等问题
// 当前实现中使用3个单词优先级：0~2，普通单词优先级为0，运算符优先级为1，
// 关键字优先级为2；当一个单词对应多个正则解析为优先级高的正则

/// @brief 修饰非终结节点产生式名
/// @param[in] production_symbol ：产生式名
/// @param[in] production_body_seq ：区分产生式体的字符
/// @return 返回裸的包装Reduct函数用的类名
/// @note
/// 例：
/// NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(example1,2)
/// 展开为 example1_2_
#define NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(production_symbol,   \
                                             production_body_seq) \
  production_symbol##_##production_body_seq##_
/// @brief 修饰非终结节点产生式名
/// @param[in] production_symbol ：产生式名
/// @param[in] production_body_seq ：区分产生式体的字符
/// @return 返回包装Reduct函数用的类名的字符串形式
/// @note
/// 例：
/// NONTERMINAL_PRODUCTION_SYMBOL_MODIFY_STR(example1,2)
/// 展开为 "example1_2_"
#define NONTERMINAL_PRODUCTION_SYMBOL_MODIFY_STR(production_symbol,   \
                                                 production_body_seq) \
#production_symbol##"_"## #production_body_seq##"_"

#ifdef GENERATOR_DEFINE_KEY_WORD
#undef GENERATOR_DEFINE_KEY_WORD
#endif  // !GENERATOR_DEFINE_KEY_WORD
/// @brief 定义关键字
/// @param[in] key_word ：待定义的关键字字符串
/// @details
/// 例：GENERATOR_DEFINE_KEY_WORD("example_key_word")
/// @note 支持正则表达式
/// 关键字词法分析优先级高于运算符和普通单词
#define GENERATOR_DEFINE_KEY_WORD(key_word)

#ifdef GENERATOR_DEFINE_BINARY_OPERATOR
#undef GENERATOR_DEFINE_BINARY_OPERATOR
#endif  // !GENERATOR_DEFINE_BINARY_OPERATOR
/// @brief 定义双目运算符
/// @param[in] operator_symbol ：运算符字符串
/// @param[in] binary_operator_associatity
/// ：双目运算符结合性（枚举OperatorAssociatityType）
/// @param[in] binary_operator_priority ：双目运算符优先级
/// @details
/// 例：GENERATOR_DEFINE_BINARY_OPERATOR("example_symbol",
///                                      OperatorAssociatityType::kLeftToRight,
///                                      2)
/// @note 运算符优先级数值越大优先级越高
/// @attention 相同operator_symbol的运算符只能定义一次
#define GENERATOR_DEFINE_BINARY_OPERATOR( \
    operator_symbol, binary_operator_associatity, binary_operator_priority)

#ifdef GENERATOR_DEFINE_UNARY_OPERATOR
#undef GENERATOR_DEFINE_UNARY_OPERATOR
#endif  // !GENERATOR_DEFINE_UNARY_OPERATOR
/// @brief 定义左侧单目运算符
/// @param[in] operator_symbol ：运算符字符串
/// @param[in] unary_operator_associatity
/// ：左侧单目运算符结合性（枚举OperatorAssociatityType）
/// @param[in] unary_operator_priority ：左侧单目运算符优先级
/// @details
/// 例：GENERATOR_DEFINE_UNARY_OPERATOR("example_symbol",
///                                      OperatorAssociatityType::kLeftToRight,
///                                      2)
/// @note 运算符优先级数值越大优先级越高
/// @attention 相同operator_symbol的运算符只能定义一次
#define GENERATOR_DEFINE_UNARY_OPERATOR( \
    operator_symbol, unary_operator_associatity, unary_operator_priority)

#ifdef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#undef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#endif  // !GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
/// @brief 定义同时具有双目和左侧单目语义的运算符
/// @param[in] operator_symbol ：双语义运算符字符串
/// @param[in] binary_operator_associatity
/// ：双目运算符结合性（枚举OperatorAssociatityType）
/// @param[in] binary_operator_priority ：双目运算符优先级
/// @param[in] unary_operator_associatity
/// ：左侧单目运算符结合性（枚举OperatorAssociatityType）
/// @param[in] unary_operator_priority ：左侧单目运算符优先级
/// @details
/// 例：GENERATOR_DEFINE_BINARY_UNARY_OPERATOR("example_symbol",
///                                      OperatorAssociatityType::kLeftToRight,
///                                      2)
/// @attention 相同operator_symbol的运算符只能定义一次
/// 不允许通过组合GENERATOR_DEFINE_UNARY_OPERATOR和
/// GENERATOR_DEFINE_BINARY_OPERATOR来声明同时支持双目和左侧单目语义的运算符，
/// 必须使用该宏一次性声明这种运算符
#define GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(                             \
    operator_symbol, binary_operator_associatity, binary_operator_priority, \
    unary_operator_associatity, unary_operator_priority)

#ifdef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_TERMINAL_PRODUCTION
/// @brief 定义终结产生式
/// @param[in] production_symbol ：终结节点名字符串
/// @param[in] production_body ：终结节点体字符串（正则表达式形式）
/// @details
/// 例：GENERATOR_DEFINE_TERMINAL_PRODUCTION("example_symbol",
///                                          "[a-zA-Z_][a-zA-Z_0-9]*")
#define GENERATOR_DEFINE_TERMINAL_PRODUCTION(production_symbol, production_body)

#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
/// @brief 定义非终结产生式
/// @param[in] production_symbol ：非终结产生式名
/// @param[in] reduct_function_name ：规约非终结产生式的函数名
/// @param[in] production_body_seq ：区分不同产生式体的字符
/// @param[in] ... ：产生式体
/// @details
/// 产生式体部分使用初始化列表书写法
/// 例：{"production_body1","production_body2"}
/// 每一个相同的产生式名下各产生式体编号必须不同，可使用[a-zA-Z0-9_]+
/// 该编号用于区分不同的产生式体并为其构建相应的包装函数
/// 例：GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(example_symbol,
///                ExampleFunctionName, 0, false, {"Id", "=", "Assignable"})
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION( \
    production_symbol, reduct_function_name, production_body_seq, ...)

#ifdef GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT
#undef GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT
#endif  // GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT
/// @brief 设置非终结产生式可以空规约
/// @param[in] production_symbol ：待设置可以空规约的非终结产生式名
/// @note
/// 未调用此方法的非终结节点默认不可以空规约
/// 调用时仅需保证给定的非终结产生式已经定义，
/// 不要求已定义非终结产生式依赖的产生式
/// 例：GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(example_symbol)
#define GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT( \
    production_symbol)

#ifdef GENERATOR_DEFINE_ROOT_PRODUCTION
#undef GENERATOR_DEFINE_ROOT_PRODUCTION
#endif  // GENERATOR_DEFINE_BINARY_OPERATOR
/// @brief 设置根产生式
/// @param[in] production_symbol ：待设置的根非终结产生式名
/// @note
/// 调用时仅需已定义该非终结节点，不要求已定义非终结产生式依赖的产生式
/// 例：GENERATOR_DEFINE_ROOT_PRODUCTION(example_symbol)
#define GENERATOR_DEFINE_ROOT_PRODUCTION(production_symbol)

// 这部分宏对两种需要产生式信息的文件特化

// 在config_construct.cpp中转化为对SyntaxGenerator成员函数的调用代码
// 存放在ConfigConstruct函数中，向SyntaxGenerator传达产生式信息
#ifdef GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_

// 防止重复包含
#ifndef GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_END
// 定义该宏以屏蔽用来允许使用IntelliSense而包含的头文件
// 在函数定义内包含头文件会导致大量奇怪错误
#define SHIELD_HEADERS_FOR_INTELLISENSE

#ifdef GENERATOR_DEFINE_KEY_WORD
#undef GENERATOR_DEFINE_KEY_WORD
#endif  // GENERATOR_DEFINE_KEY_WORD
#define GENERATOR_DEFINE_KEY_WORD(key_word) AddKeyWord(key_word);

#ifdef GENERATOR_DEFINE_BINARY_OPERATOR
#undef GENERATOR_DEFINE_BINARY_OPERATOR
#endif  // GENERATOR_DEFINE_BINARY_OPERATOR
#define GENERATOR_DEFINE_BINARY_OPERATOR(                                   \
    operator_symbol, binary_operator_associatity, binary_operator_priority) \
  AddBinaryOperator(operator_symbol, binary_operator_associatity,           \
                    OperatorPriority(binary_operator_priority));

#ifdef GENERATOR_DEFINE_UNARY_OPERATOR
#undef GENERATOR_DEFINE_UNARY_OPERATOR
#endif  // GENERATOR_DEFINE_UNARY_OPERATOR
#define GENERATOR_DEFINE_UNARY_OPERATOR(                                  \
    operator_symbol, unary_operator_associatity, unary_operator_priority) \
  AddLeftUnaryOperator(operator_symbol, unary_operator_associatity,       \
                       OperatorPriority(unary_operator_priority));

#ifdef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#undef GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#endif  // !GENERATOR_DEFINE_BINARY_UNARY_OPERATOR
#define GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(                             \
    operator_symbol, binary_operator_associatity, binary_operator_priority, \
    unary_operator_associatity, unary_operator_priority)                    \
  AddBinaryLeftUnaryOperator(operator_symbol, binary_operator_associatity,  \
                             OperatorPriority(binary_operator_priority),    \
                             unary_operator_associatity,                    \
                             OperatorPriority(unary_operator_priority));

#ifdef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_TERMINAL_PRODUCTION
#define GENERATOR_DEFINE_TERMINAL_PRODUCTION(production_symbol, \
                                             production_body)   \
  AddTerminalProduction(production_symbol, production_body);

#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(                   \
    production_symbol, reduct_function, production_body_seq, ...)  \
  AddNonTerminalProduction<NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(   \
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

#else
#error 请勿在config_construct.cpp以外包含production_config-inc.h或重复包含
#endif
#endif

// 在process_function_classes.h中转化为包装规约函数的类
// 类名修饰方法见宏NONTERMINAL_PRODUCTION_SYMBOL_MODIFY和
// NONTERMINAL_PRODUCTION_SYMBOL_MODIFY_STR
#ifdef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_
// 防止被重复包含
#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_END

#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION

/// 定义该宏以屏蔽用来允许使用IntelliSense而包含的头文件
/// 在命名空间内包含这些头文件会导致各种奇怪的错误
#define SHIELD_HEADERS_FOR_INTELLISENSE

#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(                              \
    production_symbol, reduct_function, production_body_seq, ...)             \
  class NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(production_symbol,               \
                                             production_body_seq)             \
      : public ProcessFunctionInterface {                                     \
   public:                                                                    \
    virtual ProcessFunctionInterface::UserData Reduct(                        \
        std::vector<ProcessFunctionInterface::WordDataToUser>&& word_data)    \
        const override {                                                      \
      return reduct_function(std::move(word_data));                           \
    }                                                                         \
                                                                              \
   private:                                                                   \
    friend class boost::serialization::access;                                \
    template <class Archive>                                                  \
    void serialize(Archive& ar, const unsigned int version) {                 \
      ar& boost::serialization::base_object<ProcessFunctionInterface>(*this); \
    }                                                                         \
  };
#endif
#endif

// 在process_functions_classes_register.h中转化为注册
// process_function_classes.h中派生类的宏，配合boost-serialization使用
#ifdef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER
/// 防止重复包含
#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_END

/// 定义该宏以屏蔽用来允许使用IntelliSense而包含的头文件
#define SHIELD_HEADERS_FOR_INTELLISENSE

#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  /// GENERATOR_DEFINE_NONTERMINAL_PRODUCTION

#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(                     \
    production_symbol, reduct_function, production_body_seq, ...)    \
  BOOST_CLASS_EXPORT_GUID(                                           \
      frontend::generator::syntax_generator::                        \
          NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(production_symbol,    \
                                               production_body_seq), \
      "frontend::generator::"                                        \
      "syntax_generator::" NONTERMINAL_PRODUCTION_SYMBOL_MODIFY_STR( \
          production_symbol, production_body_seq))
#else
#errror 请勿在process_functions_classes_register.h文件以外包含 \
    "production_config-inc.h"或重复包含
#endif
#endif

/// 为了使用Intellisense在这个宏里包含需要的头文件
/// 此处的头文件在代码生成过程中会被忽略
/// 用户请在user_defined_functions.h内添加自己定义的头文件
/// 在每个包含该文件的文件中都应定义SHIELD_HEADERS_FOR_INTELLISENSE宏以屏蔽这些
/// 文件，否则会出现各种奇怪报错
#ifndef SHIELD_HEADERS_FOR_INTELLISENSE
#include "Common/common.h"
#include "Config/ProductionConfig/user_defined_functions.h"
#include "Generator/export_types.h"
/// 提供运算符结合性的枚举
using OperatorAssociatityType =
    frontend::generator::syntax_generator::OperatorAssociatityType;
#endif  /// !SHIELD_HEADERS_FOR_INTELLIGENCE