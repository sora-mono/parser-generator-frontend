/// @file operator_node.h
/// @brief 操作节点
/// @details
/// 本文件内定义表示基础操作的节点，支持的详细操作参考GeneralOperationType
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
/// @brief 操作类型大类
enum class GeneralOperationType {
  kAllocate,               ///< 分配空间
  kTypeConvert,            ///< 类型转换
  kVariety,                ///< 基础变量
  kInitValue,              ///< 初始化用值
  kTemaryOperator,         ///< ?:（三目运算符）
  kAssign,                 ///< =（赋值）
  kMathematicalOperation,  ///< 数学运算
  kLogicalOperation,       ///< 逻辑运算
  kDeReference,            ///< 解引用
  kObtainAddress,          ///< 取地址
  kMemberAccess,           ///< 成员访问
  kFunctionCall            ///< 函数调用
};
/// @brief 数学运算符
enum class MathematicalOperation {
  kOr,                    ///< |（按位或）
  kXor,                   ///< ^（按位异或）
  kAnd,                   ///< &（按位与）
  kLeftShift,             ///< <<（左移）
  kRightShift,            ///< >>（右移）
  kPlus,                  ///< +（加）
  kMinus,                 ///< -（减）
  kMultiple,              ///< *（乘）
  kDivide,                ///< /（除）
  kMod,                   ///< %（求模）
  kLogicalNegative,       ///< ~（按位取反）
  kMathematicalNegative,  ///< -（取负）
  kNot,                   ///< !（逻辑非）
};
/// @brief 数学与赋值运算符
enum class MathematicalAndAssignOperation {
  kOrAssign,          ///< |=
  kXorAssign,         ///< ^=
  kAndAssign,         ///< &=
  kLeftShiftAssign,   ///< <<=
  kRightShiftAssign,  ///< >>=
  kPlusAssign,        ///< +=
  kMinusAssign,       ///< -=
  kMultipleAssign,    ///< *=
  kDivideAssign,      ///< /=
  kModAssign,         ///< %=
};
/// @brief 逻辑运算符
enum class LogicalOperation {
  kAndAnd,        ///< &&
  kOrOr,          ///< ||
  kGreater,       ///< >
  kGreaterEqual,  ///< >=
  kLess,          ///< <
  kLessEqual,     ///< <=
  kEqual,         ///< ==
  kNotEqual       ///< !=
};
/// @brief 初始化值类型
enum class InitializeType {
  kBasic,     ///< 基础类型，具体类型取决于值的范围
  kString,    ///< 字符串，固定为const char*类型
  kFunction,  ///< 用于给函数指针赋值的全局初始化常量类型
  kInitializeList  ///< 初始化列表
};
/// @brief 左值右值标记
enum class LeftRightValueTag {
  kLeftValue,  ///< 左值
  kRightValue  ///< 右值
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

/// @brief 将数学与赋值运算符转化为数学运算符
/// @param[in] mathematical_and_assign_operation ：数学与赋值运算符
/// @return 返回对应的数学运算符
MathematicalOperation MathematicalAndAssignOperationToMathematicalOperation(
    MathematicalAndAssignOperation mathematical_and_assign_operation);

/// @class OperatorNodeInterface operator_node.h
/// @brief 操作节点基类
/// @note 所有操作节点均从该类派生
class OperatorNodeInterface {
  /// @brief 操作节点ID的分发标签
  enum class IdWrapper { kOperatorNodeId };

 public:
  /// @brief 操作节点ID
  using OperatorId =
      frontend::common::ExplicitIdWrapper<size_t, IdWrapper,
                                          IdWrapper::kOperatorNodeId>;

  OperatorNodeInterface(GeneralOperationType general_operator_type)
      : general_operator_type_(general_operator_type) {}
  /// @attention 复制构造函数不复制节点编号而是生成新的编号
  OperatorNodeInterface(const OperatorNodeInterface& operator_node)
      : general_operator_type_(operator_node.general_operator_type_) {}
  virtual ~OperatorNodeInterface() {}

  OperatorNodeInterface& operator=(const OperatorNodeInterface& old_interface) {
    general_operator_type_ = old_interface.general_operator_type_;
    return *this;
  }

  /// @brief 获取该节点操作结果的const标记
  /// @return 返回操作结果的const标记
  /// @details
  /// 1.成员访问、解引用等操作的后续操作不是对原结构体/原指针操作，而是对得到的
  ///   新对象进行操作，不同操作类型操作的对象不同
  /// 2.通过这个函数抽象获取最终结果的过程以忽略操作的不同
  /// 3.以成员访问为例，该函数获取指定的成员在结构体中的const标记，以数学运算
  ///   为例，该函数获取运算结果的const标记（ConstTag::kNonConst），以变量声明
  ///   为例，该函数返回声明的变量的const标记（对象不变）
  virtual ConstTag GetResultConstTag() const = 0;
  /// @brief 获取最终结果的类型
  /// @return 返回类型链头结点的const指针
  /// @details
  /// 设计思路同GetResultConstTag
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer() const = 0;
  /// @brief 获取最终结果的节点
  /// @return 返回指向最终结果的指针
  /// @retval nullptr ：自身是最终节点
  /// @details
  /// 1.该函数用于抽象获取操作最终结果的过程
  /// 2.基础变量和初始化变量最终操作节点就是本身，所以返回nullptr
  /// 3.赋值运算返回被赋值的对象，解引用返回解引用后得到的对象，以此为例
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const = 0;
  /// @brief 拷贝自身并设置拷贝得到的对象类型为new_type
  /// @param[in] new_type ：拷贝后得到的对象的类型
  /// @param[in] new_const_tag ：拷贝后得到的对象的新const标记
  /// @return 返回指向拷贝后的对象的指针
  /// @retval nullptr ：设置初始化数据为非const
  /// @details
  /// 该函数用于类型转换
  /// @attention 仅允许对变量、初始化值和类型转换节点调用该函数
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const = 0;

  /// @brief 获取操作节点ID
  /// @return 返回操作节点ID
  OperatorId GetOperatorId() const { return operator_id_; }
  /// @brief 设置操作类型
  /// @param[in] operator_type ：操作类型
  void SetGeneralOperatorType(GeneralOperationType operator_type) {
    general_operator_type_ = operator_type;
  }
  /// @brief 获取操作类型所属大类
  /// @return 返回操作类型所属大类
  GeneralOperationType GetGeneralOperatorType() const {
    return general_operator_type_;
  }

 private:
  /// @brief 获取一个从未被分配过的OperatorId
  /// @return 返回OperatorId
  /// @note 线程间OperatorId独立
  static OperatorId GetNewOperatorId() {
    static thread_local OperatorId operator_id(0);
    return operator_id++;
  }

