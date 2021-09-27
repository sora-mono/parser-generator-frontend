#ifndef CPARSERFRONTEND_PARSE_FUNCTIONS_H_
#define CPARSERFRONTEND_PARSE_FUNCTIONS_H_
#include <format>
#include <iostream>
#include <limits>

#include "Generator/SyntaxGenerator/process_function_interface.h"
#include "c_parser_frontend.h"
#include "flow_control.h"
#include "operator_node.h"
#include "type_system.h"
namespace c_parser_frontend {
// 声明线程全局变量：C语言编译器前端的类对象
extern thread_local CParserFrontend parser_frontend;
}  // namespace c_parser_frontend

namespace c_parser_frontend::parse_functions {
using WordDataToUser = frontend::generator::syntaxgenerator::
    ProcessFunctionInterface::WordDataToUser;
using c_parser_frontend::parser_frontend;
using c_parser_frontend::flow_control::AllocateOperatorNode;
using c_parser_frontend::flow_control::DoWhileSentence;
using c_parser_frontend::flow_control::FlowInterface;
using c_parser_frontend::flow_control::FlowType;
using c_parser_frontend::flow_control::ForSentence;
using c_parser_frontend::flow_control::FunctionDefine;
using c_parser_frontend::flow_control::IfSentence;
using c_parser_frontend::flow_control::Label;
using c_parser_frontend::flow_control::LoopSentenceInterface;
using c_parser_frontend::flow_control::SimpleSentence;
using c_parser_frontend::flow_control::SwitchSentence;
using c_parser_frontend::flow_control::WhileSentence;
using c_parser_frontend::operator_node::AssignableCheckResult;
using c_parser_frontend::operator_node::AssignOperatorNode;
using c_parser_frontend::operator_node::BasicTypeInitializeOperatorNode;
using c_parser_frontend::operator_node::DereferenceOperatorNode;
using c_parser_frontend::operator_node::FunctionCallOperatorNode;
using c_parser_frontend::operator_node::GeneralOperationType;
using c_parser_frontend::operator_node::InitializeOperatorNodeInterface;
using c_parser_frontend::operator_node::InitializeType;
using c_parser_frontend::operator_node::LeftRightValueTag;
using c_parser_frontend::operator_node::ListInitializeOperatorNode;
using c_parser_frontend::operator_node::LogicalOperation;
using c_parser_frontend::operator_node::LogicalOperationOperatorNode;
using c_parser_frontend::operator_node::MathematicalAndAssignOperation;
using c_parser_frontend::operator_node::MathematicalOperation;
using c_parser_frontend::operator_node::MathematicalOperatorNode;
using c_parser_frontend::operator_node::MemberAccessOperatorNode;
using c_parser_frontend::operator_node::ObtainAddressOperatorNode;
using c_parser_frontend::operator_node::OperatorNodeInterface;
using c_parser_frontend::operator_node::TemaryOperatorNode;
using c_parser_frontend::operator_node::TypeConvert;
using c_parser_frontend::operator_node::VarietyOperatorNode;
using c_parser_frontend::type_system::AddTypeResult;
using c_parser_frontend::type_system::BasicType;
using c_parser_frontend::type_system::BuiltInType;
using c_parser_frontend::type_system::CommonlyUsedTypeGenerator;
using c_parser_frontend::type_system::ConstTag;
using c_parser_frontend::type_system::EndType;
using c_parser_frontend::type_system::EnumType;
using c_parser_frontend::type_system::FunctionType;
using c_parser_frontend::type_system::GetTypeResult;
using c_parser_frontend::type_system::PointerType;
using c_parser_frontend::type_system::SignTag;
using c_parser_frontend::type_system::StructOrBasicType;
using c_parser_frontend::type_system::StructType;
using c_parser_frontend::type_system::StructureTypeInterface;
using c_parser_frontend::type_system::TypeInterface;
using c_parser_frontend::type_system::UnionType;
using DeclineMathematicalComputeTypeResult = c_parser_frontend::operator_node::
    MathematicalOperatorNode::DeclineMathematicalComputeTypeResult;

class ObjectConstructData;
// 构建结构数据时使用的全局变量，用于优化执行逻辑
// 保存正在构建的结构数据类型
static thread_local std::shared_ptr<StructureTypeInterface>
    structure_type_constructuring;
// 构建函数类型时使用的全局变量，用于优化执行逻辑
// 保存管理构建数据的类
static thread_local std::shared_ptr<ObjectConstructData>
    function_type_construct_data;
// 构建函数调用节点时使用的全局变量，用于优化执行逻辑
static thread_local std::shared_ptr<FunctionCallOperatorNode>
    function_call_operator_node;

// 构建变量对象/函数对象时使用的数据
class ObjectConstructData {
 public:
  // 对类型链构建过程的检查结果
  enum class CheckResult {
    // 成功的情况
    kSuccess,
    // 失败的情况
    kAttachToTerminalType,  // 尾节点已经是终结类型（结构类/基础类型）
                            // 不能连接下一个节点
    kPointerEnd,      // 类型链尾部为指针而不是终结类型
    kReturnFunction,  // 函数试图返回函数而非函数指针
    kEmptyChain       // 类型链无任何数据
  };

  // 禁止使用EndType::GetEndType()，防止修改全局共享的节点
  template <class ObjectName>
  ObjectConstructData(ObjectName&& object_name)
      : object_name_(std::forward<ObjectName>(object_name)),
        type_chain_head_(std::make_shared<EndType>()),
        type_chain_tail_(type_chain_head_) {}

  // 构建指定对象
  // 仅接受构建VarietyOperatorNode（变量节点）
  // 和FunctionDefine（函数头）
  // 构建的对象写入objec_覆盖原有指针
  template <class BasicObjectType, class... Args>
  requires std::is_same_v<BasicObjectType, VarietyOperatorNode> ||
      std::is_same_v<BasicObjectType, FunctionDefine>
          CheckResult ConstructBasicObjectPart(Args... args);

