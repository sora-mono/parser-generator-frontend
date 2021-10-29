#ifndef PARSER_SYNTAXMACHINE_SYNTAXMACHINE_H_
#define PARSER_SYNTAXMACHINE_SYNTAXMACHINE_H_
#include <queue>
#include <stack>

#include "Common/object_manager.h"
#include "Generator/SyntaxGenerator/process_function_interface.h"
#include "Generator/SyntaxGenerator/syntax_analysis_table.h"
#include "Generator/export_types.h"
#include "Parser/DfaMachine/dfa_machine.h"

namespace frontend::parser::syntax_machine {

class SyntaxMachine {
  using DfaMachine = frontend::parser::dfamachine::DfaMachine;

 public:
  // DFA引擎返回的单词信息
  using WordInfo = DfaMachine::WordInfo;

  // 语法分析表ID
  using SyntaxAnalysisTableEntryId =
      frontend::generator::syntax_generator::SyntaxAnalysisTableEntryId;
  // 语法分析表类型
  using SyntaxAnalysisTableType =
      frontend::generator::syntax_generator::SyntaxAnalysisTableType;
  // 产生式体ID（ID不关联到产生式对象，在生成配置后产生式对象便没有价值）
  using ProductionNodeId =
      frontend::generator::syntax_generator::ProductionNodeId;
  // 节点类型
  using ProductionNodeType =
      frontend::generator::syntax_generator::ProductionNodeType;
  // 运算符结合性
  using OperatorAssociatityType =
      frontend::generator::syntax_generator::OperatorAssociatityType;
  // 用户定义的分析用函数、数据对象的管理器
  using ProcessFunctionClassManagerType =
      frontend::generator::syntax_generator::ProcessFunctionClassManagerType;
  // 管理器中的对象的ID
  using ProcessFunctionClassId =
      frontend::generator::syntax_generator::ProcessFunctionClassId;
  // 管理器中的对象的基类
  using ProcessFunctionInterface =
      frontend::generator::syntax_generator::ProcessFunctionInterface;
  //// 语法分析表条目
  // using SyntaxAnalysisTableEntry =
  //    frontend::generator::syntax_generator::SyntaxAnalysisTableEntry;
  // 动作和目标
  using ActionAndAttachedDataInterface = frontend::generator::syntax_generator::
      SyntaxAnalysisTableEntry::ActionAndAttachedDataInterface;
  // 具体动作
  using ActionType = frontend::generator::syntax_generator::ActionType;
  // 移入时使用的数据
  using ShiftAttachedData = frontend::generator::syntax_generator::
      SyntaxAnalysisTableEntry::ShiftAttachedData;
  // 归约时使用的数据
  using ReductAttachedData = frontend::generator::syntax_generator::
      SyntaxAnalysisTableEntry::ReductAttachedData;
  // 传递给用户的单个单词的数据
  using WordDataToUser = frontend::generator::syntax_generator::
      ProcessFunctionInterface::WordDataToUser;
  // 终结节点数据
  using TerminalWordData = ProcessFunctionInterface::TerminalWordData;
  // 非终结节点数据
  using NonTerminalWordData = ProcessFunctionInterface::NonTerminalWordData;
  // 运算符优先级，根据当前最高优先级运算符确定，0保留为非运算符优先级
  using OperatorPriority =
      frontend::generator::syntax_generator::OperatorPriority;

  // 存储解析过程使用的数据
  struct ParsingData {
    // 当前语法分析表条目ID
    SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id;
    // 在syntax_analysis_table_entry_id条目的基础上移入的产生式节点的ID
    // 提供该项为了支持空规约功能
    ProductionNodeId shift_node_id;
    // 移入的终结节点数据或非终结节点规约后用户返回的数据
    WordDataToUser word_data_to_user;
    // 非运算符优先级为0
    OperatorPriority operator_priority = OperatorPriority(0);
  };

  SyntaxMachine() { LoadConfig(); }
  SyntaxMachine(const SyntaxMachine&) = delete;
  SyntaxMachine& operator=(SyntaxMachine&&) = delete;

