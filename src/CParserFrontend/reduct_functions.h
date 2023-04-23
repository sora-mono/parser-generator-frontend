#ifndef CPARSERFRONTEND_PARSE_FUNCTIONS_H_
#define CPARSERFRONTEND_PARSE_FUNCTIONS_H_
#include <format>
#include <iostream>
#include <limits>

#include "c_parser_frontend.h"
#include "flow_control.h"
#include "operator_node.h"
#include "type_system.h"
namespace c_parser_frontend {
// 声明线程全局变量：C语言编译器前端的类对象
extern thread_local CParserFrontend c_parser_controller;
}  // namespace c_parser_frontend

namespace c_parser_frontend::parse_functions {
using c_parser_frontend::c_parser_controller;
using c_parser_frontend::flow_control::AllocateOperatorNode;
using c_parser_frontend::flow_control::ConditionBlockInterface;
using c_parser_frontend::flow_control::DoWhileSentence;
using c_parser_frontend::flow_control::FlowInterface;
using c_parser_frontend::flow_control::FlowType;
using c_parser_frontend::flow_control::ForSentence;
using c_parser_frontend::flow_control::IfSentence;
using c_parser_frontend::flow_control::Jmp;
using c_parser_frontend::flow_control::Label;
using c_parser_frontend::flow_control::LoopSentenceInterface;
using c_parser_frontend::flow_control::Return;
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
    kEmptyChain,      // 类型链无任何数据
    kConstTagNotSame  // 声明POD变量时类型左侧与类型右侧的ConstTag类型不同
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
             std::is_same_v<BasicObjectType,
                            c_parser_frontend::flow_control::FunctionDefine>
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
// 输出错误信息
void OutputError(const std::string& error);
// 输出警告信息
void OutputWarning(const std::string& warning);
// 输出info信息
void OutputInfo(const std::string& info);

// 处理函数内执行流程中声明变量不赋初始值的变量注册和获取扩展声明用类型的步骤
// 返回扩展声明时变量的类型（先去掉数组维数层指针，然后如果仍为有指针类型则去掉
// 一重指针，否则不变）
// 变量的ConstTag和获取变量时使用的操作
// 如果是合法的声明则添加变量定义并创建空间分配节点
std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
VarietyAnnounceNoAssign(std::shared_ptr<VarietyOperatorNode>&& variety_node);
// 处理函数内执行流程中声明变量且赋初始值的变量注册和获取扩展声明用类型步骤
// 返回扩展声明时变量的类型（先去掉数组维数层指针，然后如果仍为有指针类型则去掉
// 一重指针，否则不变）
// 变量的ConstTag和获取变量时使用的操作
// 如果是合法的声明则添加变量定义并创建空间分配节点和赋值节点
std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
VarietyAnnounceWithAssign(
    std::shared_ptr<VarietyOperatorNode>&& variety_node,
    const std::shared_ptr<const OperatorNodeInterface>& node_for_assign);
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

// 产生式规约时使用的函数
// 不使用std::unique_ptr因为无法复制，无法用于构造std::any&&
// 所有类外函数必须定义在.cpp文件中，否则编译报错LNK2005重复定义

// SingleConstexprValue -> Char
// 属性：InitializeType::kBasic，BuiltInType::kChar，SignTag::kSigned
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueChar(
    std::string&& word_data);
// SingleConstexprValue -> Str "[" Num "]"
// 属性：InitializeType::kBasic，BuiltInType::kChar，SignTag::kSigned
std::shared_ptr<BasicTypeInitializeOperatorNode>
SingleConstexprValueIndexedString(std::string&& str,
                                  std::string&& left_square_bracket,
                                  std::string&& num,
                                  std::string&& right_square_bracket);
// SingleConstexprValue -> Num
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueNum(
    std::string&& num);
// SingleConstexprValue -> Str
// 属性：InitializeType::String，TypeInterface:: const char*
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueString(
    std::string&& str);
// FundamentalType -> "char"
BuiltInType FundamentalTypeChar(std::string&& str);
// FundamentalType -> "short"
BuiltInType FundamentalTypeShort(std::string&& str);
// FundamentalType -> "int"
BuiltInType FundamentalTypeInt(std::string&& str);
// FundamentalType -> "long"
BuiltInType FundamentalTypeLong(std::string&& str);
// FundamentalType -> "float"
BuiltInType FundamentalTypeFloat(std::string&& str);
// FundamentalType -> "double"
BuiltInType FundamentalTypeDouble(std::string&& str);
// FundamentalType -> "void"
BuiltInType FundamentalTypeVoid(std::string&& str);
// SignTag  -> "signed"
SignTag SignTagSigned(std::string&& str);
// SignTag -> "unsigned"
SignTag SignTagUnSigned(std::string&& str);
// ConstTag -> "const"
ConstTag ConstTagConst(std::string&& str);
// IdOrEquivence -> ConstTag Id
// 使用std::shared_ptr包装，std::any不能存储不支持复制的类型
std::shared_ptr<ObjectConstructData> IdOrEquivenceConstTagId(ConstTag const_tag,
                                                             std::string&& id);
// IdOrEquivence -> IdOrEquivence "[" Num "]"
std::shared_ptr<ObjectConstructData>&& IdOrEquivenceNumAddressing(
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& left_square_bracket, std::string&& num,
    std::string&& right_square_bracket);
// IdOrEquivence -> IdOrEquivence "[" "]"
// 设置新添加的指针所对应数组大小为-1来标记此处数组大小需要根据赋值结果推断
std::shared_ptr<ObjectConstructData>&& IdOrEquivenceAnonymousAddressing(
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& left_square_bracket, std::string&& right_square_bracket);
// IdOrEquivence -> ConstTag "*" IdOrEquivence
std::shared_ptr<ObjectConstructData>&& IdOrEquivencePointerAnnounce(
    ConstTag const_tag, std::string&& operator_pointer,
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result);
// IdOrEquivence -> "(" IdOrEquivence ")"
std::shared_ptr<ObjectConstructData>&& IdOrEquivenceInBrackets(
    std::string&& left_bracket,
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& right_bracket);
// AnonymousIdOrEquivence -> "const"
// 使用std::shared_ptr，std::any不能存储不支持复制的类型
std::shared_ptr<ObjectConstructData> AnonymousIdOrEquivenceConst(
    std::string&& str_const);
// AnonymousIdOrEquivence -> AnonymousIdOrEquivence "[" Num "]"
std::shared_ptr<ObjectConstructData>&& AnonymousIdOrEquivenceNumAddressing(
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& left_square_bracket, std::string&& num,
    std::string&& right_square_bracket);
// AnonymousIdOrEquivence -> AnonymousIdOrEquivence "[" "]"
std::shared_ptr<ObjectConstructData>&&
AnonymousIdOrEquivenceAnonymousAddressing(
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& left_square_bracket, std::string&& right_square_bracket);
// AnonymousIdOrEquivence -> ConstTag "*" AnonymousIdOrEquivence
std::shared_ptr<ObjectConstructData>&& AnonymousIdOrEquivencePointerAnnounce(
    ConstTag const_tag, std::string&& operator_pointer,
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result);
// AnonymousIdOrEquivence -> "(" AnonymousIdOrEquivence ")"
std::shared_ptr<ObjectConstructData>&& AnonymousIdOrEquivenceInBrackets(
    std::string&& left_bracket,
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& right_bracket);
// NotEmptyEnumArguments -> Id
std::shared_ptr<EnumReturnData> NotEmptyEnumArgumentsIdBase(std::string&& id);
// NotEmptyEnumArguments -> Id "=" Num
std::shared_ptr<EnumReturnData> NotEmptyEnumArgumentsIdAssignNumBase(
    std::string&& id, std::string&& operator_assign, std::string&& num);
// NotEmptyEnumArguments -> NotEmptyEnumArguments "," Id
std::shared_ptr<EnumReturnData>&& NotEmptyEnumArgumentsIdExtend(
    std::shared_ptr<EnumReturnData>&& enum_data, std::string&& str_comma,
    std::string&& id);
// NotEmptyEnumArguments -> NotEmptyEnumArguments "," Id "=" Num
std::shared_ptr<EnumReturnData>&& NotEmptyEnumArgumentsIdAssignNumExtend(
    std::shared_ptr<EnumReturnData>&& enum_data, std::string&& str_comma,
    std::string&& id, std::string&& operator_assign, std::string&& num);
// EnumArguments -> NotEmptyEnumArguments
std::shared_ptr<EnumReturnData>&& EnumArgumentsNotEmptyEnumArguments(
    std::shared_ptr<EnumReturnData>&& enum_data);
// Enum -> "enum" Id "{" EnumArguments "}"
std::shared_ptr<EnumType> EnumDefine(
    std::string&& str_enum, std::string&& id, std::string&& left_curly_bracket,
    std::shared_ptr<EnumReturnData>&& enum_data,
    std::string&& right_curly_bracket);
// Enum -> "enum" "{" EnumArguments "}"
std::shared_ptr<EnumType> EnumAnonymousDefine(
    std::string&& str_enum, std::string&& left_curly_bracket,
    std::shared_ptr<EnumReturnData>&& enum_data,
    std::string&& right_curly_bracket);
// EnumAnnounce -> "enum" Id
// 返回声明的结构名与结构类型（kStruct）
// 额外返回类型为了与结构体和共用体声明返回类型保持一致
std::pair<std::string, StructOrBasicType> EnumAnnounce(std::string&& str_enum,
                                                       std::string&& id);
// StructureAnnounce -> "struct" Id
// 返回声明的结构名与结构类型（kStruct）
std::pair<std::string, StructOrBasicType> StructureAnnounceStructId(
    std::string&& str_struct, std::string&& id);
// StructureAnnounce -> "union" Id
// 返回声明的结构名与结构类型（kUnion）
std::pair<std::string, StructOrBasicType> StructureAnnounceUnionId(
    std::string&& str_union, std::string&& id);
// StructureDefineHead -> "struct"
std::pair<std::string, StructOrBasicType> StructureDefineHeadStruct(
    std::string&& str_struct);
// StructureDefineHead -> "union"
std::pair<std::string, StructOrBasicType> StructureDefineHeadUnion(
    std::string&& str_union);
// StructureDefineHead -> StructureAnnounce
// 返回值意义见StructureAnnounce
std::pair<std::string, StructOrBasicType>&&
StructureDefineHeadStructureAnnounce(
    std::pair<std::string, StructOrBasicType>&& struct_data);
// StructureDefineInitHead -> StructureDefineHead "{"
// 执行一些初始化工作
// 返回结构数据类型节点
std::shared_ptr<StructureTypeInterface> StructureDefineInitHead(
    std::pair<std::string, StructOrBasicType>&& struct_data,
    std::string&& left_purly_bracket);
// StructureDefine -> StructureDefineInitHead StructureBody "}"
// 返回值类型：std::shared_ptr<StructureTypeInterface>
// 返回结构数据类型节点
std::shared_ptr<StructureTypeInterface>&& StructureDefine(
    std::shared_ptr<StructureTypeInterface>&& struct_data, std::nullptr_t,
    std::string&& right_curly_bracket);
// StructType -> StructureDefine
std::shared_ptr<const StructureTypeInterface> StructTypeStructDefine(
    std::shared_ptr<StructureTypeInterface>&& struct_data);
// StructType -> StructAnnounce
// 返回获取到的结构化数据类型
std::shared_ptr<const StructureTypeInterface> StructTypeStructAnnounce(
    std::pair<std::string, StructOrBasicType>&& struct_data);
// BasicType -> ConstTag SignTag FundamentalType
// 返回获取到的类型与ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeFundamental(
    ConstTag const_tag, SignTag sign_tag, BuiltInType builtin_type);