  // 在尾节点后连接一个节点，调用后自动设置尾节点指向新添加的节点
  // 添加一个FunctionType后再次调用该函数时会将节点插到函数的返回类型部分
  // 调用一次只能添加一个节点
  // 调用参数中的节点必须未设置next_node，否则会被覆盖
  // 最终的节点应由ConstructObject完成
  // 这么做可以精确设置指针/变量/函数返回值的const标记
  template <class NextNodeType, class... Args>
  CheckResult AttachSingleNodeToTailNodeEmplace(Args&&... args) {
    auto new_node = std::make_shared<NextNodeType>(std::forward<Args>(args)...);
    return AttachSingleNodeToTailNodePointer(std::move(new_node));
  }
  // 语义同AttachSingleNodeToTailNodeEmplace，参数使用已经构建好的节点
  CheckResult AttachSingleNodeToTailNodePointer(
      std::shared_ptr<const TypeInterface>&& next_node);
  // 完成构建过程，返回流程对象
  // 函数参数：终结类型左边的ConstTag、最后连接的节点
  // 两个参数用来准确的设置函数返回值/变量/指针的const标记
  // 不会在尾部连接哨兵节点（应在终结类节点构建时自动设置下一个节点为EndType）
  // 自动处理获取变量名指针的过程
  // 只调用Announce函数，不调用Define函数，Define函数由上位函数决定如何调用
  std::pair<std::unique_ptr<FlowInterface>, CheckResult> ConstructObject(
      ConstTag const_tag_before_final_type,
      std::shared_ptr<const TypeInterface>&& final_node_to_attach);
  // 向尾部节点添加函数参数，要求尾部节点为函数类型
  void AddFunctionTypeArgument(
      const std::shared_ptr<const VarietyOperatorNode>& argument) {
    assert(type_chain_tail_->GetType() == StructOrBasicType::kFunction);
    return static_cast<FunctionType&>(*type_chain_tail_)
        .AddFunctionCallArgument(argument);
  }
  // 在完成规约前获取保存的对象名以便添加到映射表中
  // 返回右值引用，可以用于映射表节点名的移动构造
  std::string&& GetObjectName() { return std::move(object_name_); }
  // 获取类型链主类型（类型链头结点的类型）
  StructOrBasicType GetMainType() const {
    return type_chain_head_->GetNextNodeReference().GetType();
  }

 private:
  // 创建的对象
  std::unique_ptr<FlowInterface> object_;
  // 创建的对象名，仅做临时存储
  std::string object_name_;
  // 指向类型链的头结点
  // 全程指向头部哨兵节点
  const std::shared_ptr<TypeInterface> type_chain_head_;
  // 指向类型链的尾节点
  std::shared_ptr<TypeInterface> type_chain_tail_;
};

// 枚举参数规约时得到的数据
class EnumReturnData {
 public:
  EnumType::EnumContainerType& GetContainer() { return enum_container_; }
  // 返回插入位置的迭代器和是否插入
  std::pair<EnumType::EnumContainerType::iterator, bool> AddMember(
      std::string&& member_name, long long value);
  long long GetMaxValue() const { return max_value_; }
  long long GetMinValue() const { return min_value_; }
  long long GetLastValue() const { return last_value_; }

 private:
  void SetMaxValue(long long max_value) { max_value_ = max_value; }
  void SetMinValue(long long min_value) { min_value_ = min_value; }
  void SetLastValue(long long last_value) { last_value_ = last_value; }

