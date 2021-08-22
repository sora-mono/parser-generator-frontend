#include "action_scope_system.h"
namespace c_parser_frontend::action_scope_system {
// 添加一条数据，如需转换成栈则自动执行
// 建议先创建空节点后调用该函数，可以提升性能
// 返回值含义见该类型定义处

inline VarietyScopeSystem::VarietyData::AddVarietyResult
VarietyScopeSystem::VarietyData::AddVarietyData(
    std::shared_ptr<VarietyOperatorNode>&& operator_node,
    ActionScopeLevelType action_scope_level) {
  auto& variety_data = GetVarietyData();
  std::monostate* empty_status_pointer =
      std::get_if<std::monostate>(&variety_data);
  if (empty_status_pointer != nullptr) [[likely]] {
    // 未存储任何东西，向variety_data_中存储单个指针
    variety_data.emplace<SinglePointerType>(std::move(operator_node),
                                            action_scope_level);
    return AddVarietyResult::kNew;
  } else {
    auto* single_object = std::get_if<SinglePointerType>(&variety_data);
    if (single_object != nullptr) [[likely]] {
      // 检查是否存在重定义
      if (single_object->second == action_scope_level) [[unlikely]] {
        // 该作用域等级已经定义同名变量
        return AddVarietyResult::kReDefine;
      }
      // 原来存储单个shared_ptr，新增一个指针转化为指针栈
      // 将原来的存储方式改为栈存储
      auto stack_pointer = std::make_unique<PointerStackType>();
      // 添加原有的指针
      stack_pointer->emplace(std::move(*single_object));
      // 添加新增的指针
      stack_pointer->emplace(std::move(operator_node), action_scope_level);
      // 重新设置variety_data的内容
      variety_data = std::move(stack_pointer);
      return AddVarietyResult::kShiftToStack;
    } else {
      // 检查是否存在重定义
      if (single_object->second == action_scope_level) [[unlikely]] {
        // 该作用域等级已经定义同名变量
        return AddVarietyResult::kReDefine;
      }
      auto& stack_pointer =
          *std::get_if<std::unique_ptr<PointerStackType>>(&variety_data);
      // 已经建立指针栈，向指针栈中添加给定指针
      stack_pointer->emplace(std::move(operator_node), action_scope_level);
      return AddVarietyResult::kAddToStack;
    }
  }
}
bool VarietyScopeSystem::VarietyData::PopTopData() {
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

VarietyScopeSystem::VarietyData::SinglePointerType&
VarietyScopeSystem::VarietyData::GetTopData() {
  auto& variety_data = GetVarietyData();
  auto* single_pointer = std::get_if<SinglePointerType>(&variety_data);
  if (single_pointer != nullptr) [[likely]] {
    return *single_pointer;
  } else {
    return (*std::get_if<std::unique_ptr<PointerStackType>>(&variety_data))
        ->top();
  }
}

std::pair<std::shared_ptr<VarietyScopeSystem::VarietyOperatorNode>, bool>
VarietyScopeSystem::GetVariety(const std::string& variety_name) {
  auto iter = GetVarietyNameToOperatorNodePointer().find(variety_name);
  if (iter != GetVarietyNameToOperatorNodePointer().end()) [[likely]] {
    return std::make_pair(iter->second.GetTopData().first, true);
  } else {
    return std::make_pair(std::shared_ptr<VarietyOperatorNode>(), false);
  }
}

void VarietyScopeSystem::PopVarietyOverLevel(ActionScopeLevelType level) {
  auto& variety_stack = GetVarietyStack();
  // 因为存在哨兵，所以无需判断栈是否为空
  while (variety_stack.top()->second.GetTopData().second > level) {
    bool should_erase_this_node = variety_stack.top()->second.PopTopData();
    if (should_erase_this_node) [[likely]] {
      // 该变量名并不与任何节点绑定
      // 删除该数据节点
      GetVarietyNameToOperatorNodePointer().erase(variety_stack.top());
    }
    variety_stack.pop();
  }
}

}  // namespace c_parser_frontend::action_scope_system