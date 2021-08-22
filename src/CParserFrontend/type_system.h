#ifndef CPARSERFRONTEND_TYPE_SYSTEM_H_
#define CPARSERFRONTEND_TYPE_SYSTEM_H_

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

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
  kVoid,
  kBool,
  kChar,
  kShort,
  kInt,
  kLong,
  kFloat,
  kDouble
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
  // 不可赋值的情况
  kLowerConvert,         // 用来赋值的类型发生缩窄转换
  kCanNotConvert,        // 不可转换
  kAssignedNodeIsConst,  // 被赋值对象为const
  kAssignToRightValue,  // 被赋值对象为右值，C语言不允许赋值给右值
  kArgumentsFull        // 函数参数已满，不能添加更多参数
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
// 前向声明指针变量类，用于TypeInterface::ObtainAddress生成指针类型
class PointerType;
// 变量类型基类
class TypeInterface {
 public:
  TypeInterface(StructOrBasicType type) : type_(type){};
  TypeInterface(StructOrBasicType type,
                std::shared_ptr<TypeInterface>&& next_type_node)
      : type_(type), next_type_node_(std::move(next_type_node)) {}
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
      std::shared_ptr<const TypeInterface>&& type_interface) const = 0;

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
  std::shared_ptr<TypeInterface> GetNextNodePointer() {
    return next_type_node_;
  }
  void SetNextNodePointer(std::shared_ptr<TypeInterface>&& next_type_node) {
    next_type_node_ = std::move(next_type_node);
  }
  // 不检查是否为空指针，在EndType中请勿调用
  const TypeInterface& GetNextNodeReference() const { return *next_type_node_; }
  TypeInterface& GetNextNodeReference() { return *next_type_node_; }

  // 获取取地址后得到的类型，不检查要取地址的类型
  static std::shared_ptr<const PointerType> ObtainAddress(
      std::shared_ptr<const TypeInterface>&& type_interface) {
    // 转除type_interface为了构建PointerType，不会修改该指针指向的内容
    return std::make_shared<const PointerType>(
        ConstTag::kConst,
        std::const_pointer_cast<TypeInterface>(type_interface), 0);
  }
  // 推断数学运算后类型
  // 返回类型和推断的情况
  static std::pair<std::shared_ptr<const TypeInterface>,
                   DeclineMathematicalComputeTypeResult>
  DeclineMathematicalComputeResult(
      std::shared_ptr<const TypeInterface>&& left_compute_type,
      std::shared_ptr<const TypeInterface>&& right_compute_type);

 private:
  // 当前节点类型
  StructOrBasicType type_;
  // 指向下一个类型成分的指针，支持共用类型，节省内存
  // 必须使用shared_ptr或类似机制以支持共用一个类型的不同部分和部分释放
  std::shared_ptr<TypeInterface> next_type_node_;
};

// 该函数提供比较容器内的节点指针的方法
// 用于FunctionType比较函数参数列表
bool operator==(const std::shared_ptr<TypeInterface>& type_interface1,
                const std::shared_ptr<TypeInterface>& type_interface2);

// 预定义的基础类型
class BasicType : public TypeInterface {
 public:
  BasicType(BuiltInType built_in_type, SignTag sign_tag)
      : TypeInterface(StructOrBasicType::kBasic),
        built_in_type_(built_in_type),
        sign_tag_(sign_tag) {}
  BasicType(BuiltInType built_in_type, SignTag sign_tag,
            std::shared_ptr<TypeInterface>&& next_type_node)
      : TypeInterface(StructOrBasicType::kBasic, std::move(next_type_node)),
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
      std::shared_ptr<const TypeInterface>&& type_interface) const override;

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
  PointerType(ConstTag const_tag,
              std::shared_ptr<TypeInterface>&& next_type_node,
              size_t array_size)
      : TypeInterface(StructOrBasicType::kPointer, std::move(next_type_node)),
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
      std::shared_ptr<const TypeInterface>&& type_interface) const override;

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
  // 指针指向的数组对象个数，置0代表纯指针不指向数组
  size_t array_size_;
};

// 函数类型
class FunctionType : public TypeInterface {
 public:
  // 存储参数信息
  struct ArgumentInfo {
    bool operator==(const ArgumentInfo& argument_info) const {
      // 函数参数名不影响是否为同一参数，仅比较类型和是否为const
      return argument_const_tag == argument_info.argument_const_tag &&
             *argument_type == *argument_info.argument_type;
    }
    const std::string* argument_name;
    ConstTag argument_const_tag;
    std::shared_ptr<const TypeInterface> argument_type;
  };