  // 枚举名与值的关联容器
  EnumType::EnumContainerType enum_container_;
  // 最大的枚举值
  long long max_value_ = LLONG_MIN;
  // 最小的枚举值
  long long min_value_ = LLONG_MAX;
  // 上次添加的枚举值
  long long last_value_;
};

// 变量/函数构建过程报错用
// 输入错误情况和构建的对象名
void VarietyOrFunctionConstructError(
    ObjectConstructData::CheckResult check_result,
    const std::string& object_name);

// 从ConstTag的规约信息中获取ConstTag类型
// 需要保证输入为非终结产生式ConstTag规约得到的数据（无论是否为空规约）
ConstTag GetConstTag(const WordDataToUser& raw_const_tag_data);
// 根据声明时的初始类型获取一次声明中非第一个变量的类型
std::shared_ptr<const TypeInterface> GetExtendAnnounceType(
    const std::shared_ptr<const TypeInterface>& source_type);
// 检查赋值时类型检查结果，输出相应的错误信息，如果是error则不返回
void CheckAssignableCheckResult(AssignableCheckResult assignable_check_result);
// 检查生成数学运算节点的结果，输出相应错误信息，如果是error则不返回
void CheckMathematicalComputeTypeResult(
    DeclineMathematicalComputeTypeResult
        decline_mathematical_compute_type_result);
// 检查声明/定义类型时的结果，输出相应错误信息，如果是error则不返回
void CheckAddTypeResult(AddTypeResult add_type_result);

// 产生式规约时使用的函数
// 不使用std::unique_ptr因为无法复制，无法用于构造std::any

// SingleConstexprValue-> Char
// 属性：InitializeType::kBasic，BuiltInType::kChar，SignTag::kSigned
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueChar(
    std::vector<WordDataToUser>&& word_data);
// SingleConstexprValue-> Str "[" Num "]"
// 属性：InitializeType::kBasic，BuiltInType::kChar，SignTag::kSigned
std::shared_ptr<BasicTypeInitializeOperatorNode>
SingleConstexprValueIndexedString(std::vector<WordDataToUser>&& word_data);
// SingleConstexprValue-> Num
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
  return BuiltInType::kInt8;
}
// FundamentalType->"short"
BuiltInType FundamentalTypeShort(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "short");
  return BuiltInType::kInt16;
}
// FundamentalType->"int"
BuiltInType FundamentalTypeInt(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "int");
  return BuiltInType::kInt32;
}
// FundamentalType->"long"
BuiltInType FundamentalTypeLong(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "long");
  return BuiltInType::kInt32;
}
// FundamentalType->"float"
BuiltInType FundamentalTypeFloat(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "float");
  return BuiltInType::kFloat32;
}
// FundamentalType->"double"
BuiltInType FundamentalTypeDouble(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "double");
  return BuiltInType::kFloat64;
}
// FundamentalType->"void"
BuiltInType FundamentalTypeVoid(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  assert(word_data.front().GetTerminalWordData().word == "void");
  return BuiltInType::kVoid;
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
// 分配在堆上，避免any容器中复制对象
// 使用std::shared_ptr代替std::shared_ptr，std::any不能存储不支持复制的类型
std::shared_ptr<ObjectConstructData> IdOrEquivenceConstTagId(
    std::vector<WordDataToUser>&& word_data);
// IdOrEquivence->ConstTag
// 分配在堆上，避免any容器中复制对象
// 使用std::shared_ptr代替std::shared_ptr，std::any不能存储不支持复制的类型
std::shared_ptr<ObjectConstructData> IdOrEquivenceConstTag(
    std::vector<WordDataToUser>&& word_data);
// IdOrEquivence->IdOrEquivence "[" Num "]"
// 返回值类型：std::shared_ptr<ObjectConstructData>
// 返回std::any防止移动构造VarietyConstructData
std::any IdOrEquivenceNumAddressing(std::vector<WordDataToUser>&& word_data);
// IdOrEquivence->IdOrEquivence "[" "]"
// 返回值类型：std::shared_ptr<ObjectConstructData>
// 返回std::any防止移动构造VarietyConstructData
// 设置新添加的指针所对应数组大小为-1来标记此处数组大小需要根据赋值结果推断
std::any IdOrEquivenceAnonymousAddressing(
    std::vector<WordDataToUser>&& word_data);
// IdOrEquivence->Consttag "*" IdOrEquivence
// 返回值类型：std::shared_ptr<ObjectConstructData>
// 返回std::any防止移动构造VarietyConstructData
std::any IdOrEquivencePointerAnnounce(std::vector<WordDataToUser>&& word_data);
// IdOrEquivence->"(" IdOrEquivence ")"
// 返回值类型：std::shared_ptr<ObjectConstructData>
// 返回std::any防止移动构造VarietyConstructData
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
  return std::move(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// Enum->"enum" Id "{" EnumArguments "}"
std::shared_ptr<EnumType> EnumDefine(std::vector<WordDataToUser>&& word_data);
// Enum->"enum" "{" EnumArguments "}"
std::shared_ptr<EnumType> EnumAnonymousDefine(
    std::vector<WordDataToUser>&& word_data);
// EnumAnnounce->"enum" Id
// 返回声明的结构名与结构类型（kStruct）
// 额外返回类型为了与结构体和共用体声明返回类型保持一致
inline std::pair<std::string, StructOrBasicType> EnumAnnounce(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  std::string& enum_name = word_data[1].GetTerminalWordData().word;
  return std::make_pair(std::move(enum_name), StructOrBasicType::kEnum);
}
// StructureAnnounce->"struct" Id
// 返回声明的结构名与结构类型（kStruct）
inline std::pair<std::string, StructOrBasicType> StructureAnnounceStructId(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  return std::make_pair(std::move(word_data[1].GetTerminalWordData().word),
                        StructOrBasicType::kStruct);
}
// StructureAnnounce->"union" Id
// 返回声明的结构名与结构类型（kUnion）
inline std::pair<std::string, StructOrBasicType> StructureAnnounceUnionId(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  return std::make_pair(std::move(word_data[1].GetTerminalWordData().word),
                        StructOrBasicType::kUnion);
}
// StructureDefineHead->"struct"
inline std::pair<std::string, StructOrBasicType> StructureDefineHeadStruct(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::make_pair(std::string(), StructOrBasicType::kStruct);
}
// StructureDefineHead->"union"
std::pair<std::string, StructOrBasicType> StructureDefineHeadUnion(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::make_pair(std::string(), StructOrBasicType::kUnion);
}
// StructureDefineHead->StructureAnnounce
// 返回值类型：std::pair<std::string, StructOrBasicType>
// 返回值意义见StructureAnnounce
std::any StructureDefineHeadStructureAnnounce(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::move(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// StructureDefineInitHead-> StructureDefineHead "{"
// 执行一些初始化工作
// 返回结构数据类型节点
std::shared_ptr<StructureTypeInterface> StructureDefineInitHead(
    std::vector<WordDataToUser>&& word_data);
// StructureDefine-> StructureDefineInitHead StructureBody "}"
// 返回值类型：std::shared_ptr<StructureTypeInterface>
// 返回结构数据类型节点
std::any StructureDefine(std::vector<WordDataToUser>&& word_data);
// StructType-> StructureDefine
std::shared_ptr<const StructureTypeInterface> StructTypeStructDefine(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::any_cast<std::shared_ptr<StructureTypeInterface>&>(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// StructType-> StructAnnounce
// 返回获取到的结构化数据类型
std::shared_ptr<const StructureTypeInterface> StructTypeStructAnnounce(
    std::vector<WordDataToUser>&& word_data);
// BasicType-> ConstTag SignTag FundamentalType
// 返回获取到的类型与ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeFundamental(
    std::vector<WordDataToUser>&& word_data);
// BasicType-> ConstTag StructType
// 返回获取到的类型与ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeStructType(
    std::vector<WordDataToUser>&& word_data);
// BasicType-> ConstTag Id
// 返回获取到的类型与ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeId(
    std::vector<WordDataToUser>&& word_data);
// BasicType-> ConstTag EnumAnnounce
// 返回获取到的类型与ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeEnumAnnounce(
    std::vector<WordDataToUser>&& word_data);
// FunctionRelaventBasePartFunctionInit-> IdOrEquivence "("
// 做一些初始化工作
// 返回值类型：std::shared_ptr<ObjectConstructData>
std::any FunctionRelaventBasePartFunctionInit(
    std::vector<WordDataToUser>&& word_data);
// FunctionRelaventBasePart-> FunctionRelaventBasePartFunctionInit
//  FunctionRelaventArguments ")"
// 返回值类型：std::shared_ptr<ObjectConstructData>
std::any FunctionRelaventBasePartFunction(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 3);
  return std::move(word_data[0].GetNonTerminalWordData().user_returned_data);
}
// FunctionRelaventBasePart-> ConstTag "*" FunctionRelaventBasePart
// 返回值类型：std::shared_ptr<ObjectConstructData>
std::any FunctionRelaventBasePartPointer(
    std::vector<WordDataToUser>&& word_data);
// FunctionRelaventBasePart-> "(" FunctionRelaventBasePart ")"
// 返回值类型：std::shared_ptr<ObjectConstructData>
std::any FunctionRelaventBasePartBranckets(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 3);
  return std::move(word_data[1].GetNonTerminalWordData().user_returned_data);
}
// FunctionRelaventBasePartFunctionInit-> FunctionRelaventBasePart "("
// 做一些声明函数指针时的初始化工作
// 返回值类型：std::shared_ptr<ObjectConstructData>
std::any FunctionRelaventBasePartFunctionInit(
    std::vector<WordDataToUser>&& word_data);
