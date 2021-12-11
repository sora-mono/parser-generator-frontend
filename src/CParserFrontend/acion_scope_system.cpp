#include <format>

#include "action_scope_system.h"
namespace c_parser_frontend::action_scope_system {
// 添加一条数据，如需转换成栈则自动执行
// 建议先创建空节点后调用该函数，可以提升性能
// 返回值含义见该类型定义处

inline DefineVarietyResult ActionScopeSystem::VarietyData::AddVarietyOrInitData(
    const std::shared_ptr<const OperatorNodeInterface>& operator_node,
    ActionScopeLevel action_scope_level) {
  assert(operator_node->GetGeneralOperatorType() ==
             operator_node::GeneralOperationType::kVariety ||
         operator_node->GetGeneralOperatorType() ==
             operator_node::GeneralOperationType::kInitValue);
  auto& variety_data = GetVarietyData();
  std::monostate* empty_status_pointer =
      std::get_if<std::monostate>(&variety_data);
  if (empty_status_pointer != nullptr) [[likely]] {
    // 未存储任何东西，向variety_data_中存储单个指针
    variety_data.emplace<SinglePointerType>(operator_node, action_scope_level);
    return DefineVarietyResult::kNew;
  } else {
    auto* single_object = std::get_if<SinglePointerType>(&variety_data);
    if (single_object != nullptr) [[likely]] {
      // 检查是否存在重定义
      if (single_object->second == action_scope_level) [[unlikely]] {
        // 该作用域等级已经定义同名变量
        return DefineVarietyResult::kReDefine;
      }
      // 原来存储单个shared_ptr，新增一个指针转化为指针栈
      // 将原来的存储方式改为栈存储
      auto stack_pointer = std::make_unique<PointerStackType>();
      // 添加原有的指针
      stack_pointer->emplace(std::move(*single_object));
      // 添加新增的指针
      stack_pointer->emplace(operator_node, action_scope_level);
      // 重新设置variety_data的内容
      variety_data = std::move(stack_pointer);
      return DefineVarietyResult::kShiftToStack;
    } else {
      auto& stack_pointer =
          *std::get_if<std::unique_ptr<PointerStackType>>(&variety_data);
      // 检查是否存在重定义
      if (stack_pointer->top().second == action_scope_level) [[unlikely]] {
        // 该作用域等级已经定义同名变量
        return DefineVarietyResult::kReDefine;
      }
      // 已经建立指针栈，向指针栈中添加给定指针
      stack_pointer->emplace(operator_node, action_scope_level);
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

ActionScopeSystem::VarietyData::SinglePointerType
ActionScopeSystem::VarietyData::GetTopData() const {
  auto& variety_data = GetVarietyData();
  assert(std::get_if<std::monostate>(&variety_data) == nullptr);
  auto* single_pointer = std::get_if<SinglePointerType>(&variety_data);
  if (single_pointer != nullptr) [[likely]] {
    return *single_pointer;
  } else {
    return (*std::get_if<std::unique_ptr<PointerStackType>>(&variety_data))
        ->top();
  }
}

ActionScopeSystem::~ActionScopeSystem() {
  if (!flow_control_stack_.empty()) {
    // 需要正确弹出栈中的函数对象指针，防止释放无管辖权的指针
    while (flow_control_stack_.size() > 1) {
      flow_control_stack_.pop();
    }
    // 当前流程控制语句栈仅剩正在构建的函数对象指针
    // release该指针以防止被释放
    flow_control_stack_.top().first.release();
  }
}

inline std::pair<ActionScopeSystem::ActionScopeContainerType::const_iterator,
                 DefineVarietyResult>
ActionScopeSystem::DefineVariety(
    const std::shared_ptr<const VarietyOperatorNode>& variety_node_pointer) {
  return DefineVarietyOrInitValue(variety_node_pointer->GetVarietyName(),
                                  variety_node_pointer);
}

std::pair<ActionScopeSystem::ActionScopeContainerType::const_iterator,
          DefineVarietyResult>
ActionScopeSystem::DefineVarietyOrInitValue(
    const std::string& name,
    const std::shared_ptr<const OperatorNodeInterface>& operator_node_pointer) {
  assert(
      operator_node_pointer->GetGeneralOperatorType() ==
          c_parser_frontend::operator_node::GeneralOperationType::kInitValue ||
      operator_node_pointer->GetGeneralOperatorType() ==
              c_parser_frontend::operator_node::GeneralOperationType::
                  kVariety &&
          name ==
              static_cast<const VarietyOperatorNode&>(*operator_node_pointer)
                  .GetVarietyName());
  auto [iter, inserted] =
      GetVarietyOrFunctionNameToOperatorNodePointer().emplace(name,
                                                              VarietyData());
  DefineVarietyResult add_variety_result = iter->second.AddVarietyOrInitData(
      operator_node_pointer, GetActionScopeLevel());
  switch (add_variety_result) {
    case DefineVarietyResult::kNew:
    case DefineVarietyResult::kAddToStack:
    case DefineVarietyResult::kShiftToStack:
      // 全局变量不会被弹出，无需入栈
      if (GetActionScopeLevel() != 0) [[likely]] {
        // 添加该节点的信息，以便作用域失效时精确弹出
        // 可以避免用户提供需要弹出的序列，简化操作
        // 同时避免遍历整个变量名到节点映射表，也无需每个指针都存储对应的level
        GetVarietyStack().emplace(iter);
      }
      break;
    case DefineVarietyResult::kReDefine:
      break;
    default:
      assert(false);
      break;
  }
  return std::make_pair(std::move(iter), add_variety_result);
}

std::pair<std::shared_ptr<const operator_node::OperatorNodeInterface>, bool>
ActionScopeSystem::GetVarietyOrFunction(const std::string& target_name) const {
  auto iter = GetVarietyOrFunctionNameToOperatorNodePointer().find(target_name);
  if (iter != GetVarietyOrFunctionNameToOperatorNodePointer().end())
      [[likely]] {
    auto pointer_data = iter->second.GetTopData();
    return std::make_pair(pointer_data.first, true);
  } else {
    return std::make_pair(std::shared_ptr<VarietyOperatorNode>(), false);
  }
}

bool ActionScopeSystem::PushFunctionFlowControlNode(
    c_parser_frontend::flow_control::FunctionDefine* function_data) {
  assert(function_data);
  if (GetActionScopeLevel() != 0) [[unlikely]] {
    // 函数只能定义在0级（全局）作用域
    return false;
  }
  // 定义全局初始化常量，在给函数指针赋值时使用
  auto [ignore_iter, define_variety_result] = DefineVarietyOrInitValue(
      function_data->GetFunctionTypeReference().GetFunctionName(),
      std::make_shared<operator_node::BasicTypeInitializeOperatorNode>(
          function_data->GetFunctionTypePointer()));
  // 在此之前应该已经判断过是否存在重定义/重载问题（在添加到FlowControlSystem时）
  assert(define_variety_result != DefineVarietyResult::kReDefine);
  // 将函数数据指针压入栈，该指针管辖权不在ActionScopeSystem
  // 而在FlowControlSystem，所以弹出时要调用release防止数据被unique_ptr释放
  bool push_result = PushFlowControlSentence(
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>(
          function_data));
  // 如果压入不成功会导致unique_ptr在函数结束析构时抛异常
  assert(push_result);
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
    const std::string& argument_name =
        argument_node.variety_operator_node->GetVarietyName();
    // 如果参数有名则添加到作用域中
    if (!argument_name.empty()) [[likely]] {
      DefineVariety(argument_node.variety_operator_node);
    }
  }
  return true;
}

bool ActionScopeSystem::PushFlowControlSentence(
    std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>&&
        flow_control_sentence) {
  // 流程控制栈中有且仅有一个函数流程控制节点，位于底部，作用域等级为1
  if (flow_control_sentence->GetFlowType() !=
      flow_control::FlowType::kFunctionDefine) [[likely]] {
    if (flow_control_stack_.empty()) {
      return false;
    }
  } else if (!flow_control_stack_.empty()) [[unlikely]] {
    return false;
  }
  // 先提升作用域等级，后设置流程控制语句的作用域等级
  // 这样在弹出作用域等级时可以一并弹出流程控制语句
  AddActionScopeLevel();
  flow_control_stack_.emplace(
      std::make_pair(std::move(flow_control_sentence), GetActionScopeLevel()));
  return true;
}

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