  /// @brief 操作节点ID，同时作为存放结果的LLVM的寄存器编号
  const OperatorId operator_id_ = GetOperatorId();
  /// @brief 操作所属大类
  GeneralOperationType general_operator_type_;
};

/// @class AllocateOperatorNode operator_node.h
/// @brief 分配栈上空间
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
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 设置要分配内存的节点
  /// @param[in] target_variety ：待分配内存的节点
  /// @return 返回是否可以分配
  /// @retval true ：设置成功
  /// @retval false ：不能分配该变量
  /// @note 不能分配则不会设置
  /// @attention 只有GeneralOperatorType::kVariety类型节点可以作为参数
  bool SetTargetVariety(
      const std::shared_ptr<const OperatorNodeInterface>& target_variety);
  /// @brief 获取要分配内存的变量节点
  /// @return 返回指向变量节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetTargetVarietyPointer() const {
    return target_variety_;
  }
  /// @brief 获取要分配内存的变量节点
  /// @return 返回指向变量节点的const引用
  const OperatorNodeInterface& GetTargetVarietyReference() const {
    return *target_variety_;
  }

  /// @brief 检查操作节点是否可以分配栈上空间
  /// @return 返回是否可以分配
  /// @retval true ：可以分配
  /// @retval false ：不可以分配
  /// @attention 仅允许传入GeneralOperatorType::kVariety类型节点
  static bool CheckAllocatable(const OperatorNodeInterface& node_to_allocate) {
    assert(node_to_allocate.GetGeneralOperatorType() ==
           GeneralOperationType::kVariety);
    return true;
  }

 private:
  /// @brief 待分配栈上空间的节点
  std::shared_ptr<const OperatorNodeInterface> target_variety_;
};

/// @class TypeConvert operator_node.h
/// @brief 类型转换
/// @note 源节点需要支持GetResultOperatorNode()
class TypeConvert : public OperatorNodeInterface {
 public:
  /// @note 允许使用任意有运算结果的节点作为source_node
  /// source_node必须支持SelfCopy
  TypeConvert(const std::shared_ptr<const OperatorNodeInterface>& source_node,
              const std::shared_ptr<const TypeInterface>& new_type,
              ConstTag new_const_tag)
      : OperatorNodeInterface(GeneralOperationType::kTypeConvert),
        source_node_(source_node) {
    GenerateDestinationNode(new_type, new_const_tag);
  }
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
  /// @attention 不能拷贝类型转换节点，应该拷贝destination_node_
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 获取被转换的节点
  /// @return 返回指向被转换节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetSourceNodePointer() const {
    return source_node_;
  }
  /// @brief 获取被转换的节点
  /// @return 返回被转换节点的const引用
  const OperatorNodeInterface& GetSourceNodeReference() const {
    return *source_node_;
  }
  /// @brief 获取转换后的节点
  /// @return 返回指向被转换后节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetDestinationNodePointer()
      const {
    return destination_node_;
  }
  /// @brief 获取转换后的节点
  /// @return 返回转换后节点的const引用
  const OperatorNodeInterface& GetDestinationNodeReference() const {
    return *destination_node_;
  }
  /// @brief 设置待转换的节点
  /// @param[in] source_node ：待转换的节点
  /// @note 设置新的待转换节点后要调用GenerateDestinationNode以生成转换后的节点
  void SetSourceNode(
      const std::shared_ptr<const OperatorNodeInterface>& source_node) {
    source_node_ = source_node;
  }
  /// @brief 生成转换后的节点
  /// @param[in] new_type ：新节点的类型
  /// @param[in] new_const_tag ：新节点的const标记
  /// @return 返回是否可以转换
  /// @retval true ：可以转换
  /// @retval false ：转换初始化数据为非const
  /// @note 不能转换则不会生成目标节点
  bool GenerateDestinationNode(
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag);

 private:
  /// @brief 待转换的节点
  std::shared_ptr<const OperatorNodeInterface> source_node_;
  /// @brief 转换后的节点
  std::shared_ptr<OperatorNodeInterface> destination_node_;
};

/// @class VarietyOperatorNode operator_node.h
/// @brief 表示变量
class VarietyOperatorNode : public OperatorNodeInterface {
 public:
  VarietyOperatorNode(const std::string& variety_name, ConstTag const_tag,
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
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override;

  /// @brief 设置变量名
  /// @param[in] variety_name ：变量名
  void SetVarietyName(const std::string& variety_name) {
    variety_name_ = variety_name;
  }
  /// @brief 获取变量名
  /// @return 返回变量名的const引用
  const std::string& GetVarietyName() const { return variety_name_; }
  /// @brief 设置变量的const标记
  /// @param[in] const_tag ：const标记
  void SetConstTag(ConstTag const_tag) { variety_const_tag_ = const_tag; }
  /// @brief 获取变量的const标记
  /// @return 返回变量的const标记
  /// @retval ConstTag::kConst ：变量不允许修改
  /// @retval ConstTag::kNonConst ：变量允许修改
  ConstTag GetConstTag() const { return variety_const_tag_; }
  /// @brief 设置变量左右值标记
  /// @param[in] left_right_value_tag ：左右值标记
  void SetLeftRightValueTag(LeftRightValueTag left_right_value_tag) {
    variety_left_right_value_tag_ = left_right_value_tag;
  }
  /// @brief 获取变量的左右值标记
  /// @return 返回左右值标记
  /// @retval LeftRightValueTag::kLeftValue ：左值
  /// @retval LeftRightValueTag::kRightValue ：右值
  LeftRightValueTag GetLeftRightValueTag() const {
    return variety_left_right_value_tag_;
  }
  /// @brief 设置变量的类型
  /// @param[in] variety_type ：类型链头结点const指针
  /// @return 返回是否可以设置给定类型
  /// @retval true ：可以设置给定类型
  /// @retval false ：不可以设置给定类型
  /// @note 变量允许使用的类型参考CheckVarietyTypeValid
  bool SetVarietyType(const std::shared_ptr<const TypeInterface>& variety_type);
  /// @brief 设置变量的类型，允许使用函数类型
  /// @param[in] variety_type ：类型链头结点const指针
  /// @return 返回是否可以设置给定类型
  /// @retval true ：可以设置给定类型
  /// @retval false ：不可以设置给定类型
  /// @details
  /// 1.如果不可以设置则不会设置
  /// 2.相比于SetVarietyType，允许使用FunctionType作为变量类型
  /// 3.该函数仅用于初始化列表赋值时构建参与检查的VarietyType和解引用
  bool SetVarietyTypeNoCheckFunctionType(
      const std::shared_ptr<const TypeInterface>& variety_type);
  /// @brief 获取变量类型链
  /// @return 返回指向变量类型链头结点的const指针
  std::shared_ptr<const TypeInterface> GetVarietyTypePointer() const {
    return variety_type_;
  }
  /// @brief 获取变量类型链
  /// @return 返回变量类型链头结点的const引用
  const TypeInterface& GetVarietyTypeReference() const {
    return *variety_type_;
  }

  /// @brief 检查给定类型是否可以作为普通变量的类型
  /// @param[in] variety_type ：待检查的类型
  /// @return 返回给定类型是否可以作为普通变量类型
  /// @retval true ：给定类型可以作为普通变量类型
  /// @retval false ：给定类型是StructOrBasicType::kFunction或
  /// StructOrBasicType::kInitializeList
  /// @details
  /// 变量仅允许使用
  /// StructOrBasicType::kBasic
  /// StructOrBasicType::kPointer
  /// StructOrBasicType::kStruct
  /// StructOrBasicType::kUnion
  /// StructOrBasicType::kEnum
  static bool CheckVarietyTypeValid(const TypeInterface& variety_type);

 private:
  /// @brief 变量名
  std::string variety_name_;
  /// @brief 变量类型
  std::shared_ptr<const TypeInterface> variety_type_;
  /// @brief 变量本身的const标记
  ConstTag variety_const_tag_;
  /// @brief 变量的左右值标记
  LeftRightValueTag variety_left_right_value_tag_;
};

/// @class InitializeOperatorNodeInterface operator_node.h
/// @brief 所有初始化数据（编译期常量）的基类
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

