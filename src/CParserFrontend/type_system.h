/// @file type_system.h
/// @brief 类型系统
/// @details
/// 1.类型系统通过“类型链”表示一个对象的类型
/// 2.表示类型的最小单位为节点，一个节点可以是一重指针(数组)、函数头、
///   基础类型（内置类型/自定义结构）等，节点通过单向链表串成一串得到类型链
/// 3.类型链构造顺序同变量定义时规约的顺序，头结点是最靠近变量名的部分
/// 4.所有类型链尾部都共用一个哨兵
/// 5.类型链例：
///       const double* (*func(unsigned int x, const short y))(float z);
/// 注：argument_infos_存在简化，仅体现类型部分
///
/// ********************************                      [0].variety_type_
/// * type_ =                      *argument_infos_*****************************
/// * StructOrBasicType::kFunction *==============>* type_ =                   *
/// * next_type_node_ =            *               * StructOrBasicType::kBasic *
/// *       EndType::GetEndType()  *               * next_type_node_ =         *
/// * function_name_ = "func"      *               *     EndType::GetEndType() *
/// * return_type_const_tag_ =     *               * built_in_type_ =          *
/// *         ConstTag::kNonConst  *               *       BuiltInType::kInt32 *
/// ********************************               * sign_tag_ =               *
///             │                                 *        SignTag::kUnSigned *
///             │return_type_                     *****************************
///             ↓                                       [1].variety_type_
/// ********************************               *****************************
/// * type_ =                      *               * type_ =                   *
/// * StructOrBasicType::kPointer  *               * StructOrBasicType::kBasic *
/// * array_size_ = 0              *               * next_type_node_ =         *
/// * variety_const_tag_ =         *               *     EndType::GetEndType() *
/// *          ConstTag::kNonConst *               * built_in_type_ =          *
/// ********************************               *       BuiltInType::kInt16 *
///              │                                * sign_tag_ =               *
///              │next_type_node_                 *          SignTag::kSigned *
///              ↓                                *****************************
/// ********************************
/// * type_ =                      *                    [0].variety_type_
/// *  StructOrBasicType::kFunction*argument_infos_*****************************
/// * next_type_node =             *==============>* type_ =                   *
/// *        EndType::GetEndType() *               * StructOrBasicType::kBasic *
/// * function_name_ = ""          *               * next_type_node_ =         *
/// * return_type_const_tag_ =     *               *     EndType::GetEndType() *
/// *             ConstTag::kConst *               * built_in_type_ =          *
/// ********************************               *     BuiltInType::kFloat32 *
///              │                                * sign_tag_ =               *
///              │return_type_                    *          SignTag::kSigned *
///              ↓                                *****************************
/// ********************************
/// * type_ =                      *
/// *  StructOrBasicType::kPointer *
/// * array_size_ = 0              *
/// * variety_const_tag_ =         *
/// *          ConstTag::kNonConst *
/// ********************************
///              │
///              │next_type_node_
///              ↓
/// ********************************
/// * type_ =                      *
/// * StructOrBasicType::kBasic    *
/// * next_type_node_ =            *
/// *       EndType::GetEndType()  *
/// * built_in_type_ =             *
/// *       BuiltInType::kFloat64  *
/// * sign_tag_ = SignTag::kSigned *
/// ********************************
///
#ifndef CPARSERFRONTEND_TYPE_SYSTEM_H_
#define CPARSERFRONTEND_TYPE_SYSTEM_H_

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Common/id_wrapper.h"

namespace c_parser_frontend::operator_node {
// 前向声明ListInitializeOperatorNode，用于实现AssignedByInitializeList
class ListInitializeOperatorNode;
// 前向声明VarietyOperatorNode，用于定义函数参数
class VarietyOperatorNode;
}  // namespace c_parser_frontend::operator_node

