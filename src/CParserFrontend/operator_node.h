#ifndef CPARSERFRONTEND_OPERATOR_NODE_H_
#define CPARSERFRONTEND_OPERATOR_NODE_H_
#include "Common/id_wrapper.h"
#include "type_system.h"
// 操作节点
namespace c_parser_frontend::operator_node {
// 运算节点大类
enum class GeneralOperationType {
  kAllocate,               // 分配空间
  kVariety,                // 基础变量
  kInitValue,              // 初始化用值
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
  kPlus,        // +（加）
  kMinus,       // -（减）
  kMultiple,    // *（乘）
  kDivide,      // /（除）
  kMod,         // %（求模）
  kLeftShift,   // <<
  kRightShift,  // >>
  kAnd,         // &
  kOr,          // |
  kNot,         // !
  kXor,         // ^
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

class OperatorNodeInterface {
  enum class IdWrapper { kOperatorNodeId };

 public:
  // 操作节点ID
  using OperatorId =
      frontend::common::ExplicitIdWrapper<size_t, IdWrapper,
                                          IdWrapper::kOperatorNodeId>;
  OperatorNodeInterface() : operator_id_(GetOperatorId()) {}
  OperatorNodeInterface(GeneralOperationType general_operator_type)
      : operator_id_(GetNewOperatorId()),
        general_operator_type_(general_operator_type) {}
  virtual ~OperatorNodeInterface();

  // 获取该节点结果的ConstTag
  virtual ConstTag GetResultConstTag() const = 0;
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer() const = 0;

  void SetOperatorId(OperatorId operator_id) { operator_id_ = operator_id; }
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
    static OperatorId operator_id(0);
    return operator_id++;
  }

  // 操作节点ID，同时作为存放结果的LLVM的寄存器编号
  OperatorId operator_id_;
  // 较模糊的运算符类型
  GeneralOperationType general_operator_type_;
};

class AllocateOperatorNode : public OperatorNodeInterface {
 public:
  AllocateOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kAllocate) {}

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

  // 设置要分配内存的节点
  // 返回是否可以分配，不能分配则不会设置
  bool SetTargetVariety(
      std::shared_ptr<const OperatorNodeInterface>&& target_variety);
  // 函数参数为要添加的维数的大小
  // 在设置维数大小前必须设置要分配内存的节点
  // 添加顺序同声明顺序，先添加里层后添加外层
  // 如果数组类型不支持多添加一维则返回false
  // 非数组不要设置，此时num_to_allocate_.size() == 0
  bool AddNumToAllocate(size_t num);
  const std::list<size_t>& GetNumToAllocate() const { return num_to_allocate_; }
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
  // 待分配空间的大小，优先压入靠近变量名一端的大小
  std::list<size_t> num_to_allocate_;
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

  virtual ConstTag GetResultConstTag() const override { return GetConstTag(); }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return variety_type_;
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
  bool SetVarietyType(std::shared_ptr<const TypeInterface>&& variety_type);

  // 检查给定类型是否可以作为变量的类型
  static bool CheckVarietyTypeValid(const TypeInterface& variety_type);

 private:
  // 变量名
  const std::string* variety_name_;
  // 变量类型
  std::shared_ptr<const TypeInterface> variety_type_;
  // 变量本身的const标记
  ConstTag variety_const_tag_;
  // 变量的左右值标记
  LeftRightValueTag variety_left_right_value_tag_;
};

// 存储初始化数据（编译期常量）
class InitializeOperatorNode : public OperatorNodeInterface {
 public:
  InitializeOperatorNode(InitializeType initialize_type)
      : OperatorNodeInterface(GeneralOperationType::kInitValue),
        initialize_type_(initialize_type) {}

  virtual ConstTag GetResultConstTag() const override {
    return ConstTag::kConst;
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return initialize_value_type_;
  }

  void SetInitlizeType(InitializeType initialize_type) {
    initialize_type_ = initialize_type;
  }
  InitializeType GetInitializeType() const { return initialize_type_; }
  // 设置该节点的类型
  // 返回是否为有效的初始化数据类型，如果无效则不添加
  bool SetInitValueType(std::shared_ptr<const TypeInterface>&& init_value_type);
  // 返回是否为有效的初始化数据类型
  static bool CheckInitValueTypeValid(const TypeInterface& init_value_type);

