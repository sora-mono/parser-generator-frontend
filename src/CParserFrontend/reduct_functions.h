#ifndef CPARSERFRONTEND_PARSE_FUNCTIONS_H_
#define CPARSERFRONTEND_PARSE_FUNCTIONS_H_
#include "Generator/SyntaxGenerator/process_function_interface.h"
#include "c_parser_frontend.h"
#include "operator_node.h"
#include "type_system.h"
namespace c_parser_frontend::parse_functions {
using WordDataToUser = frontend::generator::syntaxgenerator::
    ProcessFunctionInterface::WordDataToUser;
using c_parser_frontend::operator_node::BasicTypeInitializeOperatorNode;
using c_parser_frontend::operator_node::InitializeType;
using c_parser_frontend::type_system::BuiltInType;
using c_parser_frontend::type_system::CommonlyUsedTypeGenerator;
using c_parser_frontend::type_system::ConstTag;
using c_parser_frontend::type_system::SignTag;
// 编译器前端
c_parser_frontend::CParserFrontend parser_front_end;

// 产生式规约时使用的函数

// SingleConstexprValue->Char
// 属性：InitializeType::kBasic，BuiltInType::kChar，SignTag::kSigned
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueChar(
    std::vector<WordDataToUser>&& word_data);
// SingleConstexprValue->Num
// 属性：InitializeType::kBasic，BuiltInType::kChar，SignTag::kSigned
// TODO 精确获取给定数值对应的类型
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueNum(
    std::vector<WordDataToUser>&& word_data);
// SingleConstexprValue->Str
// 属性：InitializeType::String，TypeInterface:: const char*
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueString(
    std::vector<WordDataToUser>&& word_data);
// FundamentalType->"char"
std::any FundamentalTypeChar(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "char");
  return BuiltInType::kChar;
}
// FundamentalType->"short"
BuiltInType FundamentalTypeShort(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "short");
  return BuiltInType::kShort;
}
// FundamentalType->"int"
BuiltInType FundamentalTypeInt(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "int");
  return BuiltInType::kInt;
}
// FundamentalType->"long"
BuiltInType FundamentalTypeLong(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "long");
  return BuiltInType::kLong;
}
// FundamentalType->"float"
BuiltInType FundamentalTypeFloat(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "float");
  return BuiltInType::kFloat;
}
// FundamentalType->"double"
BuiltInType FundamentalTypeDouble(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "double");
  return BuiltInType::kDouble;
}
// SignTag->"signed"
SignTag SignTagSigned(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "signed");
  return SignTag::kSigned;
}
// SignTag->"unsigned"
SignTag SignTagUnSigned(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "unsigned");
  return SignTag::kUnsigned;
}
// ConstTag->"const"
ConstTag ConstTagConst(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "const");
  return ConstTag::kConst;
}
// IdOrEquivence->ConstTag Id
std::shared_ptr<VarietyOperatorNode> IdOrEquivenceConstTagId(std::vector<WordDataToUser>&& word_data);
}  // namespace c_parser_frontend::parse_functions

#endif