namespace c_parser_frontend::type_system {
/// @brief const标记
enum class ConstTag { kNonConst, kConst };
/// @brief 符号标记
enum class SignTag { kSigned, kUnsigned };

/// @brief 宽泛的变量类型
enum class StructOrBasicType {
  kBasic,           ///< 基本类型
  kPointer,         ///< 指针（*）
  kFunction,        ///< 函数
  kStruct,          ///< 结构体
  kUnion,           ///< 共用体
  kEnum,            ///< 枚举
  kInitializeList,  ///< 初始化列表
  kEnd,             ///< 类型系统结尾，用于优化判断逻辑
  kNotSpecified  ///< 未确定类型，使用标准类型查找规则，用于查找名称对应的变量
};
/// @brief 内置变量类型
/// @details
/// void类型仅可声明为函数的返回类型，变量声明中必须声明为指针
/// C语言没有正式支持bool，bool仅应出现在条件判断结果中
/// void仅在函数返回值中可以作为对象出现，否则必须为指针类型的下一个节点
/// kBuiltInTypeSize存储各种类型所占空间大小
/// @attention 必须维持低值枚举项可以隐式转换为高值的原则，这样设计用于加速判断
/// 一种类型是否可以隐式转换为另一种类型
enum class BuiltInType {
  kInt1,     /// bool
  kInt8,     /// char
  kInt16,    /// short
  kInt32,    /// int,long
  kFloat32,  /// float
  kFloat64,  /// double
  kVoid      /// void
};
/// @brief 检查是否可以赋值的结果
enum class AssignableCheckResult {
  /// 可以赋值的情况
  kNonConvert,    /// 两种类型相同，无需转换即可赋值
  kUpperConvert,  /// 用来赋值的类型转换为待被赋值类型
  kConvertToVoidPointer,  /// 被赋值的对象为void指针，赋值的对象为同等维数指针
  kZeroConvertToPointer,  /// 0值转换为指针
  kUnsignedToSigned,  /// 无符号值赋值给有符号值，建议使用警告
  kSignedToUnsigned,  /// 有符号值赋值给无符号值，建议使用警告
  /// 需要具体判断的情况
  kMayBeZeroToPointer,  /// 如果是将0赋值给指针则合法，否则非法
  kInitializeList,  /// 使用初始化列表，需要结合给定值具体判断
                    /// 仅在CanBeAssignedBy中返回
  /// 不可赋值的情况
  kLowerConvert,         /// 用来赋值的类型发生缩窄转换
  kCanNotConvert,        /// 不可转换
  kAssignedNodeIsConst,  /// 被赋值对象为const
  kAssignToRightValue,  /// 被赋值对象为右值，C语言不允许赋值给右值
  kArgumentsFull,  /// 函数参数已满，不能添加更多参数
  kInitializeListTooLarge  /// 初始化列表中给出的数据数目多于指针声明时大小
};
/// @brief 推断数学运算类型的结果
enum class DeclineMathematicalComputeTypeResult {
  /// 可以计算的情况
  kComputable,              /// 可以计算
  kLeftPointerRightOffset,  /// 左类型为指针，右类型为偏移量
  kLeftOffsetRightPointer,  /// 左类型为偏移量，右类型为指针
  kConvertToLeft,           /// 提升右类型到左类型后运算
  kConvertToRight,          /// 提升左类型到右类型后运算
  /// 不可以计算的情况
  kLeftNotComputableType,   /// 左类型不是可以计算的类型
  kRightNotComputableType,  /// 右类型不是可以计算的类型
  kLeftRightBothPointer,    /// 左右类型均为指针
  kLeftNotIntger,  /// 右类型是指针，左类型不是是整数不能作为偏移量使用
  kRightNotIntger  /// 左类型是指针，右类型不是是整数不能作为偏移量使用
};
/// @brief 添加类型的结果
enum class AddTypeResult {
  /// 成功添加的情况
  kAbleToAdd,  /// 可以添加，仅在CheckFunctionDefineAddResult中使用
               /// 最终返回时被转换为更具体的值
  kNew,        /// 以前不存在该类型名，比kFunctionDefine优先返回
  kFunctionDefine,  /// 添加了函数定义
  kShiftToVector,  /// 添加前该类型名对应一个类型，添加后转换为vector存储
  kAddToVector,  /// 添加前该类型名对应至少2个类型
  kAnnounceOrDefineBeforeFunctionAnnounce,  /// 函数声明前已有声明/定义
  /// 添加失败的情况
  kTypeAlreadyIn,  /// 待添加的类型所属大类已经有一种同名类型
  kDoubleAnnounceFunction,  /// 声明已经定义/声明的函数
  kOverrideFunction         /// 试图重载函数
};
/// @brief 根据名称获取类型的结果
enum class GetTypeResult {
  /// 成功匹配的情况
  kSuccess,  /// 成功匹配
  /// 匹配失败的情况
  kTypeNameNotFound,   /// 类型名不存在
  kNoMatchTypePrefer,  /// 不存在匹配类型选择倾向的类型链指针
  kSeveralSameLevelMatches  /// 匹配到多个平级结果
};

/// @brief 指针类型大小
constexpr size_t kPointerSize = 4;
/// @brief 内置类型大小（单位：字节）
constexpr size_t kBuiltInTypeSize[7] = {1, 1, 2, 4, 4, 8, 4};

/// @brief 计算存储给定值所需的最小类型
/// @param[in] value ：待计算存储所需最小类型的值
/// @return 返回存储给定值所需的最小类型
/// @retval BuiltInType::kVoid ：value超出最大可存储值大小
/// @note
/// 不会推断出BuiltInType::kInt1类型，最小推断出BuiltInType::kInt8
BuiltInType CalculateBuiltInType(const std::string& value);

// 前向声明指针变量类，用于TypeInterface::ObtainAddress生成指针类型
class PointerType;

/// @class TypeInterface type_system.h
/// @brief 类型基类
/// @note 所有类型均从该基类派生
class TypeInterface {
 public:
  TypeInterface(StructOrBasicType type) : type_(type){};
  TypeInterface(StructOrBasicType type,
                const std::shared_ptr<const TypeInterface>& next_type_node)
      : type_(type), next_type_node_(next_type_node) {}
  virtual ~TypeInterface() {}

  /// @brief 比较两个类型是否完全相同
  /// @param[in] type_interface ：待比较的两个类型
  /// @return 返回两个类型是否完全相同
  /// @retval true ：两个类型完全相同
  /// @retval false ：两个类型不完全相同
  /// @note 自动调用下一级比较函数
  virtual bool operator==(const TypeInterface& type_interface) const = 0;
  /// @brief 判断给定类型是否可以给该对象赋值
  /// @param[in] type_interface ：待判断的类型
  /// @return 返回检查结果
  /// @note 返回值的意义见其定义
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const = 0;
  /// @brief 获取存储该类型变量所需空间的大小
  /// @return 返回存储该类型变量所需空间大小
  /// @note 指针均为kPointerSize字节
  virtual size_t GetTypeStoreSize() const = 0;
  /// @brief 获取sizeof语义下类型的大小
  /// @return 返回sizeof语义下类型的大小
  /// @note 指向数组的指针结果为类型大小乘以各维度大小
  virtual size_t TypeSizeOf() const = 0;

  /// @note operator==()的子过程，仅比较该级类型，不调用下一级比较函数
  /// @param[in] type_interface ：待比较的类型
  /// @details
  /// 派生类均应重写该函数，先调用基类的同名函数，后判断派生类新增部分
  /// 从而实现为接受一个基类输入且可以安全输出结果的函数
  /// 不使用虚函数可以提升性能
  /// @note if分支多，建议内联
  bool IsSameObject(const TypeInterface& type_interface) const {
    /// 一般情况下写代码，需要比较的类型都是相同的，出错远少于正确
    return GetType() == type_interface.GetType();
  }

  /// @brief 设置类型节点的类型
  /// @param[in] type ：类型节点的类型
  void SetType(StructOrBasicType type) { type_ = type; }
  /// @brief 获取类型节点的类型
  /// @return 返回类型节点的类型
  StructOrBasicType GetType() const { return type_; }

  /// @brief 获取下一个类型节点的指针
  /// @return 返回指向下一个类型节点的const指针
  std::shared_ptr<const TypeInterface> GetNextNodePointer() const {
    return next_type_node_;
  }
  /// @brief 设置下一个节点
  /// @param[in] next_type_node ：指向下一个节点的const指针
  void SetNextNode(const std::shared_ptr<const TypeInterface>& next_type_node) {
    next_type_node_ = next_type_node;
  }
  /// @brief 获取下一个节点的引用
  /// @return 返回下一个节点的const引用
  const TypeInterface& GetNextNodeReference() const { return *next_type_node_; }

  /// @brief 获取给定类型取地址后得到的类型
  /// @param[in] type_interface ：取地址前的类型
  /// @param[in] variety_const_tag ：被取地址的变量自身的const标记
  /// @return 返回取地址后的类型
  static std::shared_ptr<const PointerType> ObtainAddressOperatorNode(
      const std::shared_ptr<const TypeInterface>& type_interface,
      ConstTag variety_const_tag) {
    // 转除type_interface的const为了构建PointerType，不会修改该指针指向的内容
    return std::make_shared<PointerType>(variety_const_tag, 0, type_interface);
  }
  /// @brief 推断两个类型进行数学运算后得到的类型和运算情况
  /// @param[in] left_compute_type ：运算符左侧对象的类型
  /// @param[in] right_compute_type ：运算符右侧对象的类型
  /// @return 前半部分为运算后得到的类型，后半部分为运算情况
  /// @note 两钟类型不一定可以运算，必须先检查推断结果
  static std::pair<std::shared_ptr<const TypeInterface>,
                   DeclineMathematicalComputeTypeResult>
  DeclineMathematicalComputeResult(
      const std::shared_ptr<const TypeInterface>& left_compute_type,
      const std::shared_ptr<const TypeInterface>& right_compute_type);