// BasicType -> ConstTag StructType
// 返回获取到的类型与ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeStructType(
    ConstTag const_tag,
    std::shared_ptr<const StructureTypeInterface>&& struct_data);
//// BasicType -> ConstTag Id
//// 返回获取到的类型与ConstTag
// std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeId(
//    std::vector<WordDataToUser>&& word_data);
// BasicType -> ConstTag EnumAnnounce
// 返回获取到的类型与ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeEnumAnnounce(
    ConstTag const_tag, std::pair<std::string, StructOrBasicType>&& enum_data);
// FunctionRelaventBasePartFunctionInit -> IdOrEquivence "("
// 做一些初始化工作
std::shared_ptr<ObjectConstructData>&& FunctionRelaventBasePartFunctionInitBase(
    std::shared_ptr<ObjectConstructData>&& construct_data,
    std::string&& left_bracket);
// FunctionRelaventBasePartFunctionInit -> FunctionRelaventBasePart "("
// 做一些声明函数/函数指针时的初始化工作
std::shared_ptr<ObjectConstructData>&&
FunctionRelaventBasePartFunctionInitExtend(
    std::shared_ptr<ObjectConstructData>&& construct_data,
    std::string&& left_bracket);
// FunctionRelaventBasePart -> FunctionRelaventBasePartFunctionInit
// FunctionRelaventArguments ")"
std::shared_ptr<ObjectConstructData>&& FunctionRelaventBasePartFunction(
    std::shared_ptr<ObjectConstructData>&& construct_data, std::nullptr_t&&,
    std::string&& right_bracket);
