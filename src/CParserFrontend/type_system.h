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

namespace c_parser_frontend::flow_control {
// 前向声明FlowInterface，用于定义函数内执行的语句
// 前向声明FlowType，用于实现添加函数内执行的语句的相关功能
class FlowInterface;
enum class FlowType;
}  // namespace c_parser_frontend::flow_control

// 类型系统
namespace c_parser_frontend::type_system {
// const标记，必须保证当前声明顺序
// 可以将判断是否可以赋值时的两次if优化为一次switch
enum class ConstTag { kNonConst, kConst };
// 符号标记
enum class SignTag { kSigned, kUnsigned };

// 变量类型大类
enum class StructOrBasicType {
  kBasic,           // 基本类型
  kPointer,         // 指针（*）
  kFunction,        // 函数
  kStruct,          // 结构体
  kUnion,           // 共用体
  kEnum,            // 枚举
  kInitializeList,  // 初始化列表
  kEnd,             // 类型系统结尾，用于优化判断逻辑
  kNotSpecified  // 未确定需要的类型，使用标准类型查找规则，用于查找名称对应的值
};
// 内置类型，必须保证当前声明顺序，可以用于判断两种类型是否可以隐式转换
// void类型仅可声明为函数的返回类型，变量声明中必须声明为指针
// C语言没有正式支持bool，bool仅应出现在条件判断中
// kVoid仅在函数返回值中可以作为对象出现，作为变量类型时必须为指针
enum class BuiltInType {
  kInt1,     // bool
  kInt8,     // char
  kInt16,    // short
  kInt32,    // int,long
  kFloat32,  // float
  kFloat64,  // double
  kVoid      // void
};
// 检查是否可以赋值的结果
enum class AssignableCheckResult {
  // 可以赋值的情况
  kNonConvert,    // 两种类型相同，无需转换即可赋值
  kUpperConvert,  // 用来赋值的类型转换为待被赋值类型
  kConvertToVoidPointer,  // 被赋值的对象为void指针，赋值的对象为同等维数指针
  kZeroConvertToPointer,  // 0值转换为指针
  kUnsignedToSigned,  // 无符号值赋值给有符号值，建议使用警告
  kSignedToUnsigned,  // 有符号值赋值给无符号值，建议使用警告
  // 需要具体判断的情况
  kMayBeZeroToPointer,  // 如果是将0赋值给指针则合法，否则非法
  kInitializeList,  // 使用初始化列表，需要结合给定值具体判断
                    // 仅在CanBeAssignedBy中返回
  // 不可赋值的情况
  kLowerConvert,         // 用来赋值的类型发生缩窄转换
  kCanNotConvert,        // 不可转换
  kAssignedNodeIsConst,  // 被赋值对象为const
  kAssignToRightValue,  // 被赋值对象为右值，C语言不允许赋值给右值
  kArgumentsFull,       // 函数参数已满，不能添加更多参数
  kInitializeListTooLarge  // 初始化列表中给出的数据数目多于指针声明时大小
};
// 推断数学运算获取类型
enum class DeclineMathematicalComputeTypeResult {
  // 可以计算的情况
  kComputable,              // 可以计算
  kLeftPointerRightOffset,  // 左类型为指针，右类型为偏移量
  kLeftOffsetRightPointer,  // 左类型为偏移量，右类型为指针
  kConvertToLeft,           // 提升右类型到左类型后运算
  kConvertToRight,          // 提升左类型到右类型后运算
  // 不可以计算的情况
  kLeftNotComputableType,   // 左类型不是可以计算的类型
  kRightNotComputableType,  // 右类型不是可以计算的类型
  kLeftRightBothPointer,    // 左右类型均为指针
  kLeftNotIntger,  // 右类型是指针，左类型不是是整数不能作为偏移量使用
  kRightNotIntger  // 左类型是指针，右类型不是是整数不能作为偏移量使用
};
// 添加类型时返回的状态
enum class AddTypeResult {
  // 成功添加的情况
  kAbleToAdd,  // 可以添加，仅在CheckFunctionDefineAddResult中使用
               // 最终返回时被转换为更具体的值
  kNew,        // 以前不存在该类型名，比kFunctionDefine优先返回
  kFunctionDefine,  // 添加了函数定义
  kShiftToVector,  // 添加前该类型名对应一个类型，添加后转换为vector存储
  kAddToVector,  // 添加前该类型名对应至少2个类型
  kAnnounceOrDefineBeforeFunctionAnnounce,  // 函数声明前已有声明/定义
  // 添加失败的情况
  kTypeAlreadyIn,  // 待添加的类型所属大类已经有一种同名类型
  kRedefineFunction,  // 重定义函数
};
enum class GetTypeResult {
  // 成功匹配的情况
  kSuccess,  // 成功匹配
  // 匹配失败的情况
  kTypeNameNotFound,   // 类型名不存在
  kNoMatchTypePrefer,  // 不存在匹配类型选择倾向的类型链指针
  kSeveralSameLevelMatches  // 匹配到多个平级结果
};

// 定义指针类型大小
constexpr size_t kPointerSize = 4;
// 定义内置类型大小
constexpr size_t kBuiltInTypeSize[7] = {1, 1, 2, 4, 4, 8, 4};

// 根据给定数值计算存储所需的最小类型
// 不会推断出int1类型
// 返回BuiltInType::kVoid代表给定数值超出最大支持的范围
BuiltInType CalculateBuiltInType(const std::string& value);

// 前向声明指针变量类，用于TypeInterface::ObtainAddress生成指针类型
class PointerType;

using c_parser_frontend::flow_control::FlowInterface;
using c_parser_frontend::flow_control::FlowType;

// 变量类型基类
class TypeInterface {
 public:
  TypeInterface(StructOrBasicType type) : type_(type){};
  TypeInterface(StructOrBasicType type,
                const std::shared_ptr<const TypeInterface>& next_type_node)
      : type_(type), next_type_node_(next_type_node) {}
  virtual ~TypeInterface();