 private:
  /// @brief 类型节点的类型
  StructOrBasicType type_;
  /// @brief 指向下一个类型节点的指针
  /// @details
  /// 解引用、取地址都是引用同一条类型链不同部分，共用节点可以节省内存
  /// 使用shared_ptr以支持共享类型链节点和仅释放类型链部分节点的功能
  std::shared_ptr<const TypeInterface> next_type_node_;
};

/// @brief 比较节点指针指向的节点是否相同
/// @param[in] type_interface1 ：一个类型节点
/// @param[in] type_interface2 ：另一个类型节点
/// @return 返回两个节点是否完全相同
/// @retval true ：两个节点完全相同
/// @retval false ：两个节点不完全相同
/// @note 用于FunctionType比较函数参数列表，比较节点而非比较指向节点的指针
bool operator==(const std::shared_ptr<TypeInterface>& type_interface1,
                const std::shared_ptr<TypeInterface>& type_interface2);

/// @class EndType type_system.h
/// @brief 类型链尾部哨兵节点
/// @details 避免比较类型链是否相同时需要判断下一个节点指针是否为空
/// @note 该节点的下一个节点为nullptr
class EndType : public TypeInterface {
 public:
  EndType() : TypeInterface(StructOrBasicType::kEnd, nullptr) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    // EndType中不会调用下一级类型节点的operator==()
    // 它自身是结尾，也没有下一级类型节点
    return IsSameObject(type_interface);
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override {
    return AssignableCheckResult::kNonConvert;
  }
  /// @attention 该函数不应被调用
  virtual size_t GetTypeStoreSize() const override {
    assert(false);
    // 防止警告
    return size_t();
  }
  /// @attention 该函数不应被调用
  virtual size_t TypeSizeOf() const override {
    assert(false);
    // 防止警告
    return size_t();
  }

  /// @brief 获取EndType节点指针
  /// @return 返回指向全局共用的EndType节点的const指针
  /// @note 全局共用一个节点，节省内存
  static std::shared_ptr<const EndType> GetEndType() {
    static std::shared_ptr<EndType> end_type_pointer =
        std::make_shared<EndType>();
    return end_type_pointer;
  }

  bool IsSameObject(const TypeInterface& type_interface) const {
    // this == &type_interface是优化手段
    // 类型系统设计思路是尽可能多的共享一条类型链
    // 所以容易出现指向同一个节点的情况
    return this == &type_interface ||
           TypeInterface::IsSameObject(type_interface);
  }
};

/// @class BasicType type_system.h
/// @brief 预定义的基础类型
/// @details 该节点后连接的是EndType节点
class BasicType : public TypeInterface {
 public:
  BasicType(BuiltInType built_in_type, SignTag sign_tag)
      : TypeInterface(StructOrBasicType::kBasic, EndType::GetEndType()),
        built_in_type_(built_in_type),
        sign_tag_(sign_tag) {}

  virtual bool operator==(const TypeInterface& type_interface) const override;
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override;
  virtual size_t GetTypeStoreSize() const override;
  virtual size_t TypeSizeOf() const override {
    return BasicType::GetTypeStoreSize();
  }

  /// @brief 设置内置类型
  /// @param[in] built_in_type ：待设置的内置类型
  void SetBuiltInType(BuiltInType built_in_type) {
    built_in_type_ = built_in_type;
  }
  /// @brief 获取内置类型
  /// @return 返回内置类型
  BuiltInType GetBuiltInType() const { return built_in_type_; }
  /// @brief 设置内置类型的符号属性（有符号/无符号）
  /// @param[in] sign_tag ：符号标记
  void SetSignTag(SignTag sign_tag) { sign_tag_ = sign_tag; }
  /// @brief 获取内置类型的符号标记
  /// @return 返回符号标记
  SignTag GetSignTag() const { return sign_tag_; }

  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  /// @brief 内置类型
  BuiltInType built_in_type_;
  /// @brief 符号标记
  SignTag sign_tag_;
};

/// 指针类型，一个*对应一个节点
class PointerType : public TypeInterface {
 public:
  PointerType(ConstTag const_tag, size_t array_size)
      : TypeInterface(StructOrBasicType::kPointer),
        variety_const_tag_(const_tag),
        array_size_(array_size) {}
  PointerType(ConstTag const_tag, size_t array_size,
              const std::shared_ptr<const TypeInterface>& next_node)
      : TypeInterface(StructOrBasicType::kPointer, next_node),
        variety_const_tag_(const_tag),
        array_size_(array_size) {}

  virtual bool operator==(const TypeInterface& type_interface) const override;
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override;
  virtual size_t GetTypeStoreSize() const override { return kPointerSize; }
  virtual size_t TypeSizeOf() const override;

  /// @brief 设置指针指向的对象的const状态
  /// @param[in] const_tag ：const标记
  /// @note 该标记表示解引用后得到的引用是否可以修改
  void SetConstTag(ConstTag const_tag) { variety_const_tag_ = const_tag; }
  /// @brief 获取指针指向的对象的const状态
  /// @return 返回解引用后得到的对象的const标记
  /// @note 该标记表示解引用后得到的引用是否可以修改
  ConstTag GetConstTag() const { return variety_const_tag_; }
  /// @brief 设置指针指向的数组大小
  /// @param[in] array_size ：数组大小
  /// @details
  /// 0 ：纯指针
  /// -1 ：待根据后面的数组初始化列表填写数组大小
  /// 其它：大小为array_size的数组
  void SetArraySize(size_t array_size) { array_size_ = array_size; }
  /// @brief 获取指针指向的数组大小
  /// @return 返回指针指向的数组大小
  /// @retval 0 ：纯指针
  /// @retval -1 ：待根据后面的数组初始化列表填写数组大小
  /// @retval 其它：大小为array_size的数组
  size_t GetArraySize() const { return array_size_; }
  bool IsSameObject(const TypeInterface& type_interface) const;
  /// @brief 获取该指针解引用后得到的类型
  /// @return 前半部分为解引用后的类型，后半部分为解引用得到的对象是否为const
  std::pair<std::shared_ptr<const TypeInterface>, ConstTag> DeReference()
      const {
    return std::make_pair(GetNextNodePointer(), GetConstTag());
  }