// FunctionRelaventBasePart -> ConstTag "*" FunctionRelaventBasePart
// 返回值类型：std::shared_ptr<ObjectConstructData>
std::shared_ptr<ObjectConstructData>&& FunctionRelaventBasePartPointer(
    ConstTag const_tag, std::string&& str_pointer,
    std::shared_ptr<ObjectConstructData>&& construct_data);
// FunctionRelaventBasePart -> "(" FunctionRelaventBasePart ")"
// 返回值类型：std::shared_ptr<ObjectConstructData>
std::shared_ptr<ObjectConstructData>&& FunctionRelaventBasePartBranckets(
    std::string&& left_bracket,
    std::shared_ptr<ObjectConstructData>&& construct_data,
    std::string&& right_bracket);
// FunctionRelavent -> BasicType FunctionRelaventBasePart
// 返回值包装的指针只可能为std::shared_ptr<FunctionType>（函数声明）
// 或std::shared_ptr<VarietyOperatorNode>（变量声明）
// Define操作交给上位函数进行
std::shared_ptr<FlowInterface> FunctionRelavent(
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& basic_type_data,
    std::shared_ptr<ObjectConstructData>&& construct_data);
// SingleAnnounceNoAssign -> BasicType IdOrEquivence
// AnonymousSingleAnnounceNoAssign -> BasicType AnonymousIdOrEquivence
// 两个产生式共用该规约函数，通过is_anonymous控制细微差别
// is_anonymous控制是否在声明匿名变量（匿名变量是类型）
// 返回值包装的指针只可能为std::shared_ptr<FunctionType>（函数声明）
// 或std::shared_ptr<VarietyOperatorNode>（变量声明）
// 不执行DefineVariety/DefineType也不添加空间分配节点
template <bool is_anonymous>
std::shared_ptr<FlowInterface> SingleAnnounceNoAssignVariety(
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& basic_type_data,
    std::shared_ptr<ObjectConstructData>&& construct_data);
// SingleAnnounceNoAssign -> ConstTag Id IdOrEquivence
// 不执行DefineVariety/DefineType也不添加空间分配节点
std::shared_ptr<FlowInterface> SingleAnnounceNoAssignNotPodVariety(
    ConstTag const_tag, std::string&& id,
    std::shared_ptr<ObjectConstructData>&& construct_data);
// SingleAnnounceNoAssign -> FunctionRelavent
// AnonymousSingleAnnounceNoAssign -> FunctionRelavent
// 两个产生式共用该规约函数，通过is_anonymous控制细微差别
// is_anonymous控制是否在声明匿名变量（匿名变量是类型）
// 不执行DefineVariety/DefineType也不添加空间分配节点
template <bool is_anonymous>
std::shared_ptr<FlowInterface>&& SingleAnnounceNoAssignFunctionRelavent(
    std::shared_ptr<FlowInterface>&& flow_control_node);
// TypeDef -> "typedef" SingleAnnounceNoAssign
// 不返回数据
std::nullptr_t TypeDef(std::string&& str_typedef,
                       std::shared_ptr<FlowInterface>&& flow_control_node);
// NotEmptyFunctionRelaventArguments -> SingleAnnounceNoAssign
std::nullptr_t NotEmptyFunctionRelaventArgumentsBase(
    std::shared_ptr<FlowInterface>&& flow_control_node);
