#include "flow_control.h"

#include "action_scope_system.h"
namespace c_parser_frontend::flow_control {
// 添加一条函数内执行的语句（按出现顺序添加）
// 成功添加返回true，不返还控制权
// 如果不能添加则返回false与控制权

// 检查给定语句是否可以作为函数内出现的语句

bool SimpleSentence::SetSentenceOperateNode(
    const std::shared_ptr<OperatorNodeInterface>& sentence_operate_node) {
  if (CheckOperatorNodeValid(*sentence_operate_node)) [[likely]] {
    sentence_operate_node_ = sentence_operate_node;
    return true;
  } else {
    return false;
  }
}
inline bool SimpleSentence::CheckOperatorNodeValid(
    const OperatorNodeInterface& operator_node) {
  switch (operator_node.GetGeneralOperatorType()) {
    case GeneralOperationType::kAllocate:
    case GeneralOperationType::kAssign:
    case GeneralOperationType::kDeReference:
    case GeneralOperationType::kFunctionCall:
    case GeneralOperationType::kLogicalOperation:
    case GeneralOperationType::kMathematicalOperation:
    case GeneralOperationType::kMemberAccess:
    case GeneralOperationType::kObtainAddress:
      // 以上为可以成为基础语句的节点
      return true;
      break;
    default:
      return false;
      break;
  }
}

// 根据if语句类型判断应该添加到哪个部分
// 如果是FlowType::kIfSentence则添加到true_branch
// 如果是FlowType::kIfElseSentence则添加到false_branch

bool IfSentence::AddMainSentence(std::unique_ptr<FlowInterface>&& sentence) {
  if (GetFlowType() == FlowType::kIfSentence) [[likely]] {
    return AddTrueBranchSentence(std::move(sentence));
  } else {
    assert(GetFlowType() == FlowType::kIfElseSentence);
    return AddFalseBranchSentence(std::move(sentence));
  }
}

bool IfSentence::AddMainSentences(
    std::list<std::unique_ptr<FlowInterface>>&& sentences) {
  if (GetFlowType() == FlowType::kIfSentence) [[likely]] {
    return AddTrueBranchSentences(std::move(sentences));
  } else {
    assert(GetFlowType() == FlowType::kIfElseSentence);
    return AddFalseBranchSentences(std::move(sentences));
  }
}

void IfSentence::ConvertToIfElse() {
  assert(GetFlowType() == FlowType::kIfSentence);
  // 修改语句类型
  SetFlowType(FlowType::kIfElseSentence);
  // 创建else节点容器
  else_body_ = std::make_shared<std::list<std::unique_ptr<FlowInterface>>>();
}

inline bool IfSentence::AddFalseBranchSentence(
    std::unique_ptr<FlowInterface>&& else_body_sentence) {
  if (CheckElseBodySentenceValid(*else_body_sentence)) [[likely]] {
    else_body_->emplace_back(std::move(else_body_sentence));
    return true;
  } else {
    return false;
  }
}
bool IfSentence::AddFalseBranchSentences(
    std::list<std::unique_ptr<FlowInterface>>&& else_body_sentences) {
  for (const auto& sentence : else_body_sentences) {
    if (!CheckElseBodySentenceValid(*sentence)) [[unlikely]] {
      return false;
    }
  }
  else_body_->merge(std::move(else_body_sentences));
  return true;
}

inline bool ForSentence::AddForInitSentence(
    std::unique_ptr<FlowInterface>&& init_body_sentence) {
  if (CheckForBodySentenceValid(*init_body_sentence)) [[likely]] {
    AddSentence(std::move(init_body_sentence));
    return true;
  } else {
    return false;
  }
}
bool ForSentence::AddForInitSentences(
    std::list<std::unique_ptr<FlowInterface>>&& init_body_sentences) {
  for (const auto& sentence : init_body_sentences) {
    if (!CheckForBodySentenceValid(*sentence)) [[unlikely]] {
      return false;
    }
  }
  init_block_.merge(std::move(init_body_sentences));
  return true;
}
inline bool ForSentence::AddForRenewSentence(
    std::unique_ptr<FlowInterface>&& after_body_sentence) {
  if (CheckForBodySentenceValid(*after_body_sentence)) [[likely]] {
    AddSentence(std::move(after_body_sentence));
    return true;
  } else {
    return false;
  }
}

bool ForSentence::AddForRenewSentences(
    std::list<std::unique_ptr<FlowInterface>>&& after_body_sentences) {
  for (const auto& sentence : after_body_sentences) {
    if (!CheckForBodySentenceValid(*sentence)) [[unlikely]] {
      return false;
    }
  }
  after_body_sentences_.merge(std::move(after_body_sentences));
  return true;
}

// 默认对循环条件节点进行类型检查的函数
// 返回给定节点是否可以作为条件分支类语句的条件

// 向主块内添加语句，做参数检查，如果参数检查不通过则不添加

bool ConditionBlockInterface::SetCondition(
    const std::shared_ptr<const OperatorNodeInterface>& condition,
    std::list<std::unique_ptr<FlowInterface>>&& sentences_to_get_condition) {
  if (DefaultConditionCheck(*condition)) [[likely]] {
    condition_ = condition;
    sentence_to_get_condition_ = std::move(sentences_to_get_condition);
    return true;
  } else {
    return false;
  }
}

bool ConditionBlockInterface::AddSentence(
    std::unique_ptr<FlowInterface>&& sentence) {
  if (DefaultMainBlockSentenceCheck(*sentence)) {
    main_block_.emplace_back(std::move(sentence));
    return true;
  } else {
    return false;
  }
}

bool ConditionBlockInterface::AddSentences(
    std::list<std::unique_ptr<FlowInterface>>&& sentences) {
  for (const auto& sentence : sentences) {
    if (!DefaultMainBlockSentenceCheck(*sentence)) [[unlikely]] {
      return false;
    }
  }
  main_block_.merge(std::move(sentences));
  return true;
}

inline bool ConditionBlockInterface::DefaultConditionCheck(
    const OperatorNodeInterface& condition_node) {
  switch (condition_node.GetResultTypePointer()->GetType()) {
    case StructOrBasicType::kBasic:
    case StructOrBasicType::kPointer:
      return true;
    default:
      return false;
      break;
  }
}

// 默认检查主体内语句的函数
// 返回给定流程控制节点是否可以作为条件分支语句主体的内容

inline bool ConditionBlockInterface::DefaultMainBlockSentenceCheck(
    const FlowInterface& flow_interface) {
  switch (flow_interface.GetFlowType()) {
    case FlowType::kJmp:
    case FlowType::kLabel:
    case FlowType::kReturn:
    case FlowType::kWhileSentence:
    case FlowType::kDoWhileSentence:
    case FlowType::kForSentence:
    case FlowType::kIfSentence:
    case FlowType::kSimpleSentence:
    case FlowType::kSwitchSentence:
      return true;
      break;
    default:
      // 只有函数定义不允许在if语句中出现
      return false;
      break;
  }
}

bool SwitchSentence::CheckSwitchCaseAbleToAdd(
    const SwitchSentence& switch_node,
    const BasicTypeInitializeOperatorNode& case_value) {
  if (case_value.GetInitializeType() != InitializeType::kBasic) [[unlikely]] {
    // case的条件必须为立即数
    return false;
  }
  // 判断给定case条件是否已经存在
  auto iter = switch_node.GetSimpleCases().find(case_value.GetValue());
  return iter == switch_node.GetSimpleCases().end();
}

// 添加普通的case，返回是否添加成功，如果添加失败则不修改参数

bool SwitchSentence::AddSimpleCase(
    const std::shared_ptr<const BasicTypeInitializeOperatorNode>& case_value) {
  if (CheckSwitchCaseAbleToAdd(*this, *case_value)) [[likely]] {
    // 可以添加
    auto [label_for_jmp, label_in_body] =
        ConvertCaseValueToLabel(*this, *case_value);
    auto [iter, inserted] = simple_cases_.emplace(
        case_value->GetValue(),
        std::make_pair(case_value, std::move(label_for_jmp)));
    if (inserted) [[likely]] {
      // 标签未重名，复制一份标签添加到switch主体
      AddSentence(std::move(label_in_body));
    }
    return inserted;
  } else {
    return false;
  }
}
bool SwitchSentence::AddDefaultCase() {
  if (default_case_ != nullptr) [[unlikely]] {
    // 已经设置了默认标签
    return false;
  }
  auto [label_for_jmp, label_in_body] = GetDefaultLabel(*this);
  AddSentence(std::move(label_in_body));
  default_case_ = std::move(label_for_jmp);
  return true;
}
std::pair<std::unique_ptr<Label>, std::unique_ptr<Label>>
SwitchSentence::ConvertCaseValueToLabel(
    const SwitchSentence& switch_sentence,
    const BasicTypeInitializeOperatorNode& case_value) {
  // switch语句ID，用来生成独一无二的标签
  auto switch_node_id = switch_sentence.GetFlowId();
  // 生成标签名
  std::string label_name =
      std::format("switch_{:}_{:}", switch_node_id.GetThisNodeValue(),
                  case_value.GetValue());
  // switch跳转语句中使用的标签
  auto label_for_jmp = std::make_unique<Label>(label_name);
  // switch主体内使用的标签
  auto label_in_body = std::make_unique<Label>(std::move(label_name));
  return std::make_pair(std::move(label_for_jmp), std::move(label_in_body));
}
std::pair<std::unique_ptr<Label>, std::unique_ptr<Label>>
SwitchSentence::GetDefaultLabel(const SwitchSentence& switch_sentence) {
  // switch语句ID，用来生成独一无二的标签
  auto switch_node_id = switch_sentence.GetFlowId();
  // 生成标签名
  std::string label_name =
      std::format("switch_{:}_default", switch_node_id.GetThisNodeValue());
  // switch跳转语句中使用的标签
  auto label_for_jmp = std::make_unique<Label>(label_name);
  // switch主体内使用的标签
  auto label_in_body = std::make_unique<Label>(std::move(label_name));
  return std::make_pair(std::move(label_for_jmp), std::move(label_in_body));
}
bool Return::SetReturnTarget(
    const std::shared_ptr<const c_parser_frontend::type_system::FunctionType>
        function_to_return_from,
    const std::shared_ptr<const OperatorNodeInterface>& return_target) {
  assert(function_to_return_from != nullptr);
  if (return_target == nullptr) {
    auto& function_return_type =
        function_to_return_from->GetReturnTypeReference();
    if (function_return_type.GetType() == StructOrBasicType::kBasic &&
        static_cast<const c_parser_frontend::type_system::BasicType&>(
            function_return_type)
                .GetBuiltInType() ==
            c_parser_frontend::type_system::BuiltInType::kVoid) [[likely]] {
      // 函数无返回值
      return_target_ = return_target;
      function_to_return_from_ = function_to_return_from;
      return true;
    } else {
      return false;
    }
  }
  // 构建临时变量用于调用CheckAssignable函数
  c_parser_frontend::operator_node::VarietyOperatorNode
      temp_node_to_be_assigned(
          nullptr, ConstTag::kNonConst,
          c_parser_frontend::operator_node::LeftRightValueTag::kLeftValue);
  bool result = temp_node_to_be_assigned.SetVarietyType(
      function_to_return_from->GetReturnTypePointer());
  assert(result);
  switch (c_parser_frontend::operator_node::AssignOperatorNode::CheckAssignable(
      temp_node_to_be_assigned, *return_target, true)) {
    case AssignableCheckResult::kZeroConvertToPointer:
    case AssignableCheckResult::kUpperConvert:
    case AssignableCheckResult::kConvertToVoidPointer:
    case AssignableCheckResult::kNonConvert:
    case AssignableCheckResult::kSignedToUnsigned:
    case AssignableCheckResult::kUnsignedToSigned:
      return_target_ = return_target;
      function_to_return_from_ = function_to_return_from;
      return true;
      break;
    default:
      return false;
      break;
  }
}
}  // namespace c_parser_frontend::flow_control

