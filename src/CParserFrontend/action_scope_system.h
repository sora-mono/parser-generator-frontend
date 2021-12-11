/// @file action_scope_system.h
/// @brief 作用域系统
#ifndef CPARSESRFRONTEND_ACTION_SCOPE_SYSTEM_H_
#define CPARSESRFRONTEND_ACTION_SCOPE_SYSTEM_H_
#include <memory>
#include <stack>
#include <unordered_map>
#include <variant>

#include "Common/id_wrapper.h"
#include "flow_control.h"
#include "operator_node.h"

namespace c_parser_frontend::action_scope_system {
/// @brief 定义变量的检查结果
enum class DefineVarietyResult {
  ///< 可以添加的情况
  kNew,           ///< 新增指针，该节点不曾存储指针
  kShiftToStack,  ///< 该节点以前存储了一个指针，现在存储第二个，转化为指针栈
  kAddToStack,    ///< 向指针栈中添加一个指针
  ///< 不可以添加的情况
  kReDefine,  ///< 重定义
};
/// @class ActionScopeSystem action_scope_system.h
/// @brief 作用域系统
/// @details
/// 1.作用域系统管理变量的作用域/可见域和流程控制语句的构建
/// 2.根据作用域的变化自动弹出作用域超出范围的变量和构建完成的流程控制语句
/// 3.每个变量/流程控制语句都有自己的作用域等级，作用域等级等于声明处外面的流程
///   控制语句（花括号）层数，全局变量为0级，正在构建的函数为1级
/// @attention
/// 作用域系统在构建函数时从FlowControlSystem提取指向当前活跃函数节点的指针并用
/// unique_ptr存储该指针，将它压入流程控制栈中，当前函数构建完毕后弹出该指针不
/// 释放内存，这样可以避免每次添加语句时都需要判断流程控制栈是否为空，函数节点
/// 的控制权一直属于FlowControlSystem，作用域系统只拥有访问权
class ActionScopeSystem {
  using OperatorNodeInterface =
      c_parser_frontend::operator_node::OperatorNodeInterface;
  using VarietyOperatorNode =
      c_parser_frontend::operator_node::VarietyOperatorNode;
  /// @brief 作用域等级的分发标签
  enum IdWrapperLabel { kActionScopeLevelType };
  /// @brief 存储当前作用域等级的类型
  using ActionScopeLevel = frontend::common::ExplicitIdWrapper<
      size_t, IdWrapperLabel, IdWrapperLabel::kActionScopeLevelType>;
  /// @class VarietyData action_scope_system.h
  /// @brief 管理变量信息的存储
  /// @details
  /// 1.默认存储指向变量节点的指针
  /// 2.存在同名变量覆盖浅层变量的情况时转换为栈
  /// 3.弹出同名变量后自动恢复单个指针存储
  class VarietyData {
   public:
    /// @brief 存储单个指针的类型
    using SinglePointerType =
        std::pair<std::shared_ptr<const OperatorNodeInterface>,
                  ActionScopeLevel>;
    /// @brief 存储多个指针的栈类型
    using PointerStackType = std::stack<SinglePointerType>;

    VarietyData() = default;
    template <class Data>
    VarietyData(Data&& data) : variety_data_(std::forward<Data>(data)) {}

    /// @brief 添加一条变量数据或初始化数据
    /// @param[in] operator_node ：变量数据或初始化数据节点
    /// @param[in] action_scope_level ：作用域等级
    /// @return 返回检查结果，意义见定义
    /// @details
    /// 建议先创建空节点后调用该函数，可以提升性能
    /// @note 如需转换成栈则自动执行
    DefineVarietyResult AddVarietyOrInitData(
        const std::shared_ptr<const OperatorNodeInterface>& operator_node,
        ActionScopeLevel action_scope_level);
    /// @brief 弹出最顶层数据
    /// @return 返回是否应该删除VarietyData节点
    /// @retval true ：所有数据均弹出，应删除该节点
    /// @retval false ：节点中仍有数据
    /// @note 自动处理转换
    bool PopTopData();
    /// @brief 获取最顶层变量指针
    /// @return 返回变量指针
    SinglePointerType GetTopData() const;
    /// @brief 判断容器是否未存储任何指针
    /// @return 返回容器是否未存储任何指针
    /// @retval true 容器内未存储任何指针
    /// @retval false 容器内存储至少一个指针
    bool Empty() const {
      return std::get_if<std::monostate>(&variety_data_) != nullptr;
    }

   private:
    /// @brief 获取存储指针的容器
    /// @return 返回存储指针的容器的引用
    auto& GetVarietyData() { return variety_data_; }
    /// @brief 获取存储指针的容器
    /// @return 返回存储指针的容器的引用
    const auto& GetVarietyData() const { return variety_data_; }

    /// @brief 存储指向节点的指针，std::monostate为容器空时保存的内容
    std::variant<std::monostate, SinglePointerType,
                 std::unique_ptr<PointerStackType>>
        variety_data_;
  };

