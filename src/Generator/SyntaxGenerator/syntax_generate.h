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
// 所有的宏中的产生式名/运算符名都无需加双引号
// 仅产生式体需要按照初始化列表的写法书写

// 修饰非终结节点产生式名以获取包装Reduct函数用的类名
#define NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(production_symbol,   \
                                             production_body_seq) \
  (production_symbol##production_body_seq##_)

// 以下定义除GENERATOR_DEFINE_NONTERMINAL_PRODUCTION特殊说明外
// 字符串类参数均需要写成字符串形式
// 关键字定义
#ifdef GENERATOR_DEFINE_KEY_WORD
#undef GENERATOR_DEFINE_KEY_WORD
#endif  // !GENERATOR_DEFINE_KEY_WORD
#define GENERATOR_DEFINE_KEY_WORD(key_word)

// 运算符符号，结合性(kLeftToRight, kRightAssociate)，优先级
#ifdef GENERATOR_DEFINE_OPERATOR
#undef GENERATOR_DEFINE_OPERATOR
#endif  // !GENERATOR_DEFINE_OPERATOR
#define GENERATOR_DEFINE_OPERATOR(operator_symbol, operator_associatity, \
                                  operator_priority)

// 根产生式定义，需要在给定产生式被正式添加后使用
// （即该产生式所依赖的全部节点都能够添加后）
#ifdef GENERATOR_DEFINE_ROOT_PRODUCTION
#undef GENERATOR_DEFINE_ROOT_PRODUCTION
#endif  // GENERATOR_DEFINE_OPERATOR
#define GENERATOR_DEFINE_ROOT_PRODUCTION(production_symbol)

// 终结节点定义，production_body为正则表达式
#ifdef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_TERMINAL_PRODUCTION
#define GENERATOR_DEFINE_TERMINAL_PRODUCTION(production_symbol, production_body)

// 非终结节点定义，产生式体部分请使用初始化列表书写法
// 例：{"production_body1","production_body2"}
// 四个参数都不在两侧加双引号
// reduct_function是规约用函数
// production_body_seq是产生式体编号
// 每一个相同的产生式名下各产生式体编号必须不同，可使用[a-zA-Z0-9_]+
// 该编号用于区分不同的产生式体并为其构建相应的包装函数
// could_empty_reduct表示该产生式是否可以空规约
// 所有规约到同一个production_symbol的产生式体中只需出现一次
#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(                              \
    production_symbol, production_body, reduct_function, production_body_seq, \
    could_empty_reduct)

// 这部分对三种需要产生式信息的文件特化
#ifdef GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_

#ifdef GENERATOR_DEFINE_KEY_WORD
#undef GENERATOR_DEFINE_KEY_WORD
#endif  // GENERATOR_DEFINE_KEY_WORD
#define GENERATOR_DEFINE_KEY_WORD(key_word) AddKeyWord((key_word));

#ifdef GENERATOR_DEFINE_OPERATOR
#undef GENERATOR_DEFINE_OPERATOR
#endif  // GENERATOR_DEFINE_OPERATOR
#define GENERATOR_DEFINE_OPERATOR(operator_symbol, operator_associatity, \
                                  operator_priority)                     \
  AddOperatorNode((operator_symbol), (operator_associatity),             \
                  (OperatorPriority(operator_priority)));

#ifdef GENERATOR_DEFINE_ROOT_PRODUCTION
#undef GENERATOR_DEFINE_ROOT_PRODUCTION
#endif  // GENERATOR_DEFINE_ROOT_PRODUCTION
#define GENERATOR_DEFINE_ROOT_PRODUCTION(production_symbol) \
  SetRootProduction((production_symbol));

#ifdef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_TERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_TERMINAL_PRODUCTION
#define GENERATOR_DEFINE_TERMINAL_PRODUCTION(production_symbol, \
                                             production_body)   \
  AddTerminalNode((production_symbol), (production_body));

#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(                              \
    production_symbol, production_body, reduct_function, production_body_seq, \
    could_empty_reduct)                                                       \
  AddNonTerminalNode<NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(                    \
      production_symbol, production_body_seq)>(                               \
      (#production_symbol), (production_body), (could_empty_reduct));

#elif defined GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_

#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION

#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(                              \
    production_symbol, production_body, reduct_function, production_body_seq, \
    could_empty_reduct)                                                       \
  class NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(production_symbol,               \
                                             production_body_seq)             \
      : public ProcessFunctionInterface {                                     \
   public:                                                                    \
    virtual ProcessFunctionInterface::UserData Reduct(                        \
        std::vector<ProcessFunctionInterface::WordDataToUser>&& word_data)    \
        override {                                                            \
      return reduct_function(std::move(word_data));                           \
    }                                                                         \
  };

#elif defined GENERATOR_SYNTAXGENERATOR_USER_DEFINED_FUNCTION_AND_DATA_REGISTER_

#ifdef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#undef GENERATOR_DEFINE_NONTERMINAL_PRODUCTION
#endif  // GENERATOR_DEFINE_NONTERMINAL_PRODUCTION

#define GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(                              \
    production_symbol, production_body, reduct_function, production_body_seq, \
    could_empty_reduct)                                                       \
  BOOST_CLASS_EXPORT_GUID(                                                    \
      frontend::generator::syntaxgenerator::                                  \
          NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(production_symbol,             \
                                               production_body_seq),          \
      "frontend::generator::"                                                 \
      "syntaxgenerator:" #NONTERMINAL_PRODUCTION_SYMBOL_MODIFY(               \
          production_symbol, production_body_seq))

#else

#error 该文件仅且必须最终被Generator/SyntaxGenerator下的\
process_functions_classes.h \
user_defined_function_and_data_register.h \
config_construct.cpp所包含

#endif  // GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_MACRO