  /// @brief 设置初始化数据类型
  /// @param[in] initialize_type ：初始化数据类型
  void SetInitializeType(InitializeType initialize_type) {
    initialize_type_ = initialize_type;
  }
  /// @brief 获取初始化数据所属分类
  /// @return 返回初始化数据所属分类
  /// @note 分流含义见InitializeType定义
  InitializeType GetInitializeType() const { return initialize_type_; }
  /// @brief 设置初始化数据的具体类型
  /// @param[in] init_value_type ：初始化数据的类型链头结点const指针
  /// @return 返回是否允许设置
  /// @retval true ：允许设置
  /// @retval false ：不允许设置，不是有效的初始化数据类型
  /// @details
  /// 合法的初始化类型参考CheckInitValueTypeValid
  /// @note 如果不允许设置则不会设置
  bool SetInitValueType(
      const std::shared_ptr<const TypeInterface>& init_value_type);
  /// @brief 获取初始化数据的类型链
  /// @return 返回指向类型链头结点的const指针
  const std::shared_ptr<const TypeInterface>& GetInitValueType() const {
    return initialize_value_type_;
  }
  /// @brief 检查给定类型是否为有效的初始化数据类型
  /// @param[in] init_value_type ：类型链头结点的const引用
  /// @return 返回是否为有效的初始化数据类型
  /// @retval true ：有效的初始化数据类型
  /// @retval false ：无效的初始化数据类型
  static bool CheckInitValueTypeValid(const TypeInterface& init_value_type);

 private:
  /// @brief 初始化数据类型链
  std::shared_ptr<const TypeInterface> initialize_value_type_;
  /// @brief 初始化数据所属分类
  InitializeType initialize_type_;
};

/// @class BasicTypeInitializeOperatorNode operator_node.h
/// @brief 基础初始化类型（数值/字符串/函数）
/// @note 保存函数时value留空
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
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override;

  /// @brief 设置初始化数据值
  /// @param[in] value ：字符串形式表示的初始化数据
  /// @note 使用字符串形式表示以防止精度损失等问题
  template <class Value>
  void SetValue(Value&& value) {
    value_ = std::forward<Value>(value);
  }
  /// @brief 获取初始化数据的值
  /// @return 返回初始化数据字符串的const引用
  const std::string& GetValue() const { return value_; }
  /// @brief 设置初始化用数据的类型
  /// @param[in] data_type ：初始化数据类型链头结点指针
  /// @note 允许的类型参考CheckBasicTypeInitializeValid
  bool SetInitDataType(const std::shared_ptr<const TypeInterface>& data_type);
  /// @brief 检查给定类型是否可以作为基础初始化数据的类型
  /// @param[in] variety_type ：待检查的类型
  /// @return 返回给定类型是否可以作为基础初始化数据的类型
  /// @retval true ：有效的基础初始化数据类型
  /// @retval false ：无效的基础初始化数据类型
  /// @note 允许使用基础类型/const char*/函数类型
  static bool CheckBasicTypeInitializeValid(const TypeInterface& variety_type);

 private:
  /// @note
  /// 1.外部访问时不应使用该函数设置类型，使用SetInitDataType代替
  /// 2.调用该函数设置初始化数据类型会导致跳过CheckBasicTypeInitializeValid检查
  /// 3.放在private供内部函数调用
  bool SetInitValueType(
      const std::shared_ptr<const TypeInterface>& init_value_type) {
    return InitializeOperatorNodeInterface::SetInitValueType(init_value_type);
  }

  /// @brief 初始化值
  /// @note 使用字符串形式避免精度损失
  /// 函数类型初始化数据该项维持应默认值
  std::string value_;
};

/// @class ListInitializeOperatorNode operator_node.h
/// @brief 初始化列表
class ListInitializeOperatorNode : public InitializeOperatorNodeInterface {
 public:
  ListInitializeOperatorNode()
      : InitializeOperatorNodeInterface(InitializeType::kInitializeList) {}
  ListInitializeOperatorNode(const ListInitializeOperatorNode&) = default;

  ListInitializeOperatorNode& operator=(const ListInitializeOperatorNode&) =
      default;

  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    /// 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 设置初始化列表中含有的全部初始化值
  /// @param[in] list_values ：初始化列表中全部初始化值
  /// @return 返回是否所有值均为合法的初始化值
  /// @retval true ：全部值均为合法的初始化值
  /// @retval false ：存在不是合法初始化值类型的对象
  /// @note list_values中初始化值的顺序为书写顺序
  bool SetListValues(
      std::list<std::shared_ptr<const InitializeOperatorNodeInterface>>&&
          list_values);
  /// @brief 获取初始化列表中所有的初始化值
  /// @return 返回存储初始化列表中初始化值的容器的const引用
  const std::list<std::shared_ptr<const InitializeOperatorNodeInterface>>&
  GetListValues() const {
    return list_values_;
  }

 private:
  /// @brief 初始化列表中所有的初始化值
  /// @note 允许初始化列表嵌套
  std::list<std::shared_ptr<const InitializeOperatorNodeInterface>>
      list_values_;
};

