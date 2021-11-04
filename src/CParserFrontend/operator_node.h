#ifndef CPARSERFRONTEND_OPERATOR_NODE_H_
#define CPARSERFRONTEND_OPERATOR_NODE_H_
#include "Common/id_wrapper.h"
#include "type_system.h"

namespace c_parser_frontend::flow_control {
// 前向声明FlowInterface供三目运算符节点（TemaryOperatorNode）
// 中SetBranchCondition使用
class FlowInterface;
}  // namespace c_parser_frontend::flow_control

// 操作节点
namespace c_parser_frontend::operator_node {
// 运算节点大类
enum class GeneralOperationType {
  kAllocate,               // 分配空间
  kTypeConvert,            // 类型转换
  kVariety,                // 基础变量
  kInitValue,              // 初始化用值
  kTemaryOperator,         // ?:（三目运算符）
  kAssign,                 // =（赋值）
  kMathematicalOperation,  // 数学运算
  kLogicalOperation,       // 逻辑运算
  kDeReference,            // 解引用
  kObtainAddress,          // 取地址
  kMemberAccess,           // 成员访问
  kFunctionCall            // 函数调用
};
// 数学运算符
enum class MathematicalOperation {
  kOr,                    // |（按位或）
  kXor,                   // ^（按位异或）
  kAnd,                   // &（按位与）
  kLeftShift,             // <<（左移）
  kRightShift,            // >>（右移）
  kPlus,                  // +（加）
  kMinus,                 // -（减）
  kMultiple,              // *（乘）
  kDivide,                // /（除）
  kMod,                   // %（求模）
  kLogicalNegative,       // ~（按位取反）
  kMathematicalNegative,  // -（取负）
  kNot,                   // !（逻辑非）
};
// 数学与赋值运算符
enum class MathematicalAndAssignOperation {
  kOrAssign,          // |=
  kXorAssign,         // ^=
  kAndAssign,         // &=
  kLeftShiftAssign,   // <<=
  kRightShiftAssign,  // >>=
  kPlusAssign,        // +=
  kMinusAssign,       // -=
  kMultipleAssign,    // *=
  kDivideAssign,      // /=
  kModAssign,         // %=
};
// 逻辑运算符
enum class LogicalOperation {
  kAndAnd,        // &&
  kOrOr,          // ||
  kGreater,       // >
  kGreaterEqual,  // >=
  kLess,          // <
  kLessEqual,     // <=
  kEqual,         // ==
  kNotEqual       // !=
};
// 流程类型
enum class ProcessType {
  kCommon,   // 普通运算语句
  kIf,       // if分支语句
  kSwitch,   // Switch分支语句
  kDoWhile,  // Do-While循环语句
  kWhile,    // While循环语句
  kFor       // For循环语句
};
// 初始化值类型
enum class InitializeType {
  kBasic,          // 基础类型，具体类型取决于值的范围
  kString,         // 字符串，固定为const char*类型
  kFunction,       // 用于给函数指针赋值的全局初始化常量类型
  kInitializeList  // 初始化列表
};
// 左右值类型
enum class LeftRightValueTag {
  kLeftValue,  // 左值
  kRightValue  // 右值
};

// 引用变量类型基类
using c_parser_frontend::type_system::TypeInterface;
// 引用变量大类
using c_parser_frontend::type_system::StructOrBasicType;
// 引用变量系统
using c_parser_frontend::type_system::TypeSystem;
// 引用常用类型生成器
using c_parser_frontend::type_system::CommonlyUsedTypeGenerator;
// 引用判断是否可以赋值结果的枚举
using c_parser_frontend::type_system::AssignableCheckResult;
// 引用const标记
using c_parser_frontend::type_system::ConstTag;
// 引用符号标记
using c_parser_frontend::type_system::SignTag;
// 引用预定义类型
using c_parser_frontend::type_system::BuiltInType;

// 将数学与赋值运算符转化为数学运算符
MathematicalOperation MathematicalAndAssignOperationToMathematicalOperation(
    MathematicalAndAssignOperation mathematical_and_assign_operation);

class OperatorNodeInterface {
  enum class IdWrapper { kOperatorNodeId };

 public:
  // 操作节点ID
  using OperatorId =
      frontend::common::ExplicitIdWrapper<size_t, IdWrapper,
                                          IdWrapper::kOperatorNodeId>;

  OperatorNodeInterface(GeneralOperationType general_operator_type)
      : general_operator_type_(general_operator_type) {}
  // 复制构造函数不复制节点编号而是生成新的编号
  OperatorNodeInterface(const OperatorNodeInterface& operator_node)
      : general_operator_type_(operator_node.general_operator_type_) {}
  virtual ~OperatorNodeInterface() {}

  OperatorNodeInterface& operator=(const OperatorNodeInterface& old_interface) {
    general_operator_type_ = old_interface.general_operator_type_;
    return *this;
  }

  // 获取该节点结果的ConstTag
  virtual ConstTag GetResultConstTag() const = 0;
  // 获取最终结果的类型
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer() const = 0;
  // 获取最终结果的节点，返回nullptr代表自身
  // 如果不返回空指针则一定是VarietyOperatorNode或InitializeOperatorNodeInterface
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const = 0;
  // 拷贝自身生成shared_ptr并设置拷贝得到的对象类型为new_type
  // 如果不能拷贝则返回nullptr
  // 用于类型转换
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const = 0;