  // 比较两个类型是否完全相同，会自动调用下一级比较函数
  virtual bool operator==(const TypeInterface& type_interface) const {
    if (IsSameObject(type_interface)) [[likely]] {
      // 任何派生类都应先调用最低级继承类的IsSameObject而不是operator==()
      // 所有类型链最后都以StructOrBasicType::kEnd结尾
      // 对应的EndType类的operator==()直接返回true以终结调用链
      return GetNextNodeReference() == type_interface.GetNextNodeReference();
    } else {
      return false;
    }
  }
  // 判断给定类型是否可以给该对象赋值
  // 返回值的意义见其类型定义
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const = 0;
  // 获取存储该类型变量所需空间的大小（指针均为4字节）
  virtual size_t GetTypeStoreSize() const = 0;
  // 获取sizeof语义下类型的大小（指向数组的指针要用指针大小乘以各维度大小）
  virtual size_t TypeSizeOf() const = 0;

  // operator==()的子过程，仅比较该级类型，不会调用下一级比较函数
  // 派生类均应重写该函数，先调用基类的同名函数，后判断派生类新增部分
  // 从而实现为接受一个基类输入且可以安全输出结果的函数
  // 不使用虚函数可以提升性能
  // if分支多，建议内联
  bool IsSameObject(const TypeInterface& type_interface) const {
    // 一般情况下写代码，需要比较的类型都是相同的，出错远少于正确
    return GetType() == type_interface.GetType();
  }

  void SetType(StructOrBasicType type) { type_ = type; }
  StructOrBasicType GetType() const { return type_; }

  std::shared_ptr<const TypeInterface> GetNextNodePointer() const {
    return next_type_node_;
  }
  void SetNextNode(const std::shared_ptr<const TypeInterface>& next_type_node) {
    next_type_node_ = next_type_node;
  }
  // 不检查是否为空指针，在EndType中请勿调用
  const TypeInterface& GetNextNodeReference() const { return *next_type_node_; }

  // 获取取地址后得到的类型，不检查要取地址的类型
  static std::shared_ptr<const PointerType> ObtainAddressOperatorNode(
      const std::shared_ptr<const TypeInterface>& type_interface) {
    // 转除type_interface的const为了构建PointerType，不会修改该指针指向的内容
    return std::make_shared<PointerType>(
        ConstTag::kConst, 0,
        std::const_pointer_cast<TypeInterface>(type_interface));
  }
  // 推断数学运算后类型
  // 返回类型和推断的情况
  static std::pair<std::shared_ptr<const TypeInterface>,
                   DeclineMathematicalComputeTypeResult>
  DeclineMathematicalComputeResult(
      const std::shared_ptr<const TypeInterface>& left_compute_type,
      const std::shared_ptr<const TypeInterface>& right_compute_type);

 private:
  // 当前节点类型
  StructOrBasicType type_;
  // 指向下一个类型成分的指针，支持共用类型，节省内存
  // 必须使用shared_ptr或类似机制以支持共用一个类型的不同部分和部分释放
  std::shared_ptr<const TypeInterface> next_type_node_;
};

// 该函数提供比较容器内的节点指针的方法
// 用于FunctionType比较函数参数列表
bool operator==(const std::shared_ptr<TypeInterface>& type_interface1,
                const std::shared_ptr<TypeInterface>& type_interface2);
// 类型链尾部哨兵节点，避免比较类型是否相同时需要判断下一个节点指针是否为空
// 会设置下一个节点为nullptr
class EndType : public TypeInterface {
 public:
  EndType()
      : TypeInterface(StructOrBasicType::kEnd,
                      std::shared_ptr<TypeInterface>(nullptr)) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    // EndType中不会调用下一级类型节点的operator==()
    // 它自身是结尾，也没有下一级类型节点
    return IsSameObject(type_interface);
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override {
    return AssignableCheckResult::kNonConvert;
  }
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