  template <class FunctionName, class ReturnType>
  FunctionType(FunctionName&& function_name, ConstTag return_type_const_tag,
               ReturnType&& return_type)
      : TypeInterface(StructOrBasicType::kFunction),
        function_name_(std::forward<FunctionName>(function_name)),
        return_type_const_tag_(return_type_const_tag),
        return_type_(std::forward<ReturnType>(return_type)) {}
  template <class FunctionName, class ReturnType>
  FunctionType(FunctionName&& function_name, ConstTag return_type_const_tag,
               ReturnType&& return_type,
               std::shared_ptr<TypeInterface>&& next_node_pointer)
      : TypeInterface(StructOrBasicType::kFunction,
                      std::move(next_node_pointer)),
        function_name_(std::forward<FunctionName>(function_name)),
        return_type_const_tag_(return_type_const_tag),
        return_type_(std::forward<ReturnType>(return_type)) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() == type_interface.GetNextNodeReference();
    } else {
      return false;
    }
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      std::shared_ptr<const TypeInterface>&& type_interface) const override {
    // 函数不能被赋值
    return AssignableCheckResult::kCanNotConvert;
  }

  bool IsSameObject(const TypeInterface& type_interface) const;

  template <class ReturnTypePointer>
  void SetReturnTypePointer(ReturnTypePointer&& return_type) {
    return_type_ = std::forward<ReturnTypePointer>(return_type);
  }
  std::shared_ptr<const TypeInterface> GetReturnTypePointer() const {
    return return_type_;
  }
  const TypeInterface& GetReturnTypeReference() const { return *return_type_; }
  void SetReturnTypeConstTag(ConstTag return_type_const_tag) {
    return_type_const_tag_ = return_type_const_tag;
  }
  ConstTag GetReturnTypeConstTag() const { return return_type_const_tag_; }
  void AddArgumentNameAndType(
      const std::string* argument_name, ConstTag const_tag,
      std::shared_ptr<const TypeInterface>&& argument_type) {
    argument_infos_.emplace_back(
        ArgumentInfo{.argument_name = argument_name,
                     .argument_const_tag = const_tag,
                     .argument_type = std::move(argument_type)});
  }
  const auto& GetArgumentTypes() const { return argument_infos_; }
  template <class FunctionName>
  void SetFunctionName(FunctionName&& function_name) {
    function_name_ = function_name;
  }
  const std::string& GetFunctionName() const { return function_name_; }
  // 获取函数的一级函数指针形式
  // 返回shared_ptr指向一个指针节点，这个指针节点指向该函数
  static std::shared_ptr<const PointerType> ConvertToFunctionPointer(
      std::shared_ptr<const TypeInterface>&& function_type) {
    assert(function_type->GetType() == StructOrBasicType::kFunction);
    // 转除const为了可以构造指针节点，不会修改function_type
    return std::make_shared<const PointerType>(
        ConstTag::kConst, std::const_pointer_cast<TypeInterface>(function_type),
        0);
  }

 private:
  // 函数名
  // 作为函数指针的节点出现时函数名置空
  std::string function_name_;
  // 返回值的const标记
  ConstTag return_type_const_tag_;
  // 函数返回类型
  std::shared_ptr<const TypeInterface> return_type_;
  // 参数类型和参数名
  std::vector<ArgumentInfo> argument_infos_;
};

class StructType : public TypeInterface {
 public:
  // 结构体的内部存储类型
  // ConstTag标识成员的const性
  using StructContainerType = std::unordered_map<
      std::string, std::pair<ConstTag, std::shared_ptr<const TypeInterface>>>;

