#include "type_system.h"

namespace c_parser_frontend::type_system {
bool operator==(const std::shared_ptr<TypeInterface>& type_interface1,
                const std::shared_ptr<TypeInterface>& type_interface2) {
  return *type_interface1 == *type_interface2;
}

std::pair<std::shared_ptr<const TypeInterface>, TypeSystem::GetTypeResult>
TypeSystem::GetType(const std::string& type_name,
                    StructOrBasicType type_prefer) {
  auto iter = GetTypeNameToNode().find(type_name);
  if (iter == GetTypeNameToNode().end()) [[unlikely]] {
    return std::make_pair(std::shared_ptr<TypeInterface>(),
                          GetTypeResult::kTypeNameNotFound);
  }
  std::shared_ptr<const TypeInterface>* shared_pointer =
      std::get_if<std::shared_ptr<const TypeInterface>>(&iter->second);
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
    auto& vector_pointer = *std::get_if<
        std::unique_ptr<std::vector<std::shared_ptr<const TypeInterface>>>>(
        &iter->second);
    // 判断是否指定类型选择倾向
    if (type_prefer == StructOrBasicType::kNotSpecified) [[likely]] {
      if (vector_pointer->front()->GetType() == StructOrBasicType::kBasic) {
        // 优先匹配kBasic
        return std::make_pair(vector_pointer->front(),
                              GetTypeResult::kSuccess);
      } else {
        // vector中没有kBasic，并且一定存储至少两个指针，这些指针同级
        return std::make_pair(std::shared_ptr<TypeInterface>(),
                              GetTypeResult::kSeveralSameLevelMatches);
      }
    } else {
      // 遍历vector搜索与给定类型选择倾向相同的指针
      for (auto& pointer : *vector_pointer) {
        if (pointer->GetType() == type_prefer) [[unlikely]] {
          return std::make_pair(pointer, GetTypeResult::kSuccess);
        }
      }
      // 没有匹配的类型选择倾向
      return std::make_pair(std::shared_ptr<TypeInterface>(),
                            GetTypeResult::kNoMatchTypePrefer);
    }
  }
}

inline AssignableCheckResult BasicType::CanBeAssignedBy(
    std::shared_ptr<const TypeInterface>&& type_interface) const {
  // 初步检查用来赋值的类型
  switch (type_interface->GetType()) {
    // 基础类型只能用基础类型赋值
    // c语言中不能使用初始化列表初始化基础类型（存疑）
    case StructOrBasicType::kBasic:
      break;
    case StructOrBasicType::kNotSpecified:
      // 该类型仅可出现在根据类型名获取类型操作中
      assert(false);
      break;
    default:
      return AssignableCheckResult::kCanNotConvert;
      break;
  }
  // 检查转换方法
  const BasicType& basic_type = static_cast<const BasicType&>(*type_interface);
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

inline bool BasicType::IsSameObject(const TypeInterface& type_interface) const {
  // this == &basic_type是优化手段，类型系统设计思路是尽可能多的共享一条类型链
  // 所以容易出现指向同一个节点的情况
  const BasicType& basic_type = static_cast<const BasicType&>(type_interface);
  return this == &type_interface ||
         TypeInterface::IsSameObject(type_interface) &&
             GetBuiltInType() == basic_type.GetBuiltInType() &&
             GetSignTag() == basic_type.GetSignTag();
}

inline bool FunctionType::IsSameObject(
    const TypeInterface& type_interface) const {
  // this == &type_interface是优化手段
  // 类型系统设计思路是尽可能多的共享一条类型链
  // 所以容易出现指向同一个节点的情况
  if (this == &type_interface) [[likely]] {
    return true;
  }
  if (TypeInterface::IsSameObject(type_interface)) [[likely]] {
    if (GetFunctionName().empty()) {
      // 函数指针的一个节点，函数名置空
      // 函数指针需要比较参数和返回值
      const FunctionType& function_type =
          static_cast<const FunctionType&>(type_interface);
      return function_type.GetFunctionName().empty() &&
             GetArgumentTypes() == function_type.GetArgumentTypes() &&
             GetReturnTypeReference() == function_type.GetReturnTypeReference();
    } else {
      // 函数声明/函数调用/函数定义的一个节点
      // C语言不支持函数重载，所以只需判断函数名是否相同
      return GetFunctionName() ==
             static_cast<const FunctionType&>(type_interface).GetFunctionName();
    }
  }
}

inline AssignableCheckResult PointerType::CanBeAssignedBy(
    std::shared_ptr<const TypeInterface>&& type_interface) const {
  // 初步检查用来赋值的类型
  switch (type_interface->GetType()) {
    case StructOrBasicType::kBasic:
      // 仅当值为0时可以赋值给指针
      return AssignableCheckResult::kMayBeZeroToPointer;
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
        // 将用来赋值的函数转换为函数指针形式
        std::shared_ptr<const TypeInterface> function_pointer =
            FunctionType::ConvertToFunctionPointer(std::move(type_interface));
        // 不使用尾递归优化防止function_pointer被提前释放
        AssignableCheckResult check_result = CanBeAssignedBy(
            std::shared_ptr<const TypeInterface>(function_pointer));
        return check_result;
      }
      break;
    default:
      return AssignableCheckResult::kCanNotConvert;
      break;
  }
  // 检查const情况
  const PointerType& pointer_type =
      static_cast<const PointerType&>(*type_interface);
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
  switch (GetNextNodeReference().GetType()) {
    case StructOrBasicType::kPointer:
      // 下一个节点是指针，则继续比较
      return GetNextNodeReference().CanBeAssignedBy(
          type_interface->GetNextNodePointer());
    default:
      // 下一个节点不是指针则比较是否为相同类型
      if (GetNextNodeReference() == type_interface->GetNextNodeReference())
          [[likely]] {
        return AssignableCheckResult::kNonConvert;
      } else if (type_interface->GetNextNodeReference().GetType() !=
                     StructOrBasicType::kPointer &&
                 GetNextNodeReference() ==
                     *CommonlyUsedTypeGenerator::GetBasicType<
                         BuiltInType::kVoid, SignTag::kUnsigned>()) {
        // 被赋值的对象为void指针，赋值的对象为同等维数指针
        return AssignableCheckResult::kConvertToVoidPointer;
      }
      break;
  }
}