  OperatorId GetOperatorId() const { return operator_id_; }
  void SetGeneralOperatorType(GeneralOperationType operator_type) {
    general_operator_type_ = operator_type;
  }
  GeneralOperationType GetGeneralOperatorType() const {
    return general_operator_type_;
  }

 private:
  // 获取一个从未被分配过的OperatorId
  static OperatorId GetNewOperatorId() {
    static thread_local OperatorId operator_id(0);
    return operator_id++;
  }

  // 操作节点ID，同时作为存放结果的LLVM的寄存器编号
  const OperatorId operator_id_ = GetOperatorId();
  // 较模糊的运算符类型
  GeneralOperationType general_operator_type_;
};

class AllocateOperatorNode : public OperatorNodeInterface {
 public:
  AllocateOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kAllocate) {}
  AllocateOperatorNode(const AllocateOperatorNode&) = delete;

  AllocateOperatorNode& operator=(const AllocateOperatorNode&) = delete;

  virtual ConstTag GetResultConstTag() const override {
    assert(false);
    // 防止警告
    return ConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<const TypeInterface>();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<const OperatorNodeInterface>();
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  // 设置要分配内存的节点
  // 返回是否可以分配，不能分配则不会设置
  // 同时设置待分配空间大小
  bool SetTargetVariety(
      const std::shared_ptr<const OperatorNodeInterface>& target_variety);
  std::shared_ptr<const OperatorNodeInterface> GetTargetVarietyPointer() const {
    return target_variety_;
  }
  const OperatorNodeInterface& GetTargetVarietyReference() const {
    return *target_variety_;
  }

  static bool CheckAllocatable(const OperatorNodeInterface& node_to_allocate) {
    assert(node_to_allocate.GetGeneralOperatorType() ==
           GeneralOperationType::kVariety);
    return true;
  }

 private:
  // 分配空间后存储到的指针
  std::shared_ptr<const OperatorNodeInterface> target_variety_;
};

// 类型转换
// 源节点需要支持GetResultOperatorNode()
class TypeConvert : public OperatorNodeInterface {
 public:
  TypeConvert(const std::shared_ptr<const OperatorNodeInterface>& source_node,
              const std::shared_ptr<const TypeInterface>& new_type)
      : OperatorNodeInterface(GeneralOperationType::kTypeConvert),
        source_node_(source_node),
        destination_node_(source_node->SelfCopy(new_type)) {}
  TypeConvert(const TypeConvert&) = delete;

  virtual ConstTag GetResultConstTag() const override {
    return GetDestinationNodeReference().GetResultConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetDestinationNodeReference().GetResultTypePointer();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return GetDestinationNodePointer();
  }
  // 不能拷贝类型转换节点，应该拷贝destination_node_
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  std::shared_ptr<const OperatorNodeInterface> GetSourceNodePointer() const {
    return source_node_;
  }
  const OperatorNodeInterface& GetSourceNodeReference() const {
    return *source_node_;
  }
  std::shared_ptr<OperatorNodeInterface> GetDestinationNodePointer() const {
    return destination_node_;
  }
  OperatorNodeInterface& GetDestinationNodeReference() const {
    return *destination_node_;
  }
  void SetSourceNode(
      const std::shared_ptr<const OperatorNodeInterface>& source_node) {
    source_node_ = source_node;
  }
  // 生成目标节点
  // 输入新的类型和新的节点的const标记
  // 返回是否可以转换，如果不能转换则不会生成目标节点
  bool GenerateDestinationNode(
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag);

 private:
  // 待转换的节点
  std::shared_ptr<const OperatorNodeInterface> source_node_;
  // 转换得到的目的节点
  std::shared_ptr<OperatorNodeInterface> destination_node_;
};

// 存储变量
class VarietyOperatorNode : public OperatorNodeInterface {
 public:
  VarietyOperatorNode(const std::string* variety_name, ConstTag const_tag,
                      LeftRightValueTag left_right_value_tag)
      : OperatorNodeInterface(GeneralOperationType::kVariety),
        variety_name_(variety_name),
        variety_const_tag_(const_tag),
        variety_left_right_value_tag_(left_right_value_tag) {}
  VarietyOperatorNode(const VarietyOperatorNode& variety_node) = default;

  VarietyOperatorNode& operator=(const VarietyOperatorNode& variety_node) =
      default;

  virtual ConstTag GetResultConstTag() const override { return GetConstTag(); }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return variety_type_;
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return nullptr;
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    auto new_node = std::make_shared<VarietyOperatorNode>(*this);
    new_node->SetVarietyType(new_type);
    return new_node;
  }