  template <class EnumMembers>
  StructType(const std::string* struct_name, EnumMembers&& members)
      : TypeInterface(StructOrBasicType::kStruct),
        struct_name_(struct_name),
        struct_members_(std::forward<EnumMembers>(members)) {}
  template <class EnumMembers>
  StructType(const std::string* struct_name, EnumMembers&& members,
             std::shared_ptr<TypeInterface>&& next_type_node_pointer)
      : TypeInterface(StructOrBasicType::kStruct,
                      std::move(next_type_node_pointer)),
        struct_name_(struct_name),
        struct_members_(std::forward<EnumMembers>(members)) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() ==
             static_cast<const StructType&>(type_interface)
                 .GetNextNodeReference();
    } else {
      return false;
    }
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      std::shared_ptr<const TypeInterface>&& type_interface) const override {
    return *this == *type_interface ? AssignableCheckResult::kNonConvert
                                    : AssignableCheckResult::kCanNotConvert;
  }

  template <class EnumMembers>
  void SetStructMembers(EnumMembers&& member_datas) {
    struct_members_ = std::forward<EnumMembers>(member_datas);
  }
  // 返回指向数据的迭代器和是否成功插入
  // 当已存在该名字时不插入并返回false
  template <class MemberName>
  auto AddStructMember(
      MemberName&& member_name,
      std::shared_ptr<const TypeInterface>&& member_type_pointer) {
    return struct_members_.insert(std::make_pair(
        std::forward<MemberName>(member_name), std::move(member_type_pointer)));
  }
  // 获取结构体内成员的类型，如果给定成员名不存在则返回false
  std::pair<StructContainerType::const_iterator, bool> GetStructMemberInfo(
      const std::string& member_name) const {
    auto iter = GetStructMembers().find(member_name);
    return std::make_pair(iter, iter != GetStructMembers().end());
  }
  void SetStructName(const std::string* struct_name) {
    struct_name_ = struct_name;
  }
  const std::string* GetStructName() const { return struct_name_; }

  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  const StructContainerType& GetStructMembers() const {
    return struct_members_;
  }
  StructContainerType& GetStructMembers() { return struct_members_; }

  const std::string* struct_name_;
  StructContainerType struct_members_;
};

class UnionType : public TypeInterface {
 public:
  // 共用体内部存储结构
  // ConstTag标识成员的const性
  using UnionContainerType = std::unordered_map<
      std::string, std::pair<ConstTag, std::shared_ptr<const TypeInterface>>>;

  UnionType() : TypeInterface(StructOrBasicType::kUnion) {}
  UnionType(const std::string* union_name,
            std::shared_ptr<TypeInterface>&& next_node_pointer)
      : TypeInterface(StructOrBasicType::kUnion, std::move(next_node_pointer)),
        union_name_(union_name) {}
  template <class UnionMembers>
  UnionType(const std::string* union_name, UnionMembers&& union_members,
            std::shared_ptr<TypeInterface>&& next_node_pointer)
      : TypeInterface(StructOrBasicType::kUnion, std::move(next_node_pointer)),
        union_name_(union_name),
        union_members_(std::forward<UnionMembers>(union_members)) {}

  virtual bool operator==(const TypeInterface& type_interface) {
    if (IsSameObject(type_interface)) [[likely]] {
      return type_interface.GetNextNodeReference() ==
             type_interface.GetNextNodeReference();
    } else {
      return false;
    }
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      std::shared_ptr<const TypeInterface>&& type_interface) const override {
    return *this == *type_interface ? AssignableCheckResult::kNonConvert
                                    : AssignableCheckResult::kCanNotConvert;
  }

  template <class UnionMembers>
  void SetUnionMembers(UnionMembers&& union_members) {
    union_members_ = std::forward<UnionMembers>(union_members);
  }
  // 返回指向数据的迭代器和是否成功插入
  // 当已存在该名字时不插入并返回false
  template <class MemberName>
  auto AddUnionMember(
      MemberName&& member_name,
      std::shared_ptr<const TypeInterface>&& member_type_pointer) {
    return union_members_.insert(std::make_pair(
        std::forward<MemberName>(member_name), std::move(member_type_pointer)));
  }
  // 获取共用体内成员信息，如果成员名不存在则返回false
  std::pair<UnionContainerType::const_iterator, bool> GetUnionMemberInfo(
      const std::string& member_name) const {
    auto iter = GetUnionMembers().find(member_name);
    return std::make_pair(iter, iter != GetUnionMembers().end());
  }
  void SetUnionName(const std::string* union_name) { union_name_ = union_name; }
  const std::string* GetUnionName() const { return union_name_; }
  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  const UnionContainerType& GetUnionMembers() const { return union_members_; }
  UnionContainerType& GetUnionMembers() { return union_members_; }

  // 共用体名
  const std::string* union_name_;
  // 该共用体可能使用的类型，键值为成员名，值为指向类型的指针
  UnionContainerType union_members_;
};

class EnumType : public TypeInterface {
 public:
  // 枚举内部存储结构
  using EnumContainerType = std::unordered_map<std::string, long long>;

  EnumType(const std::string* enum_name,
           std::shared_ptr<TypeInterface>&& next_node_pointer)
      : TypeInterface(StructOrBasicType::kEnum, std::move(next_node_pointer)),
        enum_name_(enum_name) {}
  template <class EnumMembers>
  EnumType(const std::string* enum_name, EnumMembers&& enum_members,
           std::shared_ptr<TypeInterface>&& next_node_pointer)
      : TypeInterface(StructOrBasicType::kEnum, std::move(next_node_pointer)),
        enum_name_(enum_name),
        enum_members_(std::forward<EnumMembers>(enum_members)) {}

