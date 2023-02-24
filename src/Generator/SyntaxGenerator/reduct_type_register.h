/// @file reduct_type_register.h
/// @brief 注册非终结产生式规约函数返回类型
/// @details
/// 注册非终结产生式规约函数返回类型，用于规约时实现自动类型转换和类型检查

#ifndef GENERATOR_SYNTAXGENERATOR_REDUCT_TYPE_REGISTER_H_
#define GENERATOR_SYNTAXGENERATOR_REDUCT_TYPE_REGISTER_H_

#include <string>

#include "syntax_generator_type_traits.h"

#define GENERATOR_SYNTAXGENERATOR_REDUCT_TYPE_REGISTER
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_SYNTAXGENERATOR_REDUCT_TYPE_REGISTER

#endif  // !GENERATOR_SYNTAXGENERATOR_REDUCT_TYPE_REGISTER_H_