  void SetVarietyName(const std::string* variety_name) {
    variety_name_ = variety_name;
  }
  const std::string* GetVarietyNamePointer() const { return variety_name_; }
  void SetConstTag(ConstTag const_tag) { variety_const_tag_ = const_tag; }
  ConstTag GetConstTag() const { return variety_const_tag_; }
  void SetLeftRightValueTag(LeftRightValueTag left_right_value_tag) {
    variety_left_right_value_tag_ = left_right_value_tag;
  }
  LeftRightValueTag GetLeftRightValueTag() const {
    return variety_left_right_value_tag_;
  }
  bool SetVarietyType(const std::shared_ptr<const TypeInterface>& variety_type);
  // 相比于SetVarietyType，允许使用FunctionType
  // 用于初始化列表赋值时构建参与检查的VarietyType和解引用
  bool SetVarietyTypeNoCheckFunctionType(
      const std::shared_ptr<const TypeInterface>& variety_type);
  std::shared_ptr<const TypeInterface> GetVarietyTypePointer() const {
    return variety_type_;
  }
  const TypeInterface& GetVarietyTypeReference() const {
    return *variety_type_;
  }

  // 检查给定类型是否可以作为变量的类型
  static bool CheckVarietyTypeValid(const TypeInterface& variety_type);

 private:
  // 变量名，出错提示时用
  const std::string* variety_name_;
  // 变量类型
  std::shared_ptr<const TypeInterface> variety_type_;
  // 变量本身的const标记
  ConstTag variety_const_tag_;
  // 变量的左右值标记
  LeftRightValueTag variety_left_right_value_tag_;
};

// 存储初始化数据（编译期常量）
class InitializeOperatorNodeInterface : public OperatorNodeInterface {
 public:
  InitializeOperatorNodeInterface(InitializeType initialize_type)
      : OperatorNodeInterface(GeneralOperationType::kInitValue),
        initialize_type_(initialize_type) {}
  InitializeOperatorNodeInterface(
      const InitializeOperatorNodeInterface& initialize_node) = default;

  InitializeOperatorNodeInterface& operator=(
      const InitializeOperatorNodeInterface& initialize_node) = default;

  virtual ConstTag GetResultConstTag() const override final {
    return ConstTag::kConst;
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override final {
    return initialize_value_type_;
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override final {
    return nullptr;
  }

  void SetInitlizeType(InitializeType initialize_type) {
    initialize_type_ = initialize_type;
  }
  InitializeType GetInitializeType() const { return initialize_type_; }
  // 设置该节点的类型
  // 返回是否为有效的初始化数据类型，如果无效则不添加
  bool SetInitValueType(
      const std::shared_ptr<const TypeInterface>& init_value_type);
  const std::shared_ptr<const TypeInterface>& GetInitValueType() const {
    return initialize_value_type_;
  }
  // 返回是否为有效的初始化数据类型
  static bool CheckInitValueTypeValid(const TypeInterface& init_value_type);

 private:
  // 初始化数据类型
  std::shared_ptr<const TypeInterface> initialize_value_type_;
  // 初始化节点类型
  InitializeType initialize_type_;
};

// 基础初始化类型（数值/字符串/函数）
// 保存函数时value留空
class BasicTypeInitializeOperatorNode : public InitializeOperatorNodeInterface {
 public:
  template <class Value>
  BasicTypeInitializeOperatorNode(InitializeType initialize_type, Value&& value)
      : InitializeOperatorNodeInterface(initialize_type),
        value_(std::forward<Value>(value)) {}
  BasicTypeInitializeOperatorNode(
      const std::shared_ptr<const type_system::FunctionType>& function_type)
      : InitializeOperatorNodeInterface(InitializeType::kFunction) {
    bool result = SetInitDataType(function_type);
    assert(result);
  }
  BasicTypeInitializeOperatorNode(const BasicTypeInitializeOperatorNode&) =
      default;

  BasicTypeInitializeOperatorNode& operator=(
      const BasicTypeInitializeOperatorNode&) = default;

  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    auto new_node = std::make_shared<BasicTypeInitializeOperatorNode>(*this);
    new_node->SetInitDataType(new_type);
    return new_node;
  }

  template <class Value>
  void SetValue(Value&& value) {
    value_ = std::forward<Value>(value);
  }
  const std::string& GetValue() const { return value_; }
  // 设置初始化用数据类型（基础类型/const char*/函数类型）
  bool SetInitDataType(const std::shared_ptr<const TypeInterface>& data_type);
  // 该对象是否可以使用给定的类型
  // 有效类型返回true，无效类型返回false
  static bool CheckBasicTypeInitializeValid(const TypeInterface& variety_type);

 private:
  // 外部访问时不应使用该函数设置类型，使用SetInitDataType代替
  // 放在private防止误用
  bool SetInitValueType(
      const std::shared_ptr<const TypeInterface>& init_value_type) {
    return InitializeOperatorNodeInterface::SetInitValueType(init_value_type);
  }

  // 字符串形式存储以防精度损失
  std::string value_;
};

// 初始化列表
class ListInitializeOperatorNode : public InitializeOperatorNodeInterface {
 public:
  ListInitializeOperatorNode()
      : InitializeOperatorNodeInterface(InitializeType::kInitializeList) {}
  ListInitializeOperatorNode(const ListInitializeOperatorNode&) = default;

