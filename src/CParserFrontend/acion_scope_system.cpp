#include <format>

#include "Parser/DfaMachine/dfa_machine.h"
#include "action_scope_system.h"
namespace c_parser_frontend::action_scope_system {
// 添加一条数据，如需转换成栈则自动执行
// 建议先创建空节点后调用该函数，可以提升性能
// 返回值含义见该类型定义处

inline DefineVarietyResult ActionScopeSystem::VarietyData::AddVarietyData(
    const std::shared_ptr<const VarietyOperatorNode>& operator_node,
    ActionScopeLevelType action_scope_level_) {
  auto& variety_data = GetVarietyData();
  std::monostate* empty_status_pointer =
      std::get_if<std::monostate>(&variety_data);
  if (empty_status_pointer != nullptr) [[likely]] {
    // 未存储任何东西，向variety_data_中存储单个指针
    variety_data.emplace<SinglePointerType>(operator_node, action_scope_level_);
    return DefineVarietyResult::kNew;
  } else {
    auto* single_object = std::get_if<SinglePointerType>(&variety_data);
    if (single_object != nullptr) [[likely]] {
      // 检查是否存在重定义
      if (single_object->second == action_scope_level_) [[unlikely]] {
        // 该作用域等级已经定义同名变量
        return DefineVarietyResult::kReDefine;
      }
      // 原来存储单个shared_ptr，新增一个指针转化为指针栈
      // 将原来的存储方式改为栈存储
      auto stack_pointer = std::make_unique<PointerStackType>();
      // 添加原有的指针
      stack_pointer->emplace(std::move(*single_object));
      // 添加新增的指针
      stack_pointer->emplace(operator_node, action_scope_level_);
      // 重新设置variety_data的内容
      variety_data = std::move(stack_pointer);
      return DefineVarietyResult::kShiftToStack;
    } else {
      auto& stack_pointer =
          *std::get_if<std::unique_ptr<PointerStackType>>(&variety_data);
      // 检查是否存在重定义
      if (stack_pointer->top().second == action_scope_level_) [[unlikely]] {
        // 该作用域等级已经定义同名变量
        return DefineVarietyResult::kReDefine;
      }
      // 已经建立指针栈，向指针栈中添加给定指针
      stack_pointer->emplace(operator_node, action_scope_level_);
      return DefineVarietyResult::kAddToStack;
    }
  }
}
bool ActionScopeSystem::VarietyData::PopTopData() {
  auto& variety_data = GetVarietyData();
  auto* single_pointer = std::get_if<SinglePointerType>(&variety_data);
  if (single_pointer != nullptr) [[likely]] {
    // 返回true代表所有数据已经删除，应移除该节点
    return true;
  } else {
    auto& stack_pointer =
        *std::get_if<std::unique_ptr<PointerStackType>>(&variety_data);
    stack_pointer->pop();
    if (stack_pointer->size() == 1) {
      // 栈中只剩一个指针，退化回只存储该指针的结构
      // 重新赋值variety_data_，则栈自动销毁
      // 此处不可使用移动语义，防止赋值前栈已经销毁
      variety_data = SinglePointerType(stack_pointer->top());
    }
    return false;
  }
}

std::pair<ActionScopeSystem::VarietyData::SinglePointerType, bool>
ActionScopeSystem::VarietyData::GetTopData() {
  auto& variety_data = GetVarietyData();
  if (std::get_if<std::monostate>(&variety_data) != nullptr) [[unlikely]] {
    // 该节点未存储任何变量指针
    return std::make_pair(SinglePointerType(), false);
  }
  auto* single_pointer = std::get_if<SinglePointerType>(&variety_data);
  if (single_pointer != nullptr) [[likely]] {
    return std::make_pair(*single_pointer, true);
  } else {
    return std::make_pair(
        (*std::get_if<std::unique_ptr<PointerStackType>>(&variety_data))->top(),
        true);
  }
}

std::pair<std::shared_ptr<const ActionScopeSystem::VarietyOperatorNode>, bool>
ActionScopeSystem::GetVariety(const std::string& variety_name) {
  auto iter = GetVarietyNameToOperatorNodePointer().find(variety_name);
  if (iter != GetVarietyNameToOperatorNodePointer().end()) [[likely]] {
    auto [pointer_data, has_pointer] = iter->second.GetTopData();
    return std::make_pair(pointer_data.first, has_pointer);
  } else {
    return std::make_pair(std::shared_ptr<VarietyOperatorNode>(), false);
  }
}

bool ActionScopeSystem::PushFunctionFlowControlNode(
    c_parser_frontend::flow_control::FunctionDefine* function_data) {
  if (GetActionScopeLevel() != 0) [[unlikely]] {
    // 函数只能定义在0级（全局）作用域
    return false;
  }
  auto iter = AnnounceVarietyName(
      function_data->GetFunctionTypeReference().GetFunctionName());
  // 定义全局函数变量供赋值使用，该变量为const右值，防止被赋值/修改
  auto [ignore_iter, result] = DefineVariety(
      iter->first, std::make_shared<operator_node::VarietyOperatorNode>(
                       &iter->first, type_system::ConstTag::kConst,
                       operator_node::LeftRightValueTag::kRightValue));
  // 在此之前应该已经判断过是否存在重定义/重载问题（在添加到FlowControlSystem时）
  assert(result != DefineVarietyResult::kReDefine);
  // 将函数数据指针压入栈，该指针管辖权不在ActionScopeSystem
  // 而在FlowControlSystem，所以弹出时要调用release防止数据被unique_ptr释放
  PushFlowControlSentence(
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>(
          function_data));
  return true;
}

bool ActionScopeSystem::SetFunctionToConstruct(
    c_parser_frontend::flow_control::FunctionDefine* function_data) {
  assert(function_data != nullptr);
  // 添加全局的函数类型变量，用于函数指针赋值
  // 并将函数的flow_control节点压入作用域栈底
  bool result = PushFunctionFlowControlNode(function_data);
  if (!result) [[unlikely]] {
    return false;
  }
  // 将函数参数添加到作用域中
  for (auto& argument_node :
       function_data->GetFunctionTypeReference().GetArguments()) {
    const std::string* argument_name =
        argument_node.variety_operator_node->GetVarietyNamePointer();
    // 如果参数有名则添加到作用域中
    if (argument_name != nullptr) [[likely]] {
      DefineVariety(*argument_name, std::shared_ptr<const VarietyOperatorNode>(
                                        argument_node.variety_operator_node));
    }
  }
  return true;
}

// 将构建中的流程控制节点压栈，自动增加一级作用域等级

bool ActionScopeSystem::PushFlowControlSentence(
    std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>&&
        flow_control_sentence) {
  if (flow_control_stack_.empty()) {
    return false;
  }
  // 先提升作用域等级，后设置流程控制语句的作用域等级
  AddActionScopeLevel();
  flow_control_stack_.emplace(
      std::make_pair(std::move(flow_control_sentence), GetActionScopeLevel()));
  return true;
}

bool ActionScopeSystem::RemoveEmptyNode(
    const std::string& empty_node_to_remove_name) {
  auto iter =
      variety_name_to_operator_node_pointer_.find(empty_node_to_remove_name);
  // 目标节点空，删除节点
  if (iter != variety_name_to_operator_node_pointer_.end() &&
      iter->second.Empty()) [[likely]] {
    variety_name_to_operator_node_pointer_.erase(iter);
    return true;
  }
  return false;
}

// 将if流程控制语句转化为if-else语句
// 如果顶层控制语句不为if则返回false
// 将if流程控制语句转化为if-else语句
// 如果顶层控制语句不为if则返回false

// 向switch语句中添加普通case
// 如果当前顶层控制语句不为switch则返回false

bool ActionScopeSystem::AddSwitchSimpleCase(
    const std::shared_ptr<
        c_parser_frontend::flow_control::BasicTypeInitializeOperatorNode>&
        case_value) {
  if (GetTopFlowControlSentence().GetFlowType() !=
      c_parser_frontend::flow_control::FlowType::kSwitchSentence) [[unlikely]] {
    return false;
  }
  return static_cast<c_parser_frontend::flow_control::SwitchSentence&>(
             GetTopFlowControlSentence())
      .AddSimpleCase(case_value);
}

bool ActionScopeSystem::AddSwitchDefaultCase() {
  if (GetTopFlowControlSentence().GetFlowType() !=
      c_parser_frontend::flow_control::FlowType::kSwitchSentence) [[unlikely]] {
    return false;
  }
  return static_cast<c_parser_frontend::flow_control::SwitchSentence&>(
             GetTopFlowControlSentence())
      .AddDefaultCase();
}

// 向switch语句中添加普通case
// 如果当前顶层控制语句不为switch则返回false

// 向当前活跃的函数内执行的语句尾部附加语句
// 返回是否添加成功，添加失败则不修改参数
// 如果当前无流程控制语句则返回false

// 添加待执行语句的集合
// 返回是否添加成功，添加失败则不修改参数
// 如果当前无流程控制语句则返回false

bool ActionScopeSystem::AddSentences(
    std::list<std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&&
        sentence_container) {
  if (flow_control_stack_.empty()) [[unlikely]] {
    return false;
  }
  return flow_control_stack_.top().first->AddMainSentences(
      std::move(sentence_container));
}

}  // namespace c_parser_frontend::action_scope_system