// 使用前向声明后定义移到这里以使用智能指针
namespace c_parser_frontend::type_system {
FunctionType::~FunctionType() {}

bool FunctionType::CheckSentenceInFunctionValid(
    const FlowInterface& flow_interface) {
  switch (flow_interface.GetFlowType()) {
    case FlowType::kFunctionDefine:
      // C语言不允许嵌套定义函数
      return false;
      break;
    default:
      return true;
      break;
  }
}

const std::list<std::unique_ptr<FlowInterface>>& FunctionType::GetSentences()
    const {
  return *sentences_in_function_;
}

bool FunctionType::AddSentence(
    std::unique_ptr<FlowInterface>&& sentence_to_add) {
  if (CheckSentenceInFunctionValid(*sentence_to_add)) [[likely]] {
    sentences_in_function_->emplace_back(std::move(sentence_to_add));
    return true;
  } else {
    // 不能添加，返回控制权
    return false;
  }
}

bool FunctionType::AddSentences(
    std::list<std::unique_ptr<FlowInterface>>&& sentence_container) {
  // 检查是否容器内所有节点都可以添加
  for (const auto& sentence : sentence_container) {
    if (CheckSentenceInFunctionValid(*sentence)) [[unlikely]] {
      return false;
    }
  }
  // 通过检查，将容器内所有语句全部合并到主容器中
  sentences_in_function_->merge(std::move(sentence_container));
  return true;
}
}  // namespace c_parser_frontend::type_system

