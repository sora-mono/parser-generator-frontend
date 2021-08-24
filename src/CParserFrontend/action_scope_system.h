#ifndef CPARSESRFRONTEND_ACTION_SCOPE_SYSTEM_H_
#define CPARSESRFRONTEND_ACTION_SCOPE_SYSTEM_H_
#include <memory>
#include <stack>
#include <unordered_map>
#include <variant>

#include "Common/id_wrapper.h"
#include "operator_node.h"
// 作用域系统
namespace c_parser_frontend::action_scope_system {
enum class DefineVarietyResult {
  // 可以添加的情况
  kNew,           // 新增指针，该节点不曾存储指针
  kShiftToStack,  // 该节点以前存储了一个指针，现在存储第二个，转化为指针栈
  kAddToStack,    // 向指针栈中添加一个指针
  // 不可以添加的情况
  kReDefine,  // 重定义
};
class VarietyScopeSystem {
  using OperatorNodeInterface =
      c_parser_frontend::operator_node::OperatorNodeInterface;
  using VarietyOperatorNode =
      c_parser_frontend::operator_node::VarietyOperatorNode;
  enum IdWrapperLabel { kActionScopeLevelType };
  // 存储当前作用域等级的类型
  using ActionScopeLevelType = frontend::common::ExplicitIdWrapper<
      size_t, IdWrapperLabel, IdWrapperLabel::kActionScopeLevelType>;
  // 管理变量信息的存储，在需要的时候转换两种存储方式
  class VarietyData {
   public:
    // 存储单个指针的类型
    using SinglePointerType =
        std::pair<std::shared_ptr<VarietyOperatorNode>, ActionScopeLevelType>;
    // 存储多个指针的类型
    using PointerStackType = std::stack<SinglePointerType>;

    VarietyData() = default;
    template <class Data>
    VarietyData(Data&& data) : variety_data_(std::forward<Data>(data)) {}

    // 添加一条数据，如需转换成栈则自动执行
    // 建议先创建空节点后调用该函数，可以提升性能
    // 返回值含义见该类型定义处
    DefineVarietyResult AddVarietyData(
        std::shared_ptr<VarietyOperatorNode>&& operator_node,
        ActionScopeLevelType action_scope_level);
    // 弹出最顶层数据，自动处理转换
    // 如果弹出了最后一个数据则返回true，应移除该节点
    bool PopTopData();
    // 获取顶层数据
    VarietyScopeSystem::VarietyData::SinglePointerType& GetTopData();
    // 判断容器是否未存储任何指针
    bool Empty() {
      return std::get_if<std::monostate>(&variety_data_) == nullptr;
    }

   private:
    auto& GetVarietyData() { return variety_data_; }
    // 存储指向节点的指针，std::monostate为默认构造时所拥有的对象
    std::variant<std::monostate, SinglePointerType,
                 std::unique_ptr<PointerStackType>>
        variety_data_;
    // 上次弹出该变量信息时所在的层数
  };

 public:
  // 存储不同作用域变量的结构
  using ActionScopeContainerType = std::unordered_map<std::string, VarietyData>;

  VarietyScopeSystem() { VarietyScopeSystemInit(); }

  // 初始化
  void VarietyScopeSystemInit() {
    variety_name_to_operator_node_pointer_.clear();
    while (!variety_stack_.empty()) {
      variety_stack_.pop();
    }
    // 压入哨兵，弹出所有高于给定层数花括号的变量时无需判断栈是否为空
    auto [iter, inserted] = variety_name_to_operator_node_pointer_.emplace(
        std::string(),
        VarietyData::SinglePointerType(nullptr, ActionScopeLevelType(0)));
    assert(inserted == true);
    variety_stack_.emplace(std::move(iter));
  }
  // 定义一个变量，level代表该变量外花括号的层数，0代表全局变量
  // 返回指向插入位置的迭代器与插入的结果，结果含义见定义
  template <class VarietyName>
  std::pair<ActionScopeContainerType::const_iterator, DefineVarietyResult> DefineVariety(
      VarietyName&& variety_name,
      std::shared_ptr<VarietyOperatorNode>&& operator_node_pointer);
  // 声明一个变量，保证返回的迭代器有效且管理节点存在，但不会添加变量指针
  // 添加变量指针请使用AddVarietyDefine()
  // 返回指向插入位置的迭代器
  template <class VarietyName>
  ActionScopeContainerType::const_iterator AnnounceVariety(
      VarietyName&& variety_name);
  // 返回指向变量节点的智能指针和变量是否存在
  std::pair<std::shared_ptr<VarietyOperatorNode>, bool> GetVariety(
      const std::string& variety_name);

  ActionScopeLevelType GetActionScopeLevel() const {
    return action_scope_level;
  }
  // 增加一级作用域等级
  void AddActionScopeLevel() { ++action_scope_level; }
  // 弹出一级作用域，会弹出该作用域内所有的变量
  void PopActionScope() {
    assert(action_scope_level != 0);
    --action_scope_level;
    PopVarietyOverLevel(GetActionScopeLevel());
  }

 private:
  // 弹出所有高于给定作用域等级的变量
  void PopVarietyOverLevel(ActionScopeLevelType level);
  ActionScopeContainerType& GetVarietyNameToOperatorNodePointer() {
    return variety_name_to_operator_node_pointer_;
  }
  auto& GetVarietyStack() { return variety_stack_; }

  // 存储变量名和对应同名不同作用域变量
  // 键值为变量名，值使用variant因为变量覆盖情况较少
  // 在发生覆盖时创建变量栈转换结构可以降低内存占用和stack的性能消耗
  ActionScopeContainerType variety_name_to_operator_node_pointer_;
  // 按顺序存储添加的变量，用来支持作用域结束后清除该作用域内全部变量功能
  // iterator指向变量和对应数据
  std::stack<ActionScopeContainerType::iterator> variety_stack_;
  // 存储当前作用域等级，全局变量为0级，每个{}增加一级
  ActionScopeLevelType action_scope_level;
};
template <class VarietyName>
inline std::pair<VarietyScopeSystem::ActionScopeContainerType::const_iterator,
                 DefineVarietyResult>
VarietyScopeSystem::DefineVariety(
    VarietyName&& variety_name,
    std::shared_ptr<VarietyOperatorNode>&& operator_node_pointer) {
  auto [iter, inserted] = GetVarietyNameToOperatorNodePointer().emplace(
      std::forward<VarietyName>(variety_name), VarietyData());
  DefineVarietyResult add_variety_result = iter->second.AddVarietyData(
      std::move(operator_node_pointer), GetActionScopeLevel());
  switch (add_variety_result) {
    case DefineVarietyResult::kNew:
    case DefineVarietyResult::kAddToStack:
    case DefineVarietyResult::kShiftToStack:
      // 全局变量不会被弹出，无需入栈
      if (GetActionScopeLevel() != 0) [[likely]] {
        // 添加该节点的信息，以便作用域失效时精确弹出
        // 可以避免用户提供需要弹出的序列，简化操作
        // 同时避免遍历整个变量名到节点映射表，也无需每个指针都存储对应的level
        GetVarietyStack().emplace(std::move(iter));
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
template <class VarietyName>
inline VarietyScopeSystem::ActionScopeContainerType::const_iterator
VarietyScopeSystem::AnnounceVariety(VarietyName&& variety_name) {
  return GetVarietyNameToOperatorNodePointer()
      .emplace(std::forward<VarietyName>(variety_name), VarietyData())
      .first;
}
}  // namespace c_parser_frontend::action_scope_system

#endif  // !CPARSESRFRONTEND_ACTION_SCOPE_SYSTEM_H_