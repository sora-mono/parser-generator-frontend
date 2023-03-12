/// @file process_functions_classes.h
/// @brief 根据用户定义的非终结产生式定义包装规约函数的类
/// @details
/// 为了避免写链接脚本，通过虚函数机制来调用用户定义的规约函数，以达到根据
/// 不同产生式调用不同规约函数的目的
/// 每个非终结产生式体都会定义对应的唯一包装规约函数的类，Generator在构建配置时
/// 每个类都实例化唯一的对象，这些对象在保存/加载配置时一并序列化
/// 规约时先获取对应产生式的对象，然后调用Reduct虚函数规约
#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESS_FUNCTIONS_CLASSES_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESS_FUNCTIONS_CLASSES_H_
#include "process_function_interface.h"
#include "reduct_type_register.h"
#include <list>
#include "Logger/logger.h"

namespace frontend::generator::syntax_generator {
// 下面的宏将包含的文件中用户定义的非终结产生式转化为包装规约函数的类
// 类名修饰方法见syntax_generate.h中NONTERMINAL_PRODUCTION_SYMBOL_MODIFY和
// NONTERMINAL_PRODUCTION_SYMBOL_MODIFY_STR
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_
}  // namespace frontend::generator::syntax_generator

#endif  /// !GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESSFUNCTIONS_H_