 private:
  // 初始化数据类型
  std::shared_ptr<const TypeInterface> initialize_value_type_;
  // 初始化数据类型
  InitializeType initialize_type_;
};

// 基础类型（数值/字符串）
class BasicTypeInitializeOperatorNode : public InitializeOperatorNode {
 public:
  template <class Value>
  BasicTypeInitializeOperatorNode(InitializeType initialize_type, Value&& value)
      : InitializeOperatorNode(initialize_type),
        value_(std::forward<Value>(value)) {
    assert(initialize_type == InitializeType::kBasic ||
           initialize_type == InitializeType::kString);
  }

  virtual ConstTag GetResultConstTag() const override {
    return InitializeOperatorNode::GetResultConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return InitializeOperatorNode::GetResultTypePointer();
  }

  template <class Value>
  void SetValue(Value&& value) {
    value_ = std::forward<Value>(value);
  }
  const std::string& GetValue() const { return value_; }
  // 设置初始化用数据类型（基础类型/const char*）
  bool SetInitDataType(std::shared_ptr<const TypeInterface>&& data_type);
  // 该对象是否可以使用给定的类型
  // 有效类型返回true，无效类型返回false
  static bool CheckBasicTypeInitializeValid(const TypeInterface& variety_type);

 private:
  // 遮盖函数，防止误用
  bool SetInitValueType(std::shared_ptr<const TypeInterface>&& init_value_type);
  // 字符串形式存储以防精度损失
  std::string value_;
};

// 初始化列表
class ListInitializeOperatorNode : public InitializeOperatorNode {
  ListInitializeOperatorNode()
      : InitializeOperatorNode(InitializeType::kInitializeList) {}

  virtual ConstTag GetResultConstTag() const override {
    return InitializeOperatorNode::GetResultConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return InitializeOperatorNode::GetResultTypePointer();
  }

  // 向初始化列表中添加初始化值
  // 返回值是否为有效的初始化列表可用类型，如果类型无效则不执行插入操作
  // 值从左到右顺序添加
  bool AddListValue(std::shared_ptr<const InitializeOperatorNode>&& list_value);
  const std::vector<std::shared_ptr<const InitializeOperatorNode>>&
  GetListValues() const {
    return list_values_;
  }
  bool SetInitListType(std::shared_ptr<const TypeInterface>&& list_type);
  // 检查给定的初始化列表类型是否合法
  static bool CheckInitListTypeValid(const TypeInterface& list_type) {
    return list_type.GetType() == StructOrBasicType::kInitializeList;
  }
  // 检查给定的初始化值是否合法
  static bool CheckInitListValueTypeValid(
      const TypeInterface& list_value_type) {
    return InitializeOperatorNode::CheckInitValueTypeValid(list_value_type);
  }

 private:
  // 初始化列表中的值
  std::vector<std::shared_ptr<const InitializeOperatorNode>> list_values_;
};

// 赋值节点
class AssignOperatorNode : public OperatorNodeInterface {
 public:
  AssignOperatorNode() : OperatorNodeInterface(GeneralOperationType::kAssign) {}

  virtual ConstTag GetResultConstTag() const override {
    return GetNodeToBeAssignedReference().GetResultConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    // 赋值语句等价于被赋值的变量
    return GetNodeToBeAssignedReference().GetResultTypePointer();
  }

