/// 该文件为需要序列化的SyntaxGenerator中定义的内部实现需要的类的注册代码
/// 注册这些类以便boost::serialization可以序列化这些类
#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_CLASSES_REGISTER_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_CLASSES_REGISTER_H_

#include <boost/serialization/export.hpp>

#include "Generator/export_types.h"
#include "syntax_analysis_table.h"

/// 注册序列化需要使用的类
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::ProductionNodeId,
    "frontend::generator::syntax_generator::ProductionNodeId")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::ProcessFunctionClassId,
    "frontend::generator::syntax_generator::ProcessFunctionClassId")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::ProcessFunctionClassManagerType,
    "frontend::generator::syntax_generator::ProcessFunctionClassManagerType")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::SyntaxAnalysisTableEntryId,
    "frontend::generator::syntax_generator::SyntaxAnalysisTableEntryId")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::SyntaxAnalysisTableEntry::
        ActionAndAttachedDataInterface,
    "frontend::generator::syntax_generator::SyntaxAnalysisTableEntry::"
    "ActionAndAttachedDataInterface")
BOOST_CLASS_EXPORT_GUID(frontend::generator::syntax_generator::
                            SyntaxAnalysisTableEntry::ShiftAttachedData,
                        "frontend::generator::syntax_generator::"
                        "SyntaxAnalysisTableEntry::ShiftAttachedData")
BOOST_CLASS_EXPORT_GUID(frontend::generator::syntax_generator::
                            SyntaxAnalysisTableEntry::ReductAttachedData,
                        "frontend::generator::syntax_generator::"
                        "SyntaxAnalysisTableEntry::ReductAttachedData")
BOOST_CLASS_EXPORT_GUID(frontend::generator::syntax_generator::
                            SyntaxAnalysisTableEntry::ShiftReductAttachedData,
                        "frontend::generator::syntax_generator::"
                        "SyntaxAnalysisTableEntry::ShiftReductAttachedData")
BOOST_CLASS_EXPORT_GUID(frontend::generator::syntax_generator::
                            SyntaxAnalysisTableEntry::AcceptAttachedData,
                        "frontend::generator::syntax_generator::"
                        "SyntaxAnalysisTableEntry::AcceptAttachedData")

#endif  /// !GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_CLASSES_REGISTER_H_