  virtual bool operator==(const TypeInterface& type_interface) const override {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() == type_interface.GetNextNodeReference();
    } else {
      return false;
    }
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      std::shared_ptr<const TypeInterface>&& type_interface) const override {
    return *this == *type_interface ? AssignableCheckResult::kNonConvert
                                    : AssignableCheckResult::kCanNotConvert;
  }

  template <class EnumMembers>
  void SetEnumMembers(EnumMembers&& enum_members) {
    enum_members_ = std::forward<EnumMembers>(enum_members);
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
  // 根据成员个数和最大index计算
  void CalculateContainerType();
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
  void SetContainerType(std::shared_ptr<const TypeInterface>&& container_type) {
    container_type_ = std::move(container_type);
  }
  // 枚举名字
  const std::string* enum_name_;
  // 前半部分为成员名，后半部分为枚举对应的值
  EnumContainerType enum_members_;
  // 存储枚举值用的类型
  std::shared_ptr<const TypeInterface> container_type_;
};

class InitializeListType : public TypeInterface {
 public:
  InitializeListType() : TypeInterface(StructOrBasicType::kInitializeList) {}
  InitializeListType(std::shared_ptr<TypeInterface>&& next_node_pointer)
      : TypeInterface(StructOrBasicType::kInitializeList,
                      std::move(next_node_pointer)) {}

  virtual bool operator==(const TypeInterface& type_interface) {
    if (IsSameObject(type_interface)) [[likely]] {
      return GetNextNodeReference() == type_interface.GetNextNodeReference();
    } else {
      return false;
    }
  }
  virtual AssignableCheckResult CanBeAssignedBy(
      std::shared_ptr<const TypeInterface>&& type_interface) const override {
    // 初始化列表不能被赋值
    return AssignableCheckResult::kCanNotConvert;
  }

  void AddListType(std::shared_ptr<const TypeInterface>&& type_pointer) {
    list_types_.emplace_back(std::move(type_pointer));
  }
  const auto& GetListTypes() const { return list_types_; }
  bool IsSameObject(const TypeInterface& type_interface) const;