  void SetNodeToBeAssigned(
      std::shared_ptr<const OperatorNodeInterface>&& node_to_be_assigned) {
    node_to_be_assigned_ = std::move(node_to_be_assigned);
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
  // 声明时赋值会忽略被赋值的节点自身的const属性
  // 需要先设置被赋值的节点
  AssignableCheckResult SetNodeForAssign(
      std::shared_ptr<const OperatorNodeInterface>&& node_for_assign,
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
  // 当is_announce == true时忽略node_to_be_assigned的const标记
  static AssignableCheckResult CheckAssignable(
      const OperatorNodeInterface& node_to_be_assigned,
      const OperatorNodeInterface& node_for_assign, bool is_announce);

 private:
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

  virtual ConstTag GetResultConstTag() const override {
    return ConstTag::kConst;
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetComputeResultTypeReference().GetResultTypePointer();
  }

  void SetMathematicalOperation(MathematicalOperation mathematical_operation) {
    mathematical_operation_ = mathematical_operation;
  }
  MathematicalOperation GetMathematicalOperation() const {
    return mathematical_operation_;
  }
  // 先设置运算类型后设置节点
  bool SetLeftOperatorNode(
      std::shared_ptr<const OperatorNodeInterface>&& left_operator_node);
  std::shared_ptr<const OperatorNodeInterface> GetLeftOperatorNodePointer()
      const {
    return left_operator_node_;
  }
  const OperatorNodeInterface& GetLeftOperatorNodeReference() const {
    return *left_operator_node_;
  }
  // 先设置左节点后设置右节点
  DeclineMathematicalComputeTypeResult SetRightOperatorNode(
      std::shared_ptr<const OperatorNodeInterface>&& right_operator_node);
  std::shared_ptr<const OperatorNodeInterface> GetRightOperatorNodePointer()
      const {
    return right_operator_node_;
  }
  const OperatorNodeInterface& GetRightOperatorNodeReference() const {
    return *right_operator_node_;
  }
  std::shared_ptr<const VarietyOperatorNode> GetComputeResultTypePointer()
      const {
    return compute_result_node_;
  }
  const VarietyOperatorNode& GetComputeResultTypeReference() const {
    return *compute_result_node_;
  }

  // 推算运算后结果，返回运算后结果和推算的具体情况
  // 如果不能运算则返回nullptr
  static std::pair<std::shared_ptr<const VarietyOperatorNode>,
                   DeclineMathematicalComputeTypeResult>
  DeclineComputeResult(
      MathematicalOperation mathematical_operation,
      std::shared_ptr<const OperatorNodeInterface>&& left_operator_node,
      std::shared_ptr<const OperatorNodeInterface>&& right_operator_node);

 private:
  void SetComputeResultNode(
      std::shared_ptr<const VarietyOperatorNode>&& compute_result_node) {
    compute_result_node_ = std::move(compute_result_node);
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

  virtual ConstTag GetResultConstTag() const override {
    return ConstTag::kConst;
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetComputeResultNodeReference().GetResultTypePointer();
  }

  LogicalOperation GetLogicalOperation() const { return logical_operation_; }
  bool SetLeftOperatorNode(
      std::shared_ptr<const OperatorNodeInterface>&& left_operator_node);
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
      std::shared_ptr<const OperatorNodeInterface>&& right_operator_node);
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
        CommonlyUsedTypeGenerator::GetBasicType<BuiltInType::kBool,
                                                SignTag::kUnsigned>());
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
  DereferenceOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kDeReference) {}

  virtual ConstTag GetResultConstTag() const override {
    return GetDereferencedObjectConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetDereferencedNodeReference().GetResultTypePointer();
  }

  // 设定待解引用的节点
  // 函数参数为待解引用的节点
  // 函数返回是否成功设置
  bool SetNodeToDereference(
      std::shared_ptr<const OperatorNodeInterface>&& node_to_dereference);

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
      const OperatorNodeInterface& node_to_dereference) {
    // 当且仅当为指针时可以解引用
    return node_to_dereference.GetResultTypePointer()->GetType() ==
           StructOrBasicType::kPointer;
  }

 private:
  void SetDereferencedNode(
      std::shared_ptr<const VarietyOperatorNode>&& dereferenced_node) {
    dereferenced_node_ = std::move(dereferenced_node);
  }

  // 将要被解引用的节点
  std::shared_ptr<const OperatorNodeInterface> node_to_dereference_;
  // 解引用后得到的对象的节点
  std::shared_ptr<const VarietyOperatorNode> dereferenced_node_;
};

class ObtainAddress : public OperatorNodeInterface {
  ObtainAddress()
      : OperatorNodeInterface(GeneralOperationType::kObtainAddress) {}

  virtual ConstTag GetResultConstTag() const override {
    // 取地址获得的值都是非const
    return ConstTag::kNonConst;
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetObtainedAddressNodeReference().GetResultTypePointer();
  }