// FunctionRelavent-> BasicType FunctionRelaventBasePart
// 返回值包装的指针只可能为std::shared_ptr<FunctionType>（函数声明）
// 或std::shared_ptr<VarietyOperatorNode>（变量声明）
// Define操作交给上位函数进行
std::shared_ptr<FlowInterface> FunctionRelavent(
    std::vector<WordDataToUser>&& word_data);
// SingleAnnounceNoAssign-> BasicType IdOrEquivence
// 返回值包装的指针只可能为std::shared_ptr<FunctionType>（函数声明）
// 或std::shared_ptr<VarietyOperatorNode>（变量声明）
// 不执行DefineVariety/DefineType也不添加空间分配节点
std::shared_ptr<FlowInterface> SingleAnnounceNoAssignVariety(
    std::vector<WordDataToUser>&& word_data);
// SingleAnnounceNoAssign-> FunctionRelavent
// 返回值类型：std::shared_ptr<FlowInterface>
// 不执行DefineVariety/DefineType也不添加空间分配节点
std::any SingleAnnounceNoAssignFunctionRelavent(
    std::vector<WordDataToUser>&& word_data);
// TypeDef-> "typedef" SingleAnnounceNoAssign
// 不返回数据
std::any TypeDef(std::vector<WordDataToUser>&& word_data);
// NotEmptyFunctionPointerArguments-> SingleAnnounceNoAssign
// 不返回数据
std::any NotEmptyFunctionPointerArgumentsBase(
    std::vector<WordDataToUser>&& word_data);
// NotEmptyFunctionPointerArguments-> NotEmptyFunctionPointerArguments ","
// SingleAnnounceNoAssign
// 返回值类型：std::shared_ptr<FunctionType::ArgumentInfoContainer>
std::any NotEmptyFunctionPointerArgumentsExtend(
    std::vector<WordDataToUser>&& word_data);
// FunctionRelaventArguments-> NotEmptyFunctionPointerArguments
// 返回值类型：std::shared_ptr<FunctionType:ArgumentInfoContainer>
std::any FunctionRelaventArguments(std::vector<WordDataToUser>&& word_data);
// FunctionDefineHead-> FunctionRelavent "{"
// 对函数自身和每个参数执行Define并注册函数类型和函数对应的变量（利于查找）
std::shared_ptr<FunctionDefine> FunctionDefineHead(
    std::vector<WordDataToUser>&& word_data);
// FunctionDefine-> FunctionDefineHead Sentences "}"
// 返回值类型：std::shared_ptr<FunctionDefine>
// 弹出最后一层作用域，重置当前活跃函数
// 不返回任何数据
std::any FunctionDefineMain(std::vector<WordDataToUser>&& word_data);
// SingleStructureBody-> SingleAnnounceNoAssign
// 返回扩展声明的ID使用的类型和变量本身的ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag>
SingleStructureBodyBase(std::vector<WordDataToUser>&& word_data);
// SingleStructureBody-> SingleStructureBody "," Id
// 返回值类型：std::pair<std::shared_ptr<const TypeInterface>, ConstTag>
// 返回扩展声明的ID使用的类型和变量本身的ConstTag
std::any SingleStructureBodyExtend(std::vector<WordDataToUser>&& word_data);
// NotEmptyStructureBody-> SingleStructureBody
// 不返回任何数据
inline std::any NotEmptyStructureBodyBase(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::any();
}
// NotEmptyStructureBody-> NotEmptyStructureBody SingleStructureBody ";"
// 不返回任何数据
inline std::any NotEmptyStructureBodyExtend(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 3);
  return std::any();
}
// StructureBody-> NotEmptyStructureBody
// 不返回任何数据
// StructureBody-> NotEmptyStructureBody
inline std::any StructureBody(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::any();
}
// InitializeList-> "{" InitializeListArguments "}"
std::shared_ptr<ListInitializeOperatorNode> InitializeList(
    std::vector<WordDataToUser>&& word_data);