  ListInitializeOperatorNode& operator=(const ListInitializeOperatorNode&) =
      default;

  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  // 向初始化列表中添加初始化值
  // 返回值是否为有效的初始化列表可用类型，如果类型无效则不执行插入操作
  // 初始化值的顺序为书写顺序
  bool SetListValues(
      std::list<std::shared_ptr<InitializeOperatorNodeInterface>>&&
          list_values);
  const std::list<std::shared_ptr<const InitializeOperatorNodeInterface>>&
  GetListValues() const {
    return list_values_;
  }
  // 检查给定的初始化列表类型是否合法
  static bool CheckInitListTypeValid(const TypeInterface& list_type) {
    return list_type.GetType() == StructOrBasicType::kInitializeList;
  }
  // 检查给定的初始化值是否合法
  static bool CheckInitListValueTypeValid(
      const TypeInterface& list_value_type) {
    return InitializeOperatorNodeInterface::CheckInitValueTypeValid(
        list_value_type);
  }

 private:
  // 初始化列表中的值
  std::list<std::shared_ptr<const InitializeOperatorNodeInterface>>
      list_values_;
};

// 三目运算符
class TemaryOperatorNode : public OperatorNodeInterface {
 public:
  TemaryOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kTemaryOperator) {}

  virtual ConstTag GetResultConstTag() const override {
    return GetResultReference().GetResultConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    // 赋值语句等价于被赋值的变量
    return GetResultReference().GetResultTypePointer();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return GetResultReference().GetResultOperatorNode();
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  // 设置分支条件
  // 如果给定参数无法作为分支条件则不设置且返回false
  // 先设置分支条件后设置分支内容
  // 设置分支条件后如果需要改动则需要重新设置一遍真假分支
  // 此时仍可以使用Get函数获取之前存储的分支信息
  bool SetBranchCondition(
      const std::shared_ptr<const OperatorNodeInterface>& branch_condition,
      const std::shared_ptr<const std::list<
          std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
          flow_control_node_container);
  std::shared_ptr<const OperatorNodeInterface> GetBranchConditionPointer()
      const {
    return branch_condition_;
  }
  const OperatorNodeInterface& GetBranchConditionReference() const {
    return *branch_condition_;
  }
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
  GetFlowControlNodeToGetConditionPointer() const {
    return condition_flow_control_node_container_;
  }
  const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&
  GetFlowControlNodeToGetConditionReference() const;
  // 设置条件为真时的分支
  // 先设置分支条件后设置分支内容
  // 如果给定参数无法作为分支则不设置且返回false
  // 设置分支条件后如果需要改动则需要重新设置一遍真假分支
  // 此时仍可以使用Get函数获取之前存储的分支信息
  bool SetTrueBranch(
      const std::shared_ptr<const OperatorNodeInterface>& true_branch,
      const std::shared_ptr<const std::list<
          std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
          flow_control_node_container);
  std::shared_ptr<const OperatorNodeInterface> GetTrueBranchPointer() const {
    return true_branch_;
  }
  const OperatorNodeInterface& GetTrueBranchReference() const {
    return *true_branch_;
  }
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
  GetFlowControlNodeToGetTrueBranchPointer() const {
    return true_branch_flow_control_node_container_;
  }
  const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&
  GetFlowControlNodeToGetTrueBranchReference() const;
  // 设置条件为假时的分支
  // 先设置分支条件后设置分支内容
  // 如果给定参数无法作为分支则不设置且返回false
  // 设置分支条件后如果需要改动则需要重新设置一遍真假分支
  // 此时仍可以使用Get函数获取之前存储的分支信息
  bool SetFalseBranch(
      const std::shared_ptr<const OperatorNodeInterface>& false_branch,
      const std::shared_ptr<const std::list<
          std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
          flow_control_node_container);
  std::shared_ptr<const OperatorNodeInterface> GetFalseBranchPointer() const {
    return false_branch_;
  }
  const OperatorNodeInterface& GetFalseBranchReference() const {
    return *false_branch_;
  }
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
  GetFlowControlNodeToGetFalseBranchPointer() const {
    return false_branch_flow_control_node_container_;
  }
  const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&
  GetFlowControlNodeToGetFalseBranchReference() const;
  std::shared_ptr<const OperatorNodeInterface> GetResultPointer() const {
    return result_;
  }
  const OperatorNodeInterface& GetResultReference() const { return *result_; }

  // 检查分支条件是否有效（是否有值）
  static bool CheckBranchConditionValid(
      const OperatorNodeInterface& branch_condition);
  // 检查条件分支是否有效（是否有值）
  static bool CheckBranchValid(const OperatorNodeInterface& branch);

 private:
  // 创建返回结果的节点
  // 设置分支条件和两个分支后调用
  // 如果两个分支不能互相转化则返回false
  // 如果分支条件为编译期常量则不做任何操作返回true
  // 不支持非编译期常量条件下使用初始化列表
  bool ConstructResultNode();

  // 分支条件
  std::shared_ptr<const OperatorNodeInterface> branch_condition_;
  // 获取分支条件的操作
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
      condition_flow_control_node_container_;
  // 条件为真时的值
  std::shared_ptr<const OperatorNodeInterface> true_branch_ = nullptr;
  // 获取真分支的操作
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
      true_branch_flow_control_node_container_;
  // 条件为假时的值
  std::shared_ptr<const OperatorNodeInterface> false_branch_ = nullptr;
  // 获取假分支的操作
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
      false_branch_flow_control_node_container_;
  // 存储返回结果的节点
  std::shared_ptr<const OperatorNodeInterface> result_;
};

// 赋值节点
class AssignOperatorNode : public OperatorNodeInterface {
 public:
  AssignOperatorNode() : OperatorNodeInterface(GeneralOperationType::kAssign) {}
  AssignOperatorNode(const AssignOperatorNode&) = delete;

