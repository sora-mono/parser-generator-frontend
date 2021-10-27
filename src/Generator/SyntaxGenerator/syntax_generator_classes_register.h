// 该文件为需要序列化的SyntaxGenerator中定义的内部实现需要的类的注册代码
// 注册这些类以便boost::serialization可以序列化这些类
#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_CLASSES_REGISTER_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_CLASSES_REGISTER_H_

#include "syntax_generator.h"

// 注册序列化需要使用的类
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::
        ExportSyntaxGeneratorInsideTypeForSerialization::ProductionNodeId,
    "frontend::generator::syntax_generator::SyntaxGenerator::ProductionNodeId")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::
        ExportSyntaxGeneratorInsideTypeForSerialization::ProcessFunctionClassId,
    "frontend::generator::syntax_generator::SyntaxGenerator::"
    "ProcessFunctionClassId")
BOOST_CLASS_EXPORT_GUID(frontend::generator::syntax_generator::
                            ExportSyntaxGeneratorInsideTypeForSerialization::
                                ProcessFunctionClassManagerType,
                        "frontend::generator::syntax_generator::"
                        "SyntaxGenerator::ProcessFunctionClassManagerType")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::
        ExportSyntaxGeneratorInsideTypeForSerialization::ParsingTableType,
    "frontend::generator::syntax_generator::SyntaxGenerator::ParsingTableType")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::
        ExportSyntaxGeneratorInsideTypeForSerialization::ParsingTableEntry,
    "frontend::generator::syntax_generator::SyntaxGenerator::ParsingTableEntry")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::
        ExportSyntaxGeneratorInsideTypeForSerialization::ParsingTableEntryId,
    "frontend::generator::syntax_generator::SyntaxGenerator::"
    "ParsingTableEntryId")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::
        ExportSyntaxGeneratorInsideTypeForSerialization::
            ParsingTableEntryActionAndReductDataInterface,
    "frontend::generator::syntax_generator::SyntaxGenerator::"
    "ParsingTableEntryActionAndReductDataInterface")
BOOST_CLASS_EXPORT_GUID(frontend::generator::syntax_generator::
                            ExportSyntaxGeneratorInsideTypeForSerialization::
                                ParsingTableEntryShiftAttachedData,
                        "frontend::generator::syntax_generator::"
                        "SyntaxGenerator::ParsingTableEntryShiftAttachedData")
BOOST_CLASS_EXPORT_GUID(frontend::generator::syntax_generator::
                            ExportSyntaxGeneratorInsideTypeForSerialization::
                                ParsingTableEntryReductAttachedData,
                        "frontend::generator::syntax_generator::"
                        "SyntaxGenerator::ParsingTableEntryReductAttachedData")
BOOST_CLASS_EXPORT_GUID(
    frontend::generator::syntax_generator::
        ExportSyntaxGeneratorInsideTypeForSerialization::
            ParsingTableEntryShiftReductAttachedData,
    "frontend::generator::syntax_generator::SyntaxGenerator::"
    "ParsingTableEntryShiftReductAttachedData")

#endif  // !GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_CLASSES_REGISTER_H_