  // 获取EndType节点指针，全局共用一个节点，节省内存
  static std::shared_ptr<EndType> GetEndType() {
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

// 预定义的基础类型
class BasicType : public TypeInterface {
 public:
  BasicType(BuiltInType built_in_type, SignTag sign_tag)
      : TypeInterface(StructOrBasicType::kBasic),
        built_in_type_(built_in_type),
        sign_tag_(sign_tag) {}
  BasicType(BuiltInType built_in_type, SignTag sign_tag,
            const std::shared_ptr<TypeInterface>& next_node)
      : TypeInterface(StructOrBasicType::kBasic, next_node),
        built_in_type_(built_in_type),
        sign_tag_(sign_tag) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() == type_interface.GetNextNodeReference();
    } else {
      return false;
    }
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override;
  virtual size_t GetTypeStoreSize() const override;
  virtual size_t TypeSizeOf() const override {
    return BasicType::GetTypeStoreSize();
  }

  void SetBuiltInType(BuiltInType built_in_type) {
    built_in_type_ = built_in_type;
  }
  BuiltInType GetBuiltInType() const { return built_in_type_; }
  void SetSignTag(SignTag sign_tag) { sign_tag_ = sign_tag; }
  SignTag GetSignTag() const { return sign_tag_; }

  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  BuiltInType built_in_type_;
  SignTag sign_tag_;
};

// 指针类型，一个*对应一个节点
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

  // 比较两个类型是否完全相同，会自动调用下一级比较函数
  virtual bool operator==(const TypeInterface& type_interface) const {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() == type_interface.GetNextNodeReference();
    } else {
      return false;
    }
  };
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override;
  virtual size_t GetTypeStoreSize() const override { return 4; }
  virtual size_t TypeSizeOf() const override;

  void SetConstTag(ConstTag const_tag) { variety_const_tag_ = const_tag; }
  ConstTag GetConstTag() const { return variety_const_tag_; }
  void SetArraySize(size_t array_size) { array_size_ = array_size; }
  size_t GetArraySize() const { return array_size_; }
  bool IsSameObject(const TypeInterface& type_interface) const;
  // 获取解引用得到的类型
  // 返回得到的对象是否为const和对象的类型
  std::pair<std::shared_ptr<const TypeInterface>, ConstTag> DeReference()
      const {
    return std::make_pair(GetNextNodePointer(), GetConstTag());
  }

 private:
  // const标记
  ConstTag variety_const_tag_;
  // 指针指向的数组对象个数
  // 0代表纯指针不指向数组
  // -1代表指针指向的数组大小待定（仅限声明）
  size_t array_size_;
};

// 函数类型
class FunctionType : public TypeInterface {
  // 存储参数信息
  struct ArgumentInfo {
    bool operator==(const ArgumentInfo& argument_info) const;

    // 指向变量名的指针不稳定，如果为函数声明则在声明结束后被清除
    // 如果为函数定义则在定义结束后被清除
    std::shared_ptr<const c_parser_frontend::operator_node::VarietyOperatorNode>
        variety_operator_node;
  };
  // 存储参数信息的容器
  using ArgumentInfoContainer = std::vector<ArgumentInfo>;

 public:
  FunctionType(const std::string* function_name)
      : TypeInterface(StructOrBasicType::kFunction, EndType::GetEndType()),
        function_name_(function_name) {}
  template <class ArgumentContainter>
  FunctionType(const std::string* function_name,
               ArgumentContainter&& argument_infos)
      : TypeInterface(StructOrBasicType::kFunction, EndType::GetEndType()),
        function_name_(function_name),
        argument_infos_(std::forward<ArgumentContainter>(argument_infos)) {}
  template <class ArgumentContainter>
  FunctionType(const std::string* function_name, ConstTag return_type_const_tag,
               const std::shared_ptr<const TypeInterface>& return_type,
               ArgumentContainter&& argument_infos)
      : TypeInterface(StructOrBasicType::kFunction, EndType::GetEndType()),
        function_name_(function_name),
        return_type_const_tag_(return_type_const_tag),
        return_type_(return_type),
        argument_infos_(std::forward<ArgumentContainter>(argument_infos)) {}
  ~FunctionType();

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
    assert(false);
    // 防止警告
    return size_t();
  }
  virtual size_t TypeSizeOf() const override {
    assert(false);
    // 防止警告
    return size_t();
  }

  // 返回是否为函数声明（函数声明中不存在任何流程控制语句）
  bool IsFunctionAnnounce() const;
  // 只检查函数签名是否相同，不检查函数名和函数内执行的语句
  bool IsSameSignature(const FunctionType& function_type) const {
    return GetFunctionName() == function_type.GetFunctionName() &&
           GetArguments() == function_type.GetArguments() &&
           GetReturnTypeReference() == function_type.GetReturnTypeReference();
  }
  // 不比较函数内执行的语句
  bool IsSameObject(const TypeInterface& type_interface) const;

  void SetReturnTypePointer(
      const std::shared_ptr<const TypeInterface>& return_type) {
    return_type_ = return_type;
  }
  std::shared_ptr<const TypeInterface> GetReturnTypePointer() const {
    return return_type_;
  }
  const TypeInterface& GetReturnTypeReference() const { return *return_type_; }
  void SetReturnTypeConstTag(ConstTag return_type_const_tag) {
    return_type_const_tag_ = return_type_const_tag;
  }
  ConstTag GetReturnTypeConstTag() const { return return_type_const_tag_; }
  // 返回是否添加成功，如果不成功则不添加
  void AddFunctionCallArgument(
      const std::shared_ptr<
          const c_parser_frontend::operator_node::VarietyOperatorNode>&
          argument);
  const ArgumentInfoContainer& GetArguments() const { return argument_infos_; }
  void SetFunctionName(const std::string* function_name) {
    function_name_ = function_name;
  }
  const std::string& GetFunctionName() const { return *function_name_; }
  const std::list<std::unique_ptr<FlowInterface>>& GetSentences() const;
  // 添加一条函数内执行的语句（按出现顺序添加）
  // 成功添加返回true，不返还控制权
  // 如果不能添加则返回false，不修改入参
  bool AddSentence(std::unique_ptr<FlowInterface>&& sentence_to_add);
  // 添加一个容器内的全部语句（按照给定迭代器begin->end顺序添加）
  // 成功添加后作为参数的容器空
  // 如果有返回false的CheckSentenceInFunctionValid结果则返回false且不修改参数
  // 成功添加则返回true
  bool AddSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentence_container);
  // 获取函数的一级函数指针形式
  // 返回shared_ptr指向一个指针节点，这个指针节点指向该函数
  static std::shared_ptr<const PointerType> ConvertToFunctionPointer(
      const std::shared_ptr<const TypeInterface>& function_type) {
    assert(function_type->GetType() == StructOrBasicType::kFunction);
    // 转除const为了可以构造指针节点，不会修改function_type
    return std::make_shared<const PointerType>(
        ConstTag::kConst, 0,
        std::const_pointer_cast<TypeInterface>(function_type));
  }
  // 检查给定语句是否可以作为函数内出现的语句
  static bool CheckSentenceInFunctionValid(const FlowInterface& flow_interface);

 private:
  // 函数名
  // 作为函数指针的节点出现时函数名置nullptr
  const std::string* function_name_;
  // 返回值的const标记
  ConstTag return_type_const_tag_;
  // 函数返回类型
  std::shared_ptr<const TypeInterface> return_type_;
  // 参数类型和参数名
  ArgumentInfoContainer argument_infos_;
  // 函数内执行的语句
  std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>
      sentences_in_function_;
};

