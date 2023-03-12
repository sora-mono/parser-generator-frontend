/// @file process_functions_classes_register.h
/// @brief 向boost-serialization注册process_functions_classes.h中定义的派生类
/// @details
/// boost序列化库序列化派生类前必须先注册，否则会抛异常
/// 该文件中定义注册process_functions_classes.h中派生类的宏
#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_H_
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_H_

#include <boost/serialization/export.hpp>

#include "process_function_interface_register.h"
#include "process_functions_classes.h"

/// 下面的宏生成process_functions_classes.h中的类在boost::serialization中注册
/// 的代码
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER

#endif  /// !GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTIONS_CLASSES_REGISTER_H_