  // 返回是否可以设置，如果不可设置则不会设置
  bool SetNodeToObtainAddress(
      std::shared_ptr<const OperatorNodeInterface>&& node_to_obtain_address);

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
      std::shared_ptr<const OperatorNodeInterface>&& node_obtained_address) {
    obtained_address_node_ = std::move(node_obtained_address);
  }

  // 将要被取地址的节点
  std::shared_ptr<const OperatorNodeInterface> node_to_obtain_address_;
  // 取地址后获得的节点
  std::shared_ptr<const OperatorNodeInterface> obtained_address_node_;
};

class MemberAccessOperatorNode : public OperatorNodeInterface {
  MemberAccessOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kMemberAccess) {}

  virtual ConstTag GetResultConstTag() const override {
    return GetAccessedNodeReference().GetResultConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetAccessedNodeReference().GetResultTypePointer();
  }

  // 返回给定节点是否可以作为被访问成员的节点
  // 如果不可以则不会设置
  bool SetNodeToAccess(
      std::shared_ptr<const OperatorNodeInterface>&& node_to_access) {
    if (CheckNodeToAccessValid(*node_to_access)) [[likely]] {
      node_to_access_ = std::move(node_to_access);
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
  const std::string& GetMemberName() const { return member_name_; }
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
      std::shared_ptr<const OperatorNodeInterface>&& node_accessed) {
    node_accessed_ = std::move(node_accessed);
  }

  // 要访问的节点
  std::shared_ptr<const OperatorNodeInterface> node_to_access_;
  // 要访问的节点成员名
  std::string member_name_;
  // 访问后得到的节点
  std::shared_ptr<const OperatorNodeInterface> node_accessed_;
};

class FunctionCallOperatorNode : public OperatorNodeInterface {
  FunctionCallOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kFunctionCall) {}

  virtual ConstTag GetResultConstTag() const override {
    return ConstTag::kConst;
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    return GetReturnObjectReference().GetResultTypePointer();
  }

  bool SetFunctionType(std::shared_ptr<const TypeInterface>&& type_pointer);
  std::shared_ptr<const TypeInterface> GetFunctionTypePointer() const {
    return function_type_;
  }
  const TypeInterface& GetFunctionTypeReference() const {
    return *function_type_;
  }
  std::shared_ptr<const VarietyOperatorNode> GetReturnObjectPointer() const {
    return return_object_;
  }
  const VarietyOperatorNode& GetReturnObjectReference() const {
    return *return_object_;
  }
  // 获取最终提供给函数的参数（类型与函数类型所定义的完全匹配），按书写顺序排列
  const auto& GetFunctionArgumentsForCall() const {
    return function_arguments_for_call_;
  }
  // 获取调用者提供的原始函数参数，按书写顺序排列
  const auto& GetFunctionArgumentsOfferred() const {
    return function_arguments_offerred_;
  }
  // 添加一个参数，参数添加顺序从左到右
  // 返回待添加的参数是否通过检验，未通过检验则不会添加
  AssignableCheckResult AddArgument(
      std::shared_ptr<const OperatorNodeInterface>&& argument_node);
  static bool CheckFunctionTypeValid(const TypeInterface& type_interface) {
    return type_interface.GetType() == StructOrBasicType::kFunction;
  }

 private:
  std::vector<std::shared_ptr<const OperatorNodeInterface>>&
  GetFunctionArgumentsForCall() {
    return function_arguments_for_call_;
  }
  std::vector<std::shared_ptr<const OperatorNodeInterface>>&
  GetFunctionArgumentsOfferred() {
    return function_arguments_offerred_;
  }

  // 函数返回的对象
  std::shared_ptr<const VarietyOperatorNode> return_object_;
  // 该函数的类型
  std::shared_ptr<const c_parser_frontend::type_system::FunctionType>
      function_type_;
  // 调用者提供的原始函数参数，按书写顺序排列
  std::vector<std::shared_ptr<const OperatorNodeInterface>>
      function_arguments_offerred_;
  // 最终提供给函数的参数（类型与函数类型所定义的完全匹配），按书写顺序排列
  std::vector<std::shared_ptr<const OperatorNodeInterface>>
      function_arguments_for_call_;
};

}  // namespace c_parser_frontend::operator_node

#endif  // !CPARSERFRONTEND_OPERATOR_NODE_H_