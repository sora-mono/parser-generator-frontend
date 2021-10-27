#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_REGISTER_H_
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_REGISTER_H_

#include "process_function_interface.h"

// 注册基类以允许序列化
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::ProcessFunctionInterface,
    "frontend::generator::syntax_generator::ProcessFunctionInterface")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::RootReductClass,
    "frontend::generator::syntax_generator::RootReductClass")

#endif  // !GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_REGISTER_H_