// NotEmptyFunctionRelaventArguments -> AnonymousSingleAnnounceNoAssign
std::nullptr_t NotEmptyFunctionRelaventArgumentsAnonymousBase(
    std::shared_ptr<FlowInterface>&& flow_control_node);
// NotEmptyFunctionRelaventArguments -> NotEmptyFunctionRelaventArguments ","
// SingleAnnounceNoAssign
std::nullptr_t NotEmptyFunctionRelaventArgumentsExtend(
    std::nullptr_t&&, std::string&& str_comma,
    std::shared_ptr<FlowInterface>&& flow_control_node);
// NotEmptyFunctionRelaventArguments -> NotEmptyFunctionRelaventArguments ","
// AnonymousSingleAnnounceNoAssign
std::nullptr_t NotEmptyFunctionRelaventArgumentsAnonymousExtend(
    std::nullptr_t&&, std::string&& str_comma,
    std::shared_ptr<FlowInterface>&& flow_control_node);
// FunctionRelaventArguments -> NotEmptyFunctionRelaventArguments
std::nullptr_t FunctionRelaventArguments(std::nullptr_t);
// FunctionDefineHead -> FunctionRelavent "{"
// 对函数自身和每个参数执行Define并注册函数类型和函数对应的变量（利于查找）
std::shared_ptr<c_parser_frontend::flow_control::FunctionDefine>
FunctionDefineHead(std::shared_ptr<FlowInterface>&& function_head,
                   std::string&& left_curly_bracket);
// FunctionDefine -> FunctionDefineHead Statements "}"
// 返回值类型：std::shared_ptr<FunctionDefine>
// 检查添加的函数体是否为空，如果为空则添加无返回值语句或Error
// 弹出最后一层作用域，重置当前活跃函数
// 不返回任何数据
std::nullptr_t FunctionDefine(
    std::shared_ptr<c_parser_frontend::flow_control::FunctionDefine>&&,
    std::nullptr_t, std::string&& right_curly_bracket);
// SingleStructureBody -> SingleAnnounceNoAssign
// 返回扩展声明的ID使用的类型和变量本身的ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag>
SingleStructureBodyBase(std::shared_ptr<FlowInterface>&& flow_control_node);
// SingleStructureBody -> SingleStructureBody "," Id
// 返回值类型：std::pair<std::shared_ptr<const TypeInterface>, ConstTag>
// 返回扩展声明的ID使用的类型和变量本身的ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&&
SingleStructureBodyExtend(
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& struct_data,
    std::string&& str_comma, std::string&& new_member_name);
// NotEmptyStructureBody -> SingleStructureBody
std::nullptr_t NotEmptyStructureBodyBase(
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&&);
// NotEmptyStructureBody -> NotEmptyStructureBody SingleStructureBody ";"
std::nullptr_t NotEmptyStructureBodyExtend(
    std::nullptr_t, std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&&,
    std::string&& semicolon);
// StructureBody -> NotEmptyStructureBody
std::nullptr_t StructureBody(std::nullptr_t);
// InitializeList -> "{" InitializeListArguments "}"
std::shared_ptr<ListInitializeOperatorNode> InitializeList(
    std::string&& left_curly_bracket,
    std::shared_ptr<
        std::list<std::shared_ptr<InitializeOperatorNodeInterface>>>&&
        list_arguments,
    std::string&& right_curly_bracket);
// SingleInitializeListArgument -> SingleConstexprValue
std::shared_ptr<InitializeOperatorNodeInterface>&&
SingleInitializeListArgumentConstexprValue(
    std::shared_ptr<BasicTypeInitializeOperatorNode>&& value);
// SingleInitializeListArgument -> InitializeList
std::shared_ptr<InitializeOperatorNodeInterface>&&
SingleInitializeListArgumentList(
    std::shared_ptr<ListInitializeOperatorNode>&& value);
// InitializeListArguments -> SingleInitializeListArgument
std::shared_ptr<std::list<std::shared_ptr<InitializeOperatorNodeInterface>>>
InitializeListArgumentsBase(
    std::shared_ptr<InitializeOperatorNodeInterface>&& init_data_pointer);
// InitializeListArguments -> InitializeListArguments ","
// SingleInitializeListArgument
std::shared_ptr<std::list<std::shared_ptr<InitializeOperatorNodeInterface>>>&&
InitializeListArgumentsExtend(
    std::shared_ptr<std::list<
        std::shared_ptr<InitializeOperatorNodeInterface>>>&& list_pointer,
    std::string&& str_comma,
    std::shared_ptr<InitializeOperatorNodeInterface>&& init_data_pointer);
// AnnounceAssignable -> Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
AnnounceAssignableAssignable(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value);
// AnnounceAssignable -> InitializeList
// 返回空容器
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AnnounceAssignableInitializeList(
    std::shared_ptr<ListInitializeOperatorNode>&& initialize_list);
// SingleAnnounceAndAssign -> SingleAnnounceNoAssign
// 返回扩展声明时变量的类型（先去掉数组维数层指针，
// 如果仍为有指针类型则去掉一重指针）
// 变量的ConstTag和获取变量使用的操作
// 如果是合法的声明则添加变量定义并创建空间分配节点
std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
SingleAnnounceAndAssignNoAssignBase(
    std::shared_ptr<FlowInterface>&& flow_control_node);
// SingleAnnounceAndAssign -> SingleAnnounceNoAssign "=" AnnounceAssignable
// 返回扩展声明时变量的类型（如果最初为有指针类型则去掉一重指针，否则不变）
// 变量的ConstTag和获取变量使用的操作
// 如果是合法的声明则添加变量定义并创建空间分配节点和赋值节点
std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
SingleAnnounceAndAssignWithAssignBase(
    std::shared_ptr<FlowInterface>&& flow_control_node,
    std::string&& str_assign,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        container);
