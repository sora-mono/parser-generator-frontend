#include "flow_control.h"

namespace c_parser_frontend::flow_control {
// 添加一条函数内执行的语句（按出现顺序添加）
// 成功添加返回true，不返还控制权
// 如果不能添加则返回false与控制权

inline std::pair<std::unique_ptr<FlowInterface>, bool>
FunctionDefine::AddSentence(std::unique_ptr<FlowInterface>&& sentence_to_add) {
  if (CheckSentenceInFunctionValid(*sentence_to_add)) [[likely]] {
    sentences_in_function_.emplace_back(std::move(sentence_to_add));
    return std::make_pair(std::unique_ptr<FlowInterface>(), true);
  } else {
    // 不能添加，返回控制权
    return std::make_pair(std::move(sentence_to_add), false);
  }
}

inline bool FunctionDefine::CheckSentenceInFunctionValid(
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
inline bool SimpleSentence::SetSentenceOperateNode(
    std::shared_ptr<const OperatorNodeInterface>&& sentence_operate_node) {
  if (CheckOperatorNodeValid(*sentence_operate_node)) [[likely]] {
    sentence_operate_node_ = std::move(sentence_operate_node);
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
inline bool IfSentenceInterface::SetIfCondition(
    std::shared_ptr<const OperatorNodeInterface>&& if_condition) {
  if (CheckIfConditionValid(*if_condition)) [[likely]] {
    SetCondition(std::move(if_condition));
    return true;
  } else {
    return false;
  }
}

// 添加if语句内执行的内容
// 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
// 添加成功则不返还流程节点控制权

inline std::pair<std::unique_ptr<FlowInterface>, bool>
IfSentenceInterface::AddIfBodySentence(
    std::unique_ptr<FlowInterface>&& if_body_sentence) {
  if (CheckIfBodySentenceValid(*if_body_sentence)) [[likely]] {
    AddMainBlockSentence(std::move(if_body_sentence));
    return std::make_pair(std::unique_ptr<FlowInterface>(), true);
  } else {
    return std::make_pair(std::move(if_body_sentence), false);
  }
}
// 检查给定语句是否可以作为if内部的语句

// 添加else语句内执行的内容
// 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
// 添加成功则不返还流程节点控制权

inline std::pair<std::unique_ptr<FlowInterface>, bool>
IfElseSentence::AddElseBodySentence(
    std::unique_ptr<FlowInterface>&& else_body_sentence) {
  if (CheckElseBodySentenceValid(*else_body_sentence)) [[likely]] {
    else_body_.emplace_back(std::move(else_body_sentence));
    return std::make_pair(std::unique_ptr<FlowInterface>(), true);
  } else {
    return std::make_pair(std::move(else_body_sentence), false);
  }
}
inline bool WhileSentence::SetWhileCondition(
    std::shared_ptr<const OperatorNodeInterface>&& while_condition) {
  if (CheckWhileConditionValid(*while_condition)) [[unlikely]] {
    SetCondition(std::move(while_condition));
    return true;
  } else {
    return false;
  }
}

// 添加while语句内执行的内容
// 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
// 添加成功则不返还流程节点控制权

inline std::pair<std::unique_ptr<FlowInterface>, bool>
WhileSentence::AddWhileBodySentence(
    std::unique_ptr<FlowInterface>&& while_body_sentence) {
  if (CheckWhileBodySentenceValid(*while_body_sentence)) [[likely]] {
    AddMainBlockSentence(std::move(while_body_sentence));
    return std::make_pair(std::unique_ptr<FlowInterface>(), true);
  } else {
    return std::make_pair(std::move(while_body_sentence), false);
  }
}

// 添加for语句内执行的内容
// 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
// 添加成功则不返还流程节点控制权

// 返回是否可以设置if判断条件，如果不能设置则返回false且不设置

inline bool ForSentence::SetForCondition(
    std::shared_ptr<const OperatorNodeInterface>&& for_condition) {
  if (CheckForConditionValid(*for_condition)) [[likely]] {
    SetCondition(std::move(for_condition));
    return true;
  } else {
    return false;
  }
}

inline std::pair<std::unique_ptr<FlowInterface>, bool>
ForSentence::AddForBodySentence(
    std::unique_ptr<FlowInterface>&& for_body_sentence) {
  if (CheckForBodySentenceValid(*for_body_sentence)) [[likely]] {
    AddMainBlockSentence(std::move(for_body_sentence));
    return std::make_pair(std::unique_ptr<FlowInterface>(), true);
  } else {
    return std::make_pair(std::move(for_body_sentence), false);
  }
}
inline std::pair<std::unique_ptr<FlowInterface>, bool>
ForSentence::AddForInitSentence(
    std::unique_ptr<FlowInterface>&& init_body_sentence) {
  if (CheckForBodySentenceValid(*init_body_sentence)) [[likely]] {
    AddMainBlockSentence(std::move(init_body_sentence));
    return std::make_pair(std::unique_ptr<FlowInterface>(), true);
  } else {
    return std::make_pair(std::move(init_body_sentence), false);
  }
}
inline std::pair<std::unique_ptr<FlowInterface>, bool>
ForSentence::AddForAfterBodySentence(
    std::unique_ptr<FlowInterface>&& after_body_sentence) {
  if (CheckForBodySentenceValid(*after_body_sentence)) [[likely]] {
    AddMainBlockSentence(std::move(after_body_sentence));
    return std::make_pair(std::unique_ptr<FlowInterface>(), true);
  } else {
    return std::make_pair(std::move(after_body_sentence), false);
  }
}

// 添加普通的case，返回是否添加成功，如果添加失败则返回case_label的控制权

// 默认对循环条件节点进行类型检查的函数
// 返回给定节点是否可以作为条件分支类语句的条件

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
bool CheckSwitchCaseAbleToAdd(
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

// 添加普通的case，返回是否添加成功，如果添加失败则返回case_label的控制权

inline std::pair<std::unique_ptr<Label>, bool> SwitchSentence::AddSimpleCase(
    std::shared_ptr<BasicTypeInitializeOperatorNode>&& case_value,
    std::unique_ptr<Label>&& case_label) {
  if (CheckSwitchCaseAbleToAdd(*this, *case_value)) [[likely]] {
    // 可以添加
    simple_cases_.emplace(case_value->GetValue(),
                          std::make_pair(case_value, std::move(case_label)));
    return std::make_pair(std::unique_ptr<Label>(), true);
  } else {
    return std::make_pair(std::move(case_label), false);
  }
}

// 添加switch语句主体部分
// 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
// 添加成功则不返还流程节点控制权

inline std::pair<std::unique_ptr<FlowInterface>, bool>
SwitchSentence::AddSwitchBodySentence(
    std::unique_ptr<FlowInterface>&& switch_body_sentence) {
  if (CheckSwitchBodySentenceValid(*switch_body_sentence)) [[likely]] {
    AddMainBlockSentence(std::move(switch_body_sentence));
    return std::make_pair(std::unique_ptr<FlowInterface>(), true);
  } else {
    return std::make_pair(std::move(switch_body_sentence), false);
  }
}

inline bool SwitchSentence::SetSwitchCondition(
    std::shared_ptr<const OperatorNodeInterface>&& switch_condition) {
  if (CheckSwitchConditionValid(*switch_condition)) [[likely]] {
    SetCondition(std::move(switch_condition));
    return true;
  } else {
    return false;
  }
}
}  // namespace c_parser_frontend::flow_control