/// @class TemaryOperatorNode operator_node.h
/// @brief 三目运算符
/// @details
/// 支持constexpr三目运算符语义
class TemaryOperatorNode : public OperatorNodeInterface {
 public:
  TemaryOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kTemaryOperator) {}

  virtual ConstTag GetResultConstTag() const override {
    return GetResultReference().GetResultConstTag();
  }
  virtual std::shared_ptr<const TypeInterface> GetResultTypePointer()
      const override {
    /// 赋值语句等价于被赋值的变量
    return GetResultReference().GetResultTypePointer();
  }
  virtual std::shared_ptr<const OperatorNodeInterface> GetResultOperatorNode()
      const override {
    return GetResultReference().GetResultOperatorNode();
  }
  virtual std::shared_ptr<OperatorNodeInterface> SelfCopy(
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    /// 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 设置分支条件
  /// @param[in] branch_condition ：分支条件
  /// @param[in] flow_control_node_container ：获取分支条件的所有操作
  /// @return 返回是否可以设置
  /// @retval true ：可以设置
  /// @retval false ：branch_condition无法作为分支条件
  /// @details
  /// 1.分支条件的要求参考CheckBranchConditionValid
  /// 2.设置分支条件后如果需要改动则需要重新设置一遍真假分支，此时仍可以使用Get
  /// 函数获取之前存储的分支信息
  /// @note 编译期常量的分支条件必须设置value为"0"或"1"
  /// @attention 先设置分支条件后设置分支内容
  bool SetBranchCondition(
      const std::shared_ptr<const OperatorNodeInterface>& branch_condition,
      const std::shared_ptr<const std::list<
          std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
          flow_control_node_container);
  /// @brief 获取分支条件
  /// @return 返回指向分支条件节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetBranchConditionPointer()
      const {
    return branch_condition_;
  }
  /// @brief 获取分支条件
  /// @return 返回分支条件节点的const引用
  const OperatorNodeInterface& GetBranchConditionReference() const {
    return *branch_condition_;
  }
  /// @brief 获取得到分支条件的全部操作
  /// @return 返回指向获取分支条件操作的容器的const指针
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
  GetFlowControlNodeToGetConditionPointer() const {
    return condition_flow_control_node_container_;
  }
  /// @brief 获取得到分支条件的全部操作
  /// @return 返回获取分支条件操作的容器的const引用
  const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&
  GetFlowControlNodeToGetConditionReference() const;
  /// @brief 设置条件为真时的分支
  /// @param[in] true_branch ：真分支结果
  /// @param[in] flow_control_node_container ：获取真分支结果的操作
  /// @retval true ：可以设置
  /// @retval false ：true_branch无法作为真分支结果
  /// @details
  /// 1.true_branch的要求参考CheckBranchValid
  /// 2.设置分支条件后如果需要改动则需要重新设置一遍真假分支
  ///   此时仍可以使用Get函数获取之前存储的分支信息
  /// @attention 先设置分支条件后设置分支内容
  bool SetTrueBranch(
      const std::shared_ptr<const OperatorNodeInterface>& true_branch,
      const std::shared_ptr<const std::list<
          std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
          flow_control_node_container);
  /// @brief 获取真分支结果
  /// @return 返回指向真分支结果的const指针
  std::shared_ptr<const OperatorNodeInterface> GetTrueBranchPointer() const {
    return true_branch_;
  }
  /// @brief 获取真分支结果
  /// @return 返回真分支结果的const引用
  const OperatorNodeInterface& GetTrueBranchReference() const {
    return *true_branch_;
  }
  /// @brief 获取得到真分支结果的操作
  /// @return 返回指向存储操作的容器的const指针
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
  GetFlowControlNodeToGetTrueBranchPointer() const {
    return true_branch_flow_control_node_container_;
  }
  /// @brief 获取得到真分支结果的操作
  /// @return 返回存储操作的容器的const引用
  const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&
  GetFlowControlNodeToGetTrueBranchReference() const {
    return *true_branch_flow_control_node_container_;
  }
  /// @brief 设置条件为假时的分支
  /// @param[in] false_branch ：假分支结果
  /// @param[in] flow_control_node_container ：获取假分支结果的操作
  /// @retval true ：可以设置
  /// @retval false ：false_branch无法作为假分支结果
  /// @details
  /// 1.false_branch的要求参考CheckBranchValid
  /// 2.设置分支条件后如果需要改动则需要重新设置一遍真假分支
  ///   此时仍可以使用Get函数获取之前存储的分支信息
  /// @attention 先设置分支条件后设置分支内容
  bool SetFalseBranch(
      const std::shared_ptr<const OperatorNodeInterface>& false_branch,
      const std::shared_ptr<const std::list<
          std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
          flow_control_node_container);
  /// @brief 获取假分支结果
  /// @return 返回指向假分支结果的const指针
  std::shared_ptr<const OperatorNodeInterface> GetFalseBranchPointer() const {
    return false_branch_;
  }
  /// @brief 获取假分支结果
  /// @return 返回假分支结果的const引用
  const OperatorNodeInterface& GetFalseBranchReference() const {
    return *false_branch_;
  }
  /// @brief 获取得到假分支结果的操作
  /// @return 返回指向存储操作的容器的const指针
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
  GetFlowControlNodeToGetFalseBranchPointer() const {
    return false_branch_flow_control_node_container_;
  }
  /// @brief 获取得到假分支结果的操作
  /// @return 返回存储操作的容器的const引用
  const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&
  GetFlowControlNodeToGetFalseBranchReference() const;
  std::shared_ptr<const OperatorNodeInterface> GetResultPointer() const {
    return result_;
  }
  /// @brief 获取结果
  /// @return 返回结果节点的const引用
  const OperatorNodeInterface& GetResultReference() const { return *result_; }

  /// @brief 检查分支条件是否有效
  /// @param[in] branch_condition ：分支条件
  /// @return 返回分支条件是否有效
  /// @retval true ：分支条件有效
  /// @retval false ：分支条件无效
  /// @note 只有StructOrBasicType::kBasic和StructOrBasicType::kPointer可以作为
  /// 分支条件
  static bool CheckBranchConditionValid(
      const OperatorNodeInterface& branch_condition);
  /// @brief 检查条件分支结果是否有效
  /// @param[in] branch ：条件分支结果
  /// @return 返回条件分支结果是否有效
  /// @retval true ：分支条件结果有效
  /// @retval false ：分支条件结果无效
  /// @note 只有StructOrBasicType::kBasic和StructOrBasicType::kPointer可以作为
  /// 分支条件
  static bool CheckBranchValid(const OperatorNodeInterface& branch) {
    return CheckBranchConditionValid(branch);
  }

 private:
  /// @brief 创建返回结果的节点
  /// @return 返回创建结果是否成功
  /// @retval true ：创建成功
  /// @details
  /// 1.设置分支条件和两个分支后调用
  /// 2.如果两个分支不能互相转化则返回false
  /// 3.如果分支条件为编译期常量则不做任何操作返回true
  /// 4.不支持非编译期常量条件下使用初始化列表
  bool ConstructResultNode();

  /// @brief 分支条件
  std::shared_ptr<const OperatorNodeInterface> branch_condition_;
  /// @brief 获取分支条件的操作
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
      condition_flow_control_node_container_;
  /// @brief 条件为真时表达式的结果
  std::shared_ptr<const OperatorNodeInterface> true_branch_ = nullptr;
  /// @brief 获取真分支结果的操作
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
      true_branch_flow_control_node_container_;
  /// @brief 条件为假时表达式的结果
  std::shared_ptr<const OperatorNodeInterface> false_branch_ = nullptr;
  /// @brief 获取假分支结果的操作
  std::shared_ptr<const std::list<
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>
      false_branch_flow_control_node_container_;
  /// @brief 存储返回结果的节点
  std::shared_ptr<const OperatorNodeInterface> result_;
};

