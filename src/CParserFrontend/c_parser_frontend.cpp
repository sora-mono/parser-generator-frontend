#include "c_parser_frontend.h"

#include <format>
/// 获取函数信息
/// 如果不存在给定名称的函数则返回空指针
namespace c_parser_frontend {

thread_local CParserFrontend c_parser_frontend;

/// 声明函数使用该接口

std::pair<CParserFrontend::TypeSystem::TypeNodeContainerIter,
          CParserFrontend::AddTypeResult>
CParserFrontend::AnnounceFunction(
    const std::shared_ptr<const FunctionType>& function_type) {
  FlowControlSystem::FunctionCheckResult check_result =
      flow_control_system_.AnnounceFunction(function_type);
  switch (check_result) {
    case FlowControlSystem::FunctionCheckResult::kSuccess:
      break;
    case FlowControlSystem::FunctionCheckResult::kOverrideFunction:
      return std::make_pair(TypeSystem::TypeNodeContainerIter(),
                            AddTypeResult::kOverrideFunction);
      break;
    case FlowControlSystem::FunctionCheckResult::kDoubleAnnounce:
      return std::make_pair(TypeSystem::TypeNodeContainerIter(),
                            AddTypeResult::kDoubleAnnounceFunction);
      break;
    case FlowControlSystem::FunctionCheckResult::kFunctionConstructed:
      return std::make_pair(TypeSystem::TypeNodeContainerIter(),
                            AddTypeResult::kTypeAlreadyIn);
      break;
    default:
      assert(false);
      break;
  }
  return type_system_.AnnounceFunctionType(function_type);
}

/// 设置当前待构建函数
/// 在作用域内声明函数的全部参数和函数类型的对象且自动提升作用域等级
/// 返回是否设置成功

bool CParserFrontend::SetFunctionToConstruct(
    const std::shared_ptr<const FunctionType>& function_to_construct) {
  auto result =
      flow_control_system_.SetFunctionToConstruct(function_to_construct);
  if (result == FlowControlSystem::FunctionCheckResult::kSuccess) [[likely]] {
    return action_scope_system_.SetFunctionToConstruct(
        flow_control_system_.GetActiveFunctionPointer());
  } else {
    return false;
  }
}

bool CParserFrontend::PushFlowControlSentence(
    std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>&&
        flow_control_sentence) {
  bool result = action_scope_system_.PushFlowControlSentence(
      std::move(flow_control_sentence));
  return result;
}

}  /// namespace c_parser_frontend

namespace c_parser_frontend::action_scope_system {
void ActionScopeSystem::PopOverLevel(ActionScopeLevelType level) {
  auto& variety_stack = GetVarietyStack();
  /// 因为存在哨兵，所以无需判断栈是否为空
  /// 弹出变量
  while (variety_stack.top()->second.GetTopData().first.second > level) {
    bool should_erase_this_node = variety_stack.top()->second.PopTopData();
    if (should_erase_this_node) [[likely]] {
      /// 该变量名并不与任何节点绑定
      /// 删除该数据节点
      GetVarietyOrFunctionNameToOperatorNodePointer().erase(variety_stack.top());
    }
    variety_stack.pop();
  }
  while (!flow_control_stack_.empty() &&
         flow_control_stack_.top().second > level) {
    /// 弹出流程控制语句
    auto flow_control_sentence = std::move(flow_control_stack_.top().first);
    flow_control_stack_.pop();
    if (!flow_control_stack_.empty()) [[likely]] {
      /// 如果栈不空则添加到栈顶的流程控制语句中
      AddSentence(std::move(flow_control_sentence));
    } else {
      /// 如果栈空则说明当前构建的函数已完成
      /// 函数构建完成，创建构建完成的函数类型对象并添加到全局成员的作用域
      assert(flow_control_sentence->GetFlowType() ==
             c_parser_frontend::flow_control::FlowType::kFunctionDefine);
      /// 释放指向函数数据的指针，防止该指针在flow_control_sentence析构时被释放
      /// 该指针管辖权在FlowControlSystem
      flow_control_sentence.release();
      /// 通知控制器完成函数构建
      c_parser_frontend::c_parser_frontend.FinishFunctionConstruct();
    }
  }
}
}  /// namespace c_parser_frontend::action_scope_system