  AssignOperatorNode& operator=(const AssignOperatorNode&) = delete;

  virtual ConstTag GetResultConstTag() const override {
    return GetNodeToBeAssignedReference().GetResultConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    // 赋值语句等价于被赋值的变量
    return GetNodeToBeAssignedReference().GetResultTypePointer();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return GetNodeToBeAssignedPointer();
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  // 先设置被赋值的节点，后设置用来赋值的节点
  void SetNodeToBeAssigned(
      const std::shared_ptr<const OperatorNodeInterface>& node_to_be_assigned) {
    node_to_be_assigned_ = node_to_be_assigned;
  }
  std::shared_ptr<const OperatorNodeInterface> GetNodeToBeAssignedPointer()
      const {
    return node_to_be_assigned_;
  }
  const OperatorNodeInterface& GetNodeToBeAssignedReference() const {
    return *node_to_be_assigned_;
  }
  // 如果不是可以赋值的情况则不会设置
  // 输入指向要设置的用来赋值的节点的指针和是否为声明时赋值
  // 声明时赋值会忽略被赋值的节点自身的const属性和LeftRightValue属性
  // 并且允许使用初始化列表
  // 需要先设置被赋值的节点（SetNodeToBeAssigned）
  // 如果被赋值的节点与用来赋值的节点类型完全相同（operator==()）则
  // 设置被赋值的节点的类型链指针指向用来赋值的类型链以节省内存
  AssignableCheckResult SetNodeForAssign(
      const std::shared_ptr<const OperatorNodeInterface>& node_for_assign,
      bool is_announce);
  std::shared_ptr<const OperatorNodeInterface> GetNodeForAssignPointer() const {
    return node_for_assign_;
  }
  const OperatorNodeInterface& GetNodeForAssignReference() const {
    return *node_for_assign_;
  }

  // 检查给定节点的类型是否可以赋值
  // 函数的模板参数表示是否为声明时赋值
  // 自动检查AssignableCheckResut::kMayBeZeroToPointer的具体情况并改变返回结果
  // 当is_announce == true时
  // 忽略const标记和LeftRightValue标记的检查且允许使用初始化列表
  // 如果使用自动数组大小推断则更新数组大小（违反const原则）
  // 如果被赋值的节点与用来赋值的节点类型完全相同（operator==()）且为变量类型
  // 则设置被赋值的节点的类型链指针指向用来赋值的类型链以节省内存(违反const原则)
  static AssignableCheckResult CheckAssignable(
      const OperatorNodeInterface& node_to_be_assigned,
      const OperatorNodeInterface& node_for_assign, bool is_announce);

 private:
  // CheckAssignable的子过程，处理使用初始化变量列表初始化变量的情况
  // 输入待初始化的变量和所用的初始化列表，要求已经验证为声明时
  // 不会返回AssignableCheckResult::kInitializeList
  // 如果使用自动数组大小推断则更新数组大小（违反const原则）
  static AssignableCheckResult VarietyAssignableByInitializeList(
      const VarietyOperatorNode& variety_node,
      const ListInitializeOperatorNode& list_initialize_operator_node);

  // 将要被赋值的节点
  std::shared_ptr<const OperatorNodeInterface> node_to_be_assigned_;
  // 用来赋值的节点
  std::shared_ptr<const OperatorNodeInterface> node_for_assign_;
};

class MathematicalOperatorNode : public OperatorNodeInterface {
 public:
  using DeclineMathematicalComputeTypeResult =
      c_parser_frontend::type_system::DeclineMathematicalComputeTypeResult;

  MathematicalOperatorNode(MathematicalOperation mathematical_operation)
      : OperatorNodeInterface(GeneralOperationType::kMathematicalOperation),
        mathematical_operation_(mathematical_operation) {}
  MathematicalOperatorNode(const MathematicalOperatorNode&) = delete;

  MathematicalOperatorNode& operator=(const MathematicalOperatorNode&) = delete;

