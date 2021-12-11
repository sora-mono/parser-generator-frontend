#include "type_system.h"

namespace c_parser_frontend::type_system {
BuiltInType CalculateBuiltInType(const std::string& value) {
  // 转换过程中转换的字符数
  size_t converted_characters;
  uint64_t integer_value = std::stoull(value, &converted_characters);
  if (converted_characters == value.size()) {
    // 全部转换，value代表整数
    if (integer_value <= UINT16_MAX) {
      if (integer_value <= UINT8_MAX) {
        return BuiltInType::kInt8;
      } else {
        return BuiltInType::kInt16;
      }
    } else {
      if (integer_value <= UINT32_MAX) {
        return BuiltInType::kInt32;
      } else {
        // C语言不支持long long
        return BuiltInType::kVoid;
      }
    }
  } else {
    // 未全部转换，可能是浮点数也可能是超出最大支持数值大小
    long double float_value = std::stold(value, &converted_characters);
    if (converted_characters != value.size()) [[unlikely]] {
      // 超出最大支持数值大小
      return BuiltInType::kVoid;
    }
    if (float_value <= FLT_MAX) {
      return BuiltInType::kFloat32;
    } else if (float_value <= DBL_MAX) {
      return BuiltInType::kFloat64;
    } else {
      // C语言不支持long double
      return BuiltInType::kVoid;
    }
  }
}
bool operator==(const std::shared_ptr<TypeInterface>& type_interface1,
                const std::shared_ptr<TypeInterface>& type_interface2) {
  return *type_interface1 == *type_interface2;
}

std::pair<std::shared_ptr<const TypeInterface>, GetTypeResult>
TypeSystem::GetType(const std::string& type_name,
                    StructOrBasicType type_prefer) {
  auto iter = GetTypeNameToNode().find(type_name);
  if (iter == GetTypeNameToNode().end()) [[unlikely]] {
    return std::make_pair(std::shared_ptr<TypeInterface>(),
                          GetTypeResult::kTypeNameNotFound);
  }
  return iter->second.GetType(type_prefer);
}

bool TypeInterface::operator==(const TypeInterface& type_interface) const {
  if (IsSameObject(type_interface)) [[likely]] {
    // 任何派生类都应先调用最低级继承类的IsSameObject而不是operator==()
    // 所有类型链最后都以StructOrBasicType::kEnd结尾
    // 对应的EndType类的operator==()直接返回true以终结调用链
    return GetNextNodeReference() == type_interface.GetNextNodeReference();
  } else {
    return false;
  }
}
bool BasicType::operator==(const TypeInterface& type_interface) const {
  if (IsSameObject(type_interface)) [[likely]] {
    return GetNextNodeReference() == type_interface.GetNextNodeReference();
  } else {
    return false;
  }
}

inline AssignableCheckResult BasicType::CanBeAssignedBy(
    const TypeInterface& type_interface) const {
  // 初步检查用来赋值的类型
  switch (type_interface.GetType()) {
    // 基础类型只能用基础类型赋值
    // c语言中不能使用初始化列表初始化基础类型（存疑）
    case StructOrBasicType::kBasic:
      break;
    case StructOrBasicType::kNotSpecified:
      // 该类型仅可出现在根据类型名获取类型操作中
      assert(false);
      break;
    case StructOrBasicType::kInitializeList:
      // 初始化列表，交给operator_node处理
      return AssignableCheckResult::kInitializeList;
      break;
    default:
      return AssignableCheckResult::kCanNotConvert;
      break;
  }
  // 检查转换方法
  const BasicType& basic_type = static_cast<const BasicType&>(type_interface);
  // 检查符号类型
  if (GetSignTag() != basic_type.GetSignTag()) [[unlikely]] {
    if (GetSignTag() == SignTag::kSigned) {
      return AssignableCheckResult::kUnsignedToSigned;
    } else {
      return AssignableCheckResult::kSignedToUnsigned;
    }
  }
  int differ = static_cast<int>(GetBuiltInType()) -
               static_cast<int>(basic_type.GetBuiltInType());
  if (differ > 0) {
    // 参数优先级低，应向该对象的类型转换
    return AssignableCheckResult::kUpperConvert;
  } else if (differ == 0) {
    // 参数与该对象类型相同，无需转换
    return AssignableCheckResult::kNonConvert;
  } else {
    // 参数优先级高，应向参数类型转换
    return AssignableCheckResult::kLowerConvert;
  }
}

inline size_t BasicType::GetTypeStoreSize() const {
  switch (GetBuiltInType()) {
    case BuiltInType::kInt1:
      // x86下bool类型也要使用1个字节
      return 1;
      break;
    case BuiltInType::kInt8:
      return 1;
      break;
    case BuiltInType::kInt16:
      return 2;
      break;
    case BuiltInType::kInt32:
      return 4;
      break;
    case BuiltInType::kFloat32:
      return 4;
      break;
    case BuiltInType::kFloat64:
      return 8;
      break;
    case BuiltInType::kVoid:
      // 用于void指针
      return 4;
      break;
    default:
      assert(false);
      // 防止警告
      return size_t();
      break;
  }
}

inline bool BasicType::IsSameObject(const TypeInterface& type_interface) const {
  // this == &basic_type是优化手段，类型系统设计思路是尽可能多的共享一条类型链
  // 所以容易出现指向同一个节点的情况
  const BasicType& basic_type = static_cast<const BasicType&>(type_interface);
  return this == &type_interface ||
         TypeInterface::IsSameObject(type_interface) &&
             GetBuiltInType() == basic_type.GetBuiltInType() &&
             GetSignTag() == basic_type.GetSignTag();
}

bool PointerType::operator==(const TypeInterface& type_interface) const {
  if (IsSameObject(type_interface)) [[likely]] {
    return GetNextNodeReference() == type_interface.GetNextNodeReference();
  } else {
    return false;
  }
};
bool FunctionType::operator==(const TypeInterface& type_interface) const {
  if (IsSameObject(type_interface)) [[likely]] {
    return GetNextNodeReference() == type_interface.GetNextNodeReference();
  } else {
    return false;
  }
}
AssignableCheckResult FunctionType::CanBeAssignedBy(
    const TypeInterface& type_interface) const {
  // 在赋值给函数指针的过程中调用
  // 函数类型只能使用函数赋值
  // 初步检查基础数据是否相同
  if (TypeInterface::IsSameObject(type_interface)) [[likely]] {
    // 检查函数签名是否相同
    if (IsSameSignature(static_cast<const FunctionType&>(type_interface)))
        [[likely]] {
      return AssignableCheckResult::kNonConvert;
    }
  }
  return AssignableCheckResult::kCanNotConvert;
}

// 返回是否为函数声明（函数声明中不存在任何流程控制语句）

// 检查给定语句是否可以作为函数内出现的语句

AssignableCheckResult PointerType::CanBeAssignedBy(
    const TypeInterface& type_interface) const {
  // 初步检查用来赋值的类型
  switch (type_interface.GetType()) {
    case StructOrBasicType::kBasic:
      // 如果是浮点数则一定不能转换为指针
      // 检查是否为整数
      if (static_cast<const BasicType&>(type_interface).GetBuiltInType() <
          BuiltInType::kFloat32) [[likely]] {
        // 仅当值为0时可以赋值给指针
        return AssignableCheckResult::kMayBeZeroToPointer;
      } else {
        return AssignableCheckResult::kCanNotConvert;
      }
      break;
    case StructOrBasicType::kPointer:
      break;
    case StructOrBasicType::kNotSpecified:
      // 该类型仅可出现在根据类型名获取类型操作中
      assert(false);
      break;
    case StructOrBasicType::kFunction:
      // 函数可以作为一级函数指针对待
      {
        // 检查是否下一个节点为函数节点
        if (GetNextNodeReference().GetType() == StructOrBasicType::kFunction)
            [[likely]] {
          return GetNextNodeReference().CanBeAssignedBy(type_interface);
        } else {
          // 函数名只能作为一重函数指针使用
          return AssignableCheckResult::kCanNotConvert;
        }
      }
      break;
    case StructOrBasicType::kInitializeList:
      // 初始化列表在operator_node中检查
      return AssignableCheckResult::kInitializeList;
      break;
    default:
      return AssignableCheckResult::kCanNotConvert;
      break;
  }
  // 检查const情况
  const PointerType& pointer_type =
      static_cast<const PointerType&>(type_interface);
  switch (static_cast<int>(GetConstTag()) -
          static_cast<int>(pointer_type.GetConstTag())) {
    case -1:
      [[unlikely]]
      // 被赋值的类型为非const，用来赋值的是const
      // 这种情况下不可以赋值
      return AssignableCheckResult::kAssignedNodeIsConst;
      break;
    case 0:
    case 1:
      // 其余三种可以赋值的情况
      break;
    default:
      assert(false);
  }
  auto assignable_check_result = GetNextNodeReference().CanBeAssignedBy(
      type_interface.GetNextNodeReference());
  // 检查是否为将指针赋值给同等维数的void指针的过程
  if (assignable_check_result == AssignableCheckResult::kCanNotConvert)
      [[unlikely]] {
    if (GetNextNodeReference() ==
        *CommonlyUsedTypeGenerator::GetBasicType<BuiltInType::kVoid,
                                                 SignTag::kUnsigned>())
        [[likely]] {
      // 被赋值的对象为void指针，赋值的对象为同等维数指针
      assignable_check_result = AssignableCheckResult::kConvertToVoidPointer;
    }
  }
  return assignable_check_result;
}

size_t PointerType::TypeSizeOf() const {
  size_t array_size = GetArraySize();
  assert(array_size != -1);
  if (array_size == 0) {
    // 纯指针，直接返回指针大小
    return PointerType::GetTypeStoreSize();
  } else {
    // 指向数组的指针，各维度大小相乘，最后乘以基础单元大小
    return array_size * GetNextNodeReference().TypeSizeOf();
  }
}

AssignableCheckResult StructType::CanBeAssignedBy(
    const TypeInterface& type_interface) const {
  if (type_interface.GetType() == StructOrBasicType::kInitializeList)
      [[likely]] {
    // 赋值用的类型为初始化列表，交给opreator_node处理
    return AssignableCheckResult::kInitializeList;
  } else {
    return *this == type_interface ? AssignableCheckResult::kNonConvert
                                   : AssignableCheckResult::kCanNotConvert;
  }
}

size_t StructType::GetTypeStoreSize() const {
  // 扫描结构体以获取最大的成员
  const auto& struct_members = GetStructureMembers();
  // C语言空结构体大小为0
  // C++空结构体大小为1
  size_t biggest_member_size = 0;
  for (const auto& member : struct_members) {
    size_t member_size = member.first->GetTypeStoreSize();
    if (member_size > biggest_member_size) {
      biggest_member_size = member_size;
    }
  }
  // 获取系统基本字节单位与结构体最大成员的最小值作为字节对齐的单位
  size_t aligen_size = std::min(size_t(4), biggest_member_size);
  // C语言空结构体大小为0
  size_t struct_size = 0;
  // 填写结构体大小
  for (const auto& member : struct_members) {
    // 距离对齐剩余的空间
    size_t rest_space_to_member_align = struct_size % aligen_size;
    // 存储该成员需要的空间
    size_t type_store_size = member.first->TypeSizeOf();
    // 如果该成员所需空间大于距离对齐的空间，则这部分空间作为填充空间
    if (rest_space_to_member_align < type_store_size) {
      struct_size += aligen_size - rest_space_to_member_align;
    }
    // 已经对齐，增加存储所需的空间
    struct_size += type_store_size;
  }
  return struct_size;
}

AssignableCheckResult UnionType::CanBeAssignedBy(
    const TypeInterface& type_interface) const {
  if (type_interface.GetType() == StructOrBasicType::kInitializeList)
      [[unlikely]] {
    return AssignableCheckResult::kInitializeList;
  } else {
    return *this == type_interface ? AssignableCheckResult::kNonConvert
                                   : AssignableCheckResult::kCanNotConvert;
  }
}

size_t UnionType::GetTypeStoreSize() const {
  // 扫描共用体以获取最大的成员
  const auto& union_members = GetStructureMembers();
  // C语言空共用体大小为0
  // C++空共用体大小为1
  size_t biggest_member_size = 0;
  for (const auto& member : union_members) {
    size_t member_size = member.first->TypeSizeOf();
    if (member_size > biggest_member_size) {
      biggest_member_size = member_size;
    }
  }
  // 获取系统基本字节单位与结构体最大成员的最小值作为字节对齐的单位
  size_t aligen_size = std::min(size_t(4), biggest_member_size);
  size_t rest_space_to_aligen = biggest_member_size % aligen_size;
  if (rest_space_to_aligen != 0) {
    return biggest_member_size + aligen_size - rest_space_to_aligen;
  } else {
    return biggest_member_size;
  }
}

AssignableCheckResult EnumType::CanBeAssignedBy(
    const TypeInterface& type_interface) const {
  if (type_interface.GetType() == StructOrBasicType::kInitializeList)
      [[unlikely]] {
    return AssignableCheckResult::kInitializeList;
  } else {
    return *this == type_interface ? AssignableCheckResult::kNonConvert
                                   : AssignableCheckResult::kCanNotConvert;
  }
}

bool EnumType::IsSameObject(const TypeInterface& type_interface) const {
  // this == &type_interface是优化手段
  // 类型系统设计思路是尽可能多的共享一条类型链
  // 所以容易出现指向同一个节点的情况
  if (this == &type_interface) [[likely]] {
    // 二者共用一个节点
    return true;
  } else if (TypeInterface::IsSameObject(type_interface)) [[likely]] {
    const EnumType& enum_type = static_cast<const EnumType&>(type_interface);
    const std::string& enum_name_this = GetEnumName();
    const std::string& enum_name_argument = enum_type.GetEnumName();
    if (!enum_name_this.empty()) [[likely]] {
      if (!enum_name_argument.empty()) [[likely]] {
        // 二者都是具名枚举
        // C语言中具名枚举根据名字区分
        return enum_name_this == enum_name_argument;
      }
    }
    // 二者都是匿名枚举
    // 如果是同一个枚举类型应该在第一个if返回
  }
  return false;
}

bool InitializeListType::IsSameObject(
    const TypeInterface& type_interface) const {
  // this == &type_interface是优化手段
  // 类型系统设计思路是尽可能多的共享一条类型链
  // 所以容易出现指向同一个节点的情况
  return this == &type_interface ||
         TypeInterface::IsSameObject(type_interface) &&
             GetListTypes() ==
                 static_cast<const InitializeListType&>(type_interface)
                     .GetListTypes();
}

bool PointerType::IsSameObject(const TypeInterface& type_interface) const {
  // this == &basic_type是优化手段，类型系统设计思路是尽可能多的共享一条类型链
  // 所以容易出现指向同一个节点的情况
  return this == &type_interface ||
         TypeInterface::IsSameObject(type_interface) &&
             GetConstTag() ==
                 static_cast<const PointerType&>(type_interface).GetConstTag();
}

std::shared_ptr<const BasicType>
CommonlyUsedTypeGenerator::GetBasicTypeNotTemplate(BuiltInType built_in_type,
                                                   SignTag sign_tag) {
  switch (built_in_type) {
    case BuiltInType::kVoid:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kVoid, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kVoid, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kInt1:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kInt1, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kInt1, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kInt8:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kInt8, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kInt8, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kInt16:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kInt16, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kInt16, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kInt32:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kInt32, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kInt32, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kFloat32:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kFloat32, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kFloat32, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kFloat64:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kFloat64, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kFloat64, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    default:
      assert(false);
      break;
  }
  // 防止警告
  return GetBasicType<BuiltInType::kVoid, SignTag::kUnsigned>();
}

std::pair<std::shared_ptr<const TypeInterface>,
          DeclineMathematicalComputeTypeResult>
TypeInterface::DeclineMathematicalComputeResult(
    const std::shared_ptr<const TypeInterface>& left_compute_type,
    const std::shared_ptr<const TypeInterface>& right_compute_type) {
  switch (left_compute_type->GetType()) {
    case StructOrBasicType::kBasic: {
      switch (right_compute_type->GetType()) {
        case StructOrBasicType::kBasic:
          if (static_cast<long long>(left_compute_type->GetType()) >=
              static_cast<long long>(right_compute_type->GetType())) {
            // 右侧类型提升至左侧
            return std::make_pair(
                left_compute_type,
                DeclineMathematicalComputeTypeResult::kConvertToLeft);
          } else {
            // 左侧类型提升至右侧
            return std::make_pair(
                right_compute_type,
                DeclineMathematicalComputeTypeResult::kConvertToRight);
          }
          break;
        case StructOrBasicType::kPointer:
          // 指针与偏移量组合
          if (static_cast<long long>(left_compute_type->GetType()) <
              static_cast<long long>(BuiltInType::kFloat32)) [[likely]] {
            // 偏移量是整型，可以作为指针的偏移量
            return std::make_pair(
                right_compute_type,
                DeclineMathematicalComputeTypeResult::kLeftOffsetRightPointer);
          } else {
            // 浮点数类型的偏移量不适用于指针
            return std::make_pair(
                std::shared_ptr<const TypeInterface>(),
                DeclineMathematicalComputeTypeResult::kLeftNotIntger);
          }
          break;
        default:
          return std::make_pair(
              std::shared_ptr<const TypeInterface>(),
              DeclineMathematicalComputeTypeResult::kRightNotComputableType);
          break;
      }
    } break;
    case StructOrBasicType::kPointer: {
      switch (right_compute_type->GetType()) {
        case StructOrBasicType::kBasic:
          // 指针与偏移量组合
          if (static_cast<long long>(right_compute_type->GetType()) <
              static_cast<long long>(BuiltInType::kFloat32)) [[likely]] {
            // 偏移量是整型，可以作为指针的偏移量
            return std::make_pair(
                left_compute_type,
                DeclineMathematicalComputeTypeResult::kLeftPointerRightOffset);
          } else {
            // 浮点数类型的偏移量不适用于指针
            return std::make_pair(
                std::shared_ptr<const TypeInterface>(),
                DeclineMathematicalComputeTypeResult::kRightNotIntger);
          }
          break;
        case StructOrBasicType::kPointer:
          // 指针与指针组合
          return std::make_pair(
              std::shared_ptr<const TypeInterface>(),
              DeclineMathematicalComputeTypeResult::kLeftRightBothPointer);
          break;
        default:
          return std::make_pair(
              std::shared_ptr<const TypeInterface>(),
              DeclineMathematicalComputeTypeResult::kRightNotComputableType);
          break;
      }
    } break;
    default:
      return std::make_pair(
          std::shared_ptr<const TypeInterface>(),
          DeclineMathematicalComputeTypeResult::kLeftNotComputableType);
      break;
  }
}

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
              CheckFunctionDefineAddResult(
                  static_cast<const FunctionType&>(**shared_pointer),
                  static_cast<const FunctionType&>(*type_to_add));
          if (function_define_add_result == AddTypeResult::kTypeAlreadyIn)
              [[likely]] {
            // 如果函数签名不同应返回AddTypeResult::kOverrideFunction
            // 使用新的指针替换原有指针，从而更新函数参数名
            *shared_pointer = type_to_add;
            function_define_add_result = AddTypeResult::kFunctionDefine;
          }
          return function_define_add_result;
        } else {
          // 待添加的类型与已有类型相同，产生冲突
          return AddTypeResult::kTypeAlreadyIn;
        }
      }
      auto pointers =
          std::make_unique<std::list<std::shared_ptr<const TypeInterface>>>();
      // 将StructOrBasicType::kBasic或StructOrBasicType::kPointer类型放在最前面
      // 从而优化查找类型的速度
      if (IsSameKind(type_to_add->GetType(), StructOrBasicType::kBasic)) {
        pointers->emplace_back(type_to_add);
        pointers->emplace_back(std::move(*shared_pointer));
      } else {
        // 原来的类型不一定属于该大类，但新添加的类型一定不属于该大类
        pointers->emplace_back(std::move(*shared_pointer));
        pointers->emplace_back(type_to_add);
      }
      type_data_ = std::move(pointers);
      return AddTypeResult::kShiftToVector;
    } else {
      // 该节点已经使用list存储
      std::unique_ptr<std::list<std::shared_ptr<const TypeInterface>>>&
          pointers = *std::get_if<
              std::unique_ptr<std::list<std::shared_ptr<const TypeInterface>>>>(
              &type_data_);
      assert(pointers != nullptr);
      // 检查待添加的类型与已存在的类型是否属于同一大类
      for (auto& stored_pointer : *pointers) {
        if (IsSameKind(stored_pointer->GetType(), type_to_add->GetType()))
            [[unlikely]] {
          // 检查是否在添加函数类型
          if (type_to_add->GetType() == StructOrBasicType::kFunction)
              [[unlikely]] {
            AddTypeResult function_define_add_result =
                CheckFunctionDefineAddResult(
                    static_cast<const FunctionType&>(*stored_pointer),
                    static_cast<const FunctionType&>(*type_to_add));
            if (function_define_add_result == AddTypeResult::kTypeAlreadyIn)
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
      pointers->emplace_back(type_to_add);
      if (IsSameKind(type_to_add->GetType(), StructOrBasicType::kBasic)) {
        // 新插入的类型为StructOrBasicType::kBasic/kPointer
        // 将该类型放到第一个位置以便优化无类型倾向性时的查找逻辑
        std::swap(pointers->front(), pointers->back());
      }
      return AddTypeResult::kAddToVector;
    }
  }
}

std::pair<std::shared_ptr<const TypeInterface>, GetTypeResult>
TypeSystem::TypeData::GetType(StructOrBasicType type_prefer) const {
  if (std::get_if<std::monostate>(&type_data_) != nullptr) [[unlikely]] {
    // 未存储任何节点，等效于该名称对应的类型不存在
    return std::make_pair(std::shared_ptr<TypeInterface>(),
                          GetTypeResult::kTypeNameNotFound);
  }
  const std::shared_ptr<const TypeInterface>* shared_pointer =
      std::get_if<std::shared_ptr<const TypeInterface>>(&type_data_);
  if (shared_pointer != nullptr) [[likely]] {
    // 仅存储1个指针
    if (type_prefer == StructOrBasicType::kNotSpecified ||
        (*shared_pointer)->GetType() == type_prefer) [[likely]] {
      // 无类型选择倾向或者类型选择倾向符合
      return std::make_pair(*shared_pointer, GetTypeResult::kSuccess);
    } else {
      // 没有匹配的类型选择倾向
      return std::make_pair(std::shared_ptr<TypeInterface>(),
                            GetTypeResult::kNoMatchTypePrefer);
    }
  } else {
    // 存储多个指针
    auto& pointers = *std::get_if<
        std::unique_ptr<std::list<std::shared_ptr<const TypeInterface>>>>(
        &type_data_);
    // 判断是否指定类型选择倾向
    if (type_prefer == StructOrBasicType::kNotSpecified) [[likely]] {
      if (pointers->front()->GetType() == StructOrBasicType::kBasic) {
        // 优先匹配kBasic
        return std::make_pair(pointers->front(), GetTypeResult::kSuccess);
      } else {
        // vector中没有kBasic，并且一定存储至少两个指针，这些指针同级
        return std::make_pair(std::shared_ptr<const TypeInterface>(),
                              GetTypeResult::kSeveralSameLevelMatches);
      }
    } else {
      // 遍历vector搜索与给定类型选择倾向相同的指针
      for (auto& pointer : *pointers) {
        if (pointer->GetType() == type_prefer) [[unlikely]] {
          return std::make_pair(pointer, GetTypeResult::kSuccess);
        }
      }
      // 没有匹配的类型选择倾向
      return std::make_pair(std::shared_ptr<const TypeInterface>(),
                            GetTypeResult::kNoMatchTypePrefer);
    }
  }
}

bool TypeSystem::TypeData::IsSameKind(StructOrBasicType type1,
                                      StructOrBasicType type2) {
  unsigned long long type1_ = static_cast<unsigned long long>(type1);
  unsigned long long type2_ = static_cast<unsigned long long>(type2);
  constexpr unsigned long long kBasicType =
      static_cast<unsigned long long>(StructOrBasicType::kBasic);
  constexpr unsigned long long kPointerType =
      static_cast<unsigned long long>(StructOrBasicType::kPointer);
  // 第二部分的运算用来检测是否均属于kPointer/kBasic
  return type1 == type2 || ((type1_ ^ kBasicType) |
                            (type1_ ^ kPointerType) & (type2_ ^ kBasicType) |
                            (type2_ ^ kPointerType));
}
StructureTypeInterface::StructureMemberContainer::MemberIndex
StructureTypeInterface::StructureMemberContainer::GetMemberIndex(
    const std::string& member_name) const {
  auto iter = member_name_to_index_.find(member_name);
  if (iter != member_name_to_index_.end()) [[likely]] {
    return iter->second;
  } else {
    return MemberIndex::InvalidId();
  }
}

bool StructureTypeInterface::IsSameObject(
    const TypeInterface& type_interface) const {
  // this == &type_interface是优化手段
  // 类型系统设计思路是尽可能多的共享一条类型链
  // 所以容易出现指向同一个节点的情况
  if (this == &type_interface) [[likely]] {
    // 二者共用一个节点
    return true;
  } else if (TypeInterface::IsSameObject(type_interface)) [[likely]] {
    const StructureTypeInterface& structure_type =
        static_cast<const StructType&>(type_interface);
    const std::string& structure_name_this = GetStructureName();
    const std::string& structure_name_another =
        structure_type.GetStructureName();
    if (!structure_name_this.empty()) [[likely]] {
      if (!structure_name_another.empty()) [[likely]] {
        // 二者都是具名结构
        // C语言中具名结构根据名字区分
        return structure_name_this == structure_name_another;
      }
    }
    // 二者都是匿名结构
    // 如果匿名结构相同则二者使用相同的类型指针，在第一个if就返回
  }
  return false;
}

}  // namespace c_parser_frontend::type_system
