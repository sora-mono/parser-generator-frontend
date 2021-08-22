#include "operator_node.h"

namespace c_parser_frontend::operator_node {
inline bool VarietyOperatorNode::SetVarietyType(
    std::shared_ptr<const TypeInterface>&& variety_type) {
  if (CheckVarietyTypeValid(*variety_type)) [[likely]] {
    // 仅当符合变量的类型的标准时才可以设置
    variety_type_ = std::move(variety_type);
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
    std::shared_ptr<const TypeInterface>&& variety_type) {
  if (CheckBasicTypeInitializeValid(*variety_type)) [[likely]] {
    return SetInitValueType(std::move(variety_type));
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
inline bool InitializeOperatorNode::SetInitValueType(
    std::shared_ptr<const TypeInterface>&& init_value_type) {
  if (CheckInitValueTypeValid(*init_value_type)) [[likely]] {
    initialize_value_type_ = std::move(init_value_type);
    return true;
  } else {
    return false;
  }
}

inline bool InitializeOperatorNode::CheckInitValueTypeValid(
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

inline bool ListInitializeOperatorNode::AddListValue(
    std::shared_ptr<const InitializeOperatorNode>&& list_value) {
  if (CheckInitListValueTypeValid(*list_value->GetResultTypePointer()))
      [[likely]] {
    list_values_.emplace_back(std::move(list_value));
    return true;
  } else {
    return false;
  }
}

inline bool ListInitializeOperatorNode::SetInitListType(
    std::shared_ptr<const TypeInterface>&& list_type) {
  if (CheckInitListTypeValid(*list_type)) [[likely]] {
    return SetInitValueType(std::move(list_type));
  } else {
    return false;
  }
}

inline bool MathematicalOperatorNode::SetLeftOperatorNode(
    std::shared_ptr<const OperatorNodeInterface>&& left_operator_node) {
  switch (GetMathematicalOperation()) {
    case MathematicalOperation::kNot: {
      // 取反为一元运算符，结果类型与运算数类型相同
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
      left_operator_node_ = std::move(left_operator_node);
      break;
  }
  return true;
}

MathematicalOperatorNode::DeclineMathematicalComputeTypeResult
MathematicalOperatorNode::SetRightOperatorNode(
    std::shared_ptr<const OperatorNodeInterface>&& right_operator_node) {
  switch (GetMathematicalOperation()) {
    case MathematicalOperation::kNot:
      // 一元运算符不应调用此函数
      assert(false);
      break;
    default:
      break;
  }
  auto [compute_result_node, decline_result] = DeclineComputeResult(
      GetMathematicalOperation(), GetLeftOperatorNodePointer(),
      std::shared_ptr<const OperatorNodeInterface>(right_operator_node));
  if (compute_result_node != nullptr) [[likely]] {
    // 可以运算，设置运算右节点和运算结果节点
    right_operator_node_ = std::move(right_operator_node);
    compute_result_node_ = std::move(compute_result_node);
  }
  return decline_result;
}

std::pair<std::shared_ptr<const VarietyOperatorNode>,
          MathematicalOperatorNode::DeclineMathematicalComputeTypeResult>
MathematicalOperatorNode::DeclineComputeResult(
    MathematicalOperation mathematical_operation,
    std::shared_ptr<const OperatorNodeInterface>&& left_operator_node,
    std::shared_ptr<const OperatorNodeInterface>&& right_operator_node) {
  std::shared_ptr<const TypeInterface> left_operator_node_type =
      left_operator_node->GetResultTypePointer();
  std::shared_ptr<const TypeInterface> right_operator_node_type =
      left_operator_node->GetResultTypePointer();
  auto [compute_result_type, compute_result] =
      TypeInterface::DeclineMathematicalComputeResult(
          std::shared_ptr<const TypeInterface>(left_operator_node_type),
          std::shared_ptr<const TypeInterface>(right_operator_node_type));
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
      return std::make_pair(std::shared_ptr<const VarietyOperatorNode>(nullptr),
                            compute_result);
      break;
  }
}

bool FunctionCallOperatorNode::SetFunctionType(
    std::shared_ptr<const TypeInterface>&& type_pointer) {
  // 判断类型是否合法
  if (CheckFunctionTypeValid(*type_pointer)) [[likely]] {
    // 获取最精确的类型
    const c_parser_frontend::type_system::FunctionType& function_type =
        static_cast<const c_parser_frontend::type_system::FunctionType&>(
            *type_pointer);
    if (CheckFunctionTypeValid(*type_pointer)) [[likely]] {
      // 成功设置函数调用后返回的类型
      // 清除原来生成的函数参数对象指针
      function_arguments_for_call_.clear();
      function_arguments_offerred_.clear();
      // 设置函数类型
      function_type_ =
          std::move(std::static_pointer_cast<
                    const c_parser_frontend::type_system::FunctionType>(
              type_pointer));
      // 根据函数类型创建返回类型对象
      // 返回类型匿名
      auto return_object = std::make_shared<VarietyOperatorNode>(
          nullptr, function_type.GetReturnTypeConstTag(),
          LeftRightValueTag::kRightValue);
      return_object->SetVarietyType(function_type.GetReturnTypePointer());
      return_object_ = return_object;
      // 根据函数参数创建最终提供给函数的参数的对象
      for (auto& argument : function_type.GetArgumentTypes()) {
        auto variety_node = std::make_shared<VarietyOperatorNode>(
            argument.argument_name, argument.argument_const_tag,
            LeftRightValueTag::kRightValue);
        bool result = variety_node->SetVarietyType(
            std::shared_ptr<const TypeInterface>(argument.argument_type));
        assert(result == true);
        function_arguments_for_call_.emplace_back(std::move(variety_node));
      }
      return true;
    }
  }
  return false;
}

// 添加一个参数，参数添加顺序从左到右
// 返回待添加的参数是否通过检验，未通过检验则不会添加

AssignableCheckResult FunctionCallOperatorNode::AddArgument(
    std::shared_ptr<const OperatorNodeInterface>&& argument_node) {
  // 将要放置节点的index
  size_t new_argument_index = function_arguments_offerred_.size();
  if (new_argument_index >= GetFunctionArgumentsForCall().size()) [[unlikely]] {
    // 函数参数已满，不能添加更多参数
    // 暂不支持可变参数
    return AssignableCheckResult::kArgumentsFull;
  }
  // 函数参数创建过程等同于声明并赋值
  AssignableCheckResult check_result = AssignOperatorNode::CheckAssignable(
      *GetFunctionArgumentsForCall()[new_argument_index], *argument_node, true);
  switch (check_result) {
    case AssignableCheckResult::kNonConvert:
    case AssignableCheckResult::kUpperConvert:
    case AssignableCheckResult::kConvertToVoidPointer:
    case AssignableCheckResult::kZeroConvertToPointer:
    case AssignableCheckResult::kUnsignedToSigned:
    case AssignableCheckResult::kSignedToUnsigned:
      // 可以添加
      function_arguments_for_call_.emplace_back(std::move(argument_node));
      break;
    default:
      // 不可以添加
      break;
  }
  return check_result;
}

inline bool SetTargetVariety(
    std::shared_ptr<const OperatorNodeInterface>&& target_variety) {
  switch (target_variety->GetGeneralOperatorType()) {
    case GeneralOperationType::kVariety:
      // 待分配空间的节点只能是变量
      target_variety = std::move(target_variety);
      return true;
      break;
    default:
      assert(false);
      // 防止警告
      return false;
      break;
  }
}

inline bool AllocateOperatorNode::SetTargetVariety(
    std::shared_ptr<const OperatorNodeInterface>&& target_variety) {
  if (CheckAllocatable(*target_variety)) [[likely]] {
    target_variety = std::move(target_variety);
    return true;
  } else {
    return false;
  }
}

bool AllocateOperatorNode::AddNumToAllocate(size_t num) {
  std::shared_ptr<const TypeInterface> type_now =
      target_variety_->GetResultTypePointer();
  // 检查是否有足够的维数给该数组个数
  for (size_t i = 0; i < num_to_allocate_.size(); i++) {
    if (type_now->GetType() == StructOrBasicType::kPointer) [[likely]] {
      type_now = type_now->GetNextNodePointer();
    } else {
      return false;
    }
  }
  // 检查是否可以容纳新增的一维
  if (type_now->GetType() == StructOrBasicType::kPointer) [[likely]] {
    num_to_allocate_.push_back(num);
    return true;
  } else {
    return false;
  }
}

inline bool MemberAccessOperatorNode::CheckNodeToAccessValid(
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
      auto [struct_member_info_iter, struct_member_exist] =
          static_cast<const c_parser_frontend::type_system::StructType&>(
              *accessed_node_type)
              .GetStructMemberInfo(member_name_to_set);
      if (!struct_member_exist) [[unlikely]] {
        // 不存在给定的成员名
        return false;
      }
      // 生成访问成员后得到的节点
      auto struct_node = std::make_shared<VarietyOperatorNode>(
          nullptr, struct_member_info_iter->second.first,
          LeftRightValueTag::kLeftValue);
      struct_node->SetVarietyType(std::shared_ptr<const TypeInterface>(
          struct_member_info_iter->second.second));
      SetAccessedNode(struct_node);
      member_name_ = std::move(member_name_to_set);
      return true;
    } break;
    case StructOrBasicType::kUnion: {
      auto [union_member_info_iter, union_member_exist] =
          static_cast<const c_parser_frontend::type_system::UnionType&>(
              *accessed_node_type)
              .GetUnionMemberInfo(member_name_to_set);
      if (!union_member_exist) [[unlikely]] {
        // 不存在给定的成员名
        return false;
      }
      // 生成访问成员后得到的节点
      auto union_node = std::make_shared<VarietyOperatorNode>(
          nullptr, union_member_info_iter->second.first,
          LeftRightValueTag::kLeftValue);
      union_node->SetVarietyType(std::shared_ptr<const TypeInterface>(
          union_member_info_iter->second.second));
      SetAccessedNode(union_node);
      member_name_ = std::move(member_name_to_set);
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
      member_name_ = std::move(member_name_to_set);
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
// 声明时赋值会忽略被赋值的节点自身的const属性
AssignableCheckResult AssignOperatorNode::SetNodeForAssign(
    std::shared_ptr<const OperatorNodeInterface>&& node_for_assign,
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
      node_for_assign_ = std::move(node_for_assign);
      break;
    default:
      // 不可以赋值的情况，直接返回
      break;
  }
  return assignable_check_result;
}

// 检查给定节点的类型是否可以赋值
// 函数的模板参数表示是否为声明时赋值
// 当is_announce == true时忽略node_to_be_assigned的const标记
AssignableCheckResult AssignOperatorNode::CheckAssignable(
    const OperatorNodeInterface& node_to_be_assigned,
    const OperatorNodeInterface& node_for_assign, bool is_announce) {
  if (!is_announce) {
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
    case GeneralOperationType::kVariety:
      // 变量可能是左值也可能是右值的中间变量，需要检验
      if (static_cast<const VarietyOperatorNode&>(node_to_be_assigned)
              .GetLeftRightValueTag() == LeftRightValueTag::kRightValue)
          [[unlikely]] {
        // 右值不能被赋值
        check_result = AssignableCheckResult::kAssignToRightValue;
        break;
      }
      [[fallthrough]];
    case GeneralOperationType::kAssign:
    case GeneralOperationType::kMemberAccess:
    case GeneralOperationType::kDeReference: {
      check_result =
          node_to_be_assigned.GetResultTypePointer()->CanBeAssignedBy(
              node_for_assign.GetResultTypePointer());
      // 如果可能是将0赋值给指针则需要额外检查
      // 只有使用编译期常量初始化才可能出现使用0赋值给指针的情况
      if (check_result == AssignableCheckResult::kMayBeZeroToPointer &&
          node_for_assign.GetGeneralOperatorType() ==
              GeneralOperationType::kInitValue) [[unlikely]] {
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
      }
    } break;
    default:
      assert(false);
      break;
  }
  return check_result;
}

// 返回是否可以设置，如果不可设置则不会设置

bool ObtainAddress::SetNodeToObtainAddress(
    std::shared_ptr<const OperatorNodeInterface>&& node_to_obtain_address) {
  bool check_result = CheckNodeToObtainAddress(*node_to_obtain_address);
  if (check_result) [[likely]] {
    // 可以取地址
    // 设置将要被取地址的指针
    node_to_obtain_address_ = node_to_obtain_address;
    // 生成取地址后得到的变量的类型
    auto obtained_type = TypeInterface::ObtainAddress(
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

inline bool ObtainAddress::CheckNodeToObtainAddress(
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
    std::shared_ptr<const OperatorNodeInterface>&& node_to_dereference) {
  if (CheckNodeDereferenceAble(*node_to_dereference)) [[likely]] {
    // 设置待解引用的节点
    node_to_dereference_ = std::move(node_to_dereference);
    // 创建解引用后得到的节点
    auto node_to_dereference_type = std::static_pointer_cast<
        const c_parser_frontend::type_system::PointerType>(
        node_to_dereference->GetResultTypePointer());
    // 获取解引用后的类型和解引用的结果
    auto [dereferenced_node_type, dereferenced_node_const_tag] =
        node_to_dereference_type->DeReference();
    auto dereferenced_node = std::make_shared<VarietyOperatorNode>(
        nullptr, dereferenced_node_const_tag, LeftRightValueTag::kRightValue);
    bool result =
        dereferenced_node->SetVarietyType(std::move(dereferenced_node_type));
    assert(result == true);
    SetDereferencedNode(std::move(dereferenced_node));
    return true;
  } else {
    return false;
  }
}

bool LogicalOperationOperatorNode::SetLeftOperatorNode(
    std::shared_ptr<const OperatorNodeInterface>&& left_operator_node) {
  std::shared_ptr<const TypeInterface> left_operator_node_type =
      left_operator_node->GetResultTypePointer();
  if (CheckLogicalTypeValid(*left_operator_node_type)) [[likely]] {
    left_operator_node_ = std::move(left_operator_node);
    return true;
  } else {
    return false;
  }
}

bool LogicalOperationOperatorNode::SetRightOperatorNode(
    std::shared_ptr<const OperatorNodeInterface>&& right_operator_node) {
  std::shared_ptr<const TypeInterface> right_operator_node_type =
      right_operator_node->GetResultTypePointer();
  if (CheckLogicalTypeValid(*right_operator_node_type)) [[likely]] {
    right_operator_node_ = std::move(right_operator_node);
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

}  // namespace c_parser_frontend::operator_node