// 使用前向声明后定义移到这里以使用智能指针
namespace c_parser_frontend::operator_node {
FunctionCallOperatorNode::FunctionCallArgumentsContainer::
    FunctionCallArgumentsContainer() {}

FunctionCallOperatorNode::FunctionCallArgumentsContainer::
    ~FunctionCallArgumentsContainer() {}

const std::list<
    std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&
TemaryOperatorNode::GetFlowControlNodeToGetConditionReference() const {
  return *condition_flow_control_node_container_;
}

const std::list<
    std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&
TemaryOperatorNode::GetFlowControlNodeToGetTrueBranchReference() const {
  return *true_branch_flow_control_node_container_;
}

const std::list<
    std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&
TemaryOperatorNode::GetFlowControlNodeToGetFalseBranchReference() const {
  return *false_branch_flow_control_node_container_;
}

bool FunctionCallOperatorNode::SetArguments(
    FunctionCallArgumentsContainer&& container) {
  FunctionCallArgumentsContainer::ContainerType& raw_container =
      container.GetFunctionCallArguments();
  // 标准的函数调用参数，待设置容器中所有参数必须可以转换到位置相同的参数
  const auto& function_argument_standard =
      GetFunctionTypePointer()->GetArguments();
  if (raw_container.size() != function_argument_standard.size()) [[unlikely]] {
    // 调用给定的参数数目与函数参数数目不相等
    return false;
  }
  // 检查是否每个参数都可以转化为最终调用使用的参数类型
  auto iter_standard = function_argument_standard.begin();
  auto iter_container_to_set = raw_container.begin();
  for (size_t i = 0; i < raw_container.size(); i++) {
    AssignableCheckResult check_result = AssignOperatorNode::CheckAssignable(
        *iter_standard->variety_operator_node, *iter_container_to_set->first,
        true);
    switch (check_result) {
      case AssignableCheckResult::kNonConvert:
      case AssignableCheckResult::kUpperConvert:
      case AssignableCheckResult::kConvertToVoidPointer:
      case AssignableCheckResult::kZeroConvertToPointer:
      case AssignableCheckResult::kUnsignedToSigned:
      case AssignableCheckResult::kSignedToUnsigned:
        // 可以添加
        break;
      default:
        // 不可以添加
        return false;
        break;
    }
  }
  // 所有参数通过检查
  function_arguments_offerred_ = std::move(container);
  return true;
}

}  // namespace c_parser_frontend::operator_node