/// @class AssignOperatorNode operator_node.h
/// @brief 赋值节点
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
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 设置待被赋值的节点
  /// @param[in] node_to_be_assigned ：待被赋值的节点
  /// @note 先设置被赋值的节点，后设置用来赋值的节点
  void SetNodeToBeAssigned(
      const std::shared_ptr<const OperatorNodeInterface>& node_to_be_assigned) {
    node_to_be_assigned_ = node_to_be_assigned;
  }
  /// @brief 获取待被赋值的节点
  /// @return 返回指向待被赋值的节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetNodeToBeAssignedPointer()
      const {
    return node_to_be_assigned_;
  }
  /// @brief 获取待被赋值的节点
  /// @return 返回待被赋值的节点的const引用
  const OperatorNodeInterface& GetNodeToBeAssignedReference() const {
    return *node_to_be_assigned_;
  }
  /// @brief 设置用来赋值的节点
  /// @param[in] node_for_assign ：用来赋值的节点
  /// @param[in] is_announce ：是否为变量声明
  /// @return 返回检查结果，具体意义见定义
  /// @details
  /// 1.如果不是可以赋值的情况则不会设置
  /// 2.声明时赋值不检查被赋值的节点自身的const属性和LeftRightValue属性
  ///   并且允许使用初始化列表
  /// 3.需要先设置被赋值的节点（调用SetNodeToBeAssigned）
  /// 4.如果被赋值的节点与用来赋值的节点类型完全相同（operator==()）则
  ///   设置被赋值的节点的类型链指针指向用来赋值的类型链以节省内存
  AssignableCheckResult SetNodeForAssign(
      const std::shared_ptr<const OperatorNodeInterface>& node_for_assign,
      bool is_announce);
  /// @brief 获取用来赋值的节点
  /// @return 返回指向用来赋值的节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetNodeForAssignPointer() const {
    return node_for_assign_;
  }
  /// @brief 获取用来赋值的节点
  /// @return 返回用来赋值的节点的const引用
  const OperatorNodeInterface& GetNodeForAssignReference() const {
    return *node_for_assign_;
  }

  /// @brief 检查两个节点是否可以赋值
  /// @param[in] node_to_be_assigned ；被赋值的节点
  /// @param[in] node_for_assign ：用来赋值的节点
  /// @param[in] is_announce
  /// @return 返回检查结果，含义见定义
  /// @details
  /// 1.自动检查AssignableCheckResut::kMayBeZeroToPointer的具体情况并改变返回结果
  /// 2.当is_announce == true时
  ///   忽略const标记和LeftRightValue标记的检查且允许使用初始化列表
  /// 3.如果使用自动数组大小推断则更新数组大小（违反const原则）
  /// 4.如果被赋值的节点与用来赋值的节点类型完全相同（operator==()）且为变量类型
  ///   则设置被赋值的节点的类型链指针指向用来赋值的类型链以节省内存
  ///   （违反const原则）
  static AssignableCheckResult CheckAssignable(
      const OperatorNodeInterface& node_to_be_assigned,
      const OperatorNodeInterface& node_for_assign, bool is_announce);

 private:
  /// @brief 检查使用初始化列表初始化变量的情况
  /// @param[in] variety_node ：被赋值的变量
  /// @param[in] list_initialize_operator_node ：用来赋值的初始化列表
  /// @return 返回检查结果，含义见定义
  /// @details
  /// 1.CheckAssignable的子过程，处理使用初始化变量列表初始化变量的情况
  /// 2.不会返回AssignableCheckResult::kInitializeList
  /// 3.如果使用自动数组大小推断则更新数组大小（违反const原则）
  static AssignableCheckResult VarietyAssignableByInitializeList(
      const VarietyOperatorNode& variety_node,
      const ListInitializeOperatorNode& list_initialize_operator_node);

  /// @brief 将要被赋值的节点
  std::shared_ptr<const OperatorNodeInterface> node_to_be_assigned_;
  /// @brief 用来赋值的节点
  std::shared_ptr<const OperatorNodeInterface> node_for_assign_;
};

/// @class MathematicalOperatorNode operator_node.h
/// @brief 数学运算
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
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    // 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 设置数学运算的符号
  /// @param[in] mathematical_operation ：数学运算符号
  void SetMathematicalOperation(MathematicalOperation mathematical_operation) {
    mathematical_operation_ = mathematical_operation;
  }
  /// @brief 获取数学运算符号
  /// @return 返回数学运算符号
  MathematicalOperation GetMathematicalOperation() const {
    return mathematical_operation_;
  }
  /// @brief 设置左侧运算数/单目运算符运算数节点
  /// @param[in] left_operator_node ：左侧运算数节点或单目运算符的运算数
  /// @return 返回是否可以设置
  /// @retval true ：运算符接受该类型运算数
  /// @retval false ：运算符不接受该类型运算数
  /// @note 如果是单目运算符则会创建结果节点
  /// @attention 先设置运算类型后设置节点
  bool SetLeftOperatorNode(
      const std::shared_ptr<const OperatorNodeInterface>& left_operator_node);
  /// @brief 获取左侧运算数/单目运算符的运算数
  /// @return 返回指向运算数节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetLeftOperatorNodePointer()
      const {
    return left_operator_node_;
  }
  /// @brief 获取左侧运算数/单目运算符的运算数
  /// @return 返回运算数节点的const引用
  const OperatorNodeInterface& GetLeftOperatorNodeReference() const {
    return *left_operator_node_;
  }
  /// @brief 设置双目运算符右侧运算数
  /// @param[in] right_operator_node ：右侧运算数
  /// @return 返回两个运算数运算结果，意义见定义
  /// @note 先设置左节点后设置右节点，如果不能运算则不会设置右侧运算数
  /// @attention 禁止对单目运算符调用该函数
  DeclineMathematicalComputeTypeResult SetRightOperatorNode(
      const std::shared_ptr<const OperatorNodeInterface>& right_operator_node);
  /// @brief 获取左侧运算数/单目运算符的运算数
  /// @return 返回指向运算数节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetRightOperatorNodePointer()
      const {
    return right_operator_node_;
  }
  /// @brief 获取左侧运算数/单目运算符的运算数
  /// @return 返回运算数节点的const引用
  const OperatorNodeInterface& GetRightOperatorNodeReference() const {
    return *right_operator_node_;
  }
  /// @brief 获取运算结果
  /// @return 返回指向运算结果节点的const指针
  std::shared_ptr<const VarietyOperatorNode> GetComputeResultNodePointer()
      const {
    return compute_result_node_;
  }
  /// @brief 获取运算结果
  /// @return 返回运算结果节点的const引用
  const VarietyOperatorNode& GetComputeResultNodeReference() const {
    return *compute_result_node_;
  }

  /// @brief 推算运算后结果
  /// @param[in] mathematical_operation ：运算符
  /// @param[in] left_operator_node ：左侧运算数
  /// @param[in] right_operator_node ：右侧运算数
  /// @return 前半部分为运算结果节点，后半部分为运算的具体情况
  /// @retval (nullptr,...) ：不能运算
  /// @note 单目运算符无需调用该函数，设置运算数时就创建了结果节点
  static std::pair<std::shared_ptr<VarietyOperatorNode>,
                   DeclineMathematicalComputeTypeResult>
  DeclineComputeResult(
      MathematicalOperation mathematical_operation,
      const std::shared_ptr<const OperatorNodeInterface>& left_operator_node,
      const std::shared_ptr<const OperatorNodeInterface>& right_operator_node);

 private:
  /// @brief 设置运算结果节点
  /// @param[in] compute_result_node ：运算结果节点的const指针
  void SetComputeResultNode(
      const std::shared_ptr<const VarietyOperatorNode>& compute_result_node) {
    compute_result_node_ = compute_result_node;
  }

  /// @brief 数学运算类型
  MathematicalOperation mathematical_operation_;
  /// @brief 左运算节点
  std::shared_ptr<const OperatorNodeInterface> left_operator_node_;
  /// @brief 右运算节点
  std::shared_ptr<const OperatorNodeInterface> right_operator_node_;
  /// @brief 运算得到的类型
  std::shared_ptr<const VarietyOperatorNode> compute_result_node_;
};

