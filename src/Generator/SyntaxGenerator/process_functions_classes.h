// 该文件将用户定义的产生式转化为包装规约函数的类
#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESS_FUNCTIONS_CLASSES_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESS_FUNCTIONS_CLASSES_H_

#include "Config/ProductionConfig/user_defined_functions.h"
#include "process_function_interface.h"

namespace frontend::generator::syntax_generator {
// 下面的宏将包含的文件中用户定义的产生式转化为定义规约函数的类
// 类名修饰方法见syntax_generate.h

// 定义该宏以屏蔽用来允许使用IntelliSense而包含的头文件
// 在命名空间内包含这些头文件会导致各种奇怪的错误
#define SHIELD_HEADERS_FOR_INTELLISENSE
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_LEXICALGENERATOR_PROCESS_FUNCTIONS_CLASSES_
// 定义结束标志
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_END
}  // namespace frontend::generator::syntax_generator

#endif  // !GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESSFUNCTIONS_H_