bool StructType::IsSameObject(const TypeInterface& type_interface) const {
  // this == &type_interface是优化手段
  // 类型系统设计思路是尽可能多的共享一条类型链
  // 所以容易出现指向同一个节点的情况
  if (this == &type_interface) [[likely]] {
    // 二者共用一个节点
    return true;
  } else if (TypeInterface::IsSameObject(type_interface)) [[likely]] {
    const StructType& struct_type =
        static_cast<const StructType&>(type_interface);
    const std::string* struct_name_this = GetStructName();
    const std::string* struct_name_argument = struct_type.GetStructName();
    if (struct_name_this != nullptr) [[likely]] {
      if (struct_name_argument != nullptr) [[likely]] {
        // 二者都是具名结构体
        // C语言中具名结构体根据名字区分
        return *struct_name_this == *struct_name_argument;
      }
    } else if (struct_name_argument == nullptr) {
      // 二者都是匿名结构体
      return GetStructMembers() == struct_type.GetStructMembers();
    }
  }
  return false;
}

bool UnionType::IsSameObject(const TypeInterface& type_interface) const {
  // this == &type_interface是优化手段
  // 类型系统设计思路是尽可能多的共享一条类型链
  // 所以容易出现指向同一个节点的情况
  if (this == &type_interface) [[likely]] {
    // 二者共用一个节点
    return true;
  } else if (TypeInterface::IsSameObject(type_interface)) [[likely]] {
    const UnionType& union_type = static_cast<const UnionType&>(type_interface);
    const std::string* union_name_this = GetUnionName();
    const std::string* union_name_argument = union_type.GetUnionName();
    if (union_name_this != nullptr) [[likely]] {
      if (union_name_argument != nullptr) [[likely]] {
        // 二者都是具名共用体
        // C语言中具名共用体根据名字区分
        return *union_name_this == *union_name_argument;
      }
    } else if (union_name_argument == nullptr) {
      // 二者都是匿名结构体
      return GetUnionMembers() == union_type.GetUnionMembers();
    }
  }
  return false;
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
    const std::string* enum_name_this = GetEnumName();
    const std::string* enum_name_argument = enum_type.GetEnumName();
    if (enum_name_this != nullptr) [[likely]] {
      if (enum_name_argument != nullptr) [[likely]] {
        // 二者都是具名枚举
        // C语言中具名枚举根据名字区分
        return *enum_name_this == *enum_name_argument;
      }
    } else if (enum_name_argument == nullptr) {
      // 二者都是匿名枚举
      return GetEnumMembers() == enum_type.GetEnumMembers();
    }
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

inline bool PointerType::IsSameObject(
    const TypeInterface& type_interface) const {
  // this == &basic_type是优化手段，类型系统设计思路是尽可能多的共享一条类型链
  // 所以容易出现指向同一个节点的情况
  return this == &type_interface ||
         TypeInterface::IsSameObject(type_interface) &&
             GetConstTag() ==
                 static_cast<const PointerType&>(type_interface).GetConstTag();
}

std::shared_ptr<TypeInterface>
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
    case BuiltInType::kBool:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kBool, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kBool, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kChar:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kChar, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kChar, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kShort:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kShort, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kShort, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kInt:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kInt, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kInt, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kLong:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kLong, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kLong, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kFloat:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kFloat, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kFloat, SignTag::kUnsigned>();
          break;
        default:
          assert(false);
          break;
      }
      break;
    case BuiltInType::kDouble:
      switch (sign_tag) {
        case c_parser_frontend::type_system::SignTag::kSigned:
          return GetBasicType<BuiltInType::kDouble, SignTag::kSigned>();
          break;
        case c_parser_frontend::type_system::SignTag::kUnsigned:
          return GetBasicType<BuiltInType::kDouble, SignTag::kUnsigned>();
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
    std::shared_ptr<const TypeInterface>&& left_compute_type,
    std::shared_ptr<const TypeInterface>&& right_compute_type) {
  switch (left_compute_type->GetType()) {
    case StructOrBasicType::kBasic: {
      switch (right_compute_type->GetType()) {
        case StructOrBasicType::kBasic:
          if (static_cast<long long>(left_compute_type->GetType()) >=
              static_cast<long long>(right_compute_type->GetType())) {
            // 右侧类型提升至左侧
            return std::make_pair(
                std::move(left_compute_type),
                DeclineMathematicalComputeTypeResult::kConvertToLeft);
          } else {
            // 左侧类型提升至右侧
            return std::make_pair(
                std::move(right_compute_type),
                DeclineMathematicalComputeTypeResult::kConvertToRight);
          }
          break;
        case StructOrBasicType::kPointer:
          // 指针与偏移量组合
          if (static_cast<long long>(left_compute_type->GetType()) <
              static_cast<long long>(BuiltInType::kFloat)) [[likely]] {
            // 偏移量是整型，可以作为指针的偏移量
            return std::make_pair(
                std::move(right_compute_type),
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
              static_cast<long long>(BuiltInType::kFloat)) [[likely]] {
            // 偏移量是整型，可以作为指针的偏移量
            return std::make_pair(
                std::move(left_compute_type),
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

}  // namespace c_parser_frontend::type_system