// SingleAnnounceAndAssign -> SingleAnnounceAndAssign "," Id
// 返回扩展声明时变量的类型（如果最初为有指针类型则去掉一重指针，否则不变）
// 变量的ConstTag和获取变量使用的操作
std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
SingleAnnounceAndAssignNoAssignExtend(
    std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
               std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        container,
    std::string&& str_comma, std::string&& variety_name);
// SingleAnnounceAndAssign -> SingleAnnounceAndAssign "," Id "="
// AnnounceAssignable
// 如果是合法的声明则添加变量定义并创建空间分配节点和赋值节点
std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
SingleAnnounceAndAssignWithAssignExtend(
    std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
               std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        container,
    std::string&& str_comma, std::string&& variety_name,
    std::string&& str_assign,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        assigned_container);
// Type -> BasicType
// 返回类型和变量的ConstTag
// 返回值类型：std::pair<std::shared_ptr<const TypeInterface>, ConstTag>
std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& TypeBasicType(
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& type_data);
// Type -> FunctionRelavent
// 返回类型和变量的ConstTag
std::pair<std::shared_ptr<const TypeInterface>, ConstTag> TypeFunctionRelavent(
    std::shared_ptr<FlowInterface>&& flow_control_node);
// MathematicalOperator -> "+"
// 返回数学运算符
MathematicalOperation MathematicalOperatorPlus(std::string&& str_operator);
// MathematicalOperator -> "-"
// 返回数学运算符
MathematicalOperation MathematicalOperatorMinus(std::string&& str_operator);
// MathematicalOperator -> "*"
// 返回数学运算符
MathematicalOperation MathematicalOperatorMultiple(std::string&& str_operator);
// MathematicalOperator -> "/"
// 返回数学运算符
MathematicalOperation MathematicalOperatorDivide(std::string&& str_operator);
// MathematicalOperator -> "%"
// 返回数学运算符
MathematicalOperation MathematicalOperatorMod(std::string&& str_operator);
// MathematicalOperator -> "<<"
// 返回数学运算符
MathematicalOperation MathematicalOperatorLeftShift(std::string&& str_operator);
// MathematicalOperator -> ">>"
// 返回数学运算符
MathematicalOperation MathematicalOperatorRightShift(
    std::string&& str_operator);
// MathematicalOperator -> "&"
// 返回数学运算符
MathematicalOperation MathematicalOperatorAnd(std::string&& str_operator);
// MathematicalOperator -> "|"
// 返回数学运算符
MathematicalOperation MathematicalOperatorOr(std::string&& str_operator);
// MathematicalOperator -> "^"
// 返回数学运算符
MathematicalOperation MathematicalOperatorXor(std::string&& str_operator);
// MathematicalOperator -> "!"
// 返回数学运算符
MathematicalOperation MathematicalOperatorNot(std::string&& str_operator);
// MathematicalAndAssignOperator -> "+="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorPlusAssign(
    std::string&& str_operator);
// MathematicalAndAssignOperator -> "-="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorMinusAssign(
    std::string&& str_operator);
// MathematicalAndAssignOperator -> "*="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorMultipleAssign(
    std::string&& str_operator);
// MathematicalAndAssignOperator -> "/="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorDivideAssign(
    std::string&& str_operator);
// MathematicalAndAssignOperator -> "%="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorModAssign(
    std::string&& str_operator);
// MathematicalAndAssignOperator -> "<<="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorLeftShiftAssign(
    std::string&& str_operator);
// MathematicalAndAssignOperator -> ">>="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorRightShiftAssign(
    std::string&& str_operator);
// MathematicalAndAssignOperator -> "&="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorAndAssign(
    std::string&& str_operator);
// MathematicalAndAssignOperator -> "|="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorOrAssign(
    std::string&& str_operator);
// MathematicalAndAssignOperator -> "^="
// 返回数学赋值运算符
MathematicalAndAssignOperation MathematicalAndAssignOperatorXorAssign(
    std::string&& str_operator);
// LogicalOperator -> "&&"
// 返回逻辑运算符
LogicalOperation LogicalOperatorAndAnd(std::string&& str_operator);
// LogicalOperator -> "||"
// 返回逻辑运算符
LogicalOperation LogicalOperatorOrOr(std::string&& str_operator);
// LogicalOperator -> ">"
// 返回逻辑运算符
LogicalOperation LogicalOperatorGreater(std::string&& str_operator);
// LogicalOperator -> ">="
// 返回逻辑运算符
LogicalOperation LogicalOperatorGreaterEqual(std::string&& str_operator);
// LogicalOperator -> "<"
// 返回逻辑运算符
LogicalOperation LogicalOperatorLess(std::string&& str_operator);
// LogicalOperator -> "<="
// 返回逻辑运算符
LogicalOperation LogicalOperatorLessEqual(std::string&& str_operator);
// LogicalOperator -> "=="
// 返回逻辑运算符
LogicalOperation LogicalOperatorEqual(std::string&& str_operator);
// LogicalOperator -> "!="
// 返回逻辑运算符
LogicalOperation LogicalOperatorNotEqual(std::string&& str_operator);
// Assignable -> SingleConstexprValue
// 返回这一步得到的最终可运算节点和获取过程的操作
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableConstexprValue(
    std::shared_ptr<BasicTypeInitializeOperatorNode>&& value);