 private:
  /// @brief 解引用后的对象是否为const标记
  ConstTag variety_const_tag_;
  /// @brief 指针指向的数组中对象个数
  /// 0 ：纯指针
  /// -1 ：待根据后面的数组初始化列表填写数组大小（仅限声明中）
  /// 其它 ：大小为array_size_的数组
  size_t array_size_;
};

/// @class FunctionType type_system.h
/// @brief 函数类型
class FunctionType : public TypeInterface {
  /// @class ArgumentInfo type_system.h
  /// @brief 存储函数参数信息
  struct ArgumentInfo {
    bool operator==(const ArgumentInfo& argument_info) const;

    /// @brief 指向变量名的指针不稳定
    /// @details
    /// 函数声明在声明结束后被清除
    /// 函数定义在定义结束后被清除
    std::shared_ptr<const c_parser_frontend::operator_node::VarietyOperatorNode>
        variety_operator_node;
  };
  /// @brief 存储参数信息的容器
  using ArgumentInfoContainer = std::vector<ArgumentInfo>;

 public:
  FunctionType(const std::string& function_name)
      : TypeInterface(StructOrBasicType::kFunction, EndType::GetEndType()),
        function_name_(function_name) {}
  template <class ArgumentContainter>
  FunctionType(const std::string& function_name,
               ArgumentContainter&& argument_infos)
      : TypeInterface(StructOrBasicType::kFunction, EndType::GetEndType()),
        function_name_(function_name),
        argument_infos_(std::forward<ArgumentContainter>(argument_infos)) {}
  template <class ArgumentContainter>
  FunctionType(const std::string& function_name, ConstTag return_type_const_tag,
               const std::shared_ptr<const TypeInterface>& return_type,
               ArgumentContainter&& argument_infos)
      : TypeInterface(StructOrBasicType::kFunction, EndType::GetEndType()),
        function_name_(function_name),
        return_type_const_tag_(return_type_const_tag),
        return_type_(return_type),
        argument_infos_(std::forward<ArgumentContainter>(argument_infos)) {}

  virtual bool operator==(const TypeInterface& type_interface) const override;
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override;
  virtual size_t GetTypeStoreSize() const override {
    assert(false);
    // 防止警告
    return size_t();
  }
  virtual size_t TypeSizeOf() const override {
    assert(false);
    // 防止警告
    return size_t();
  }
  bool IsSameObject(const TypeInterface& type_interface) const {
    // 函数类型唯一，只有指向同一个FunctionType对象才是同一个函数
    return this == &type_interface;
  }
  /// @brief 比较两个函数签名是否相同
  /// @param[in] function_type ：用来比较的另一个函数
  /// @return 返回两个函数签名是否相同
  /// @retval true ：两个函数签名相同
  /// @retval false ：两个函数签名不同
  /// @details
  /// 检查两个函数类型的函数名、函数参数、返回类型是否相同
  /// @note 只检查函数签名是否相同，不检查函数内执行的语句
  bool IsSameSignature(const FunctionType& function_type) const {
    return GetFunctionName() == function_type.GetFunctionName() &&
           GetArguments() == function_type.GetArguments() &&
           GetReturnTypeReference() == function_type.GetReturnTypeReference();
  }
  /// @brief 设置函数返回类型
  /// @param[in] return_type ：指向函数返回类型类型链头结点的指针
  void SetReturnTypePointer(
      const std::shared_ptr<const TypeInterface>& return_type) {
    return_type_ = return_type;
  }
  /// @brief 获取函数的返回类型链
  /// @return 返回指向函数的返回类型链头结点的指针
  std::shared_ptr<const TypeInterface> GetReturnTypePointer() const {
    return return_type_;
  }
  /// @brief 获取函数的返回类型链头结点引用
  /// @return 返回函数的返回类型链头结点的const引用
  const TypeInterface& GetReturnTypeReference() const { return *return_type_; }
  /// @brief 设置函数返回类型的const标记
  /// @param[in] return_type_const_tag ：函数返回类型的const标记
  /// @details
  /// 该属性决定返回的对象是否不可修改
  void SetReturnTypeConstTag(ConstTag return_type_const_tag) {
    return_type_const_tag_ = return_type_const_tag;
  }
  /// @brief 获取函数返回对象的const标记
  /// @return 返回函数返回对象的const标记
  ConstTag GetReturnTypeConstTag() const { return return_type_const_tag_; }
  /// @brief 添加函数参数
  /// @param[in] argument ：根据参数类型和参数名构建的参数变量节点
  void AddFunctionCallArgument(
      const std::shared_ptr<
          const c_parser_frontend::operator_node::VarietyOperatorNode>&
          argument);
  /// @brief 获取存储参数节点的容器
  /// @return 返回存储参数节点的容器的const引用
  const ArgumentInfoContainer& GetArguments() const { return argument_infos_; }
  /// @brief 设置函数名
  /// @param[in] function_name ：函数名
  void SetFunctionName(const std::string& function_name) {
    function_name_ = function_name;
  }
  /// @brief 获取函数名
  /// @return 返回函数名的const引用
  const std::string& GetFunctionName() const { return function_name_; }
  /// @brief 获取函数类型的函数指针形式
  /// @param[in] function_type ：待获取指针形式的函数类型节点
  /// @return 返回函数的一重函数指针形式
  /// @details
  /// 返回一个const指针节点，这个指针节点指向function_type节点
  static std::shared_ptr<const PointerType> ConvertToFunctionPointer(
      const std::shared_ptr<const TypeInterface>& function_type) {
    assert(function_type->GetType() == StructOrBasicType::kFunction);
    /// 转除const为了可以构造指针节点，不会修改function_type
    return std::make_shared<const PointerType>(ConstTag::kConst, 0,
                                               function_type);
  }

 private:
  /// @brief 函数名
  std::string function_name_;
  /// @brief 返回值的const标记
  ConstTag return_type_const_tag_;
  /// @brief 函数返回类型
  std::shared_ptr<const TypeInterface> return_type_;
  /// @brief 参数类型和参数名
  ArgumentInfoContainer argument_infos_;
};

/// @class StructureTypeInterface type_system.h
/// @brief 结构化类型（结构体/共用体）的基类
/// @note 枚举不从该类派生
class StructureTypeInterface : public TypeInterface {
 protected:
  /// @class StructureMemberContainer type_system.h
  /// @brief 存储结构化类型成员
  /// @details
  /// 键值为成员名，值前半部分为成员类型，后半部分为成员const标记
  class StructureMemberContainer {
    enum class IdWrapper { kMemberIndex };