  virtual ConstTag GetResultConstTag() const override {
    return ConstTag::kConst;
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetComputeResultNodeReference().GetResultTypePointer();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return GetComputeResultNodePointer();
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  void SetMathematicalOperation(MathematicalOperation mathematical_operation) {
    mathematical_operation_ = mathematical_operation;
  }
  MathematicalOperation GetMathematicalOperation() const {
    return mathematical_operation_;
  }
  // 先设置运算类型后设置节点
  bool SetLeftOperatorNode(
      const std::shared_ptr<const OperatorNodeInterface>& left_operator_node);
  std::shared_ptr<const OperatorNodeInterface> GetLeftOperatorNodePointer()
      const {
    return left_operator_node_;
  }
  const OperatorNodeInterface& GetLeftOperatorNodeReference() const {
    return *left_operator_node_;
  }
  // 先设置左节点后设置右节点
  DeclineMathematicalComputeTypeResult SetRightOperatorNode(
      const std::shared_ptr<const OperatorNodeInterface>& right_operator_node);
  std::shared_ptr<const OperatorNodeInterface> GetRightOperatorNodePointer()
      const {
    return right_operator_node_;
  }
  const OperatorNodeInterface& GetRightOperatorNodeReference() const {
    return *right_operator_node_;
  }
  std::shared_ptr<const VarietyOperatorNode> GetComputeResultNodePointer()
      const {
    return compute_result_node_;
  }
  const VarietyOperatorNode& GetComputeResultNodeReference() const {
    return *compute_result_node_;
  }

  // 推算运算后结果，返回运算后结果和推算的具体情况
  // 如果不能运算则返回nullptr
  static std::pair<std::shared_ptr<const VarietyOperatorNode>,
                   DeclineMathematicalComputeTypeResult>
  DeclineComputeResult(
      MathematicalOperation mathematical_operation,
      const std::shared_ptr<const OperatorNodeInterface>& left_operator_node,
      const std::shared_ptr<const OperatorNodeInterface>& right_operator_node);

 private:
  void SetComputeResultNode(
      const std::shared_ptr<const VarietyOperatorNode>& compute_result_node) {
    compute_result_node_ = compute_result_node;
  }

  // 数学运算类型
  MathematicalOperation mathematical_operation_;
  // 左运算节点
  std::shared_ptr<const OperatorNodeInterface> left_operator_node_;
  // 右运算节点（如果没有则置空）
  std::shared_ptr<const OperatorNodeInterface> right_operator_node_;
  // 运算得到的类型
  std::shared_ptr<const VarietyOperatorNode> compute_result_node_;
};

class LogicalOperationOperatorNode : public OperatorNodeInterface {
 public:
  LogicalOperationOperatorNode(LogicalOperation logical_operation)
      : OperatorNodeInterface(GeneralOperationType::kLogicalOperation),
        logical_operation_(logical_operation) {}
  LogicalOperationOperatorNode(const LogicalOperationOperatorNode&) = delete;

  LogicalOperationOperatorNode& operator=(const LogicalOperationOperatorNode&) =
      delete;

  virtual ConstTag GetResultConstTag() const override {
    return ConstTag::kConst;
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetComputeResultNodeReference().GetResultTypePointer();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return GetComputeResultNodePointer();
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  LogicalOperation GetLogicalOperation() const { return logical_operation_; }
  bool SetLeftOperatorNode(
      const std::shared_ptr<const OperatorNodeInterface>& left_operator_node);
  std::shared_ptr<const OperatorNodeInterface> GetLeftOperatorNodePointer()
      const {
    return left_operator_node_;
  }
  const OperatorNodeInterface& GetLeftOperatorNodeReference() const {
    return *left_operator_node_;
  }
  // 返回是否为可用于逻辑运算的类型，不可用于逻辑运算则不设置
  // 成功设置后创建逻辑运算结果节点
  bool SetRightOperatorNode(
      const std::shared_ptr<const OperatorNodeInterface>& right_operator_node);
  std::shared_ptr<const OperatorNodeInterface> GetRightOperatorNodePointer()
      const {
    return right_operator_node_;
  }
  const OperatorNodeInterface& GetRightOperatorNodeReference() const {
    return *right_operator_node_;
  }
  std::shared_ptr<const VarietyOperatorNode> GetComputeResultNodePointer()
      const {
    return compute_result_node_;
  }
  const VarietyOperatorNode& GetComputeResultNodeReference() const {
    return *compute_result_node_;
  }

  // 检查是否可以参与逻辑运算
  static bool CheckLogicalTypeValid(const TypeInterface& type_interface);

 private:
  // 创建并设置结果节点
  void CreateAndSetResultNode() {
    // 所有逻辑运算结果均为bool
    auto compute_result_node = std::make_shared<VarietyOperatorNode>(
        nullptr, ConstTag::kNonConst, LeftRightValueTag::kRightValue);
    compute_result_node->SetVarietyType(
        CommonlyUsedTypeGenerator::GetBasicType<BuiltInType::kInt1,
                                                SignTag::kUnsigned>());
    compute_result_node_ = compute_result_node;
  }

  // 逻辑运算类型
  LogicalOperation logical_operation_;
  // 左节点
  std::shared_ptr<const OperatorNodeInterface> left_operator_node_;
  // 右节点
  std::shared_ptr<const OperatorNodeInterface> right_operator_node_;
  // 运算结果
  std::shared_ptr<const VarietyOperatorNode> compute_result_node_;
};

// 对指针解引用
class DereferenceOperatorNode : public OperatorNodeInterface {
 public:
  DereferenceOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kDeReference) {}
  DereferenceOperatorNode(const DereferenceOperatorNode&) = delete;

  DereferenceOperatorNode& operator=(const DereferenceOperatorNode&) = delete;