// Assignable -> Id
// 返回这一步得到的最终可运算节点和获取过程的操作
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableId(std::string&& variety_name);
// Assignable -> TemaryOperator
// 返回这一步得到的最终可运算节点和获取过程的操作
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
AssignableTemaryOperator(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value);
// Assignable -> FunctionCall
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
AssignableFunctionCall(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value);
// Assignable -> "sizeof" "(" Type ")"
// 返回这一步得到的最终可运算节点和空容器（sizeof语义）
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSizeOfType(
    std::string&& str_sizeof, std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& type_data,
    std::string&& str_right_bracket);
// Assignable -> "sizeof" "(" Assignable ")"
// 返回这一步得到的最终可运算节点和空容器（sizeof语义）
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSizeOfAssignable(
    std::string&& str_sizeof, std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_right_bracket);
// Assignable -> Assignable "." Id
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMemberAccess(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_member_access, std::string&& member_name);
// Assignable -> Assignable "->" Id
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignablePointerMemberAccess(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_member_access, std::string&& member_name);
// Assignable -> "(" Assignable ")"
// 返回这一步得到的最终可运算节点和获取过程的操作
// 返回值类型：std::pair<std::shared_ptr<const OperatorNodeInterface>,
//                   std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
AssignableBracket(
    std::string&& left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value,
    std::string&& right_bracket);
// Assignable -> "(" Type ")" Assignable
// 返回这一步得到的最终可运算节点和获取过程的操作
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableTypeConvert(
    std::string&& left_bracket,
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& type_data,
    std::string&& right_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        old_variety_data);
// Assignable -> Assignable "=" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableAssign(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        destination_variety_data,
    std::string&& str_assign,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        source_variety_data);
// Assignable -> Assignable MathematicalOperator Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMathematicalOperate(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        destination_variety_data,
    MathematicalOperation mathematical_operation,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        source_variety_data);
// Assignable -> Assignable MathematicalAndAssignOperator Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMathematicalAndAssignOperate(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        destination_variety_data,
    MathematicalAndAssignOperation mathematical_and_assign_operation,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        source_variety_data);
// Assignable -> Assignable LogicalOperator Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableLogicalOperate(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&& lhr,
    LogicalOperation logical_operation,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        rhr);
// Assignable -> "!" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableNot(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data);
// Assignable -> "~" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableLogicalNegative(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data);
// Assignable -> "-" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMathematicalNegative(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data);
// Assignable -> "&" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableObtainAddress(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data);
// Assignable -> "*" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableDereference(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data);
// Assignable -> Assignable "[" Assignable "]"
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableArrayAccess(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        array_data,
    std::string&& left_square_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        index_data,
    std::string&& right_square_bracket);
// Assignable -> "++" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignablePrefixPlus(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data);
// Assignable -> "--" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignablePrefixMinus(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data);
// Assignable -> Assignable "++"
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSuffixPlus(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_operator);
// Assignable -> Assignable "--"
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSuffixMinus(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_operator);
// Return -> "return" Assignable ";"
std::nullptr_t ReturnWithValue(
    std::string&& str_return,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_semicolon);
// Return -> "return" ";"
std::nullptr_t ReturnWithoutValue(std::string&& str_return,
                                  std::string&& str_semicolon);
// TemaryOperator -> Assignable "?" Assignable ":" Assignable
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
TemaryOperator(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_question_mark,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        true_value,
    std::string&& str_colon,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        false_value);
// NotEmptyFunctionCallArguments -> Assignable
std::nullptr_t NotEmptyFunctionCallArgumentsBase(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value_data);
// NotEmptyFunctionCallArguments -> NotEmptyFunctionCallArguments "," Assignable
std::nullptr_t NotEmptyFunctionCallArgumentsExtend(
    std::nullptr_t, std::string&& str_comma,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value_data);
// FunctionCallArguments -> NotEmptyFunctionCallArguments
std::nullptr_t FunctionCallArguments(std::nullptr_t);
// FunctionCallInit -> Assignable "("
// 做一些初始化工作
// 返回函数调用对象和获取可调用对象的操作，同时设置全局变量
std::pair<std::shared_ptr<FunctionCallOperatorNode>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
FunctionCallInitAssignable(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        call_target,
    std::string&& left_bracket);
// FunctionCallInit -> Id "("
// 做一些初始化工作
// 返回函数调用对象和获取可调用对象的操作，同时设置全局变量
std::pair<std::shared_ptr<FunctionCallOperatorNode>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
FunctionCallInitId(std::string&& function_name, std::string&& str_left_bracket);
// FunctionCall -> FunctionCallInit FunctionCallArguments ")"
// 返回函数调用对象
// 第二个参数存储获取可调用对象操作和对可调用对象的调用
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
FunctionCall(
    std::pair<std::shared_ptr<FunctionCallOperatorNode>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        function_call_data,
    std::nullptr_t, std::string&& right_bracket);
// Assignables -> Assignable
// 返回保存运算过程的节点容器
std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&& AssignablesBase(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value_data);
// Assignables -> Assignables "," Assignable
std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&& AssignablesExtend(
    std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&&
        main_control_node_container,
    std::string&& str_comma,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value_data);
// Break -> "break" ";"
// 返回跳转语句（使用shared_ptr包装因为std::any&&不支持存储不可复制的值）
std::shared_ptr<std::unique_ptr<Jmp>> Break(std::string&& str_break,
                                            std::string&& str_semicolon);
// Continue -> "continue" ";"
// 返回跳转语句（使用shared_ptr包装因为std::any&&不支持存储不可复制的值）
std::shared_ptr<std::unique_ptr<Jmp>> Continue(std::string&& str_continue,
                                               std::string&& str_semicolon);