// 结构化数据的基类
class StructureTypeInterface : public TypeInterface {
 protected:
  // 键值为成员名，值前半部分为成员类型，后半部分为成员const标记
  class StructureMemberContainer {
    enum class IdWrapper { kMemberIndex };

   public:
    using MemberIndex =
        frontend::common::ExplicitIdWrapper<size_t, IdWrapper,
                                            IdWrapper::kMemberIndex>;
    // 获取成员index
    // 返回MemberIndex::InvalidId()代表不存在该成员
    MemberIndex GetMemberIndex(const std::string& member_name) const;
    const auto& GetMemberInfo(MemberIndex member_index) const {
      assert(member_index.IsValid());
      return members_[member_index];
    }
    // 返回MemberIndex，如果待添加成员名已存在则返回MemberIndex::InvalidId()
    template <class MemberName>
    MemberIndex AddMember(
        MemberName&& member_name,
        const std::shared_ptr<const TypeInterface>& member_type,
        ConstTag member_const_tag);
    const auto& GetMembers() const { return members_; }

   private:
    // 成员名到index的映射
    std::unordered_map<std::string, MemberIndex> member_name_to_index_;
    // 结构化数据的成员
    std::vector<std::pair<std::shared_ptr<const TypeInterface>, ConstTag>>
        members_;
  };
  using StructureMemberIndex = StructureMemberContainer::MemberIndex;

 public:
  using MemberIndex = StructureMemberContainer::MemberIndex;

  StructureTypeInterface(const std::string* structure_name,
                         StructOrBasicType structure_type)
      : TypeInterface(structure_type, EndType::GetEndType()),
        structure_name_(structure_name) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() ==
             static_cast<const StructureTypeInterface&>(type_interface)
                 .GetNextNodeReference();
    } else {
      return false;
    }
  }

  const std::string* GetStructureName() const { return structure_name_; }
  void SetStructureName(const std::string* structure_name) {
    structure_name_ = structure_name;
  }

  void SetStructureMembers(StructureMemberContainer&& structure_members) {
    structure_member_container_ = structure_members;
  }
  // 获取成员信息容器
  const auto& GetStructureMemberContainer() const {
    return structure_member_container_;
  }
  // 如果成员已存在则返回StructureMemberIndex::InvalidId()
  template <class MemberName>
  StructureMemberIndex AddStructureMember(
      MemberName&& member_name,
      const std::shared_ptr<const TypeInterface>& member_type,
      ConstTag member_const_tag) {
    return GetStructureMemberContainer().AddMember(
        std::forward<MemberName>(member_name), member_type, member_const_tag);
  }
  // 获取结构化数据内成员的index，如果给定成员名不存在则返回
  // StructMemberIndex::InvalidId()
  StructureMemberIndex GetStructureMemberIndex(
      const std::string& member_name) const {
    return GetStructureMemberContainer().GetMemberIndex(member_name);
  }
  const auto& GetStructureMemberInfo(StructureMemberIndex member_index) const {
    assert(member_index.IsValid());
    return GetStructureMemberContainer().GetMemberInfo(member_index);
  }
  // 获取底层存储成员信息的结构，可以用于遍历
  const auto& GetStructureMembers() const {
    return GetStructureMemberContainer().GetMembers();
  }
  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  StructureMemberContainer& GetStructureMemberContainer() {
    return structure_member_container_;
  }
  // 结构化数据的类型名
  const std::string* structure_name_;
  // 结构化数据容器
  StructureMemberContainer structure_member_container_;
};

