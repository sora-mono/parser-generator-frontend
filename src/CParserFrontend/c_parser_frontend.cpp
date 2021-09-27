#include "c_parser_frontend.h"

#include <format>
// 获取函数信息
// 如果不存在给定名称的函数则返回空指针
namespace c_parser_frontend {

thread_local CParserFrontend parser_frontend;

// 获取当前行数
size_t GetLine() { return frontend::parser::line_; }
// 获取当前列数
size_t GetColumn() { return frontend::parser::column_; }

void CParserFrontend::PushFlowControlSentence(
    std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>&&
        flow_control_sentence) {
  bool result = action_scope_system_.PushFlowControlSentence(
      std::move(flow_control_sentence));
  if (!result) [[unlikely]] {
    std::cerr << std::format(
                     "行数{:} 列数{:} 流程控制语句必须在函数定义内部使用",
                     GetLine(), GetColumn())
              << std::endl;
    exit(-1);
  }
}
}  // namespace c_parser_frontend

namespace c_parser_frontend::action_scope_system {
void ActionScopeSystem::PopVarietyOverLevel(ActionScopeLevelType level) {
  auto& variety_stack = GetVarietyStack();
  // 因为存在哨兵，所以无需判断栈是否为空
  // 弹出变量
  while (variety_stack.top()->second.GetTopData().first.second > level) {
    bool should_erase_this_node = variety_stack.top()->second.PopTopData();
    if (should_erase_this_node) [[likely]] {
      // 该变量名并不与任何节点绑定
      // 删除该数据节点
      GetVarietyNameToOperatorNodePointer().erase(variety_stack.top());
    }
    variety_stack.pop();
  }
  // 弹出流程控制语句
  if (flow_control_stack_.top().second > level) {
    auto flow_control_sentence = std::move(flow_control_stack_.top().first);
    flow_control_stack_.pop();
    if (!flow_control_stack_.empty()) [[likely]] {
      // 如果栈不空则添加到栈顶的流程控制语句中
      AddSentence(std::move(flow_control_sentence));
    } else {
      // 如果栈空则说明当前构建的函数已完成
      // 函数构建完成，创建构建完成的函数类型对象并添加到全局成员的作用域
      assert(flow_control_sentence->GetFlowType() ==
             c_parser_frontend::flow_control::FlowType::kFunctionDefine);
      auto function_type =
          static_cast<c_parser_frontend::flow_control::FunctionDefine&>(
              *flow_control_sentence)
              .GetFunctionTypePointer();
      bool result = CreateFunctionTypeVarietyAndPush(function_type);
      assert(result);
      // 通知控制器完成函数构建
      c_parser_frontend::parser_frontend.FinishFunctionConstruct(function_type);
    }
  } 
}
}  // namespace c_parser_frontend::action_scope_system