/// @class LogicalOperationOperatorNode operator_node.h
/// @brief 逻辑运算
class LogicalOperationOperatorNode : public OperatorNodeInterface {
 public:
  LogicalOperationOperatorNode(LogicalOperation logical_operation)
      : OperatorNodeInterface(GeneralOperationType::kLogicalOperation),
        logical_operation_(logical_operation) {
    CreateAndSetResultNode();
  }
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
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    /// 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 获取逻辑运算符
  /// @return 返回逻辑运算符
  LogicalOperation GetLogicalOperation() const { return logical_operation_; }
  /// @brief 设置左侧运算数/单目运算符运算数
  /// @param[in] left_operator_node ：左侧运算数/单目运算符运算数
  /// @return 返回是否设置成功
  /// @retval true ：设置成功
  /// @retval false ：left_operator_node不符合运算数的要求
  /// @note 对运算符的要求参考CheckLogicalTypeValid
  bool SetLeftOperatorNode(
      const std::shared_ptr<const OperatorNodeInterface>& left_operator_node);
  /// @brief 获取左侧运算数/单目运算符运算数
  /// @return 返回指向左侧运算数/单目运算符运算数节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetLeftOperatorNodePointer()
      const {
    return left_operator_node_;
  }
  /// @brief 获取左侧运算数/单目运算符运算数
  /// @return 返回左侧运算数/单目运算符运算数节点的const引用
  const OperatorNodeInterface& GetLeftOperatorNodeReference() const {
    return *left_operator_node_;
  }
  /// @brief 设置右侧运算数
  /// @param[in] right_operator_node ：右侧运算数
  /// @return 返回是否设置成功
  /// @retval true ：设置成功
  /// @retval false ：该运算数不可用于逻辑运算
  bool SetRightOperatorNode(
      const std::shared_ptr<const OperatorNodeInterface>& right_operator_node);
  std::shared_ptr<const OperatorNodeInterface> GetRightOperatorNodePointer()
      const {
    return right_operator_node_;
  }
  /// @brief 获取右侧运算数
  /// @return 返回指向右侧运算数节点的const指针
  const OperatorNodeInterface& GetRightOperatorNodeReference() const {
    return *right_operator_node_;
  }
  /// @brief 获取右侧运算数
  /// @return 返回右侧运算数节点的const引用
  std::shared_ptr<const VarietyOperatorNode> GetComputeResultNodePointer()
      const {
    return compute_result_node_;
  }
  /// @brief 获取运算结果
  /// @return 返回运算结果节点的const引用
  const VarietyOperatorNode& GetComputeResultNodeReference() const {
    return *compute_result_node_;
  }

  /// @brief 检查运算数是否可以参与逻辑运算
  /// @return 返回运算数是否可以参与逻辑运算
  /// @retval true ：运算数可以参与逻辑运算
  /// @retval false ：运算数不可以参与逻辑运算
  static bool CheckLogicalTypeValid(const TypeInterface& type_interface);

 private:
  /// @brief 创建并设置结果节点
  void CreateAndSetResultNode() {
    // 所有逻辑运算结果均为bool
    compute_result_node_ = std::make_shared<VarietyOperatorNode>(
        std::string(), ConstTag::kNonConst, LeftRightValueTag::kRightValue);
    compute_result_node_->SetVarietyType(
        CommonlyUsedTypeGenerator::GetBasicType<BuiltInType::kInt1,
                                                SignTag::kUnsigned>());
  }

  /// @brief 逻辑运算类型
  LogicalOperation logical_operation_;
  /// @brief 左运算数
  std::shared_ptr<const OperatorNodeInterface> left_operator_node_;
  /// @brief 右运算数
  std::shared_ptr<const OperatorNodeInterface> right_operator_node_;
  /// @brief 运算结果
  std::shared_ptr<VarietyOperatorNode> compute_result_node_;
};