class StructType : public StructureTypeInterface {
 public:
  // 结构体存储成员的结构类型
  // 更改请注意：reduct_function为了实现多态在构建struct和union时使用了大量的
  // StructureMemberContainerType而不是StructMemberContainerType
  using StructMemberContainerType = StructureMemberContainer;
  using StructMemberIndex = StructMemberContainerType::MemberIndex;

  StructType(const std::string* struct_name)
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

  template <class StructMembers>
  void SetStructMembers(StructMembers&& member_datas) {
    SetStructureMembers(std::forward<StructMembers>(member_datas));
  }
  // 返回Index，如果给定数据已存在则返回
  template <class MemberName>
  StructMemberIndex AddStructMember(
      MemberName&& member_name,
      const std::shared_ptr<const TypeInterface>& member_type_pointer,
      ConstTag member_const_tag) {
    return AddStructureMember(std::forward<MemberName>(member_name),
                              member_type_pointer, member_const_tag);
  }
  // 获取结构体内成员的index，如果给定成员名不存在则返回
  // StructMemberIndex::InvalidId()
  StructMemberIndex GetStructMemberIndex(const std::string& member_name) const {
    return GetStructureMemberIndex(member_name);
  }
  const auto& GetStructMemberInfo(StructMemberIndex member_index) const {
    assert(member_index.IsValid());
    return GetStructureMemberInfo(member_index);
  }
  void SetStructName(const std::string* struct_name) {
    SetStructureName(struct_name);
  }
  const std::string* GetStructName() const { return GetStructureName(); }

  bool IsSameObject(const TypeInterface& type_interface) const {
    return StructureTypeInterface::IsSameObject(type_interface);
  }
};

class UnionType : public StructureTypeInterface {
 public:
  // 共用体存储成员的结构类型
  // 更改请注意：reduct_function为了实现多态在构建struct和union时使用了大量的
  // StructureMemberContainerType而不是UnionMemberContainerType
  using UnionMemberContainerType = StructureMemberContainer;
  using UnionMemberIndex = UnionMemberContainerType::MemberIndex;

  UnionType(const std::string* union_name)
      : StructureTypeInterface(union_name, StructOrBasicType::kUnion) {}

  virtual bool operator==(const TypeInterface& type_interface) {
    return StructureTypeInterface::operator==(type_interface);
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override;
  virtual size_t GetTypeStoreSize() const override;
  virtual size_t TypeSizeOf() const override {
    return UnionType::GetTypeStoreSize();
  }

  template <class UnionMembers>
  void SetUnionMembers(UnionMembers&& union_members) {
    SetStructureMembers(std::forward<UnionMembers>(union_members));
  }
  // 返回指向数据的迭代器和是否成功插入
  // 当已存在该名字时不插入并返回false
  template <class MemberName>
  auto AddUnionMember(
      MemberName&& member_name,
      const std::shared_ptr<const TypeInterface>& member_type_pointer,
      ConstTag member_const_tag) {
    return AddStructureMember(std::forward<MemberName>(member_name),
                              member_type_pointer, member_const_tag);
  }
  // 获取共用体内成员index，如果成员名不存在则返回UnionMemberIndex::InvalidId()
  UnionMemberIndex GetUnionMemberIndex(const std::string& member_name) const {
    return GetStructureMemberIndex(member_name);
  }
  const auto& GetUnionMemberInfo(UnionMemberIndex member_index) const {
    assert(member_index.IsValid());
    return GetStructureMemberInfo(member_index);
  }
  void SetUnionName(const std::string* union_name) {
    SetStructureName(union_name);
  }
  const std::string* GetUnionName() const { return GetStructureName(); }
  bool IsSameObject(const TypeInterface& type_interface) const {
    return StructureTypeInterface::IsSameObject(type_interface);
  }
};

class EnumType : public TypeInterface {
 public:
  // 枚举内部存储结构
  using EnumContainerType = std::unordered_map<std::string, long long>;

  EnumType(const std::string* enum_name)
      : TypeInterface(StructOrBasicType::kEnum, EndType::GetEndType()),
        enum_name_(enum_name) {}
  template <class StructMembers>
  EnumType(const std::string* enum_name, StructMembers&& enum_members)
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

