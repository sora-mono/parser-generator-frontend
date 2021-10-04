#include "operator_node.h"

#include <format>
#include <iostream>
namespace c_parser_frontend::operator_node {
inline bool VarietyOperatorNode::SetVarietyType(
    const std::shared_ptr<const TypeInterface>& variety_type) {
  if (CheckVarietyTypeValid(*variety_type)) [[likely]] {
    // 仅当符合变量的类型的标准时才可以设置
    variety_type_ = variety_type;
    return true;
  } else {
    return false;
  }
}

bool VarietyOperatorNode::SetVarietyTypeNoCheckFunctionType(
    const std::shared_ptr<const TypeInterface>& variety_type) {
  if (variety_type->GetType() == StructOrBasicType::kFunction ||
      CheckVarietyTypeValid(*variety_type)) {
    variety_type_ = variety_type;
    return true;
  } else {
    return false;
  }
}

inline bool VarietyOperatorNode::CheckVarietyTypeValid(
    const TypeInterface& variety_type) {
  switch (variety_type.GetType()) {
    case StructOrBasicType::kEnd:
      // 该类型代表最终哨兵节点
    case StructOrBasicType::kNotSpecified:
      // 该选项仅应出现在根据类型名查询类型用函数中
      assert(false);
      [[fallthrough]];
    case StructOrBasicType::kFunction:
    // 函数不能声明为变量
    case StructOrBasicType::kInitializeList:
      // 初始化列表不能声明为变量
      return false;
      break;
    default:
      return true;
      break;
  }
}

inline bool BasicTypeInitializeOperatorNode::SetInitDataType(
    const std::shared_ptr<const TypeInterface>& variety_type) {
  if (CheckBasicTypeInitializeValid(*variety_type)) [[likely]] {
    return SetInitValueType(variety_type);
  } else {
    return false;
  }
}

inline bool BasicTypeInitializeOperatorNode::CheckBasicTypeInitializeValid(
    const TypeInterface& variety_type) {
  switch (variety_type.GetType()) {
    case StructOrBasicType::kPointer:
      // const char*
      return variety_type ==
             *CommonlyUsedTypeGenerator::GetConstExprStringType();
      break;
    case StructOrBasicType::kBasic:
      return true;
      break;
    default:
      return false;
      break;
  }
}

// 返回是否为有效的初始化数据类型，如果无效则不添加
inline bool InitializeOperatorNodeInterface::SetInitValueType(
    const std::shared_ptr<const TypeInterface>& init_value_type) {
  if (CheckInitValueTypeValid(*init_value_type)) [[likely]] {
    initialize_value_type_ = init_value_type;
    return true;
  } else {
    return false;
  }
}

inline bool InitializeOperatorNodeInterface::CheckInitValueTypeValid(
    const TypeInterface& init_value_type) {
  switch (init_value_type.GetType()) {
    case StructOrBasicType::kBasic:
      // 数字
    case StructOrBasicType::kInitializeList:
      // 初始化列表
      return true;
      break;
    case StructOrBasicType::kPointer:
      // 检查是否为字符串类型const char*
      return init_value_type ==
             *CommonlyUsedTypeGenerator::GetConstExprStringType();
      break;
    case StructOrBasicType::kEnd:
    case StructOrBasicType::kNotSpecified:
      // 以上两种类型不应在这里出现
      assert(false);
      [[fallthrough]];
    case StructOrBasicType::kEnum:
    case StructOrBasicType::kStruct:
    case StructOrBasicType::kUnion:
    case StructOrBasicType::kFunction:
      // c语言中这几种类型不能声明为初始化用变量
      return false;
    default:
      assert(false);
      // 防止警告
      return false;
      break;
  }
}

// 返回值是否为有效的初始化列表可用类型，如果类型无效则不执行插入操作

bool ListInitializeOperatorNode::AddListValue(
    const std::shared_ptr<const InitializeOperatorNodeInterface>& list_value) {
  if (CheckInitListValueTypeValid(*list_value->GetResultTypePointer()))
      [[likely]] {
    list_values_.emplace_back(list_value);
    return true;
  } else {
    return false;
  }
}

bool ListInitializeOperatorNode::SetInitListType(
    const std::shared_ptr<const TypeInterface>& list_type) {
  if (CheckInitListTypeValid(*list_type)) [[likely]] {
    return SetInitValueType(list_type);
  } else {
    return false;
  }
}

bool MathematicalOperatorNode::SetLeftOperatorNode(
    const std::shared_ptr<const OperatorNodeInterface>& left_operator_node) {
  switch (GetMathematicalOperation()) {
    case MathematicalOperation::kNot:
    case MathematicalOperation::kLogicalNegative: {
      // 按位取反和逻辑非为一元运算符，结果类型与运算数类型相同
      auto compute_result_node = std::make_shared<VarietyOperatorNode>(
          nullptr, ConstTag::kNonConst, LeftRightValueTag::kRightValue);
      bool result = compute_result_node->SetVarietyType(
          left_operator_node->GetResultTypePointer());
      if (result) [[likely]] {
        // left_operator_node是合法的参与运算的节点则可以设置
        SetComputeResultNode(std::move(compute_result_node));
      } else {
        break;
      }
    }
      [[fallthrough]];
    default:
      left_operator_node_ = left_operator_node;
      break;
  }
  return true;
}

MathematicalOperatorNode::DeclineMathematicalComputeTypeResult
MathematicalOperatorNode::SetRightOperatorNode(
    const std::shared_ptr<const OperatorNodeInterface>& right_operator_node) {
  switch (GetMathematicalOperation()) {
    case MathematicalOperation::kNot:
    case MathematicalOperation::kLogicalNegative:
      // 一元运算符不应调用此函数
      assert(false);
      break;
    default:
      break;
  }
  auto [compute_result_node, decline_result] =
      DeclineComputeResult(GetMathematicalOperation(),
                           GetLeftOperatorNodePointer(), right_operator_node);
  if (compute_result_node != nullptr) [[likely]] {
    // 可以运算，设置运算右节点和运算结果节点
    right_operator_node_ = right_operator_node;
    compute_result_node_ = std::move(compute_result_node);
  }
  return decline_result;
}

std::pair<std::shared_ptr<const VarietyOperatorNode>,
          MathematicalOperatorNode::DeclineMathematicalComputeTypeResult>
MathematicalOperatorNode::DeclineComputeResult(
    MathematicalOperation mathematical_operation,
    const std::shared_ptr<const OperatorNodeInterface>& left_operator_node,
    const std::shared_ptr<const OperatorNodeInterface>& right_operator_node) {
  std::shared_ptr<const TypeInterface> left_operator_node_type =
      left_operator_node->GetResultTypePointer();
  std::shared_ptr<const TypeInterface> right_operator_node_type =
      left_operator_node->GetResultTypePointer();
  auto [compute_result_type, compute_result] =
      TypeInterface::DeclineMathematicalComputeResult(left_operator_node_type,
                                                      right_operator_node_type);
  switch (compute_result) {
    case DeclineMathematicalComputeTypeResult::kComputable:
    case DeclineMathematicalComputeTypeResult::kLeftPointerRightOffset:
    case DeclineMathematicalComputeTypeResult::kLeftOffsetRightPointer:
    case DeclineMathematicalComputeTypeResult::kConvertToLeft:
    case DeclineMathematicalComputeTypeResult::kConvertToRight:
      // 可以运算的几种情况
      {
        // 创建运算结果节点
        auto compute_result_node = std::make_shared<VarietyOperatorNode>(
            nullptr, ConstTag::kNonConst, LeftRightValueTag::kRightValue);
        compute_result_node->SetVarietyType(std::move(compute_result_type));
        return std::make_pair(std::move(compute_result_node), compute_result);
      }
      break;
    default:
      return std::make_pair(nullptr, compute_result);
      break;
  }
}

bool FunctionCallOperatorNode::SetFunctionObjectToCall(
    const std::shared_ptr<const OperatorNodeInterface>&
        function_object_to_call) {
  // 判断类型是否合法
  auto function_type = function_object_to_call->GetResultTypePointer();
  if (CheckFunctionTypeValid(*function_type)) [[likely]] {
    // 获取最精确的类型
    auto exact_function_type = std::static_pointer_cast<
        const c_parser_frontend::type_system::FunctionType>(function_type);
    // 成功设置函数调用后返回的类型
    // 清除原来生成的函数参数对象指针
    function_arguments_offerred_.GetFunctionCallArguments().clear();
    // 设置函数类型
    function_type_ = exact_function_type;
    // 根据函数类型创建返回类型对象
    // 返回类型匿名
    auto return_object = std::make_shared<VarietyOperatorNode>(
        nullptr, exact_function_type->GetReturnTypeConstTag(),
        LeftRightValueTag::kRightValue);
    return_object->SetVarietyType(exact_function_type->GetReturnTypePointer());
    return_object_ = return_object;
    return true;
  } else {
    return false;
  }
}

// 添加一个参数，参数添加顺序从左到右
// 返回待添加的参数是否通过检验，未通过检验则不会添加

AssignableCheckResult FunctionCallOperatorNode::AddFunctionCallArgument(
    const std::shared_ptr<const OperatorNodeInterface>& argument_node,
    const std::shared_ptr<std::list<
        std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
        sentences_to_get_argument) {
  // 将要放置节点的index
  size_t new_argument_index =
      function_arguments_offerred_.GetFunctionCallArguments().size();
  if (new_argument_index >= GetFunctionTypePointer()->GetArguments().size())
      [[unlikely]] {
    // 函数参数已满，不能添加更多参数
    // 暂不支持可变参数
    return AssignableCheckResult::kArgumentsFull;
  }
  // 函数参数创建过程等同于声明并赋值
  AssignableCheckResult check_result = AssignOperatorNode::CheckAssignable(
      *GetFunctionTypePointer()
           ->GetArguments()[new_argument_index]
           .variety_operator_node,
      *argument_node, true);
  switch (check_result) {
    case AssignableCheckResult::kNonConvert:
    case AssignableCheckResult::kUpperConvert:
    case AssignableCheckResult::kConvertToVoidPointer:
    case AssignableCheckResult::kZeroConvertToPointer:
    case AssignableCheckResult::kUnsignedToSigned:
    case AssignableCheckResult::kSignedToUnsigned:
      // 可以添加
      function_arguments_offerred_.AddFunctionCallArgument(
          argument_node, sentences_to_get_argument);
      break;
    default:
      // 不可以添加
      break;
  }
  return check_result;
}

bool AllocateOperatorNode::SetTargetVariety(
    const std::shared_ptr<const OperatorNodeInterface>& target_variety) {
  if (CheckAllocatable(*target_variety)) [[likely]] {
    target_variety_ = target_variety;
    return true;
  } else {
    return false;
  }
}

bool MemberAccessOperatorNode::CheckNodeToAccessValid(
    const OperatorNodeInterface& node_to_access) {
  // 只能对结构类型变量访问成员
  switch (node_to_access.GetResultTypePointer()->GetType()) {
    case StructOrBasicType::kEnum:
    case StructOrBasicType::kStruct:
    case StructOrBasicType::kUnion:
      return true;
      break;
    default:
      return false;
      break;
  }
}

bool MemberAccessOperatorNode::SetAccessedNodeAndMemberName(
    std::string&& member_name_to_set) {
  // 要被访问成员的节点的类型
  auto accessed_node_type = GetNodeToAccessReference().GetResultTypePointer();
  switch (accessed_node_type->GetType()) {
    case StructOrBasicType::kStruct: {
      auto& struct_type =
          static_cast<const c_parser_frontend::type_system::StructType&>(
              *accessed_node_type);
      MemberIndex struct_member_index =
          struct_type.GetStructMemberIndex(member_name_to_set);
      if (!struct_member_index.IsValid()) [[unlikely]] {
        // 不存在给定的成员名
        return false;
      }
      const auto& struct_member_info =
          struct_type.GetStructMemberInfo(struct_member_index);
      // 生成访问成员后得到的节点
      auto struct_node = std::make_shared<VarietyOperatorNode>(
          nullptr, struct_member_info.second, LeftRightValueTag::kLeftValue);
      bool result = struct_node->SetVarietyType(struct_member_info.first);
      assert(result);
      SetAccessedNode(struct_node);
      member_index_ = struct_member_index;
      return true;
    } break;
    case StructOrBasicType::kUnion: {
      auto& union_type =
          static_cast<const c_parser_frontend::type_system::UnionType&>(
              *accessed_node_type);
      MemberIndex union_member_index =
          union_type.GetUnionMemberIndex(member_name_to_set);
      if (!union_member_index.IsValid()) [[unlikely]] {
        // 不存在给定的成员名
        return false;
      }
      auto& union_member_info =
          union_type.GetUnionMemberInfo(union_member_index);
      // 生成访问成员后得到的节点
      auto union_node = std::make_shared<VarietyOperatorNode>(
          nullptr, union_member_info.second, LeftRightValueTag::kLeftValue);
      bool result = union_node->SetVarietyType(union_member_info.first);
      assert(result);
      SetAccessedNode(union_node);
      member_index_ = union_member_index;
      return true;
    } break;
    case StructOrBasicType::kEnum: {
      const auto& enum_type =
          static_cast<const c_parser_frontend::type_system::EnumType&>(
              *accessed_node_type);
      auto [enum_member_info_iter, enum_member_exist] =
          enum_type.GetEnumMemberInfo(member_name_to_set);
      if (!enum_member_exist) [[unlikely]] {
        // 不存在给定的成员名
        return false;
      }
      // 生成访问成员后得到的节点
      auto enum_node = std::make_shared<BasicTypeInitializeOperatorNode>(
          InitializeType::kBasic,
          std::to_string(enum_member_info_iter->second));
      enum_node->SetInitDataType(enum_type.GetContainerTypePointer());
      SetAccessedNode(enum_node);
      return true;
    } break;
    default:
      assert(false);
      // 防止警告
      return false;
      break;
  }
}

// 如果不是可以赋值的情况则不会设置
// 输入指向要设置的用来赋值的节点的指针和是否为声明时赋值
// 声明时赋值会忽略被赋值的节点自身的const属性并且允许使用初始化列表
// 需要先设置被赋值的节点（SetNodeToBeAssigned）
AssignableCheckResult AssignOperatorNode::SetNodeForAssign(
    const std::shared_ptr<const OperatorNodeInterface>& node_for_assign,
    bool is_announce) {
  AssignableCheckResult assignable_check_result = CheckAssignable(
      GetNodeToBeAssignedReference(), *node_for_assign, is_announce);
  switch (assignable_check_result) {
    case AssignableCheckResult::kNonConvert:
    case AssignableCheckResult::kUpperConvert:
    case AssignableCheckResult::kConvertToVoidPointer:
    case AssignableCheckResult::kZeroConvertToPointer:
    case AssignableCheckResult::kUnsignedToSigned:
    case AssignableCheckResult::kSignedToUnsigned:
      node_for_assign_ = node_for_assign;
      break;
    default:
      // 不可以赋值的情况，直接返回
      break;
  }
  return assignable_check_result;
}

// 检查给定节点的类型是否可以赋值
// 函数的模板参数表示是否为声明时赋值
// 当is_announce == true时
// 忽略node_to_be_assigned的const标记且允许使用初始化列表
AssignableCheckResult AssignOperatorNode::CheckAssignable(
    const OperatorNodeInterface& node_to_be_assigned,
    const OperatorNodeInterface& node_for_assign, bool is_announce) {
  if (!is_announce) [[unlikely]] {
    // 不是声明时赋值则要检查被赋值的节点是否为const
    if (node_to_be_assigned.GetResultConstTag() == ConstTag::kConst)
        [[unlikely]] {
      // 被赋值的节点是const且不是声明语句中
      return AssignableCheckResult::kAssignedNodeIsConst;
    }
  }
  AssignableCheckResult check_result;
  switch (node_to_be_assigned.GetGeneralOperatorType()) {
    case GeneralOperationType::kLogicalOperation:
    case GeneralOperationType::kMathematicalOperation:
      // 数值运算与逻辑运算结果均为右值，不能被赋值
    case GeneralOperationType::kInitValue:
      // 初始化值为右值，不能被赋值
    case GeneralOperationType::kFunctionCall:
      // C语言中函数返回值为右值，不能被赋值
      check_result = AssignableCheckResult::kAssignToRightValue;
      break;
    case GeneralOperationType::kVariety: {
      auto variety_node =
          static_cast<const VarietyOperatorNode&>(node_to_be_assigned);
      // 变量可能是左值也可能是右值的中间变量，需要检验
      if (variety_node.GetLeftRightValueTag() == LeftRightValueTag::kRightValue)
          [[unlikely]] {
        // 右值不能被赋值
        check_result = AssignableCheckResult::kAssignToRightValue;
        break;
      }
    }
      [[fallthrough]];
    case GeneralOperationType::kAssign:
    case GeneralOperationType::kMemberAccess:
    case GeneralOperationType::kDeReference: {
      check_result =
          node_to_be_assigned.GetResultTypePointer()->CanBeAssignedBy(
              *node_for_assign.GetResultTypePointer());
      switch (check_result) {
        case AssignableCheckResult::kNonConvert: {
          // 如果被赋值的节点与用来赋值的节点类型完全相同（operator==()）
          // 且为变量类型则设置被赋值的节点的类型链指针指向用来赋值的类型链
          // 以节省内存（违反const原则）
          auto node_to_be_assigned_type_pointer =
              node_to_be_assigned.GetResultTypePointer();
          auto node_for_assign_type_pointer =
              node_for_assign.GetResultTypePointer();
          if (node_to_be_assigned.GetGeneralOperatorType() ==
                  GeneralOperationType::kVariety &&
              *node_to_be_assigned_type_pointer ==
                  *node_for_assign_type_pointer) [[likely]] {
            const_cast<VarietyOperatorNode&>(
                static_cast<const VarietyOperatorNode&>(node_to_be_assigned))
                .SetVarietyType(node_for_assign_type_pointer);
          }
        }
          [[fallthrough]];
        case AssignableCheckResult::kUpperConvert:
        case AssignableCheckResult::kConvertToVoidPointer:
        case AssignableCheckResult::kZeroConvertToPointer:
        case AssignableCheckResult::kUnsignedToSigned:
        case AssignableCheckResult::kSignedToUnsigned:
          break;
        case AssignableCheckResult::kMayBeZeroToPointer:
          // 如果可能是将0赋值给指针则需要额外检查
          // 检查用来赋值的是否为编译期常量
          if (node_for_assign.GetGeneralOperatorType() ==
              GeneralOperationType::kInitValue) [[likely]] {
            const BasicTypeInitializeOperatorNode& initialize_node =
                static_cast<const BasicTypeInitializeOperatorNode&>(
                    node_for_assign);
            // 确认初始化值是否为0
            if (initialize_node.GetInitializeType() == InitializeType::kBasic &&
                initialize_node.GetValue() == "0") [[likely]] {
              check_result = AssignableCheckResult::kZeroConvertToPointer;
            } else {
              // 不能将除了0以外的值赋值给指针
              check_result = AssignableCheckResult::kCanNotConvert;
            }
          } else {
            // 无法直接赋值，需要强制类型转换
            check_result = AssignableCheckResult::kCanNotConvert;
          }
          break;
        case AssignableCheckResult::kInitializeList:
          // 用来赋值的是初始化列表，调用相应的函数处理
          return VarietyAssignableByInitializeList(
              static_cast<const VarietyOperatorNode&>(node_to_be_assigned),
              static_cast<const ListInitializeOperatorNode&>(node_for_assign));
          break;
        case AssignableCheckResult::kLowerConvert:
        case AssignableCheckResult::kCanNotConvert:
        case AssignableCheckResult::kAssignedNodeIsConst:
        case AssignableCheckResult::kAssignToRightValue:
        case AssignableCheckResult::kArgumentsFull:
          break;
        default:
          assert(false);
          break;
      }
    } break;
    default:
      assert(false);
      break;
  }
  return check_result;
}

AssignableCheckResult AssignOperatorNode::VarietyAssignableByInitializeList(
    const VarietyOperatorNode& variety_node,
    const ListInitializeOperatorNode& list_initialize_operator_node) {
  switch (variety_node.GetVarietyTypeReference().GetType()) {
    case StructOrBasicType::kBasic:
    case StructOrBasicType::kEnum: {
      // 对基础变量使用初始化列表时初始化列表中只能存在一个值
      // 且只使用一层初始化列表
      auto initialize_members = list_initialize_operator_node.GetListValues();
      // 检查初始化列表中是否只存在一个值
      if (initialize_members.size() == 1) [[likely]] {
        auto& initialize_value = initialize_members.front();
        // 检查是否只使用一层初始化列表
        if (initialize_value->GetInitializeType() !=
            InitializeType::kInitializeList) [[unlikely]] {
          return CheckAssignable(variety_node, *initialize_value, true);
        }
      }
      // 不符合上述条件，无法赋值
      return AssignableCheckResult::kCanNotConvert;
    } break;
    case StructOrBasicType::kPointer: {
      const auto& pointer_type =
          static_cast<const c_parser_frontend::type_system::PointerType&>(
              variety_node.GetVarietyTypeReference());
      const auto& list_values = list_initialize_operator_node.GetListValues();
      size_t list_size = list_values.size();
      size_t pointer_array_size = pointer_type.GetArraySize();
      // 检查是否为纯指针使用初始化列表初始化
      if (pointer_array_size == 0) {
        // 纯指针，使用变量对变量赋值语义
        // 检查用来赋值的初始化列表是否只有一个值且这个值不是初始化列表
        auto& value_for_assign =
            static_cast<const BasicTypeInitializeOperatorNode&>(
                *list_values.front());
        if (list_values.size() == 1 && value_for_assign.GetInitializeType() !=
                                           InitializeType::kInitializeList)
            [[likely]] {
          return CheckAssignable(variety_node, value_for_assign, true);
        } else {
          return AssignableCheckResult::kCanNotConvert;
        }
      }
      // 构建参与下一轮比较的临时节点
      auto [dereferenced_type, const_tag] = pointer_type.DeReference();
      auto sub_variety_node = std::make_unique<VarietyOperatorNode>(
          nullptr, const_tag, LeftRightValueTag::kLeftValue);
      auto set_type_result =
          sub_variety_node->SetVarietyTypeNoCheckFunctionType(
              dereferenced_type);
      AssignableCheckResult check_result =
          AssignableCheckResult::kCanNotConvert;
      // 检查是否初始化列表中每一个值都可以赋值
      for (const auto& value : list_values) {
        check_result = CheckAssignable(*sub_variety_node, *value, true);
        switch (check_result) {
          case AssignableCheckResult::kNonConvert:
          case AssignableCheckResult::kUpperConvert:
          case AssignableCheckResult::kConvertToVoidPointer:
          case AssignableCheckResult::kZeroConvertToPointer:
          case AssignableCheckResult::kUnsignedToSigned:
          case AssignableCheckResult::kSignedToUnsigned:
            break;
          case AssignableCheckResult::kLowerConvert:
          case AssignableCheckResult::kCanNotConvert:
          case AssignableCheckResult::kAssignedNodeIsConst:
          case AssignableCheckResult::kInitializeListTooLarge:
            // 无法赋值的情况
            // 直接返回，不检测剩余部分
            return check_result;
            break;
          case AssignableCheckResult::kInitializeList:
            // 在调用该函数前已经被CheckAssignable拦截掉
          case AssignableCheckResult::kAssignToRightValue:
          case AssignableCheckResult::kArgumentsFull:
          default:
            assert(false);
            break;
        }
      }
      // 所有节点均通过测试，检查指针指向的数组大小
      if (pointer_array_size == -1) {
        // 需要自动计算数组大小
        const_cast<c_parser_frontend::type_system::PointerType&>(pointer_type)
            .SetArraySize(list_size);
      } else if (list_size > pointer_array_size) [[unlikely]] {
        // 初始化列表给出的数据数目大于指针声明时的数组大小
        check_result = AssignableCheckResult::kInitializeListTooLarge;
      }
      return check_result;
    } break;
    case StructOrBasicType::kUnion: {
      const auto& list_values = list_initialize_operator_node.GetListValues();
      const auto& union_size =
          static_cast<
              const c_parser_frontend::type_system::StructureTypeInterface&>(
              variety_node.GetVarietyTypeReference())
              .TypeSizeOf();
      // 检查初始化列表中是否只存储了一个成员且该成员不是初始化列表
      if (list_values.size() == 1 && list_values.front()->GetInitializeType() !=
                                         InitializeType::kInitializeList)
          [[likely]] {
        // 检查用来赋值的对象是否小于等于共用体内最大的对象
        if (union_size >=
            list_values.front()->GetResultTypePointer()->TypeSizeOf()) {
          return AssignableCheckResult::kNonConvert;
        }
      }
      return AssignableCheckResult::kCanNotConvert;
    }
    case StructOrBasicType::kStruct: {
      const auto& list_values = list_initialize_operator_node.GetListValues();
      const auto& structure_members =
          static_cast<
              const c_parser_frontend::type_system::StructureTypeInterface&>(
              variety_node.GetVarietyTypeReference())
              .GetStructureMembers();
      // 构建比较用临时节点
      auto sub_variety_node = std::make_unique<VarietyOperatorNode>(
          nullptr, ConstTag::kNonConst, LeftRightValueTag::kLeftValue);
      if (list_values.size() > structure_members.size()) [[unlikely]] {
        // 给定的初始化成员列表大小大于结构体内成员数目
        return AssignableCheckResult::kInitializeListTooLarge;
      }
      // 检验初始化列表中的值是否可以给结构体成员按顺序赋值
      AssignableCheckResult check_result =
          AssignableCheckResult::kCanNotConvert;
      auto structure_member_iter = structure_members.begin();
      auto list_iter = list_values.begin();
      for (; list_iter != list_values.end();
           ++list_iter, ++structure_member_iter) {
        // 设置结构体成员属性
        sub_variety_node->SetVarietyType(structure_member_iter->first);
        sub_variety_node->SetConstTag(structure_member_iter->second);
        check_result = CheckAssignable(*sub_variety_node, **list_iter, true);
        switch (check_result) {
          case AssignableCheckResult::kNonConvert:
          case AssignableCheckResult::kUpperConvert:
          case AssignableCheckResult::kConvertToVoidPointer:
          case AssignableCheckResult::kZeroConvertToPointer:
          case AssignableCheckResult::kUnsignedToSigned:
          case AssignableCheckResult::kSignedToUnsigned:
            break;
          case AssignableCheckResult::kLowerConvert:
          case AssignableCheckResult::kCanNotConvert:
          case AssignableCheckResult::kAssignedNodeIsConst:
          case AssignableCheckResult::kInitializeListTooLarge:
            // 无法赋值，无需判断剩余对象直接返回
            return check_result;
            break;
          case AssignableCheckResult::kInitializeList:
          case AssignableCheckResult::kAssignToRightValue:
          case AssignableCheckResult::kArgumentsFull:
          case AssignableCheckResult::kMayBeZeroToPointer:
          default:
            assert(false);
            break;
        }
      }
      // 初始化列表中所有的值都可以给结构体成员赋值
      return check_result;
    } break;
    case StructOrBasicType::kInitializeList:
    case StructOrBasicType::kFunction:
    default:
      assert(false);
      // 防止警告
      return AssignableCheckResult();
      break;
  }
}

// 返回是否可以设置，如果不可设置则不会设置

bool ObtainAddressOperatorNode::SetNodeToObtainAddress(
    const std::shared_ptr<const VarietyOperatorNode>& node_to_obtain_address) {
  bool check_result = CheckNodeToObtainAddress(*node_to_obtain_address);
  if (check_result) [[likely]] {
    // 可以取地址
    // 设置将要被取地址的指针
    node_to_obtain_address_ = node_to_obtain_address;
    // 生成取地址后得到的变量的类型
    auto obtained_type = TypeInterface::ObtainAddressOperatorNode(
        node_to_obtain_address->GetResultTypePointer());
    // 创建取地址后得到的节点（生成匿名右值中间变量）
    auto node_obtained_address = std::make_shared<VarietyOperatorNode>(
        nullptr, ConstTag::kNonConst, LeftRightValueTag::kRightValue);
    // 设置取地址后的变量节点类型
    bool result =
        node_obtained_address->SetVarietyType(std::move(obtained_type));
    assert(result == true);
    // 设置取地址后得到的节点
    SetNodeObtainedAddress(std::move(node_obtained_address));
  }
  return check_result;
}

inline bool ObtainAddressOperatorNode::CheckNodeToObtainAddress(
    const OperatorNodeInterface& node_interface) {
  switch (node_interface.GetGeneralOperatorType()) {
    case GeneralOperationType::kDeReference:
    case GeneralOperationType::kMemberAccess:
    case GeneralOperationType::kAssign:
    case GeneralOperationType::kObtainAddress:
    case GeneralOperationType::kVariety:
      return true;
      break;
    default:
      return false;
      break;
  }
}

bool DereferenceOperatorNode::SetNodeToDereference(
    const std::shared_ptr<const OperatorNodeInterface>& node_to_dereference) {
  if (CheckNodeDereferenceAble(*node_to_dereference)) [[likely]] {
    // 设置待解引用的节点
    node_to_dereference_ = node_to_dereference;
    // 创建解引用后得到的节点
    auto node_to_dereference_type = std::static_pointer_cast<
        const c_parser_frontend::type_system::PointerType>(
        node_to_dereference->GetResultTypePointer());
    // 获取解引用后的类型和解引用的结果
    auto [dereferenced_node_type, dereferenced_node_const_tag] =
        node_to_dereference_type->DeReference();
    auto dereferenced_node = std::make_shared<VarietyOperatorNode>(
        nullptr, dereferenced_node_const_tag, LeftRightValueTag::kRightValue);
    bool result = dereferenced_node->SetVarietyTypeNoCheckFunctionType(
        std::move(dereferenced_node_type));
    assert(result == true);
    SetDereferencedNode(std::move(dereferenced_node));
    return true;
  } else {
    return false;
  }
}

inline bool DereferenceOperatorNode::CheckNodeDereferenceAble(
    const OperatorNodeInterface& node_to_dereference) {
  // 当且仅当为指针时可以解引用
  // 且要求解引用得到的不是void类型
  auto result_type = node_to_dereference.GetResultTypePointer();
  bool result = result_type->GetType() == StructOrBasicType::kPointer;
  // 获取指针节点的下一个节点，判断是否为void类型
  auto& next_type_node = result_type->GetNextNodeReference();
  result &= !(next_type_node.GetType() == StructOrBasicType::kBasic &&
              static_cast<const c_parser_frontend::type_system::BasicType&>(
                  next_type_node)
                      .GetBuiltInType() == BuiltInType::kVoid);
  return result;
}

bool LogicalOperationOperatorNode::SetLeftOperatorNode(
    const std::shared_ptr<const OperatorNodeInterface>& left_operator_node) {
  std::shared_ptr<const TypeInterface> left_operator_node_type =
      left_operator_node->GetResultTypePointer();
  if (CheckLogicalTypeValid(*left_operator_node_type)) [[likely]] {
    left_operator_node_ = left_operator_node;
    return true;
  } else {
    return false;
  }
}

bool LogicalOperationOperatorNode::SetRightOperatorNode(
    const std::shared_ptr<const OperatorNodeInterface>& right_operator_node) {
  std::shared_ptr<const TypeInterface> right_operator_node_type =
      right_operator_node->GetResultTypePointer();
  if (CheckLogicalTypeValid(*right_operator_node_type)) [[likely]] {
    right_operator_node_ = right_operator_node;
    CreateAndSetResultNode();
    return true;
  } else {
    return false;
  }
}

// 检查是否可以参与逻辑运算

inline bool LogicalOperationOperatorNode::CheckLogicalTypeValid(
    const TypeInterface& type_interface) {
  switch (type_interface.GetType()) {
    case StructOrBasicType::kBasic:
    case StructOrBasicType::kPointer:
      return true;
      break;
    default:
      return false;
      break;
  }
}

bool TypeConvert::GenerateDestinationNode(
    const std::shared_ptr<const TypeInterface>& new_type,
    ConstTag new_const_tag) {
  // 获取最终结果节点作为用来转换的节点
  auto node_for_convert = GetSourceNodeReference().GetResultOperatorNode();
  // 如果返回nullptr则代表自身就是结果节点，如VarietyOperatorNode
  if (node_for_convert == nullptr) [[likely]] {
    node_for_convert = GetSourceNodePointer();
  }
  // 复制一份对象作为转换后得到的对象并修改类型为转换到的类型
  auto node_converted = node_for_convert->SelfCopy(new_type);
  // 判断是否不可复制
  if (node_converted == nullptr) [[unlikely]] {
    return false;
  }
  destination_node_ = std::move(node_converted);
  return true;
}

bool TemaryOperatorNode::SetBranchCondition(
    const std::shared_ptr<const OperatorNodeInterface>& branch_condition,
    const std::shared_ptr<const std::list<
        std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
        flow_control_node_container) {
  if (CheckBranchConditionValid(*branch_condition)) [[likely]] {
    branch_condition_ = branch_condition;
    condition_flow_control_node_container_ = flow_control_node_container;
    return true;
  } else {
    return false;
  }
}

bool TemaryOperatorNode::SetTrueBranch(
    const std::shared_ptr<const OperatorNodeInterface>& true_branch,
    const std::shared_ptr<const std::list<
        std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
        flow_control_node_container) {
  assert(GetBranchConditionPointer() != nullptr);
  if (GetBranchConditionReference().GetGeneralOperatorType() !=
      GeneralOperationType::kInitValue) [[likely]] {
    // 分支条件不为编译期常量
    if (CheckBranchValid(*true_branch)) [[likely]] {
      true_branch_ = true_branch;
      true_branch_flow_control_node_container_ = flow_control_node_container;
      return true;
    } else {
      return false;
    }
  } else {
    // 分支条件为编译期常量
    const auto& branch_condition =
        static_cast<const BasicTypeInitializeOperatorNode&>(
            GetBranchConditionReference());
    assert(branch_condition.GetInitializeType() !=
           InitializeType::kInitializeList);
    const std::string& branch_value = branch_condition.GetValue();
    assert(branch_value == "0" || branch_value == "1");
    if (branch_value == "1") {
      // 选择真分支，需要检查
      if (CheckBranchValid(*true_branch)) [[likely]] {
        true_branch_ = true_branch;
        true_branch_flow_control_node_container_ = flow_control_node_container;
        // 同时设置结果分支节点
        result_ = true_branch;
        return true;
      } else {
        return false;
      }
    } else {
      // 选择假分支，真分支随意设置
      true_branch_ = true_branch;
      true_branch_flow_control_node_container_ = flow_control_node_container;
      return true;
    }
  }
  ConstructResultNode();
}

bool TemaryOperatorNode::SetFalseBranch(
    const std::shared_ptr<const OperatorNodeInterface>& false_branch,
    const std::shared_ptr<const std::list<
        std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>>&
        flow_control_node_container) {
  assert(GetBranchConditionPointer() != nullptr);
  if (GetBranchConditionReference().GetGeneralOperatorType() !=
      GeneralOperationType::kInitValue) [[likely]] {
    // 分支条件不为编译期常量
    if (CheckBranchValid(*false_branch)) [[likely]] {
      false_branch_ = false_branch;
      false_branch_flow_control_node_container_ = flow_control_node_container;
      return true;
    } else {
      return false;
    }
  } else {
    // 分支条件为编译期常量
    const auto& branch_condition =
        static_cast<const BasicTypeInitializeOperatorNode&>(
            GetBranchConditionReference());
    assert(branch_condition.GetInitializeType() !=
           InitializeType::kInitializeList);
    const std::string& branch_value = branch_condition.GetValue();
    assert(branch_value == "0" || branch_value == "1");
    if (branch_value == "0") {
      // 选择假分支，需要检查
      if (CheckBranchValid(*false_branch)) [[likely]] {
        false_branch_ = false_branch;
        false_branch_flow_control_node_container_ = flow_control_node_container;
        // 同时设置结果分支节点
        result_ = false_branch;
        return true;
      } else {
        return false;
      }
    } else {
      // 选择真分支，假分支随意设置
      false_branch_ = false_branch;
      false_branch_flow_control_node_container_ = flow_control_node_container;
      return true;
    }
  }
  ConstructResultNode();
}

bool TemaryOperatorNode::CheckBranchConditionValid(
    const OperatorNodeInterface& branch_condition) {
  auto result_type = branch_condition.GetResultTypePointer();
  switch (result_type->GetType()) {
    case StructOrBasicType::kBasic:
      // 进一步判断是否为void
      return static_cast<const c_parser_frontend::type_system::BasicType&>(
                 *result_type)
                 .GetBuiltInType() != BuiltInType::kVoid;
      break;
    case StructOrBasicType::kPointer:
      return true;
      break;
    default:
      return false;
      break;
  }
}

bool TemaryOperatorNode::CheckBranchValid(const OperatorNodeInterface& branch) {
  return CheckBranchConditionValid(branch);
}

bool TemaryOperatorNode::ConstructResultNode() {
  // 检查分支条件是否为编译期常量
  // 如果为常量则结果节点已设置
  // 检查是否已设置两分支
  if (GetBranchConditionReference().GetGeneralOperatorType() !=
          GeneralOperationType::kInitValue &&
      GetTrueBranchPointer() != nullptr && GetFalseBranchPointer() != nullptr)
      [[likely]] {
    // 结果分支未创建
    // 获取两个分支都可以转换到的类型
    std::shared_ptr<const TypeInterface> common_type;
    auto assignable_check_result = AssignOperatorNode::CheckAssignable(
        GetTrueBranchReference(), GetFalseBranchReference(), false);
    switch (assignable_check_result) {
      case AssignableCheckResult::kNonConvert:
      case AssignableCheckResult::kUpperConvert:
      case AssignableCheckResult::kConvertToVoidPointer:
      case AssignableCheckResult::kZeroConvertToPointer:
      case AssignableCheckResult::kUnsignedToSigned:
      case AssignableCheckResult::kSignedToUnsigned:
        common_type = GetTrueBranchReference().GetResultTypePointer();
        break;
      case AssignableCheckResult::kLowerConvert:
        // 可以反向转换
        common_type = GetFalseBranchReference().GetResultTypePointer();
        break;
      case AssignableCheckResult::kInitializeList:
        // 不支持在非编译期常量条件下使用初始化列表
        return false;
        break;
      case AssignableCheckResult::kCanNotConvert:
      case AssignableCheckResult::kAssignedNodeIsConst:
      case AssignableCheckResult::kAssignToRightValue:
        // 无法从true分支转换到false分支，尝试反向转换
        assignable_check_result = AssignOperatorNode::CheckAssignable(
            GetFalseBranchReference(), GetTrueBranchReference(), false);
        switch (assignable_check_result) {
          case AssignableCheckResult::kCanNotConvert:
          case AssignableCheckResult::kAssignedNodeIsConst:
          case AssignableCheckResult::kAssignToRightValue:
            return false;
            break;
          case AssignableCheckResult::kZeroConvertToPointer:
          case AssignableCheckResult::kConvertToVoidPointer:
            common_type = GetFalseBranchReference().GetResultTypePointer();
            break;
            // 其余情况在外层已处理完毕或不应出现
          default:
            assert(false);
            break;
        }
        break;
      case AssignableCheckResult::kArgumentsFull:
      case AssignableCheckResult::kInitializeListTooLarge:
      case AssignableCheckResult::kMayBeZeroToPointer:
      default:
        assert(false);
        break;
    }
    auto result = std::make_shared<VarietyOperatorNode>(
        nullptr, ConstTag::kNonConst, LeftRightValueTag::kRightValue);
    bool set_type_result = result->SetVarietyType(common_type);
    assert(set_type_result);
    result_ = result;
  }
  return true;
}

MathematicalOperation
MathematicalAndAssignOperationToMathematicalOperation(
    MathematicalAndAssignOperation mathematical_and_assign_operation) {
  switch (mathematical_and_assign_operation) {
    case MathematicalAndAssignOperation::kOrAssign:
      return MathematicalOperation::kOr;
      break;
    case MathematicalAndAssignOperation::kXorAssign:
      return MathematicalOperation::kXor;
      break;
    case MathematicalAndAssignOperation::kAndAssign:
      return MathematicalOperation::kAnd;
      break;
    case MathematicalAndAssignOperation::kLeftShiftAssign:
      return MathematicalOperation::kLeftShift;
      break;
    case MathematicalAndAssignOperation::kRightShiftAssign:
      return MathematicalOperation::kRightShift;
      break;
    case MathematicalAndAssignOperation::kPlusAssign:
      return MathematicalOperation::kPlus;
      break;
    case MathematicalAndAssignOperation::kMinusAssign:
      return MathematicalOperation::kMinus;
      break;
    case MathematicalAndAssignOperation::kMultipleAssign:
      return MathematicalOperation::kMultiple;
      break;
    case MathematicalAndAssignOperation::kDivideAssign:
      return MathematicalOperation::kDivide;
      break;
    case MathematicalAndAssignOperation::kModAssign:
      return MathematicalOperation::kMod;
      break;
    default:
      assert(false);
      // 防止警告
      return MathematicalOperation();
      break;
  }
}

}  // namespace c_parser_frontend::operator_node

// 使用前向声明后定义移到这里以使用智能指针
namespace c_parser_frontend::type_system {
bool FunctionType::ArgumentInfo::operator==(
    const ArgumentInfo& argument_info) const {
  // 函数参数名不影响是否为同一参数，仅比较类型和是否为const
  return variety_operator_node->GetConstTag() ==
             argument_info.variety_operator_node->GetConstTag() &&
         variety_operator_node->GetVarietyTypeReference() ==
             argument_info.variety_operator_node->GetVarietyTypeReference();
}
void FunctionType::AddFunctionCallArgument(
    const std::shared_ptr<
        const c_parser_frontend::operator_node::VarietyOperatorNode>&
        argument) {
  argument_infos_.emplace_back(ArgumentInfo(argument));
}
}  // namespace c_parser_frontend::type_system