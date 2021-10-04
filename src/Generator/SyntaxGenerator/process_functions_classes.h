// 该文件将用户定义的产生式转化为包装规约函数的类
#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESS_FUNCTIONS_CLASSES_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESS_FUNCTIONS_CLASSES_H_

#include "Config/ProductionConfig/user_defined_functions.h"
#include "process_function_interface.h"

namespace frontend::generator::syntaxgenerator {
// 下面的宏将包含的文件中用户定义的产生式转化为定义规约函数的类
// 类名修饰方法见syntax_generate.h
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_LEXICALGENERATOR_PROCESS_FUNCTIONS_CLASSES_
// 定义结束标志
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_END
}  // namespace frontend::generator::syntaxgenerator

// 在boost::serialization中注册包装用户定义函数的类以序列化派生类
#include <boost/serialization/export.hpp>
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER

// 定义结束标志
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_END

#endif  // !GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESSFUNCTIONS_H_