  template <class StructMembers>
  void SetEnumMembers(StructMembers&& enum_members) {
    enum_members_ = std::forward<StructMembers>(enum_members);
  }
  // 返回指向数据的迭代器和是否成功插入
  // 当已存在该名字时不插入并返回false
  template <class EnumMemberName>
  auto AddEnumMember(EnumMemberName&& enum_member_name, long long value) {
    return enum_members_.emplace(
        std::make_pair(std::forward<EnumMemberName>(enum_member_name), value));
  }
  // 获取枚举内成员信息，如果成员名不存在则返回false
  std::pair<EnumContainerType::const_iterator, bool> GetEnumMemberInfo(
      const std::string& member_name) const {
    auto iter = GetEnumMembers().find(member_name);
    return std::make_pair(iter, iter != GetEnumMembers().end());
  }
  std::shared_ptr<const TypeInterface> GetContainerTypePointer() const {
    return container_type_;
  }
  const TypeInterface& GetContainerTypeReference() const {
    return *container_type_;
  }
  void SetEnumName(const std::string* enum_name) { enum_name_ = enum_name; }
  const std::string* GetEnumName() const { return enum_name_; }
  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  const EnumContainerType& GetEnumMembers() const { return enum_members_; }
  EnumContainerType& GetEnumMembers() { return enum_members_; }
  void SetContainerType(
      const std::shared_ptr<const TypeInterface>& container_type) {
    container_type_ = container_type;
  }
  // 枚举名
  const std::string* enum_name_;
  // 前半部分为成员名，后半部分为枚举对应的值
  EnumContainerType enum_members_;
  // 存储枚举值用的类型
  std::shared_ptr<const TypeInterface> container_type_;
};

class InitializeListType : public TypeInterface {
 public:
  // 储存初始化信息用的列表类型
  using InitializeListContainerType =
      std::vector<std::shared_ptr<const TypeInterface>>;

  InitializeListType()
      : TypeInterface(StructOrBasicType::kInitializeList,
                      EndType::GetEndType()) {}
  template <class MemberType>
  InitializeListType(MemberType&& list_types)
      : list_types_(std::forward<MemberType>(list_types)) {}

  virtual bool operator==(const TypeInterface& type_interface) {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() == type_interface.GetNextNodeReference();
    } else {
      return false;
    }
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      const TypeInterface& type_interface) const override {
    // 初始化列表不能被赋值
    return AssignableCheckResult::kCanNotConvert;
  }
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

  void AddListType(const std::shared_ptr<const TypeInterface>& type_pointer) {
    list_types_.emplace_back(type_pointer);
  }
  const auto& GetListTypes() const { return list_types_; }
  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  InitializeListContainerType list_types_;
};

// 常用类型节点的生成器，全局共享一份节点以节省内存和避免多次分配的消耗
class CommonlyUsedTypeGenerator {
 public:
  // 获取基础类型
  // void类型和bool类型使用SignTag::kUnsigned
  template <BuiltInType built_in_type, SignTag sign_tag>
  static std::shared_ptr<BasicType> GetBasicType() {
    thread_local static std::shared_ptr<BasicType> basic_type =
        std::make_shared<BasicType>(built_in_type, sign_tag,
                                    EndType::GetEndType());
    return basic_type;
  }

  static std::shared_ptr<BasicType> GetBasicTypeNotTemplate(
      BuiltInType built_in_type, SignTag sign_tag);

  // 获取初始化用字符串类型（const char*）
  static std::shared_ptr<PointerType> GetConstExprStringType() {
    thread_local static std::shared_ptr<PointerType> constexpr_string_type =
        std::make_shared<PointerType>(
            ConstTag::kConst, 0,
            std::move(GetBasicType<BuiltInType::kInt8, SignTag::kSigned>()));
    return constexpr_string_type;
  }
};

class TypeSystem {
  // 存储同一个名字下的不同变量信息
  class TypeData {
   public:
    // 添加一个类型，自动处理存储结构的转换
    // 多次添加无待执行语句的函数类型为合法操作
    // 模板参数在函数声明/定义时需要设定
    template <bool is_function_announce, bool is_function_define>
    requires(is_function_announce == false ||
             is_function_define == false) AddTypeResult
        AddType(const std::shared_ptr<const TypeInterface>& type_to_add);
    // 根据类型偏好获取类型
    std::pair<std::shared_ptr<const TypeInterface>, GetTypeResult> GetType(
        StructOrBasicType type_prefer) const;
    // 返回容器是否为空
    bool Empty() const {
      return std::get_if<std::monostate>(&type_data_) != nullptr;
    }

    // 检查两种类型是否属于同一大类
    // StructOrBasicType::kBasic和StructOrBasicType::kPointer属于同一大类
    // 除此以外类型单独成一大类
    static bool IsSameKind(StructOrBasicType type1, StructOrBasicType type2);
    // 检查已存在同名函数声明条件下是否可以添加函数声明/定义的具体情况
    // 此情况下一定不可以添加函数声明
    template <bool is_function_announce, bool is_function_define>
    requires(is_function_announce == false ||
             is_function_define == false) static AddTypeResult
        CheckFunctionDefineAddResult(const FunctionType& function_type_exist,
                                     const FunctionType& function_type_to_add);

   private:
    // 初始构建时为空，添加类型指针后根据添加的数量使用直接存储或vector存储
    // 将StructOrBasicType::kBasic/kPointer类型的指针放到最前面
    // 在无类型偏好时加速查找，无需遍历全部存储的指针
    // 指针类型与基础类型只允许声明一种，防止歧义
    std::variant<
        std::monostate, std::shared_ptr<const TypeInterface>,
        std::unique_ptr<std::vector<std::shared_ptr<const TypeInterface>>>>
        type_data_;
  };
  using TypeNodeContainerType = std::unordered_map<std::string, TypeData>;