  virtual ConstTag GetResultConstTag() const override {
    return GetDereferencedObjectConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetDereferencedNodeReference().GetResultTypePointer();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return GetDereferencedNodePointer();
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  // 设定待解引用的节点
  // 函数参数为待解引用的节点
  // 函数返回是否成功设置
  bool SetNodeToDereference(
      const std::shared_ptr<const OperatorNodeInterface>& node_to_dereference);

  std::shared_ptr<const OperatorNodeInterface> GetNodeToDereferencePointer()
      const {
    return node_to_dereference_;
  }
  const OperatorNodeInterface& GetNodeToDereferenceReference() const {
    return *node_to_dereference_;
  }
  std::shared_ptr<const VarietyOperatorNode> GetDereferencedNodePointer()
      const {
    return dereferenced_node_;
  }
  const VarietyOperatorNode& GetDereferencedNodeReference() const {
    return *dereferenced_node_;
  }

  ConstTag GetDereferencedObjectConstTag() const {
    return dereferenced_node_->GetConstTag();
  }
  static bool CheckNodeDereferenceAble(
      const OperatorNodeInterface& node_to_dereference);

 private:
  void SetDereferencedNode(
      const std::shared_ptr<VarietyOperatorNode>& dereferenced_node) {
    dereferenced_node_ = dereferenced_node;
  }

  // 将要被解引用的节点
  std::shared_ptr<const OperatorNodeInterface> node_to_dereference_;
  // 解引用后得到的对象的节点
  std::shared_ptr<VarietyOperatorNode> dereferenced_node_;
};

class ObtainAddressOperatorNode : public OperatorNodeInterface {
 public:
  ObtainAddressOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kObtainAddress) {}
  ObtainAddressOperatorNode(const ObtainAddressOperatorNode&) = delete;

  ObtainAddressOperatorNode& operator=(const ObtainAddressOperatorNode&) =
      delete;

  virtual ConstTag GetResultConstTag() const override {
    // 取地址获得的值都是非const
    return ConstTag::kNonConst;
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetObtainedAddressNodeReference().GetResultTypePointer();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return GetObtainedAddressNodePointer();
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  // 返回是否可以设置，如果不可设置则不会设置
  bool SetNodeToObtainAddress(
      const std::shared_ptr<const VarietyOperatorNode>& node_to_obtain_address);

  std::shared_ptr<const OperatorNodeInterface> GetNodeToBeObtainAddressPointer()
      const {
    return node_to_obtain_address_;
  }
  const OperatorNodeInterface& GetNodeToBeObtainAddressReference() const {
    return *node_to_obtain_address_;
  }
  std::shared_ptr<const OperatorNodeInterface> GetObtainedAddressNodePointer()
      const {
    return obtained_address_node_;
  }
  const OperatorNodeInterface& GetObtainedAddressNodeReference() const {
    return *obtained_address_node_;
  }
  static bool CheckNodeToObtainAddress(
      const OperatorNodeInterface& node_interface);

 private:
  void SetNodeObtainedAddress(
      const std::shared_ptr<const OperatorNodeInterface>&
          node_obtained_address) {
    obtained_address_node_ = node_obtained_address;
  }

  // 将要被取地址的节点
  std::shared_ptr<const VarietyOperatorNode> node_to_obtain_address_;
  // 取地址后获得的节点
  std::shared_ptr<const OperatorNodeInterface> obtained_address_node_;
};

class MemberAccessOperatorNode : public OperatorNodeInterface {
 public:
  using MemberIndex =
      c_parser_frontend::type_system::StructureTypeInterface::MemberIndex;
  MemberAccessOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kMemberAccess) {}
  MemberAccessOperatorNode(const MemberAccessOperatorNode&) = delete;

  MemberAccessOperatorNode& operator=(const MemberAccessOperatorNode&) = delete;

  virtual ConstTag GetResultConstTag() const override {
    return GetAccessedNodeReference().GetResultConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetAccessedNodeReference().GetResultTypePointer();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return GetAccessedNodePointer();
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  // 返回给定节点是否可以作为被访问成员的节点
  // 如果不可以则不会设置
  bool SetNodeToAccess(
      const std::shared_ptr<const OperatorNodeInterface>& node_to_access) {
    if (CheckNodeToAccessValid(*node_to_access)) [[likely]] {
      node_to_access_ = node_to_access;
      return true;
    } else {
      return false;
    }
  }
  std::shared_ptr<const OperatorNodeInterface> GetNodeToAccessPointer() {
    return node_to_access_;
  }
  const OperatorNodeInterface& GetNodeToAccessReference() const {
    return *node_to_access_;
  }
  // 返回给定节点是否是成员名
  // 如果不是则不会设置
  // 必须先设置节点，后设置要访问的成员名
  template <class MemberName>
  bool SetMemberName(MemberName&& member_name) {
    return SetAccessedNodeAndMemberName(
        std::string(std::forward<MemberName>(member_name)));
  }
  // 获取要访问的成员名
  MemberIndex GetMemberIndex() const { return member_index_; }
  // 获取要访问的成员类型
  // 必须成功设置要访问的节点和成员名
  std::shared_ptr<const OperatorNodeInterface> GetAccessedNodePointer() const {
    return node_accessed_;
  }
  const OperatorNodeInterface& GetAccessedNodeReference() const {
    return *node_accessed_;
  }

  // 检查给定节点是否可以作为被访问的节点
  static bool CheckNodeToAccessValid(
      const OperatorNodeInterface& node_to_access);

 private:
  // 根据待访问的节点和节点成员名推算并设置访问的节点成员类型和const属性
  // 如果成功访问则设置要访问的节点成员名
  // 如果给定节点不存在给定的成员名则返回false且不做任何更改
  bool SetAccessedNodeAndMemberName(std::string&& member_name_to_set);
  void SetAccessedNode(
      const std::shared_ptr<OperatorNodeInterface>& node_accessed) {
    node_accessed_ = node_accessed;
  }

  // 要访问的节点
  std::shared_ptr<const OperatorNodeInterface> node_to_access_;
  // 要访问的节点成员的index，枚举类型不设置该项
  MemberIndex member_index_;
  // 访问后得到的节点
  std::shared_ptr<OperatorNodeInterface> node_accessed_;
};

class FunctionCallOperatorNode : public OperatorNodeInterface {
  // 存储函数调用时参数的容器
  class FunctionCallArgumentsContainer {
   public:
    FunctionCallArgumentsContainer();
    ~FunctionCallArgumentsContainer();