// SingleStatement -> If
// 不做任何操作
std::nullptr_t SingleStatementIf(std::nullptr_t);
// SingleStatement -> DoWhile
// 不做任何操作
std::nullptr_t SingleStatementDoWhile(std::nullptr_t);
// SingleStatement -> While
// 不做任何操作
std::nullptr_t SingleStatementWhile(std::nullptr_t);
// SingleStatement -> For
// 不做任何操作
std::nullptr_t SingleStatementFor(std::nullptr_t);
// SingleStatement -> Switch
// 不做任何操作
std::nullptr_t SingleStatementSwitch(std::nullptr_t);
// SingleStatement -> Assignable ";"
// 添加获取Assignable的流程控制语句
std::nullptr_t SingleStatementAssignable(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        expression,
    std::string&& str_semicolon);
// SingleStatement -> SingleAnnounceAndAssign ";"
// 添加声明变量过程中的流程控制语句
// 不返回任何值
std::nullptr_t SingleStatementAnnounce(
    std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
               std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        announce_statement,
    std::string&& str_semicolon);
// SingleStatement -> Return
// 不做任何操作
std::nullptr_t SingleStatementReturn(std::nullptr_t);
// SingleStatement -> Break
std::nullptr_t SingleStatementBreak(
    std::shared_ptr<std::unique_ptr<Jmp>>&& jmp_sentence);
// SingleStatement -> Continue
std::nullptr_t SingleStatementContinue(
    std::shared_ptr<std::unique_ptr<Jmp>>&& jmp_sentence);
// SingleStatement -> ";"
// 不做任何操作
std::nullptr_t SingleStatementEmptyStatement(std::string&& str_semicolon);
// IfCondition -> "if" "(" Assignable ")"
// 返回if条件节点和获取if条件过程中使用的操作
// 向控制器（parser_frontend）注册流程控制语句
// 不返回任何值
std::nullptr_t IfCondition(
    std::string&& str_if, std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_right_bracket);
// IfWithElse -> IfCondition ProcessControlSentenceBody "else"
// 转换成if-else语句
std::nullptr_t IfWithElse(std::nullptr_t, std::nullptr_t, std::string str_else);
// If->IfWithElse ProcessControlSentenceBody
std::nullptr_t IfElseSence(std::nullptr_t, std::nullptr_t);
// If -> IfCondition ProcessControlSentenceBody
std::nullptr_t IfIfSentence(std::nullptr_t, std::nullptr_t);
// ForRenewSentence -> Assignables
std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&& ForRenewSentence(
    std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&& expression);
// ForInitSentence -> Assignables
std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&&
ForInitSentenceAssignables(
    std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&& expression);
// ForInitSentence -> SingleAnnounceAndAssign
std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>
ForInitSentenceAnnounce(
    std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
               std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        announce_data);
// ForInitHead -> "for"
// 做一些准备工作
std::nullptr_t ForInitHead(std::string&& str_for);
// ForHead -> ForInitHead "(" ForInitSentence ";"
//            Assignable ";" ForRenewSentence ")"
// 规约for语句的三要素
std::nullptr_t ForHead(
    std::nullptr_t, std::string&& str_left_bracket,
    std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&&
        for_init_sentences,
    std::string&& str_semicolon1,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_semicolon2,
    std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&&
        for_renew_sentences,
    std::string&& str_right_bracket);
// For -> ForHead ProcessControlSentenceBody
// 弹出作用域
std::nullptr_t For(std::nullptr_t, std::nullptr_t);
// WhileInitHead -> "while" "(" Assignable ")"
std::nullptr_t WhileInitHead(
    std::string&& str_while, std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_right_bracket);
// While -> WhileInitHead ProcessControlSentenceBody
// 弹出作用域
std::nullptr_t While(std::nullptr_t, std::nullptr_t);
// DoWhileInitHead -> "do"
// 做一些准备工作
std::nullptr_t DoWhileInitHead(std::string&& str_do);
// DoWhile -> DoWhileInitHead ProcessControlSentenceBody
// "while" "(" Assignable ")" ";"
std::nullptr_t DoWhile(
    std::nullptr_t, std::nullptr_t, std::string&& str_while,
    std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_right_bracket, std::string&& str_semicolon);
// SwitchCase -> "case" SingleConstexprValue ":"
std::nullptr_t SwitchCaseSimple(
    std::string&& str_case,
    std::shared_ptr<BasicTypeInitializeOperatorNode>&& case_data,
    std::string&& str_colon);
// SwitchCase -> "default" ":"
std::nullptr_t SwitchCaseDefault(std::string&& str_default,
                                 std::string&& str_colon);
// SingleSwitchStatement -> SwitchCase
// 不做任何操作
std::nullptr_t SingleSwitchStatementCase(std::nullptr_t);
// SingleSwitchStatement -> Statements
// 不做任何操作
std::nullptr_t SingleSwitchStatementStatements(std::nullptr_t);
// SwitchStatements -> SwitchStatements SingleSwitchStatement
// 不做任何操作
std::nullptr_t SwitchStatements(std::nullptr_t, std::nullptr_t);
// SwitchCondition -> "switch" "(" Assignable ")"
std::nullptr_t SwitchCondition(
    std::string&& str_switch, std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_right_bracket);
// Switch -> SwitchCondition "{" SwitchStatements "}"
// 不做任何操作
std::nullptr_t Switch(std::nullptr_t, std::string&& str_left_curly_bracket,
                      std::nullptr_t, std::string&& str_right_curly_bracket);