   public:
    /// @brief 存储成员信息的容器
    using MemberContainer =
        std::vector<std::pair<std::shared_ptr<const TypeInterface>, ConstTag>>;
    /// @brief 成员在结构化类型中的下标，从上到下递增
    /// @details
    /// struct {
    ///   int x;    // 下标：0
    ///   char y;   // 下标：1
    ///   double z; // 下标：2
    /// }
    using MemberIndex =
        frontend::common::ExplicitIdWrapper<size_t, IdWrapper,
                                            IdWrapper::kMemberIndex>;
    /// @brief 查询成员在结构化类型中的下标
    /// @param[in] member_name ：成员名
    /// @return 返回成员在结构化类型中的下标
    /// @retval MemberIndex::InvalidId() ：不存在该成员
    MemberIndex GetMemberIndex(const std::string& member_name) const;
    /// @brief 根据成员下标获取成员信息
    /// @param[in] member_index ：成员下标
    /// @return 前半部分为成员类型，后半部分为成员的const标记
    /// @note member_index必须有效
    const std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&
    GetMemberInfo(MemberIndex member_index) const {
      assert(member_index.IsValid());
      assert(member_index < members_.size());
      return members_[member_index];
    }
    /// @brief 添加成员
    /// @param[in] member_name ：成员名
    /// @param[in] member_type ：成员类型
    /// @param[in] member_const_tag ：成员的const标记
    /// @return 返回成员的下标
    /// @retval MemberIndex::InvalidId() ：待添加成员名已存在
    template <class MemberName>
    MemberIndex AddMember(
        MemberName&& member_name,
        const std::shared_ptr<const TypeInterface>& member_type,
        ConstTag member_const_tag);
    /// @brief 获取存储成员信息的容器
    /// @return 返回存储成员信息的容器的const引用
    const auto& GetMembers() const { return members_; }

   private:
    /// @brief 成员名到下标的映射
    std::unordered_map<std::string, MemberIndex> member_name_to_index_;
    /// @brief 存储成员类型与const标记
    MemberContainer members_;
  };

  StructureTypeInterface(const std::string& structure_name,
                         StructOrBasicType structure_type)
      : TypeInterface(structure_type, EndType::GetEndType()),
        structure_name_(structure_name) {}

 public:
  using MemberIndex = StructureMemberContainer::MemberIndex;

  virtual bool operator==(const TypeInterface& type_interface) const override {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() ==
             static_cast<const StructureTypeInterface&>(type_interface)
                 .GetNextNodeReference();
    } else {
      return false;
    }
  }

  /// @brief 获取结构化类型名
  /// @return 返回结构化类型名的const引用
  const std::string& GetStructureName() const { return structure_name_; }
  /// @brief 设置结构化类型名
  /// @param[in] structure_name ：结构化类型名
  void SetStructureName(const std::string& structure_name) {
    structure_name_ = structure_name;
  }
  /// @brief 按容器设置结构化类型的成员数据
  /// @tparam MemberContainer ：StructureMemberContainer的不同引用
  /// @param[in] structure_members ：存储结构化类型成员数据的容器
  template <class MemberContainer>
  requires std::is_same_v<std::decay_t<MemberContainer>,
                          StructureMemberContainer>
  void SetStructureMembers(MemberContainer&& structure_members) {
    structure_member_container_ = structure_members;
  }
  /// @brief 获取结构化类型存储成员信息的容器
  /// @return 返回结构化类型存储成员信息的容器的const引用
  const auto& GetStructureMemberContainer() const {
    return structure_member_container_;
  }
  /// @brief 添加结构化类型的成员
  /// @param[in] member_name ：成员名
  /// @param[in] member_type ：成员类型
  /// @param[in] member_const_tag ：成员的const标记
  /// @return 返回成员的下标
  /// @retval MemberIndex::InvalidId() ：待添加成员名已存在
  /// @note 向已有成员列表的尾部添加
  template <class MemberName>
  MemberIndex AddStructureMember(
      MemberName&& member_name,
      const std::shared_ptr<const TypeInterface>& member_type,
      ConstTag member_const_tag) {
    return GetStructureMemberContainer().AddMember(
        std::forward<MemberName>(member_name), member_type, member_const_tag);
  }
  /// @brief 查询结构化类型成员的下标
  /// @param[in] member_name ：成员名
  /// @return 返回结构化类型成员的下标
  /// @retval StructMemberIndex::InvalidId() ：给定成员名不存在
  MemberIndex GetStructureMemberIndex(const std::string& member_name) const {
    return GetStructureMemberContainer().GetMemberIndex(member_name);
  }
  /// @brief 获取结构化类型成员的信息
  /// @param[in] member_index ：成员下标
  /// @return 前半部分为成员类型，后半部分为成员const标记
  /// @note member_index必须有效
  const std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&
  GetStructureMemberInfo(MemberIndex member_index) const {
    assert(member_index.IsValid());
    return GetStructureMemberContainer().GetMemberInfo(member_index);
  }
  /// @brief 获取存储成员信息的容器
  /// @return 返回存储成员信息的容器的const引用
  const StructureMemberContainer::MemberContainer& GetStructureMembers() const {
    return GetStructureMemberContainer().GetMembers();
  }
  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  /// @brief 获取存储成员信息的容器
  /// @return 返回存储成员信息的容器的引用
  StructureMemberContainer& GetStructureMemberContainer() {
    return structure_member_container_;
  }
  /// @brief 结构化类型名
  std::string structure_name_;
  /// @brief 结构化类型成员容器
  StructureMemberContainer structure_member_container_;
};

/// @class StructType type_system.h
/// @brief 结构体类型
class StructType : public StructureTypeInterface {
 public:
  /// @brief 存储成员的结构类型
  using StructMemberContainerType = StructureMemberContainer;
  /// @brief 成员在容器中的下标
  using StructMemberIndex = StructMemberContainerType::MemberIndex;

  StructType(const std::string& struct_name)
      : StructureTypeInterface(struct_name, StructOrBasicType::kStruct) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    return StructureTypeInterface::operator==(type_interface);
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override;
  virtual size_t GetTypeStoreSize() const override;
  virtual size_t TypeSizeOf() const override {
    return StructType::GetTypeStoreSize();
  }

