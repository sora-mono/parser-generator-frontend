/// @file process_function_interface_register.h
/// @brief 注册process_function_interface.h中派生类的宏
/// @details
/// boost序列化库序列化派生类前必须先注册，否则会抛异常
/// 该文件内为注册process_function_interface.h中派生类的宏
#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_REGISTER_H_
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_REGISTER_H_

#include <boost/serialization/export.hpp>

#include "process_function_interface.h"

BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::RootReductClass,
    "frontend::generator::syntax_generator::RootReductClass")

#endif  /// !GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_REGISTER_H_