  // 加载配置
  void LoadConfig();
  // 设置DFA返回的数据
  void SetDfaReturnData(WordInfo&& dfa_return_data) {
    dfa_return_data_ = std::move(dfa_return_data);
  }
  // 获取DFA返回的待移入的节点的数据
  WordInfo& GetWaitingProcessWordInfo() { return dfa_return_data_; }
  // 获取下一个单词的数据并存在dfa_return_data_中
  void GetNextWord() { SetDfaReturnData(dfa_machine_.GetNextWord()); }
  // 设置根分析表ID
  void SetRootParsingEntryId(SyntaxAnalysisTableEntryId root_parsing_entry_id) {
    root_parsing_entry_id_ = root_parsing_entry_id;
  }
  // 获取根分析表ID
  SyntaxAnalysisTableEntryId GetRootParsingEntryId() const {
    return root_parsing_entry_id_;
  }
  // 获取用户定义的处理操作的对象
  ProcessFunctionInterface& GetProcessFunctionClass(
      ProcessFunctionClassId class_id) {
    return manager_process_function_class_[class_id];
  }
  // 获取在该终结节点条件下的动作（移入/规约）和附属数据
  // 不存在该转移条件则返回空指针
  const ActionAndAttachedDataInterface* GetActionAndTarget(
      SyntaxAnalysisTableEntryId src_entry_id, ProductionNodeId node_id) const {
    assert(src_entry_id.IsValid());
    return syntax_analysis_table_[src_entry_id].AtTerminalNode(node_id);
  }
  // 获取移入该非终结节点后到达的产生式条目
  SyntaxAnalysisTableEntryId GetNextEntryId(
      SyntaxAnalysisTableEntryId src_entry_id, ProductionNodeId node_id) const {
    assert(src_entry_id.IsValid());
    return syntax_analysis_table_[src_entry_id].AtNonTerminalNode(node_id);
  }
  // 获取当前活跃的解析数据（解析数据栈顶对象）
  ParsingData& GetParsingDataNow() { return parsing_stack_.top(); }
  template <class ParsingDataType>
  void PushParsingData(ParsingDataType&& parsing_data) {
    parsing_stack_.push(std::forward<ParsingDataType>(parsing_data));
  }
  // 弹出解析数据栈顶部数据
  void PopTopParsingData() { parsing_stack_.pop(); }
  bool IsParsingStackEmpty() const { return parsing_stack_.empty(); }
  // 分析代码文件
  bool Parse(const std::string& filename);

 private:
  // 允许序列化类访问
  friend class boost::serialization::access;

  template <class Archive>
  void load(Archive& ar, const unsigned int version) {
    ar >> root_parsing_entry_id_;
    // 转除const以允许序列化代码读取配置
    ar >> const_cast<SyntaxAnalysisTableType&>(syntax_analysis_table_);
    ar >> manager_process_function_class_;
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  // 处理待移入单词是终结节点的情况
  // 自动处理移入和归并，归并后执行一次移入非终结节点后执行GetNextWord()
  void TerminalWordWaitingProcess();
  // TerminalWordWaitingShift子过程，处理移入的情况，处理后自动GetNextWord()
  void ShiftTerminalWord(
      const ActionAndAttachedDataInterface& action_and_target);
  // TerminalWordWaitingShifg子过程，处理规约的情况
  // 处理后自动执行一次移入非终结节点的操作并GetNextWord()
  void Reduct(const ActionAndAttachedDataInterface& action_and_target);
  // 移入非终结节点
  // non_terminal_word_data是规约后用户返回的数据
  // reducted_nonterminal_node_id是规约后得到的非终结产生式ID
  void ShiftNonTerminalWord(NonTerminalWordData&& non_terminal_word_data,
                            ProductionNodeId reducted_nonterminal_node_id);
  void SetLastOperateIsReduct() { last_operate_is_reduct_ = true; }
  void SetLastOperateIsNotReduct() { last_operate_is_reduct_ = false; }
  bool LastOperateIsReduct() const { return last_operate_is_reduct_; }

  // DFA分析机
  frontend::parser::dfamachine::DfaMachine dfa_machine_;
  // 根分析表条目ID
  SyntaxAnalysisTableEntryId root_parsing_entry_id_;
  // 用户定义的分析用函数、数据对象的管理器
  ProcessFunctionClassManagerType manager_process_function_class_;
  // 语法分析表，只有加载配置时可以修改
  const SyntaxAnalysisTableType syntax_analysis_table_;

  // DFA返回的数据
  WordInfo dfa_return_data_;
  // 解析用数据栈，栈顶为当前解析数据
  std::stack<ParsingData> parsing_stack_;
  // 标记上次操作是否为规约操作
  // 用来支持运算符优先级时同一个运算符可以细分为左侧单目运算符和双目运算符功能
  bool last_operate_is_reduct_ = true;
};

}  // namespace frontend::parser::syntax_machine
#endif  // !PARSER_SYNTAXMACHINE_SYNTAXMACHINE_H_