    using ContainerType = std::list<
        std::pair<std::shared_ptr<const OperatorNodeInterface>,
                  std::shared_ptr<std::list<std::unique_ptr<
                      c_parser_frontend::flow_control::FlowInterface>>>>>;
    // 不检查参数有效性，检查过程在合并到主容器时进行
    void AddFunctionCallArgument(
        const std::shared_ptr<const OperatorNodeInterface>& argument,
        const std::shared_ptr<std::list<
            std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
            flow_control_node_container) {
      function_call_arguments_.emplace_back(
          std::make_pair(argument, flow_control_node_container));
    }
    void AddFunctionCallArgument(
        std::pair<const std::shared_ptr<const OperatorNodeInterface>,
                  const std::shared_ptr<std::list<std::unique_ptr<
                      c_parser_frontend::flow_control::FlowInterface>>>>&&
            argument_data) {
      function_call_arguments_.emplace_back(std::move(argument_data));
    }
    const ContainerType& GetFunctionCallArguments() const {
      return function_call_arguments_;
    }

   private:
    // 允许调用GetFunctionCallArgumentsNotConst()
    friend FunctionCallOperatorNode;

    ContainerType& GetFunctionCallArguments() {
      return function_call_arguments_;
    }

    ContainerType function_call_arguments_;
  };

 public:
  FunctionCallOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kFunctionCall) {}
  FunctionCallOperatorNode(const FunctionCallOperatorNode&) = delete;

  FunctionCallOperatorNode& operator=(const FunctionCallOperatorNode&) = delete;

  virtual ConstTag GetResultConstTag() const override {
    return ConstTag::kConst;
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetReturnObjectReference().GetResultTypePointer();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return GetReturnObjectPointer();
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  // 设置要调用的对象
  // 在设置参数前设置要调用的对象
  // 传入对象必须为StructOrBasicType::kFunction
  bool SetFunctionObjectToCall(
      const std::shared_ptr<const OperatorNodeInterface>&
          function_object_to_call);
  std::shared_ptr<const c_parser_frontend::type_system::FunctionType>
  GetFunctionTypePointer() const {
    return function_type_;
  }
  const c_parser_frontend::type_system::FunctionType& GetFunctionTypeReference()
      const {
    return *function_type_;
  }
  std::shared_ptr<const VarietyOperatorNode> GetReturnObjectPointer() const {
    return return_object_;
  }
  const VarietyOperatorNode& GetReturnObjectReference() const {
    return *return_object_;
  }
  // 获取调用者提供的原始函数参数，按书写顺序排列
  const auto& GetFunctionArgumentsOfferred() const {
    return function_arguments_offerred_;
  }
  // 添加一个参数，参数添加顺序从左到右
  // 函数参数为待添加的参数节点和获取参数节点的操作
  // 返回待添加的参数是否通过检验，未通过检验则不会添加
  // 在设置参数前设置要调用的对象
  AssignableCheckResult AddFunctionCallArgument(
      const std::shared_ptr<const OperatorNodeInterface>& argument_node,
      const std::shared_ptr<std::list<
          std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
          sentences_to_get_argument);
  // 添加一个容器内所有参数，参数添加顺序为begin()到end()
  // 返回是否成功添加（所有参数均符合要求）
  // 如果添加失败则不会修改参数
  // 在设置参数前设置要调用的对象
  bool SetArguments(FunctionCallArgumentsContainer&& container);

  static bool CheckFunctionTypeValid(const TypeInterface& type_interface) {
    return type_interface.GetType() == StructOrBasicType::kFunction;
  }

 private:
  FunctionCallArgumentsContainer& GetFunctionArgumentsOfferred() {
    return function_arguments_offerred_;
  }

  // 函数返回的对象
  std::shared_ptr<const VarietyOperatorNode> return_object_;
  // 被调用的对象
  std::shared_ptr<const c_parser_frontend::type_system::FunctionType>
      function_type_;
  // 调用者提供的原始函数参数与获取参数的操作，按书写顺序排列
  FunctionCallArgumentsContainer function_arguments_offerred_;
};

}  // namespace c_parser_frontend::operator_node

#endif  // !CPARSERFRONTEND_OPERATOR_NODE_H_