/// @class DereferenceOperatorNode operator_node.h
/// @brief 解引用
class DereferenceOperatorNode : public OperatorNodeInterface {
 public:
  DereferenceOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kDeReference) {}
  DereferenceOperatorNode(const DereferenceOperatorNode&) = delete;

  DereferenceOperatorNode& operator=(const DereferenceOperatorNode&) = delete;

  virtual ConstTag GetResultConstTag() const override {
    return GetDereferencedNodeReference().GetConstTag();
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
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    /// 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 设定被解引用的节点
  /// @param[in] node_to_dereference ：被解引用的节点
  /// @return 返回是否成功设置
  /// @retval true ：设置成功
  /// @retval false ：该节点不能解引用
  /// @note 节点要求参考CheckNodeDereferenceAble
  bool SetNodeToDereference(
      const std::shared_ptr<const OperatorNodeInterface>& node_to_dereference);
  /// @brief 获取被解引用的节点
  /// @return 返回指向被解引用的节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetNodeToDereferencePointer()
      const {
    return node_to_dereference_;
  }
  /// @brief 获取被解引用的节点
  /// @return 返回被解引用的节点的const引用
  const OperatorNodeInterface& GetNodeToDereferenceReference() const {
    return *node_to_dereference_;
  }
  /// @brief 获取解引用得到的节点
  /// @return 返回指向解引用得到的节点的const指针
  std::shared_ptr<const VarietyOperatorNode> GetDereferencedNodePointer()
      const {
    return dereferenced_node_;
  }
  /// @brief 获取解引用得到的节点
  /// @return 返回解引用的节点的const引用
  const VarietyOperatorNode& GetDereferencedNodeReference() const {
    return *dereferenced_node_;
  }

  /// @brief 检查节点是否可以解引用
  /// @return 返回节点是否可以解引用
  /// @retval true ：节点可以解引用
  /// @retval false ：节点不能解引用
  /// @note 只有指针类型才可以解引用
  static bool CheckNodeDereferenceAble(
      const OperatorNodeInterface& node_to_dereference);

 private:
  /// @brief 设置解引用后得到的节点
  /// @param[in] dereferenced_node ：解引用后得到的节点
  void SetDereferencedNode(
      const std::shared_ptr<VarietyOperatorNode>& dereferenced_node) {
    dereferenced_node_ = dereferenced_node;
  }

  /// @brief 被解引用的节点
  std::shared_ptr<const OperatorNodeInterface> node_to_dereference_;
  /// @brief 解引用后得到的节点
  std::shared_ptr<VarietyOperatorNode> dereferenced_node_;
};

/// @class ObtainAddressOperatorNode operator_node.h
/// @brief 取地址
class ObtainAddressOperatorNode : public OperatorNodeInterface {
 public:
  ObtainAddressOperatorNode()
      : OperatorNodeInterface(GeneralOperationType::kObtainAddress) {}
  ObtainAddressOperatorNode(const ObtainAddressOperatorNode&) = delete;

  ObtainAddressOperatorNode& operator=(const ObtainAddressOperatorNode&) =
      delete;

  virtual ConstTag GetResultConstTag() const override {
    /// 取地址获得的值都是非const
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
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    /// 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 设置被取地址的节点
  /// @param[in] node_to_obtain_address ：被取地址的节点
  /// @return 返回是否成功设置
  /// @retval true ：设置成功
  /// @retval false ：该节点不能取地址
  /// @note 节点要求参考CheckNodeToObtainAddress
  bool SetNodeToObtainAddress(
      const std::shared_ptr<const VarietyOperatorNode>& node_to_obtain_address);
  /// @brief 获取被取地址的节点
  /// @return 返回指向被取地址的节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetNodeToBeObtainAddressPointer()
      const {
    return node_to_obtain_address_;
  }
  /// @brief 获取被取地址的节点
  /// @return 返回被取地址的节点的const引用
  const OperatorNodeInterface& GetNodeToBeObtainAddressReference() const {
    return *node_to_obtain_address_;
  }
  /// @brief 获取取地址后得到的节点
  /// @return 返回指向取地址后得到的节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetObtainedAddressNodePointer()
      const {
    return obtained_address_node_;
  }
  /// @brief 获取取地址后得到的节点
  /// @return 返回取地址后得到的节点的const引用
  const OperatorNodeInterface& GetObtainedAddressNodeReference() const {
    return *obtained_address_node_;
  }
  /// @brief 检查节点是否可以取地址
  /// @param[in] node_interface ：待检查的节点
  /// @return 返回是否可以取地址
  /// @retval true ：给定节点可以取地址
  /// @retval false ：给定节点不允许取地址
  static bool CheckNodeToObtainAddress(
      const OperatorNodeInterface& node_interface);

 private:
  /// @brief 设置取地址后得到的节点
  /// @param[in] node_obtained_address ：取地址后得到的节点
  void SetNodeObtainedAddress(
      const std::shared_ptr<const OperatorNodeInterface>&
          node_obtained_address) {
    obtained_address_node_ = node_obtained_address;
  }

  /// @brief 将要被取地址的节点
  std::shared_ptr<const VarietyOperatorNode> node_to_obtain_address_;
  /// @brief 取地址后获得的节点
  std::shared_ptr<const OperatorNodeInterface> obtained_address_node_;
};

/// @class MemberAccessOperatorNode operator_node.h
/// @brief 成员访问节点
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
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    /// 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 设置要访问的结构化数据节点
  /// @param[in] node_to_access ：待访问的结构化数据节点
  /// @return 返回给定节点是否可以作为被访问成员的节点
  /// @retval true ：成功设置
  /// @retval false ：该节点不是结构化数据节点
  /// @note 如果不可以则不会设置
  bool SetNodeToAccess(
      const std::shared_ptr<const OperatorNodeInterface>& node_to_access) {
    if (CheckNodeToAccessValid(*node_to_access)) [[likely]] {
      node_to_access_ = node_to_access;
      return true;
    } else {
      return false;
    }
  }
  /// @brief 获取被访问成员的节点
  /// @return 返回指向被访问成员的节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetNodeToAccessPointer() {
    return node_to_access_;
  }
  /// @brief 获取被访问成员的节点
  /// @return 返回被访问成员的节点的const引用
  const OperatorNodeInterface& GetNodeToAccessReference() const {
    return *node_to_access_;
  }
  /// @brief 设置访问的成员名
  /// @param[in] member_name 访问的成员名
  /// @return 返回给定节点是否是成员名
  /// @retval true ：成功设置
  /// @retval false ：成员名不存在
  /// @note 如果给定成员名不存在则不会设置
  /// @attention 必须先设置节点，后设置要访问的成员名
  template <class MemberName>
  bool SetMemberName(MemberName&& member_name) {
    return SetAccessedNodeAndMemberName(
        std::string(std::forward<MemberName>(member_name)));
  }
  /// @brief 获取访问的成员在结构中的下标
  /// @return 返回成员在结构中的下标
  MemberIndex GetMemberIndex() const { return member_index_; }
  /// @brief 获取访问的成员的节点
  /// @return 返回访问的成员节点的const指针
  /// @attention 必须成功设置要访问的节点和成员名
  std::shared_ptr<const OperatorNodeInterface> GetAccessedNodePointer() const {
    return node_accessed_;
  }
  /// @brief 获取访问的成员的节点
  /// @return 返回访问的成员节点的const指针
  /// @attention 必须成功设置要访问的节点和成员名
  const OperatorNodeInterface& GetAccessedNodeReference() const {
    return *node_accessed_;
  }

  /// @brief 检查给定节点是否可以作为被访问成员的节点
  /// @param[in] node_to_access ：待检查的节点
  /// @return 返回给定节点是否可以作为被访问成员的节点
  /// @retval true ：给定成员可以被访问成员（是结构化数据类型）
  /// @retval false ：给定成员不能被访问成员（不是结构化数据类型）
  static bool CheckNodeToAccessValid(
      const OperatorNodeInterface& node_to_access);

 private:
  /// @brief 设置访问的成员名
  /// @param[in] member_name_to_set ：访问的成员名
  /// @return 返回是否设置成功
  /// @retval true ：成功设置
  /// @retval false ：不存在给定的成员
  /// @details
  /// 根据待访问的节点和节点成员名设置成员的下标，创建并设置访问的成员的下标
  /// @note 如果不存在给定的成员则不设置
  bool SetAccessedNodeAndMemberName(std::string&& member_name_to_set);
  /// @brief 设置访问后得到的节点
  /// @param[in] node_accessed ：访问后得到的节点
  void SetAccessedNode(
      const std::shared_ptr<OperatorNodeInterface>& node_accessed) {
    node_accessed_ = node_accessed;
  }

  /// @brief 要访问的节点
  std::shared_ptr<const OperatorNodeInterface> node_to_access_;
  /// @brief 访问的节点成员的index
  /// @note 枚举类型不设置该项
  MemberIndex member_index_;
  /// @brief 访问后得到的节点
  std::shared_ptr<OperatorNodeInterface> node_accessed_;
};

