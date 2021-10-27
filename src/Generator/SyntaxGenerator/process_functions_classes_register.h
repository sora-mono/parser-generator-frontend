// 该头文件为所有包装用户定义函数相关的类的注册代码
// 注册派生类为了boost::serialization可以序列化这些类
#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_H_
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_H_

#include <boost/serialization/export.hpp>

#include "process_function_interface_register.h"
#include "process_functions_classes.h"

// 在boost::serialization中注册包装用户定义函数的类以序列化派生类
// 下面的宏生成在process_functions_classes.h中生成的类
// 在boost::serialization中注册的代码
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER

// 定义结束标志
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_END

#endif  // !GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_H_