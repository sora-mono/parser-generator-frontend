#ifndef CPARSERFRONTEND_PARSE_FUNCTIONS_H_
#define CPARSERFRONTEND_PARSE_FUNCTIONS_H_
#include <format>
#include <iostream>
#include <limits>

#include "Generator/SyntaxGenerator/process_function_interface.h"
#include "c_parser_frontend.h"
#include "operator_node.h"
#include "type_system.h"
namespace c_parser_frontend::parse_functions {
using WordDataToUser = frontend::generator::syntaxgenerator::
    ProcessFunctionInterface::WordDataToUser;
using c_parser_frontend::operator_node::BasicTypeInitializeOperatorNode;
using c_parser_frontend::operator_node::InitializeType;
using c_parser_frontend::operator_node::LeftRightValueTag;
using c_parser_frontend::operator_node::OperatorNodeInterface;
using c_parser_frontend::operator_node::VarietyOperatorNode;
using c_parser_frontend::type_system::BuiltInType;
using c_parser_frontend::type_system::CommonlyUsedTypeGenerator;
using c_parser_frontend::type_system::ConstTag;
using c_parser_frontend::type_system::EndType;
using c_parser_frontend::type_system::EnumType;
using c_parser_frontend::type_system::PointerType;
using c_parser_frontend::type_system::SignTag;
using c_parser_frontend::type_system::TypeInterface;

// 声明全局变量：C语言编译器前端的类对象
c_parser_frontend::CParserFrontend parser_front_end;

// IdOrEquivence产生式规约时得到的数据
class IdOrEquivenceReturnData {
 public:
  IdOrEquivenceReturnData(std::shared_ptr<VarietyOperatorNode> variety_node,
                          std::shared_ptr<TypeInterface> type_chain_head,
                          std::shared_ptr<TypeInterface> type_chain_tail)
      : variety_node_(std::move(variety_node)),
        type_chain_head_(std::move(type_chain_head)),
        type_chain_tail_(std::move(type_chain_tail)) {}
  std::shared_ptr<VarietyOperatorNode> GetVarietyNodePoitner() const {
    return variety_node_;
  }
  VarietyOperatorNode& GetVarietyNodeReference() const {
    return *variety_node_;
  }
  void SetVarietyNode(std::shared_ptr<VarietyOperatorNode>&& variety_node) {
    variety_node_ = std::move(variety_node);
  }
  std::shared_ptr<TypeInterface> GetTypeChainHeadPointer() const {
    return type_chain_head_;
  }
  TypeInterface& GetTypeChainHeadReference() const { return *type_chain_head_; }
  void SetTypeChainHead(std::shared_ptr<TypeInterface>&& type_chain_head) {
    type_chain_head_ = std::move(type_chain_head);
  }
  std::shared_ptr<TypeInterface> GetTypeChainTailPointer() const {
    return type_chain_tail_;
  }
  TypeInterface& GetTypeChainTailReference() const { return *type_chain_tail_; }
  void SetTypeChainTail(std::shared_ptr<TypeInterface>&& type_chain_tail) {
    type_chain_tail_ = std::move(type_chain_tail);
  }
  // 完成变量的归并，去除作为优化手段在type_chain_head_设置的哨兵节点
  // 给variety_node_设置类型并返回指向variety_node_的指针
  // 调用该函数后type_chain_head_失效
  std::shared_ptr<VarietyOperatorNode> FinishVarietyNodeReduct() {
    type_chain_head_ = type_chain_head_->GetNextNodePointer();
    variety_node_->SetVarietyType(std::move(type_chain_head_));
    return variety_node_;
  }

 private:
  // 变量节点
  std::shared_ptr<VarietyOperatorNode> variety_node_;
  // 指向类型链的头结点
  std::shared_ptr<TypeInterface> type_chain_head_;
  // 指向类型链的尾节点
  std::shared_ptr<TypeInterface> type_chain_tail_;
};
// 枚举参数规约时得到的数据
class EnumReturnData {
 public:
  EnumType::EnumContainerType& GetContainer() { return enum_container_; }
  long long GetMaxValue() const { return max_value_; }
  void SetMaxValue(long long max_value) { max_value_ = max_value; }
  long long GetMinValue() const { return min_value_; }
  void SetMinValue(long long min_value) { min_value_ = min_value; }
  long long GetLastValue() const { return last_value_; }
  void SetLastValue(long long last_value) { last_value_ = last_value; }

 private:
  // 枚举名与值的关联容器
  EnumType::EnumContainerType enum_container_;
  // 最大的枚举值
  long long max_value_ = LLONG_MIN;
  // 最小的枚举值
  long long min_value_ = LLONG_MAX;
  // 上次添加的枚举值
  long long last_value_;
};

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
BuiltInType FundamentalTypeChar(std::vector<WordDataToUser>&& word_data) {
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
IdOrEquivenceReturnData IdOrEquivenceConstTagId(
    std::vector<WordDataToUser>&& word_data);
// IdOrEquivence->ConstTag
IdOrEquivenceReturnData IdOrEquivenceConstTag(
    std::vector<WordDataToUser>&& word_data);
// IdOrEquivence->IdOrEquivence "[" Num "]"
// 返回值类型：IdOrEquivenceReturnData
// 返回std::any防止移动构造IdOrEquivenceReturnData
std::any IdOrEquivenceNumAddressing(std::vector<WordDataToUser>&& word_data);
// IdOrEquivence->IdOrEquivence "[" "]"
// 返回值类型：IdOrEquivenceReturnData
// 返回std::any防止移动构造IdOrEquivenceReturnData
// 设置新添加的指针所对应数组大小为-1来标记此处数组大小需要根据赋值结果推断
std::any IdOrEquivenceAnonymousAddressing(
    std::vector<WordDataToUser>&& word_data);
// IdOrEquivence->Consttag "*" IdOrEquivence
// 返回值类型：IdOrEquivenceReturnData
// 返回std::any防止移动构造IdOrEquivenceReturnData
std::any IdOrEquivencePointerAnnounce(std::vector<WordDataToUser>&& word_data);
// IdOrEquivence->"(" IdOrEquivence ")"
// 返回值类型：IdOrEquivenceReturnData
// 返回std::any防止移动构造IdOrEquivenceReturnData
std::any IdOrEquivenceInBrackets(std::vector<WordDataToUser>&& word_data);
// NotEmptyEnumArguments-> Id
EnumReturnData NotEmptyEnumArgumentsIdInit(
    std::vector<WordDataToUser>&& word_data);
// NotEmptyEnumArguments-> Id "=" Num
EnumReturnData NotEmptyEnumArgumentsIdAssignNumInit(
    std::vector<WordDataToUser>&& word_data);
// NotEmptyEnumArguments-> NotEmptyEnumArguments "," Id
// 返回值类型：EnumReturnData
std::any NotEmptyEnumArgumentsId(std::vector<WordDataToUser>&& word_data);
// NotEmptyEnumArguments-> NotEmptyEnumArguments "," Id "=" Num
// 返回值类型：EnumReturnData
std::any NotEmptyEnumArgumentsIdAssignNum(
    std::vector<WordDataToUser>&& word_data);
// EnumArguments->NotEmptyEnumArguments
// 返回值类型：EnumReturnData
std::any EnumArgumentsNotEmptyEnumArguments(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::move(word_data.front().GetNonTerminalWordData().user_data_);
}
}  // namespace c_parser_frontend::parse_functions
#endif