/// @brief FunctionCallOperatorNode operator_node.h
/// @brief 函数调用
class FunctionCallOperatorNode : public OperatorNodeInterface {
  /// @class FunctionCallArgumentsContainer operator_node.h
  /// @brief 存储函数调用时参数的容器
  class FunctionCallArgumentsContainer {
   public:
    FunctionCallArgumentsContainer();
    ~FunctionCallArgumentsContainer();

    /// @brief 存储调用参数的容器
    using ContainerType = std::list<
        std::pair<std::shared_ptr<const OperatorNodeInterface>,
                  std::shared_ptr<std::list<std::unique_ptr<
                      c_parser_frontend::flow_control::FlowInterface>>>>>;
    /// @brief 添加函数调用参数
    /// @param[in] argument ：参数节点
    /// @param[in] flow_control_node_container ：获取参数节点的操作
    /// @details
    /// 每次添加都添加到已有参数的尾部
    /// 使用时按书写顺序排列参数
    /// @note 不检查参数有效性
    void AddFunctionCallArgument(
        const std::shared_ptr<const OperatorNodeInterface>& argument,
        const std::shared_ptr<std::list<
            std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
            flow_control_node_container) {
      function_call_arguments_.emplace_back(
          std::make_pair(argument, flow_control_node_container));
    }
    /// @brief 获取全部函数调用参数
    /// @return 返回存储函数调用参数的容器的const引用
    const ContainerType& GetFunctionCallArguments() const {
      return function_call_arguments_;
    }

   private:
    // 允许调用GetFunctionCallArgumentsNotConst()
    friend FunctionCallOperatorNode;

    /// @brief 获取全部函数调用参数
    /// @return 返回存储函数调用参数的容器的引用
    ContainerType& GetFunctionCallArguments() {
      return function_call_arguments_;
    }

    /// @brief 存储函数调用参数的容器
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
      const std::shared_ptr<const TypeInterface>& new_type,
      ConstTag new_const_tag) const override {
    assert(false);
    /// 防止警告
    return std::shared_ptr<OperatorNodeInterface>();
  }

  /// @brief 设置要调用的对象
  /// @param[in] function_object_to_call ：用来调用的对象
  /// @note 传入对象要求参考CheckFunctionTypeValid
  /// @attention 在设置参数前设置要调用的对象
  bool SetFunctionType(const std::shared_ptr<const OperatorNodeInterface>&
                           function_object_to_call);
  /// @brief 获取被调用的函数
  /// @return 返回指向被调用的函数类型的const指针
  std::shared_ptr<const c_parser_frontend::type_system::FunctionType>
  GetFunctionTypePointer() const {
    return function_type_;
  }
  /// @brief 获取被调用的函数
  /// @return 返回被调用的函数类型的const引用
  const c_parser_frontend::type_system::FunctionType& GetFunctionTypeReference()
      const {
    return *function_type_;
  }
  /// @brief 获取函数返回对象
  /// @return 返回指向函数返回对象的const指针
  std::shared_ptr<const VarietyOperatorNode> GetReturnObjectPointer() const {
    return return_object_;
  }
  /// @brief 获取函数返回对象
  /// @return 返回函数返回对象的const引用
  const VarietyOperatorNode& GetReturnObjectReference() const {
    return *return_object_;
  }
  /// @brief 获取原始调用参数
  /// @return 返回存储调用参数的容器的const引用
  /// @note 调用参数按书写顺序排列
  const auto& GetFunctionArgumentsOfferred() const {
    return function_arguments_offerred_;
  }
  /// @brief 添加调用参数
  /// @param[in] argument_node ：调用参数
  /// @param[in] sentences_to_get_argument ：获取调用参数的操作
  /// @return 返回参数的检查结果，意义见定义
  /// @details
  /// 返回待添加的参数是否通过检验，未通过检验则不会添加
  /// 每次调用该函数都在已有的参数后添加新的参数
  /// @attention 在设置参数前设置要调用的对象
  AssignableCheckResult AddFunctionCallArgument(
      const std::shared_ptr<const OperatorNodeInterface>& argument_node,
      const std::shared_ptr<std::list<
          std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
          sentences_to_get_argument);
  /// @brief 按容器设置调用参数
  /// @param[in] container ：存储调用参数的容器
  /// @return 返回是否成功添加（所有参数均符合要求）
  /// @retval true ：设置成功
  /// @retval false ：容器中存在无法隐式转换为函数定义的参数类型的参数
  /// @note 如果添加失败则不会修改参数
  /// 参数按书写顺序排列
  /// @attention 在设置参数前设置要调用的对象
  bool SetArguments(FunctionCallArgumentsContainer&& container);
  /// @brief 检查类型是否可以用于调用
  /// @param[in] type_interface ：待检查的类型
  /// @return 返回是否可以用于调用
  /// @retval true ：可以用于调用
  /// @retval false ：不是函数类型
  static bool CheckFunctionTypeValid(const TypeInterface& type_interface) {
    return type_interface.GetType() == StructOrBasicType::kFunction;
  }

 private:
  /// @brief 获取原始调用参数
  /// @return 返回存储调用参数的容器的引用
  /// @note 调用参数按书写顺序排列
  FunctionCallArgumentsContainer& GetFunctionArgumentsOfferred() {
    return function_arguments_offerred_;
  }

  /// @brief 函数返回的对象
  std::shared_ptr<const VarietyOperatorNode> return_object_;
  /// @brief 被调用的对象
  std::shared_ptr<const c_parser_frontend::type_system::FunctionType>
      function_type_;
  /// @brief 原始调用参数与获取参数的操作，按书写顺序排列
  FunctionCallArgumentsContainer function_arguments_offerred_;
};

}  // namespace c_parser_frontend::operator_node

#endif  /// !CPARSERFRONTEND_OPERATOR_NODE_H_