 public:
  /// @brief 存储不同作用域变量的结构
  using ActionScopeContainerType = std::unordered_map<std::string, VarietyData>;

  ActionScopeSystem() { VarietyScopeSystemInit(); }
  ~ActionScopeSystem();

  /// @brief 初始化变量系统
  void VarietyScopeSystemInit() {
    action_scope_level_ = ActionScopeLevel(0);
    variety_or_function_name_to_operator_node_pointer_.clear();
    while (!variety_stack_.empty()) {
      variety_stack_.pop();
    }
    // 压入哨兵，弹出所有高于给定层数花括号的变量时无需判断栈是否为空
    // 变量栈中有且仅有这一个作用域等级为0的变量
    auto [iter, inserted] =
        variety_or_function_name_to_operator_node_pointer_.emplace(
            std::string(),
            VarietyData::SinglePointerType(nullptr, ActionScopeLevel(0)));
    assert(inserted == true);
    variety_stack_.emplace(std::move(iter));
  }

  /// @brief 增加一级作用域等级
  void AddActionScopeLevel() { ++action_scope_level_; }
  /// @brief 定义变量
  /// @param[in] variety_node_pointer ：指向变量节点的指针
  /// @return 前半部分为插入位置的迭代器，后半部分为检查结果
  /// @note 变量必须已经设置变量名
  /// 与DefineVarietyOrInitValue在定义变量时等价
  std::pair<ActionScopeContainerType::const_iterator, DefineVarietyResult>
  DefineVariety(
      const std::shared_ptr<const VarietyOperatorNode>& variety_node_pointer);
  /// @brief 定义变量或初始化常量
  /// @param[in] name ：变量名/初始化常量
  /// @param[in] operator_node_pointer ：指向变量节点的指针
  /// @return 前半部分为插入位置的迭代器，后半部分为检查结果
  /// @note 变量必须已经设置变量名
  /// DefineVariety无法定义初始化值，因为初始化值不附带名字，需要从外传入
  /// 定义变量时提供的名字必须与变量节点中储存的名字相同
  std::pair<ActionScopeContainerType::const_iterator, DefineVarietyResult>
  DefineVarietyOrInitValue(const std::string& name,
                           const std::shared_ptr<const OperatorNodeInterface>&
                               operator_node_pointer);
  /// @brief 根据名字获取变量或函数名
  /// @param[in] variety_name ：查询的名字
  /// @return 前半部分为指向变量或函数节点的指针，后半部分为节点是否存在
  std::pair<std::shared_ptr<const OperatorNodeInterface>, bool>
  GetVarietyOrFunction(const std::string& variety_name) const;
  /// @brief 设置当前待构建函数
  /// @param[in] function_data ：指向待构建的函数节点的指针
  /// @return 返回是否设置成功
  /// @retval true ：设置成功
  /// @retval false ：当前定义域等级不是0级，不允许嵌套定义函数
  /// @details
  /// 1.function_data的控制权归属于FlowControlSystem，ActionScopeSystem
  /// 仅有function_data的访问权，没有控制权，禁止delete该指针
  /// 2.为了能将该指针放入流程控制节点栈中，使用std::unique_ptr<FlowInterface>
  ///   包装后压入栈，在弹出时release指针防止被释放
  /// 3.设置后自动增加一级作用域等级
  bool SetFunctionToConstruct(
      c_parser_frontend::flow_control::FunctionDefine* function_data);
  /// @brief 获取当前作用域等级
  /// @return 返回当前作用域等级
  ActionScopeLevel GetActionScopeLevel() const { return action_scope_level_; }
  /// @brief 压入流程控制节点
  /// @param[in] flow_control_sentence ：待压入的流程控制节点
  /// @return 返回是否压入成功
  /// @retval true ：压入成功
  /// @retval false ：未压入函数节点作为流程控制栈底部就压入其它流程控制节点或
  /// 栈不空时压入函数节点（嵌套定义函数）
  /// @details
  /// 1.自动增加一级作用域等级
  /// 2.先增加一级作用域等级后压入流程控制节点，这样在弹出该级作用域时就能够一并
  ///   弹出构建完成的流程控制节点
  /// @attention 必须压入可以存储流程控制节点的流程控制语句
  bool PushFlowControlSentence(
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>&&
          flow_control_sentence);
  /// @brief 弹出一级作用域
  /// @details
  /// 1.弹出该作用域内所有的变量和流程控制语句，自动添加弹出的流程控制语句
  /// 2.调用后作用域等级减1
  void PopActionScope() {
    assert(action_scope_level_ > 0);
    --action_scope_level_;
    PopOverLevel(GetActionScopeLevel());
  }
  /// @brief 获取最顶层流程控制语句
  /// @return 返回最顶层流程控制语句的引用
  auto& GetTopFlowControlSentence() const {
    assert(!flow_control_stack_.empty());
    return *flow_control_stack_.top().first;
  }
  /// @brief 将流程控制语句栈顶的if流程控制语句转化为if-else语句
  /// @note 顶部的流程控制语句必须是if流程控制语句
  void ConvertIfSentenceToIfElseSentence() {
    assert(flow_control_stack_.top().first->GetFlowType() ==
           c_parser_frontend::flow_control::FlowType::kIfSentence);
    static_cast<c_parser_frontend::flow_control::IfSentence&>(
        *flow_control_stack_.top().first)
        .ConvertToIfElse();
  }
  /// @brief 向switch语句中添加普通case
  /// @return 返回是否添加成功
  /// @retval true ：添加成功
  /// @retval false 当前顶层控制语句不为switch或与已有的值重复
  bool AddSwitchSimpleCase(
      const std::shared_ptr<
          c_parser_frontend::flow_control::BasicTypeInitializeOperatorNode>&
          case_value);
  /// @brief 向switch语句中添加default标签
  /// @return 返回是否添加成功
  /// @retval true ：添加成功
  /// @retval false ：已添加过default标签或当前顶层控制语句不为switch
  bool AddSwitchDefaultCase();
  /// @brief 向当前活跃的流程控制语句内添加一条语句
  /// @param[in] sentence ：待添加的语句
  /// @return 返回是否添加成功
  /// @retval true ：添加成功，夺取sentence控制权
  /// @retval false
  /// ：无活跃的流程控制语句或给定语句无法添加到当前的流程控制节点中，不修改参数
  /// @note 添加到已有的流程语句后
  bool AddSentence(
      std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>&&
          sentence) {
    if (flow_control_stack_.empty()) [[unlikely]] {
      return false;
    }
    return flow_control_stack_.top().first->AddMainSentence(
        std::move(sentence));
  }
  /// @brief 向当前活跃的流程控制语句内添加多条语句
  /// @param[in] sentence_container ：存储待添加的语句的容器
  /// @return 返回是否添加成功
  /// @retval true
  /// ：添加成功，将sentence_container中所有语句移动到活跃流程控制语句的主容器中
  /// @retval false ：给定语句无法添加到当前的流程控制节点中，不修改参数
  /// @note 按begin->end的顺序添加，添加到已有的流程语句后
  bool AddSentences(
      std::list<
          std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>>&&
          sentence_container);

