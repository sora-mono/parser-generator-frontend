#ifndef GENERATOR_SYNTAXGENERATOR_USER_DEFINED_FUNCTION_AND_DATA_REGISTER_H_
#define GENERATOR_SYNTAXGENERATOR_USER_DEFINED_FUNCTION_AND_DATA_REGISTER_H_
namespace frontend::generator::syntaxgenerator {
// 下面的宏将包含的文件中用户定义的产生式转化为定义规约函数的类
// 类名修饰方法见syntax_generate.h
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_

}  // namespace frontend::generator

#endif  // !GENERATOR_SYNTAXGENERATOR_USER_DEFINED_FUNCTION_AND_DATA_REGISTER_H_