 public:
  using TypeNodeContainerIter = TypeNodeContainerType::const_iterator;
  // 向类型系统中添加类型
  // type_name是类型名，type_pointer是指向类型链的指针
  // 返回指向插入位置的迭代器和插入结果
  // 如果给定类型名下待添加的类型链所属的大类已存在则添加失败，返回false
  // 大类见StructOrBasicType，不含kEnd和kNotSpecified
  template <bool is_function_announce = false, bool is_function_define = false,
            class TypeName>
  requires(is_function_announce == false || is_function_define == false) std::
      pair<TypeNodeContainerType::const_iterator, AddTypeResult> DefineType(
          TypeName&& type_name,
          const std::shared_ptr<const TypeInterface>& type_pointer);
  // 声明函数类型使用该接口
  std::pair<TypeNodeContainerType::const_iterator, AddTypeResult>
  AnnounceFunctionType(
      const std::shared_ptr<const FunctionType>& function_type) {
    return DefineType<true, false>(function_type->GetFunctionName(),
                                   function_type);
  }
  // 定义函数类型使用该接口
  std::pair<TypeNodeContainerType::const_iterator, AddTypeResult>
  DefineFunctionType(const std::shared_ptr<const FunctionType>& function_type) {
    return DefineType<false, true>(function_type->GetFunctionName(),
                                   function_type);
  }

  // 声明类型，返回指向该名字对应类型集合的迭代器
  // 保证迭代器有效且类型集合被创建
  template <class TypeName>
  TypeNodeContainerType::const_iterator AnnounceTypeName(TypeName&& type_name);
  // 根据给定类型名和选择类型的倾向获取类型
  // 类型选择倾向除了StructOrBasicType::kEnd均可选择
  // 类型选择倾向为StructOrBasicType::kNotSpecified时优先匹配kBasic
  // 其它类型平级，存在多个平级类型时无法获取类型链指针
  // 返回类型链指针和获取情况，获取情况的解释见其定义
  std::pair<std::shared_ptr<const TypeInterface>, GetTypeResult> GetType(
      const std::string& type_name, StructOrBasicType type_prefer);
  // 移除空节点，如果节点不空则不移除
  // 返回是否移除了节点
  // 用于只announce了变量名，最后没有添加变量的情况（函数指针声明过程）
  bool RemoveEmptyNode(const std::string& empty_node_to_remove_name);

 private:
  auto& GetTypeNameToNode() { return type_name_to_node_; }
  // 保存类型名到类型链的映射
  // 匿名结构不保存在这里，由该结构声明的变量维护类型链
  // 当存储多个同名类型时值将转换为指向vector的指针，通过vector保存同名类型
  // 使用vector保存时如果存在则应将StructOrBasicType::kBasic类型放在第一个位置
  // 可以避免查找全部指针，加速标准类型查找规则查找速度
  TypeNodeContainerType type_name_to_node_;
};

template <bool is_function_announce, bool is_function_define, class TypeName>
requires(is_function_announce == false ||
         is_function_define == false) inline std::
    pair<TypeSystem::TypeNodeContainerType::const_iterator,
         AddTypeResult> TypeSystem::
        DefineType(TypeName&& type_name,
                   const std::shared_ptr<const TypeInterface>& type_pointer) {
  assert(type_pointer->GetType() != StructOrBasicType::kEnd);
  assert(type_pointer->GetType() != StructOrBasicType::kNotSpecified);
  auto [iter, inserted] = GetTypeNameToNode().emplace(
      std::forward<TypeName>(type_name), TypeData());
  return std::make_pair(
      iter, iter->second.AddType<is_function_announce, is_function_define>(
                type_pointer));
}