  /// @brief 按容器设置结构体成员信息
  /// @tparam StructMembers ：StructMemberContainerType不同引用
  /// @param[in] members ：成员信息
  template <class StructMembers>
  requires std::is_same_v<std::decay_t<StructMembers>,
                          StructMemberContainerType>
  void SetStructMembers(StructMembers&& members) {
    SetStructureMembers(std::forward<StructMembers>(members));
  }
  /// @brief 添加结构体成员
  /// @param[in] member_name ：成员名
  /// @param[in] member_type_pointer ：成员类型
  /// @param[in] member_const_tag ：成员的const标记
  /// @return 返回成员的下标
  /// @retval MemberIndex::InvalidId() ：待添加成员名已存在
  /// @note 向已有成员列表的尾部添加
  template <class MemberName>
  StructMemberIndex AddStructMember(
      MemberName&& member_name,
      const std::shared_ptr<const TypeInterface>& member_type_pointer,
      ConstTag member_const_tag) {
    return AddStructureMember(std::forward<MemberName>(member_name),
                              member_type_pointer, member_const_tag);
  }
  /// @brief 查询成员的下标
  /// @param[in] member_name ：成员名
  /// @return 返回成员下标
  /// @retval StructMemberIndex::InvalidId() 给定成员名不存在
  StructMemberIndex GetStructMemberIndex(const std::string& member_name) const {
    return GetStructureMemberIndex(member_name);
  }
  /// @brief 根据成员下标获取成员信息
  /// @param[in] member_index ：成员下标
  /// @return 前半部分为成员类型，后半部分为成员的const标记
  /// @note member_index必须有效
  const std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&
  GetStructMemberInfo(StructMemberIndex member_index) const {
    assert(member_index.IsValid());
    return GetStructureMemberInfo(member_index);
  }
  /// @brief 设置结构体名
  /// @param[in] struct_name ：结构体名
  /// @note 允许使用空名字以表示匿名结构体
  void SetStructName(const std::string& struct_name) {
    SetStructureName(struct_name);
  }
  /// @brief 获取结构体名
  /// @return 返回结构体名的const引用
  const std::string& GetStructName() const { return GetStructureName(); }

  bool IsSameObject(const TypeInterface& type_interface) const {
    return StructureTypeInterface::IsSameObject(type_interface);
  }
};

class UnionType : public StructureTypeInterface {
 public:
  /// @brief 存储成员的结构类型
  using UnionMemberContainerType = StructureMemberContainer;
  /// @brief 成员在容器中的下标
  using UnionMemberIndex = UnionMemberContainerType::MemberIndex;

  UnionType(const std::string& union_name)
      : StructureTypeInterface(union_name, StructOrBasicType::kUnion) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    return StructureTypeInterface::operator==(type_interface);
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override;
  virtual size_t GetTypeStoreSize() const override;
  virtual size_t TypeSizeOf() const override {
    return UnionType::GetTypeStoreSize();
  }

  /// @brief 按容器设置共用体成员信息
  /// @tparam UnionMembers ：UnionMemberContainerType不同引用
  /// @param[in] union_members ：成员信息
  template <class UnionMembers>
  requires std::is_same_v<std::decay_t<UnionMembers>, UnionMemberContainerType>
  void SetUnionMembers(UnionMembers&& union_members) {
    SetStructureMembers(std::forward<UnionMembers>(union_members));
  }
  /// @brief 添加共用体成员
  /// @param[in] member_name ：成员名
  /// @param[in] member_type_pointer ：成员类型
  /// @param[in] member_const_tag ：成员的const标记
  /// @return 返回成员的下标
  /// @retval MemberIndex::InvalidId() ：待添加成员名已存在
  /// @note 向已有成员列表的尾部添加
  template <class MemberName>
  auto AddUnionMember(
      MemberName&& member_name,
      const std::shared_ptr<const TypeInterface>& member_type_pointer,
      ConstTag member_const_tag) {
    return AddStructureMember(std::forward<MemberName>(member_name),
                              member_type_pointer, member_const_tag);
  }
  /// @brief 查询成员的下标
  /// @param[in] member_name ：成员名
  /// @return 返回成员下标
  /// @retval StructMemberIndex::InvalidId() 给定成员名不存在
  UnionMemberIndex GetUnionMemberIndex(const std::string& member_name) const {
    return GetStructureMemberIndex(member_name);
  }
  /// @brief 根据成员下标获取成员信息
  /// @param[in] member_index ：成员下标
  /// @return 前半部分为成员类型，后半部分为成员的const标记
  /// @note member_index必须有效
  const auto& GetUnionMemberInfo(UnionMemberIndex member_index) const {
    assert(member_index.IsValid());
    return GetStructureMemberInfo(member_index);
  }
  /// @brief 设置共用体名
  /// @param[in] union_name ：共用体名
  /// @note 允许使用空名字以表示匿名共用体
  void SetUnionName(const std::string& union_name) {
    SetStructureName(union_name);
  }
  /// @brief 获取共用体名
  /// @return 返回共用体名的const引用
  const std::string& GetUnionName() const { return GetStructureName(); }
  bool IsSameObject(const TypeInterface& type_interface) const {
    return StructureTypeInterface::IsSameObject(type_interface);
  }
};

/// @class EnumType type_system.h
/// @brief 枚举类型
class EnumType : public TypeInterface {
 public:
  /// @brief 枚举内部存储成员名和对应值的结构
  using EnumContainerType = std::unordered_map<std::string, long long>;

  EnumType(const std::string& enum_name)
      : TypeInterface(StructOrBasicType::kEnum, EndType::GetEndType()),
        enum_name_(enum_name) {}
  template <class StructMembers>
  EnumType(const std::string& enum_name, StructMembers&& enum_members)
      : TypeInterface(StructOrBasicType::kEnum, EndType::GetEndType()),
        enum_name_(enum_name),
        enum_members_(std::forward<StructMembers>(enum_members)) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() == type_interface.GetNextNodeReference();
    } else {
      return false;
    }
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override;
  virtual size_t GetTypeStoreSize() const override {
    return GetContainerTypeReference().GetTypeStoreSize();
  }
  virtual size_t TypeSizeOf() const override {
    return EnumType::GetTypeStoreSize();
  }

  /// @brief 按容器设定枚举成员
  /// @tparam EnumMembers ：EnumContainerType的不同引用
  /// @param[in] enum_members ：存储枚举成员信息的容器
  template <class EnumMembers>
  requires std::is_same_v<std::decay_t<EnumMembers>, EnumContainerType>
  void SetEnumMembers(EnumMembers&& enum_members) {
    enum_members_ = std::forward<EnumMembers>(enum_members);
  }
  /// @brief 添加枚举成员
  /// @param[in] enum_member_name ：枚举成员名
  /// @param[in] value ：枚举成员值
  /// @return 前半部分为指向数据的迭代器，后半部分为是否插入
  /// @retval (...,false) enum_member_name已存在
  template <class EnumMemberName>
  auto AddEnumMember(EnumMemberName&& enum_member_name, long long value) {
    return enum_members_.emplace(
        std::make_pair(std::forward<EnumMemberName>(enum_member_name), value));
  }
  /// @brief 获取枚举成员信息
  /// @param[in] member_name ：成员名
  /// @return 前半部分为指向成员信息的迭代器，后半部分为成员是否存在
  /// @retval (...,false) 成员名不存在
  std::pair<EnumContainerType::const_iterator, bool> GetEnumMemberInfo(
      const std::string& member_name) const {
    auto iter = GetEnumMembers().find(member_name);
    return std::make_pair(iter, iter != GetEnumMembers().end());
  }
  /// @brief 获取储存枚举值的类型对应的类型链
  /// @return 返回指向存储枚举值的类型链头结点的const指针
  std::shared_ptr<const TypeInterface> GetContainerTypePointer() const {
    return container_type_;
  }
  /// @brief 获取储存枚举值的类型对应的类型链
  /// @return 返回指向存储枚举值的类型链头结点的const引用
  const TypeInterface& GetContainerTypeReference() const {
    return *container_type_;
  }
  /// @brief 设置枚举名
  /// @param[in] enum_name ：枚举名
  void SetEnumName(const std::string& enum_name) { enum_name_ = enum_name; }
  /// @brief 获取枚举名
  /// @return 返回枚举名的const引用
  const std::string& GetEnumName() const { return enum_name_; }
  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  /// @brief 获取存储成员的容器
  /// @return 返回存储成员的容器的const引用
  const EnumContainerType& GetEnumMembers() const { return enum_members_; }
  /// @brief 获取存储成员的容器
  /// @return 返回存储成员的容器的引用
  EnumContainerType& GetEnumMembers() { return enum_members_; }
  /// @brief 设置存储枚举值的类型对应的类型链
  /// @param[in] container_type ：存储枚举值的类型对应类型链头结点指针
  /// @note 不检查使用的类型能否存储所有枚举值
  void SetContainerType(
      const std::shared_ptr<const TypeInterface>& container_type) {
    container_type_ = container_type;
  }

  /// @brief 枚举名
  std::string enum_name_;
  /// @brief 存储枚举成员的容器
  /// @note 前半部分为成员名，后半部分为枚举对应的值
  EnumContainerType enum_members_;
  /// @brief 存储枚举值的类型
  /// @note 不是枚举的类型
  std::shared_ptr<const TypeInterface> container_type_;
};