 private:
  std::vector<std::shared_ptr<const TypeInterface>> list_types_;
};

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
      std::shared_ptr<const TypeInterface>&& type_interface) const override {
    assert(false);
    // EndType中该函数不应被调用
    // 防止警告
    return AssignableCheckResult::kCanNotConvert;
  }
  // 获取EndType节点指针，全局共用一个节点，节省内存
  static std::shared_ptr<TypeInterface> GetEndType() {
    static std::shared_ptr<TypeInterface> end_type_pointer(new EndType());
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

// 常用类型节点的生成器，全局共享一份节点以节省内存和避免多次分配的消耗
class CommonlyUsedTypeGenerator {
 public:
  // 获取基础类型
  // void类型和bool类型使用SignTag::kUnsigned
  template <BuiltInType built_in_type, SignTag sign_tag>
  static std::shared_ptr<TypeInterface> GetBasicType() {
    static std::shared_ptr<TypeInterface> basic_type(
        new BasicType(built_in_type, sign_tag, EndType::GetEndType()));
    return basic_type;
  }

  static std::shared_ptr<TypeInterface> GetBasicTypeNotTemplate(
      BuiltInType built_in_type, SignTag sign_tag);

  // 获取初始化用字符串类型（const char*）
  static std::shared_ptr<TypeInterface> GetConstExprStringType() {
    static std::shared_ptr<TypeInterface> basic_type(new PointerType(
        ConstTag::kConst, GetBasicType<BuiltInType::kChar, SignTag::kSigned>(),
        0));
    return basic_type;
  }
};

class TypeSystem {
 public:
  // 添加类型时返回的状态
  enum class AddTypeResult {
    // 成功添加的情况
    kNew,  // 以前不存在该类型名
    kShiftToVector,  // 添加前该类型名对应一个类型，添加后转换为vector存储
    kAddToVector,  // 添加前该类型名对应至少2个类型
    // 添加失败的情况
    kTypeAlreadyIn  // 待添加的类型所属大类已经有一种同名类型
  };
  enum class GetTypeResult {
    // 成功匹配的情况
    kSuccess,  // 成功匹配
    // 匹配失败的情况
    kTypeNameNotFound,   // 类型名不存在
    kNoMatchTypePrefer,  // 不存在匹配类型选择倾向的类型链指针
    kSeveralSameLevelMatches  // 匹配到多个平级结果
  };
  // 向类型系统中添加类型
  // type_name是类型名，type_pointer是指向类型链的指针
  // 如果成功添加类型则返回true
  // 如果给定类型名下待添加的类型链所属的大类已存在则添加失败，返回false
  // 大类见StructOrBasicType，不含kEnd和kNotSpecified
  template <class TypeName>
  AddTypeResult AddType(TypeName&& type_name,
                        std::shared_ptr<const TypeInterface>&& type_pointer);
  // 根据给定类型名和选择类型的倾向获取类型
  // 类型选择倾向除了StructOrBasicType::kEnd均可选择
  // 类型选择倾向为StructOrBasicType::kNotSpecified时优先匹配kBasic
  // 其它类型平级，存在多个平级类型时无法获取类型链指针
  // 返回类型链指针和获取情况，获取情况的解释见其定义
  std::pair<std::shared_ptr<const TypeInterface>, GetTypeResult> GetType(
      const std::string& type_name, StructOrBasicType type_prefer);

 private:
  auto& GetTypeNameToNode() { return type_name_to_node_; }
  // 保存类型名到类型链的映射
  // 匿名结构不保存在这里，由该结构声明的变量维护类型链
  // 当存储多个同名类型时值将转换为指向vector的指针，通过vector保存同名类型
  // 使用vector保存时应将StructOrBasicType::kBasic类型放在第一个位置
  // 可以避免查找全部指针，加速标准类型查找规则查找速度
  std::unordered_map<
      std::string,
      std::variant<
          std::monostate, std::shared_ptr<const TypeInterface>,
          std::unique_ptr<std::vector<std::shared_ptr<const TypeInterface>>>>>
      type_name_to_node_;
};

template <class TypeName>
inline TypeSystem::AddTypeResult TypeSystem::AddType(
    TypeName&& type_name, std::shared_ptr<const TypeInterface>&& type_pointer) {
  assert(type_pointer->GetType() != StructOrBasicType::kEnd);
  assert(type_pointer->GetType() != StructOrBasicType::kNotSpecified);
  auto& type_pointers = GetTypeNameToNode()[std::forward<TypeName>(type_name)];
  std::monostate* empty_status_pointer =
      std::get_if<std::monostate>(&type_pointers);
  if (empty_status_pointer != nullptr) [[likely]] {
    // 该节点未保存任何指针
    type_pointers = std::move(type_pointer);
    return AddTypeResult::kNew;
  } else {
    std::shared_ptr<const TypeInterface>* shared_pointer =
        std::get_if<std::shared_ptr<const TypeInterface>>(&type_pointers);
    if (shared_pointer != nullptr) [[likely]] {
      // 该节点保存了一个指针，新增一个后转为vector存储
      if ((*shared_pointer)->GetType() == type_pointer->GetType())
          [[unlikely]] {
        // 待添加的类型与已有类型相同，产生冲突
        return AddTypeResult::kTypeAlreadyIn;
      }
      auto vector_pointer =
          std::make_unique<std::vector<std::shared_ptr<const TypeInterface>>>();
      // 将StructOrBasicType::kBasic类型放在最前面
      if (type_pointer->GetType() == StructOrBasicType::kBasic) {
        vector_pointer->emplace_back(std::move(type_pointer));
        vector_pointer->emplace_back(std::move(*shared_pointer));
      } else {
        // 原来存储的类型可能是StructOrBasicType::kBasic
        // 但是待添加的类型一定不是
        vector_pointer->emplace_back(std::move(*shared_pointer));
        vector_pointer->emplace_back(std::move(type_pointer));
      }
      type_pointers = std::move(vector_pointer);
      return AddTypeResult::kShiftToVector;
    } else {
      // 该节点已经使用vector存储
      std::unique_ptr<std::vector<std::shared_ptr<const TypeInterface>>>&
          vector_pointer = *std::get_if<std::unique_ptr<
              std::vector<std::shared_ptr<const TypeInterface>>>>(
              &type_pointers);
      assert(vector_pointer != nullptr);
      for (auto& stored_pointer : *vector_pointer) {
        if (stored_pointer->GetType() == type_pointer->GetType()) [[unlikely]] {
          // 待添加的类型与已有类型相同，产生冲突
          return AddTypeResult::kTypeAlreadyIn;
        }
      }
      vector_pointer->emplace_back(std::move(type_pointer));
      if (type_pointer->GetType() == StructOrBasicType::kBasic) {
        // 新插入的类型为StructOrBasicType::kBasic
        // 将该类型放到第一个位置以便优化无类型倾向性时的查找逻辑
        std::swap(vector_pointer->front(), vector_pointer->back());
      }
      return AddTypeResult::kAddToVector;
    }
  }
}

}  // namespace c_parser_frontend::type_system
#endif  // !CPARSERFRONTEND_PARSE_CLASSES_H_