 private:
  /// @brief 弹出所有高于给定作用域等级的变量和流程控制语句
  /// @param[in] level ：弹出所有高于该等级的变量和流程控制语句
  void PopOverLevel(ActionScopeLevel level);
  /// @brief 获取存储变量/函数名到节点指针的映射的容器
  /// @return 返回容器的引用
  ActionScopeContainerType& GetVarietyOrFunctionNameToOperatorNodePointer() {
    return variety_or_function_name_to_operator_node_pointer_;
  }
  /// @brief 获取存储变量/函数名到节点指针的映射的容器
  /// @return 返回容器的const引用
  const ActionScopeContainerType&
  GetVarietyOrFunctionNameToOperatorNodePointer() const {
    return variety_or_function_name_to_operator_node_pointer_;
  }
  /// @brief 获取变量栈
  /// @return 返回变量栈的引用
  auto& GetVarietyStack() { return variety_stack_; }
  /// @brief 创建函数定义控制块并压入流程控制节点栈中
  /// @return 返回是否添加成功
  /// @retval true ：添加成功
  /// @retval false ：当前作用域等级不是0级，不允许嵌套定义函数
  /// @details
  /// 1.该函数还创建函数类型的初始化值并添加为全局变量，供赋值使用
  /// 2.从作用域等级0提升到1
  bool PushFunctionFlowControlNode(
      c_parser_frontend::flow_control::FunctionDefine* function_data);

  /// @brief 存储变量/函数名和到节点指针的映射
  /// @details
  /// 键值为变量名
  /// 在发生覆盖时创建变量栈转换结构可以降低内存占用和stack的性能消耗
  ActionScopeContainerType variety_or_function_name_to_operator_node_pointer_;
  /// @brief 按顺序存储添加的变量
  /// @details
  /// 用来支持作用域终结后清除该作用域内全部变量
  /// iterator指向变量和对应数据
  std::stack<ActionScopeContainerType::iterator> variety_stack_;
  /// @brief 当前作用域等级
  /// @note 全局变量为0级，每个{或流程控制语句增加1级
  ActionScopeLevel action_scope_level_ = ActionScopeLevel(0);
  /// @brief 存储当前所有正在构建的流程语句和流程语句的作用域等级
  /// @note 最底层为当前所在函数，作用域等级为1
  std::stack<
      std::pair<std::unique_ptr<c_parser_frontend::flow_control::FlowInterface>,
                ActionScopeLevel>>
      flow_control_stack_;
};

}  // namespace c_parser_frontend::action_scope_system

#endif  /// !CPARSESRFRONTEND_ACTION_SCOPE_SYSTEM_H_