/// @class InitializeListType type_system.h
/// @brief 初始化列表类型
class InitializeListType : public TypeInterface {
 public:
  /// @brief 储存初始化信息的列表类型
  using InitializeListContainerType =
      std::list<std::shared_ptr<const TypeInterface>>;

  InitializeListType()
      : TypeInterface(StructOrBasicType::kInitializeList,
                      EndType::GetEndType()) {}
  template <class MemberType>
  InitializeListType(MemberType&& list_types)
      : TypeInterface(StructOrBasicType::kInitializeList),
        list_types_(std::forward<MemberType>(list_types)) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() == type_interface.GetNextNodeReference();
    } else {
      return false;
    }
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override {
    /// 初始化列表不能被赋值
    return AssignableCheckResult::kCanNotConvert;
  }
  virtual size_t GetTypeStoreSize() const override {
    assert(false);
    /// 防止警告
    return size_t();
  }
  virtual size_t TypeSizeOf() const override {
    assert(false);
    /// 防止警告
    return size_t();
  }

  /// @brief 添加初始化列表内的类型
  /// @param[in] type_pointer ：指向类型链头结点的指针
  /// @note 添加到已有类型的后面
  void AddListType(const std::shared_ptr<const TypeInterface>& type_pointer) {
    list_types_.emplace_back(type_pointer);
  }
  /// @brief 获取初始化列表中所有的类型
  /// @return 返回存储类型的容器
  const auto& GetListTypes() const { return list_types_; }
  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  /// @brief 初始化列表中的类型
  InitializeListContainerType list_types_;
};

/// @brief 常用类型节点的生成器
/// @note 全局共享一份节点以节省内存和避免多次分配的消耗
class CommonlyUsedTypeGenerator {
 public:
  /// @brief 获取基础类型的类型链
  /// @tparam built_in_type ：内置类型
  /// @tparam sign_tag ：类型的符号标记
  /// @return 返回指向类型链头结点的const指针
  /// @details
  /// 返回的指针指向全局共享的一个类型节点对象
  /// @note void类型和bool类型使用SignTag::kUnsigned
  template <BuiltInType built_in_type, SignTag sign_tag>
  static std::shared_ptr<const BasicType> GetBasicType() {
    static std::shared_ptr<BasicType> basic_type =
        std::make_shared<BasicType>(built_in_type, sign_tag);
    return basic_type;
  }
  /// @brief 获取基础类型的类型链
  /// @param[in] built_in_type ：内置类型
  /// @param[in] sign_tag ：类型的符号标记
  /// @return 返回指向类型链头结点的const指针
  /// @details
  /// 返回的指针指向全局共享的一个类型节点对象
  /// 该函数用于运行期调用同名模板函数
  /// @note void类型和bool类型使用SignTag::kUnsigned
  static std::shared_ptr<const BasicType> GetBasicTypeNotTemplate(
      BuiltInType built_in_type, SignTag sign_tag);

  /// @brief 获取初始化用字符串类型（const char*）
  /// @return 返回指向类型链的const指针
  /// @note 全局共享一条类型链
  static std::shared_ptr<const PointerType> GetConstExprStringType() {
    static std::shared_ptr<PointerType> constexpr_string_type =
        std::make_shared<PointerType>(
            ConstTag::kConst, 0,
            std::move(GetBasicType<BuiltInType::kInt8, SignTag::kSigned>()));
    return constexpr_string_type;
  }
};

/// @class TypeSystem type_system.h
/// @brief 类型系统
/// @details
/// 1.类型系统存储自定义类型名到类型名对应的类型链的映射
/// 2.允许一个类型名绑定多个类型，这种情况下如果根据类型名获取类型链时不指定具体
///   类型则无法获取
/// 3.根据TypeSystem::TypeData::IsSameKind判断两个类型是否属于同一大类，
///   StructOrBasicType中除了BasicType和PointerType属于同一大类以外，其余类型
///   各自成一大类
/// 4.仅当待添加的类型与已有类型均不属于同一大类时可以添加（除相同签名函数类型）
/// 5.所有类型均有全局定义域
class TypeSystem {
  /// @class TypeData type_system.h
  /// @brief 存储某个类型名下的不同类型链信息
  class TypeData {
   public:
    /// @brief 添加一个类型
    /// @param[in] type_to_add ：待添加的类型链
    /// @return 返回添加结果，具体解释见AddTypeResult定义
    /// @details
    /// 1.添加某个类型名下的一个类型，如果需要从单个指针转换为list则自动处理
    /// 2.同类型名下每种类型大类仅允许添加一种类型，根据
    ///   TypeSystem::TypeData::IsSameKind判断两个类型是否属于同一大类，
    ///   StructOrBasicType中除了BasicType和PointerType属于同一大类以外，其余
    ///   类型各自成一大类
    /// 3.只有函数签名相同时才允许重复添加函数类型，后添加的函数类型会覆盖已有
    ///   的函数类型
    AddTypeResult AddType(
        const std::shared_ptr<const TypeInterface>& type_to_add);
    /// @brief 根据类型偏好获取类型
    /// @param[in] type_prefer ：类型偏好
    /// @return 前半部分为获取到的类型链头结点指针，后半部分为获取结果
    /// @details
    /// 1.无类型偏好时使用StructOrBasicType::kNotSpecified
    /// 2.含有多种类型时如果不指定要获取的类型则返回属于
    ///   StructOrBasicType::kBasic/kPointer大类的类型，不存在该大类的类型则返回
    ///   GetTypeResult::kSeveralSameLevelMatches
    /// 3.指定类型不存在时返回GetTypeResult::kSeveralSameLevelMatches
    std::pair<std::shared_ptr<const TypeInterface>, GetTypeResult> GetType(
        StructOrBasicType type_prefer) const;
    /// @brief 查询容器是否为空
    /// @return 返回容器是否为空
    /// @retval true ：容器空
    /// @retval false ：容器不空
    bool Empty() const {
      return std::get_if<std::monostate>(&type_data_) != nullptr;
    }