// SingleInitializeListArgument-> SingleConstexprValue
// 返回值类型：std::shared_ptr<InitializeOperatorNodeInterface>
std::any SingleInitializeListArgumentConstexprValue(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::move(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// SingleInitializeListArgument-> InitializeList
// 返回值类型：std::shared_ptr<InitializeOperatorNodeInterface>
std::any SingleInitializeListArgumentList(
    std::vector<WordDataToUser>&& word_data);
// InitializeListArguments-> SingleInitializeListArgument
// 返回值类型：
std::shared_ptr<std::list<std::shared_ptr<InitializeOperatorNodeInterface>>>
InitializeListArgumentsBase(std::vector<WordDataToUser>&& word_data);
// InitializeListArguments-> InitializeListArguments ","
// SingleInitializeListArgument
// 返回值类型：
// std::shared_ptr<std::list<std::shared_ptr<InitializeOperatorNodeInterface>>>
std::any InitializeListArgumentsExtend(std::vector<WordDataToUser>&& word_data);
// AnnounceAssignable-> Assignable
// 返回值类型：std::pair<std::shared_ptr<const OperatorNodeInterface>,
//                   std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
inline std::any AnnounceAssignableAssignable(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::move(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// AnnounceAssignable-> InitializeList
// 返回空容器
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AnnounceAssignableInitializeList(std::vector<WordDataToUser>&& word_data);
// 处理函数内执行流程中声明变量不赋初始值的变量注册和获取扩展声明用类型的步骤
// 返回扩展声明时变量的类型（如果最初为有指针类型则去掉一重指针，否则不变）
// 变量的ConstTag和获取变量时使用的操作
// 如果是合法的声明则添加变量定义并创建空间分配节点
std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
VarietyAnnounceNoAssign(std::shared_ptr<VarietyOperatorNode>&& variety_node);
// 处理函数内执行流程中声明变量且赋初始值的变量注册和获取扩展声明用类型步骤
// 返回扩展声明时变量的类型（如果最初为有指针类型则去掉一重指针，否则不变）
// 变量的ConstTag和获取变量时使用的操作
// 如果是合法的声明则添加变量定义并创建空间分配节点和赋值节点
std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
VarieytAnnounceWithAssign(
    std::shared_ptr<VarietyOperatorNode>&& variety_node,
    std::shared_ptr<OperatorNodeInterface>&& node_for_assign);
// SingleAnnounceAndAssign-> SingleAnnounce
// 返回扩展声明时变量的类型（如果最初为有指针类型则去掉一重指针，否则不变）
// 变量的ConstTag和获取变量使用的操作
// 如果是合法的声明则添加变量定义并创建空间分配节点
std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
SingleAnnounceAndAssignNoAssignBase(std::vector<WordDataToUser>&& word_data);
// SingleAnnounceAndAssign-> SingleAnnounce "=" AnnounceAssignable
// 返回扩展声明时变量的类型（如果最初为有指针类型则去掉一重指针，否则不变）
// 变量的ConstTag和获取变量使用的操作
// 如果是合法的声明则添加变量定义并创建空间分配节点和赋值节点
std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
SingleAnnounceAndAssignWithAssignBase(std::vector<WordDataToUser>&& word_data);
// SingleAnnounceAndAssign-> SingleAnnounceAndAssign "," Id
// 返回扩展声明时变量的类型（如果最初为有指针类型则去掉一重指针，否则不变）
// 变量的ConstTag和获取变量使用的操作
// 返回值类型：std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
//                   std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
std::any SingleAnnounceAndAssignNoAssignExtend(
    std::vector<WordDataToUser>&& word_data);
// SingleAnnounceAndAssign-> SingleAnnounceAndAssign "," Id "="
// AnnounceAssignable
// 如果是合法的声明则添加变量定义并创建空间分配节点和赋值节点
// 返回值类型：std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
//                   std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
std::any SingleAnnounceAndAssignWithAssignExtend(
    std::vector<WordDataToUser>&& word_data);
// Type-> BasicType
// 返回类型和变量的ConstTag
// 返回值类型：std::pair<std::shared_ptr<const TypeInterface>, ConstTag>
std::any TypeBasicType(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::move(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// Type-> FunctionRelavent
// 返回类型和变量的ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag> TypeFunctionRelavent(
    std::vector<WordDataToUser>&& word_data);
// MathematicalOperator-> "+"
// 返回数学运算符
MathematicalOperation MathematicalOperatorPlus(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "+");
  return MathematicalOperation::kPlus;
}
// MathematicalOperator-> "-"
// 返回数学运算符
MathematicalOperation MathematicalOperatorMinus(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "-");
  return MathematicalOperation::kMinus;
}
// MathematicalOperator-> "*"
// 返回数学运算符
MathematicalOperation MathematicalOperatorMultiple(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "*");
  return MathematicalOperation::kMultiple;
}
// MathematicalOperator-> "/"
// 返回数学运算符
MathematicalOperation MathematicalOperatorDivide(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "/");
  return MathematicalOperation::kDivide;
}
// MathematicalOperator-> "%"
// 返回数学运算符
MathematicalOperation MathematicalOperatorMod(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "%");
  return MathematicalOperation::kMod;
}
// MathematicalOperator-> "<<"
// 返回数学运算符
MathematicalOperation MathematicalOperatorLeftShift(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "<<");
  return MathematicalOperation::kLeftShift;
}
// MathematicalOperator-> ">>"
// 返回数学运算符
MathematicalOperation MathematicalOperatorRightShift(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == ">>");
  return MathematicalOperation::kRightShift;
}
// MathematicalOperator-> "&"
// 返回数学运算符
MathematicalOperation MathematicalOperatorAnd(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "&");
  return MathematicalOperation::kAnd;
}
// MathematicalOperator-> "|"
// 返回数学运算符
MathematicalOperation MathematicalOperatorOr(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "|");
  return MathematicalOperation::kOr;
}
// MathematicalOperator-> "^"
// 返回数学运算符
MathematicalOperation MathematicalOperatorXor(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "^");
  return MathematicalOperation::kXor;
}
// MathematicalOperator-> "!"
// 返回数学运算符
MathematicalOperation MathematicalOperatorNot(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "!");
  return MathematicalOperation::kNot;
}
// MathematicalAndAssignOperator-> "+="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorPlusAssign(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "+=");
  return MathematicalAndAssignOperation::kPlusAssign;
}
// MathematicalAndAssignOperator-> "-="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorMinusAssign(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "-=");
  return MathematicalAndAssignOperation::kMinusAssign;
}
// MathematicalAndAssignOperator-> "*="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorMultipleAssign(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "*=");
  return MathematicalAndAssignOperation::kMultipleAssign;
}
// MathematicalAndAssignOperator-> "/="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorDivideAssign(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "/=");
  return MathematicalAndAssignOperation::kDivideAssign;
}
// MathematicalAndAssignOperator-> "%="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorModAssign(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "%=");
  return MathematicalAndAssignOperation::kModAssign;
}
// MathematicalAndAssignOperator-> "<<="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorLeftShiftAssign(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "<<=");
  return MathematicalAndAssignOperation::kLeftShiftAssign;
}
// MathematicalAndAssignOperator-> ">>="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorRightShiftAssign(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == ">>=");
  return MathematicalAndAssignOperation::kRightShiftAssign;
}
// MathematicalAndAssignOperator-> "&="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorAndAssign(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "&=");
  return MathematicalAndAssignOperation::kAndAssign;
}
// MathematicalAndAssignOperator-> "|="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorOrAssign(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "|=");
  return MathematicalAndAssignOperation::kOrAssign;
}
// MathematicalAndAssignOperator-> "^="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorXorAssign(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "^=");
  return MathematicalAndAssignOperation::kXorAssign;
}
// LogicalOperator-> "&&"
// 返回逻辑运算符
LogicalOperation LogicalOperatorAndAnd(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "&&");
  return LogicalOperation::kAndAnd;
}
// LogicalOperator-> "||"
// 返回逻辑运算符
LogicalOperation LogicalOperatorOrOr(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "||");
  return LogicalOperation::kOrOr;
}
// LogicalOperator-> ">"
// 返回逻辑运算符
LogicalOperation LogicalOperatorGreater(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == ">");
  return LogicalOperation::kGreater;
}
// LogicalOperator-> ">="
// 返回逻辑运算符
LogicalOperation LogicalOperatorGreaterEqual(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == ">=");
  return LogicalOperation::kGreaterEqual;
}
// LogicalOperator-> "<"
// 返回逻辑运算符
LogicalOperation LogicalOperatorLess(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "<");
  return LogicalOperation::kLess;
}
// LogicalOperator-> "<="
// 返回逻辑运算符
LogicalOperation LogicalOperatorLessEqual(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "<=");
  return LogicalOperation::kLessEqual;
}
// LogicalOperator-> "=="
// 返回逻辑运算符
LogicalOperation LogicalOperatorEqual(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "==");
  return LogicalOperation::kEqual;
}
// LogicalOperator-> "!="
// 返回逻辑运算符
LogicalOperation LogicalOperatorNotEqual(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1 &&
         word_data.front().GetTerminalWordData().word == "!=");
  return LogicalOperation::kNotEqual;
}
// Assignable-> SingleConstexprValue
// 返回这一步得到的最终可运算节点和获取过程的操作
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableConstexprValue(std::vector<WordDataToUser>&& word_data);
// Assignable-> Id
// 返回这一步得到的最终可运算节点和获取过程的操作
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableId(std::vector<WordDataToUser>&& word_data);
// Assignable-> TemaryOperator
// 返回这一步得到的最终可运算节点和获取过程的操作
// 返回值类型：std::pair<std::shared_ptr<const OperatorNodeInterface>,
//                   std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
std::any AssignableTypeTemaryOperator(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::move(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// Assignable-> FunctionCall
// 返回值类型：std::pair<std::shared_ptr<const OperatorNodeInterface>,
//                   std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
std::any AssignableFunctionCall(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::move(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// Assignable-> "sizeof" "(" Type ")"
// 返回这一步得到的最终可运算节点和空容器（sizeof语义）
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSizeOfType(std::vector<WordDataToUser>&& word_data);
// Assignable-> "sizeof" "(" Assignable ")"
// 返回这一步得到的最终可运算节点和空容器（sizeof语义）
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSizeOfAssignable(std::vector<WordDataToUser>&& word_data);
// Assignable-> Assignable "." Id
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMemberAccess(std::vector<WordDataToUser>&& word_data);
// Assignable-> Assignable "->" Id
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignablePointerMemberAccess(std::vector<WordDataToUser>&& word_data);
// Assignable-> "(" Assignable ")"
// 返回这一步得到的最终可运算节点和获取过程的操作
// 返回值类型：std::pair<std::shared_ptr<const OperatorNodeInterface>,
//                   std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
inline std::any AssignableBracket(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::move(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// Assignable-> "(" Type ")" Assignable
// 返回这一步得到的最终可运算节点和获取过程的操作
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableTypeConvert(std::vector<WordDataToUser>&& word_data);
// Assignable-> Assignable MathematicalOperator Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMathematicalOperate(std::vector<WordDataToUser>&& word_data);
// Assignable-> Assignable MathematicalAndAssignOperator Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMathematicalAndAssignOperate(std::vector<WordDataToUser>&& word_data);
// Assignable-> Assignable LogicalOperator Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableLogicalOperate(std::vector<WordDataToUser>&& word_data);
// Assignable-> "!" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableNot(std::vector<WordDataToUser>&& word_data);
// Assignable-> "~" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableNegative(std::vector<WordDataToUser>&& word_data);
// Assignable-> "&" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableObtainAddress(std::vector<WordDataToUser>&& word_data);
// Assignable-> "*" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableDereference(std::vector<WordDataToUser>&& word_data);
// Assignable-> Assignable "[" Assignable "]"
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableArrayAccess(std::vector<WordDataToUser>&& word_data);
// 处理前缀++/--的情况
// 输入使用的运算符（仅限++/--）、待运算的节点和存储获取结果的操作的容器
// 向给定容器中添加操作节点
// 返回运算后的可运算节点
std::shared_ptr<const OperatorNodeInterface> PrefixPlusOrMinus(
    MathematicalOperation mathematical_operation,
    const std::shared_ptr<const OperatorNodeInterface>& node_to_operate,
    std::list<std::unique_ptr<FlowInterface>>* flow_control_node_container);
// 处理后缀++/--的情况
// 输入使用的运算符（仅限++/--）、待运算的节点和存储获取结果的操作的容器
// 向给定容器中添加操作节点
// 返回运算后得到的可运算节点
std::shared_ptr<const OperatorNodeInterface> SuffixPlusOrMinus(
    MathematicalOperation mathematical_operation,
    const std::shared_ptr<const OperatorNodeInterface>& node_to_operate,
    std::list<std::unique_ptr<FlowInterface>>* flow_control_node_container);
// Assignable-> "++" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignablePrefixPlus(std::vector<WordDataToUser>&& word_data);
// Assignable-> "--" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignablePrefixMinus(std::vector<WordDataToUser>&& word_data);
// Assignable-> Assignable "++"
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSuffixPlus(std::vector<WordDataToUser>&& word_data);
// Assignable-> Assignable "--"
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSuffixMinus(std::vector<WordDataToUser>&& word_data);
// TemaryOperator-> Assignable "?" Assignable ":" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
TemaryOperator(std::vector<WordDataToUser>&& word_data);
// NotEmptyFunctionCallArguments-> Assignable
// 不返回任何内容
std::any NotEmptyFunctionCallArgumentsBase(
    std::vector<WordDataToUser>&& word_data);
// NotEmptyFunctionCallArguments-> NotEmptyFunctionCallArguments "," Assignable
// 返回值类型：
// std::shared_ptr<FunctionCallOperatorNode::FunctionCallArgumentsContainer>
std::any NotEmptyFunctionCallArgumentsExtend(
    std::vector<WordDataToUser>&& word_data);
// FunctionCallArguments-> NotEmptyFunctionCallArguments
// 返回值类型：
// std::shared_ptr<FunctionCallOperatorNode::FunctionCallArgumentsContainer>
std::any FunctionCallArguments(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::move(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// FunctionCallInit-> Assignable "("
// 做一些初始化工作
// 返回函数调用对象和获取可调用对象的操作，同时设置全局变量
std::pair<std::shared_ptr<FunctionCallOperatorNode>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
FunctionCallInit(std::vector<WordDataToUser>&& word_data);
// FunctionCall-> FunctionCallInit FunctionCallArguments ")"
// 返回函数调用对象
// 第二个参数存储获取可调用对象操作和对可调用对象的调用
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
FunctionCall(std::vector<WordDataToUser>&& word_data);
// Assignables-> Assignable
// 返回保存运算过程的节点容器
std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>> AssignablesBase(
    std::vector<WordDataToUser>&& word_data);
// Assignables-> Assignables "," Assignable
// 返回值类型：std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>
std::any AssignablesExtend(std::vector<WordDataToUser>&& word_data);
// SingleStatement-> If
// 不做任何操作
std::any SingleStatementIf(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::any();
}
// SingleStatement-> DoWhile
// 不做任何操作
std::any SingleStatementDoWhile(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::any();
}
// SingleStatement-> While
// 不做任何操作
std::any SingleStatementWhile(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::any();
}
// SingleStatement-> For
// 不做任何操作
std::any SingleStatementFor(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::any();
}
// SingleStatement-> Switch
// 不做任何操作
std::any SingleStatementSwitch(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::any();
}
// SingleStatement-> Assignable ";"
// 添加获取Assignable的流程控制语句
// 不返回任何值
std::any SingleStatementAssignable(std::vector<WordDataToUser>&& word_data);
// SingleStatement-> SingleAnnounceAndAssign ";"
// 添加声明变量过程中的流程控制语句
// 不返回任何值
std::any SingleStatementAnnounce(std::vector<WordDataToUser>&& word_data);
// IfCondition-> "if" "(" Assignable ")"
// 返回if条件节点和获取if条件过程中使用的操作
// 向控制器（parser_frontend）注册流程控制语句
// 不返回任何值
std::any IfCondition(std::vector<WordDataToUser>&& word_data);
// IfWithElse-> IfCondition ProcessControlSentenceBody "else"
// 转换成if-else语句
// 不返回任何值
std::any IfWithElse(std::vector<WordDataToUser>&& word_data);
// If->IfWithElse ProcessControlSentenceBody
// 不返回任何值
std::any IfElseSence(std::vector<WordDataToUser>&& word_data) {
  parser_frontend.PopActionScope();
  return std::any();
}
// If-> IfCondition ProcessControlSentenceBody
// 不返回任何值
std::any IfIfSentence(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  parser_frontend.PopActionScope();
  return std::any();
}
// ForRenewSentences-> Assignables ";"
// 返回值类型：std::pair<std::shared_ptr<const OperatorNodeInterface>,
//                   std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
std::any ForRenewSentences(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  return std::move(
      word_data.front().GetNonTerminalWordData().user_returned_data);
}
// ForInitSentence-> Assignables ";"
std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>
ForInitSentenceAssignables(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  auto& [ignore_assignable, flow_control_node_container] = std::any_cast<
      std::pair<std::shared_ptr<const OperatorNodeInterface>,
                std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&>(
      word_data[0].GetNonTerminalWordData().user_returned_data);
  return flow_control_node_container;
}
// ForInitSentence-> SingleAnnounceAndAssign ";"
std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>
ForInitSentenceAnnounce(std::vector<WordDataToUser>&& word_data);
// ForInitHead-> "for"
// 做一些准备工作
// 不返回任何值
inline std::any ForInitHead(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  parser_frontend.PushFlowControlSentence(std::make_unique<ForSentence>());
  return std::any();
}
// ForHead-> ForInitHead "(" ForInitSentence Assignable ForRenewSentences ")"
// 规约for语句的三要素
// 不返回任何值
std::any ForHead(std::vector<WordDataToUser>&& word_data);
// For-> ForHead ProcessControlSentenceBody
// 弹出作用域
// 不返回任何值
std::any For(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  parser_frontend.PopActionScope();
  return std::any();
}
// WhileInitHead-> "while" "(" Assignable ")"
// 不返回任何值
std::any WhileInitHead(std::vector<WordDataToUser>&& word_data);
// While-> WhileInitHead ProcessControlSentenceBody
// 弹出作用域
// 不返回任何值
std::any While(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  parser_frontend.PopActionScope();
  return std::any();
}
// DoWhileInitHead-> "do"
// 做一些准备工作
// 不返回任何数据
std::any DoWhileInitHead(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  parser_frontend.PushFlowControlSentence(std::make_unique<DoWhileSentence>());
  return std::any();
}
// DoWhile-> DoWhileInitHead ProcessControlSentenceBody
// while "(" Assignable ")" ";"
// 不返回任何数据
std::any DoWhile(std::vector<WordDataToUser>&& word_data);
// SwitchCaseSimple-> "case" SingleConstexprValue ":"
// 不返回任何数据
std::any SwitchCaseSimple(std::vector<WordDataToUser>&& word_data);
// SwitchCaseSimple-> "default" ":"
// 不返回任何数据
std::any SwitchCaseDefault(std::vector<WordDataToUser>&& word_data);
// SingleSwitchStatement-> SwitchCase
// 不做任何操作
std::any SingleSwitchStatementCase(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::any();
}
// SingleSwitchStatement-> Statements
// 不做任何操作
std::any SingleSwitchStatementStatements(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  return std::any();
}
// SwitchStatements-> SwitchStatements SingleSwitchStatement
// 不作任何操作
std::any SwitchStatements(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  return std::any();
}
// SwitchCondition-> "switch" "(" Assignable ")"
// 不返回任何值
std::any SwitchCondition(std::vector<WordDataToUser>&& word_data);
// Switch-> SwitchCondition "{" SwitchStatements "}"
// 不做任何操作
std::any Switch(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 4);
  return std::any();
}
// Statements-> Statements SingleStatement
// 不做任何操作
std::any Statements(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  return std::any();
}
// 根产生式
// Root-> Root FunctionDefine
// 不作任何操作
std::any RootFunctionDefine(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 2);
  return std::any();
}
// 根产生式
// Root-> Root SingleAnnounceNoAssign ";"
// 不返回任何值
std::any RootAnnounce(std::vector<WordDataToUser>&& word_data);

template <class BasicObjectType, class... Args>
requires std::is_same_v<BasicObjectType, VarietyOperatorNode> ||
    std::is_same_v<BasicObjectType, FunctionDefine>
        ObjectConstructData::CheckResult
        ObjectConstructData::ConstructBasicObjectPart(Args... args) {
  if constexpr (std::is_same_v<BasicObjectType, VarietyOperatorNode>) {
    // 构建变量数据
    auto object = std::make_unique<SimpleSentence>();
    bool result = object->SetSentenceOperateNode(
        std::make_shared<VarietyOperatorNode>(std::forward<Args>(args)...));
    assert(result);
    object_ = std::move(object);
  } else {
    // 构建函数数据
    object_ = std::make_unique<FunctionDefine>(std::forward<Args>(args)...);
  }
  return CheckResult::kSuccess;
}

// 语义同AttachSingleNodeToTailNodeEmplace，参数使用已经构建好的节点

ObjectConstructData::CheckResult
ObjectConstructData::AttachSingleNodeToTailNodePointer(
    std::shared_ptr<const TypeInterface>&& next_node) {
  switch (type_chain_tail_->GetType()) {
    case StructOrBasicType::kFunction:
      // 函数指针构造语义，将新插入的节点放在返回值的位置
      if (next_node->GetType() == StructOrBasicType::kFunction) {
        // 待插入的第一个返回值节点为函数
        // 函数不能返回函数
        return CheckResult::kReturnFunction;
      }
      static_cast<FunctionType&>(*type_chain_tail_)
          .SetReturnTypePointer(next_node);
      // 此处为了构建类型链违反const原则
      type_chain_tail_ = std::const_pointer_cast<TypeInterface>(next_node);
      break;
    case StructOrBasicType::kPointer:
      // 需要额外检查是否在声明函数数组
      if (std::static_pointer_cast<PointerType>(type_chain_tail_)
              ->GetArraySize() != 0) [[unlikely]] {
        // 非0代表声明数组
        std::cerr << std::format("行数{:} 不支持声明指针数组", GetLine())
                  << std::endl;
        exit(-1);
      }
      [[fallthrough]];
    case StructOrBasicType::kEnd:
      // 普通节点构造语义，在原来的结构下连接
      type_chain_tail_->SetNextNode(next_node);
      // 此处为了构建类型链违反const原则
      type_chain_tail_ = std::const_pointer_cast<TypeInterface>(next_node);
      break;
    case StructOrBasicType::kBasic:
    case StructOrBasicType::kStruct:
    case StructOrBasicType::kUnion:
    case StructOrBasicType::kEnum:
      // 这些为终结节点，不能连接下一个节点
      return CheckResult::kAttachToTerminalType;
      break;
    default:
      assert(false);
      break;
  }
  return CheckResult::kSuccess;
}

}  // namespace c_parser_frontend::parse_functions
#endif