template <class TypeName>
inline TypeSystem::TypeNodeContainerType::const_iterator
TypeSystem::AnnounceTypeName(TypeName&& type_name) {
  return GetTypeNameToNode()
      .emplace(std::forward<TypeName>(type_name), TypeData())
      .first;
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
template <bool is_function_announce, bool is_function_define>
requires(is_function_announce == false || is_function_define == false)
    AddTypeResult TypeSystem::TypeData::AddType(
        const std::shared_ptr<const TypeInterface>& type_to_add) {
  std::monostate* empty_status_pointer =
      std::get_if<std::monostate>(&type_data_);
  if (empty_status_pointer != nullptr) [[likely]] {
    // 该节点未保存任何指针
    // 存入第一个指针
    type_data_ = type_to_add;
    return AddTypeResult::kNew;
  } else {
    std::shared_ptr<const TypeInterface>* shared_pointer =
        std::get_if<std::shared_ptr<const TypeInterface>>(&type_data_);
    if (shared_pointer != nullptr) [[likely]] {
      // 该节点保存了一个指针，新增一个后转为vector存储
      // 检查要添加的类型是否与已有类型重复或者重复添加kPointer/kBasic一类
      if (IsSameKind((*shared_pointer)->GetType(), type_to_add->GetType()))
          [[unlikely]] {
        // 检查是否在添加函数类型
        if (type_to_add->GetType() == StructOrBasicType::kFunction)
            [[unlikely]] {
          AddTypeResult function_define_add_result =
              CheckFunctionDefineAddResult<is_function_announce,
                                           is_function_define>(
                  static_cast<const FunctionType&>(**shared_pointer),
                  static_cast<const FunctionType&>(*type_to_add));
          if (function_define_add_result == AddTypeResult::kAbleToAdd)
              [[likely]] {
            // 使用新的指针替换原有指针，从而更新函数参数名和函数内存储的语句
            *shared_pointer = type_to_add;
            function_define_add_result = AddTypeResult::kFunctionDefine;
          }
          return function_define_add_result;
        } else {
          // 待添加的类型与已有类型相同，产生冲突
          return AddTypeResult::kTypeAlreadyIn;
        }
      }
      auto vector_pointer =
          std::make_unique<std::vector<std::shared_ptr<const TypeInterface>>>();
      // 将StructOrBasicType::kBasic或StructOrBasicType::kPointer类型放在最前面
      // 从而优化查找类型的速度
      if (IsSameKind(type_to_add->GetType(), StructOrBasicType::kBasic)) {
        vector_pointer->emplace_back(type_to_add);
        vector_pointer->emplace_back(std::move(*shared_pointer));
      } else {
        // 原来的类型不一定属于该大类，但新添加的类型一定不属于该大类
        vector_pointer->emplace_back(std::move(*shared_pointer));
        vector_pointer->emplace_back(type_to_add);
      }
      type_data_ = std::move(vector_pointer);
      return AddTypeResult::kShiftToVector;
    } else {
      // 该节点已经使用vector存储
      std::unique_ptr<std::vector<std::shared_ptr<const TypeInterface>>>&
          vector_pointer = *std::get_if<std::unique_ptr<
              std::vector<std::shared_ptr<const TypeInterface>>>>(&type_data_);
      assert(vector_pointer != nullptr);
      // 检查待添加的类型与已存在的类型是否属于同一大类
      for (auto& stored_pointer : *vector_pointer) {
        if (IsSameKind(stored_pointer->GetType(), type_to_add->GetType()))
            [[unlikely]] {
          // 检查是否在添加函数类型
          if (type_to_add->GetType() == StructOrBasicType::kFunction)
              [[unlikely]] {
            AddTypeResult function_define_add_result =
                CheckFunctionDefineAddResult<is_function_announce,
                                             is_function_define>(
                    static_cast<const FunctionType&>(*stored_pointer),
                    static_cast<const FunctionType&>(*type_to_add));
            if (function_define_add_result == AddTypeResult::kAbleToAdd)
                [[likely]] {
              // 这种情况为在函数定义前已添加函数声明
              // 使用新的指针替换原有指针
              stored_pointer = type_to_add;
              return AddTypeResult::kFunctionDefine;
            }
            return function_define_add_result;
          }
          // 待添加的类型与已有类型相同，产生冲突
          return AddTypeResult::kTypeAlreadyIn;
        }
      }
      vector_pointer->emplace_back(type_to_add);
      if (IsSameKind(type_to_add->GetType(), StructOrBasicType::kBasic)) {
        // 新插入的类型为StructOrBasicType::kBasic/kPointer
        // 将该类型放到第一个位置以便优化无类型倾向性时的查找逻辑
        std::swap(vector_pointer->front(), vector_pointer->back());
      }
      return AddTypeResult::kAddToVector;
    }
  }
}

// 检查已存在同名函数声明条件下是否可以添加函数声明/定义的具体情况
// 此情况下一定不可以添加函数声明

template <bool is_function_announce, bool is_function_define>
requires(is_function_announce == false || is_function_define == false)
    AddTypeResult TypeSystem::TypeData::CheckFunctionDefineAddResult(
        const FunctionType& function_type_exist,
        const FunctionType& function_type_to_add) {
  if constexpr (is_function_announce) {
    // 已经存在同名函数定义/声明
    assert(function_type_to_add.IsFunctionAnnounce());
    if (function_type_exist.IsSameSignature(function_type_to_add)) {
      return AddTypeResult::kAnnounceOrDefineBeforeFunctionAnnounce;
    } else {
      return AddTypeResult::kTypeAlreadyIn;
    }
  } else if constexpr (is_function_define) {
    assert(!function_type_to_add.IsFunctionAnnounce());
    // 检查待添加函数与已存在的函数声明是否为相同签名
    if (function_type_exist.IsSameSignature(function_type_to_add)) [[likely]] {
      // 如果之前存在的类型为函数声明则可以覆盖函数声明
      if (function_type_exist.IsFunctionAnnounce()) [[likely]] {
        return AddTypeResult::kAbleToAdd;
      } else {
        // 之前已存在函数定义，返回重定义错误
        return AddTypeResult::kRedefineFunction;
      }
    } else {
      // 之前存在签名不同的函数声明
      return AddTypeResult::kTypeAlreadyIn;
    }
  }
  assert(false);
  // 防止警告
  return AddTypeResult();
}

}  // namespace c_parser_frontend::type_system
#endif  // !CPARSERFRONTEND_PARSE_CLASSES_H_