    /// @brief 检查两种类型是否属于同一大类
    /// @param[in] type1 ：一种类型
    /// @param[in] type2 ：另一种类型
    /// @return 返回两种类型是否属于同一大类
    /// @retval true ：两种类型属于同一大类
    /// @retval false ：两种类型不属于同一大类
    /// @details
    /// StructOrBasicType::kBasic和StructOrBasicType::kPointer属于同一大类
    /// 除此以外类型各自成一大类
    static bool IsSameKind(StructOrBasicType type1, StructOrBasicType type2);
    /// @brief 检查已存在函数声明/定义条件下添加同名函数声明/定义的错误情况
    /// @param[in] function_type_exist ：已存在的函数类型
    /// @param[in] function_type_to_add ：待添加的函数类型
    /// @retval AddTypeResult::kTypeAlreadyIn 已存在相同的函数类型
    /// @retval AddTypeResult::kOverrideFunction 已存在参数不同的函数类型
    /// @attention 该函数仅返回以上两种结果
    static AddTypeResult CheckFunctionDefineAddResult(
        const FunctionType& function_type_exist,
        const FunctionType& function_type_to_add) {
      if (function_type_exist.IsSameSignature(function_type_to_add)) {
        return AddTypeResult::kTypeAlreadyIn;
      } else {
        return AddTypeResult::kOverrideFunction;
      }
    }

   private:
    /// @brief 存储某类型名下的全部类型
    /// @details
    /// 初始构建时为空，添加类型指针后根据添加的数量使用直接存储或list存储
    /// 将StructOrBasicType::kBasic/kPointer类型的指针放到最前面从而
    /// 在无类型偏好时加速查找，无需遍历全部存储的指针
    /// StructOrBasicType::kBasic/kPointer只允许声明一种，防止歧义
    std::variant<
        std::monostate, std::shared_ptr<const TypeInterface>,
        std::unique_ptr<std::list<std::shared_ptr<const TypeInterface>>>>
        type_data_;
  };
  /// @brief 存储类型名到类型链容器的映射
  using TypeNodeContainerType = std::unordered_map<std::string, TypeData>;

 public:
  /// @brief 指向类型名和对应类型链的迭代器
  using TypeNodeContainerIter = TypeNodeContainerType::const_iterator;
  /// @brief 向类型系统中添加类型
  /// @param[in] type_name ：类型名
  /// @param[in] type_pointer ：类型链头结点指针
  /// @return 前半部分为指向插入位置的迭代器，后半部分为添加结果
  /// @note 添加类型的规则见TypeData::AddType的注释
  /// @ref TypeData::AddType
  template <class TypeName>
  std::pair<TypeNodeContainerType::const_iterator, AddTypeResult> DefineType(
      TypeName&& type_name,
      const std::shared_ptr<const TypeInterface>& type_pointer);
  /// @brief 声明函数类型
  /// @param[in] function_type ：函数类型链头结点
  /// @return 前半部分为指向插入位置的迭代器，后半部分为添加结果
  std::pair<TypeNodeContainerType::const_iterator, AddTypeResult>
  AnnounceFunctionType(
      const std::shared_ptr<const FunctionType>& function_type) {
    assert(function_type->GetType() == StructOrBasicType::kFunction);
    return DefineType(function_type->GetFunctionName(), function_type);
  }
  /// @brief 根据类型名和类型的选择倾向获取类型
  /// @param[in] type_name ：类型名
  /// @param[in] type_prefer ：类型选择倾向
  /// @return 前半部分为指向获取到的类型链头结点指针，后半部分为获取结果
  /// @note 添加类型的规则见TypeData::AddType的注释
  /// @ref TypeData::AddType
  std::pair<std::shared_ptr<const TypeInterface>, GetTypeResult> GetType(
      const std::string& type_name, StructOrBasicType type_prefer);

 private:
  /// @brief 获取全部类型名到类型链的映射
  /// @return 返回存储类型名到类型链映射容器的引用
  TypeNodeContainerType& GetTypeNameToNode() { return type_name_to_node_; }
  /// @brief 保存类型名到类型链的映射
  TypeNodeContainerType type_name_to_node_;
};

template <class TypeName>
inline std::pair<TypeSystem::TypeNodeContainerType::const_iterator,
                 AddTypeResult>
TypeSystem::DefineType(
    TypeName&& type_name,
    const std::shared_ptr<const TypeInterface>& type_pointer) {
  assert(type_pointer->GetType() != StructOrBasicType::kEnd);
  assert(type_pointer->GetType() != StructOrBasicType::kNotSpecified);
  auto [iter, inserted] = GetTypeNameToNode().emplace(
      std::forward<TypeName>(type_name), TypeData());
  return std::make_pair(iter, iter->second.AddType(type_pointer));
}

template <class MemberName>
inline StructureTypeInterface::StructureMemberContainer::MemberIndex
StructureTypeInterface::StructureMemberContainer::AddMember(
    MemberName&& member_name,
    const std::shared_ptr<const TypeInterface>& member_type,
    ConstTag member_const_tag) {
  auto [iter, inserted] = member_name_to_index_.emplace(
      std::forward<MemberName>(member_name), MemberIndex(members_.size()));
  // 已存在给定名字的成员
  if (!inserted) [[unlikely]] {
    return MemberIndex::InvalidId();
  } else {
    return iter->second;
  }
}

}  // namespace c_parser_frontend::type_system
#endif  /// !CPARSERFRONTEND_PARSE_CLASSES_H_