// Statements -> Statements SingleStatement
// 不做任何操作
std::nullptr_t StatementsSingleStatement(std::nullptr_t, std::nullptr_t);
// StatementsLeftBrace -> Statements "{"
// 提升作用域等级
std::nullptr_t StatementsLeftBrace(std::nullptr_t,
                                   std::string&& left_curly_bracket);
// Statements -> StatementsLeftBrace Statements "}"
// 弹出作用域
std::nullptr_t StatementsBrace(std::nullptr_t, std::nullptr_t,
                               std::string&& right_curly_bracket);
// ProcessControlSentenceBody -> SingleStatement
// 不弹出作用域
std::nullptr_t ProcessControlSentenceBodySingleStatement(std::nullptr_t);
// ProcessControlSentenceBody -> "{" Statements "}"
// 不弹出作用域
// 不做任何操作
std::nullptr_t ProcessControlSentenceBodyStatements(
    std::string&& str_left_curly_bracket, std::nullptr_t,
    std::string&& str_right_curly_bracket);
// 根产生式
// Root -> Root FunctionDefine
// 不做任何操作
std::nullptr_t RootFunctionDefine(std::nullptr_t, std::nullptr_t);
// 根产生式
// Root -> Root SingleAnnounceNoAssign ";"
std::nullptr_t RootAnnounce(std::nullptr_t,
                            std::shared_ptr<FlowInterface>&& flow_control_node,
                            std::string&& str_colon);

template <class BasicObjectType, class... Args>
  requires std::is_same_v<BasicObjectType, VarietyOperatorNode> ||
           std::is_same_v<BasicObjectType,
                          c_parser_frontend::flow_control::FunctionDefine>
ObjectConstructData::CheckResult ObjectConstructData::ConstructBasicObjectPart(
    Args... args) {
  if constexpr (std::is_same_v<BasicObjectType, VarietyOperatorNode>) {
    // 构建变量数据
    auto object = std::make_unique<SimpleSentence>();
    bool result = object->SetSentenceOperateNode(
        std::make_shared<VarietyOperatorNode>(std::forward<Args>(args)...));
    assert(result);
    object_ = std::move(object);
  } else {
    // 构建函数数据
    object_ = std::make_unique<c_parser_frontend::flow_control::FunctionDefine>(
        std::forward<Args>(args)...);
  }
  return CheckResult::kSuccess;
}
template <bool is_anonymous>
std::shared_ptr<FlowInterface> SingleAnnounceNoAssignVariety(
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& basic_type_data,
    std::shared_ptr<ObjectConstructData>&& construct_data) {
  auto& [final_type, const_tag_before_final_type] = basic_type_data;
  if constexpr (is_anonymous) {
    if (construct_data == nullptr) [[likely]] {
      // 函数或函数指针声明中省略参数名
      construct_data = std::make_shared<ObjectConstructData>(std::string());
      construct_data->ConstructBasicObjectPart<VarietyOperatorNode>(
          std::string(), const_tag_before_final_type,
          LeftRightValueTag::kLeftValue);
    } else {
      assert(construct_data->GetObjectName().empty());
    }
  } else {
    if (construct_data->GetObjectName().empty()) [[unlikely]] {
      OutputError(std::format("声明的变量必须有名"));
      exit(-1);
    }
  }
  auto [flow_control_node, construct_result] = construct_data->ConstructObject(
      const_tag_before_final_type, std::move(final_type));
  // 检查是否构建成功
  if (construct_result != ObjectConstructData::CheckResult::kSuccess)
      [[unlikely]] {
    VarietyOrFunctionConstructError(construct_result,
                                    construct_data->GetObjectName());
  }
  return std::move(flow_control_node);
}

// SingleAnnounceNoAssign -> FunctionRelavent
// AnonymousSingleAnnounceNoAssign -> FunctionRelavent
// 两个产生式共用该规约函数，通过is_anonymous控制细微差别
// is_anonymous控制是否在声明匿名变量（匿名变量是类型）
// 返回值类型：std::shared_ptr<FlowInterface>
// 不执行DefineVariety/DefineType也不添加空间分配节点
template <bool is_anonymous>
std::shared_ptr<FlowInterface>&& SingleAnnounceNoAssignFunctionRelavent(
    std::shared_ptr<FlowInterface>&& flow_control_node) {
  if constexpr (is_anonymous) {
    auto& variety_node = static_cast<const VarietyOperatorNode&>(
        static_cast<SimpleSentence&>(*flow_control_node)
            .GetSentenceOperateNodeReference());
    assert(variety_node.GetVarietyName().empty());
  } else {
    const std::string* variety_name;
    switch (flow_control_node->GetFlowType()) {
      case FlowType::kSimpleSentence:
        variety_name = &static_cast<const VarietyOperatorNode&>(
                            static_cast<SimpleSentence&>(*flow_control_node)
                                .GetSentenceOperateNodeReference())
                            .GetVarietyName();
        break;
      case FlowType::kFunctionDefine:
        variety_name = &static_cast<const flow_control::FunctionDefine&>(
                            *flow_control_node)
                            .GetFunctionTypeReference()
                            .GetFunctionName();
        break;
      default:
        assert(false);
        break;
    }
    if (variety_name->empty()) [[unlikely]] {
      OutputError(std::format("声明的变量必须有名"));
      exit(-1);
    }
  }
  return std::move(flow_control_node);
}
// 语义同AttachSingleNodeToTailNodeEmplace，参数使用已经构建好的节点

}  // namespace c_parser_frontend::parse_functions
#endif