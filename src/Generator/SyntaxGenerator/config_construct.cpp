/// @file config_construct.cpp
/// @brief 定义添加构建配置所需外围信息的函数ConfigConstruct
/// @details
/// 该文件将用户通过宏定义的产生式转化为C++代码来控制构建语法分析机配置的过程
/// 包含的文件process_functions_classes.h中定义包装用户定义的规约函数的类
/// 这些类用于AddNonTerminalNode
#include "process_functions_classes.h"
#include "syntax_generator.h"
#include "Common/common.h"
#include "Config/ProductionConfig/user_defined_functions.h"

namespace frontend::generator::syntax_generator {
/// @brief 添加构建配置所需外围信息
/// @details
/// 该函数通过宏将Config/ProductionConfig/production_config-inc.h内以宏的形式
/// 定义的产生式转化为AddKeyWord、AddTerminalNode、AddBinaryOperatorNode、
/// AddLeftUnaryOperatorNode、AddBinaryUnaryOperatorNode、AddNonTerminalNode
/// 等函数来添加构建配置所需的外围信息
void SyntaxGenerator::ConfigConstruct() {
  // 下面的宏将包含的文件中用户定义的产生式转化为产生式构建配置
  // 如AddTerminalNode、AddNonTerminalNode等
#define GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_
}
}  // namespace frontend::generator::syntax_generator