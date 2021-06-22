#include <queue>

#include "Common/object_manager.h"
#include "Generator/LexicalGenerator/lexical_generator.h"
#include "Parser/DfaMachine/dfa_machine.h"
#ifndef PARSER_LEXICALMACHINE_LEXICALMACHINE_H_
#define PARSER_LEXICALMACHINE_LEXICALMACHINE_H_

namespace frontend::parser::lexicalmachine {

class LexicalMachine {
  using LexicalGenerator =
      frontend::generator::lexicalgenerator::LexicalGenerator;
  using DfaMachine = frontend::parser::dfamachine::DfaMachine;

 public:
  // DFA引擎返回的数据
  using DfaReturnData = DfaMachine::ReturnData;
  // 语法分析表ID
  using ParsingTableEntryId = LexicalGenerator::ParsingTableEntryId;
  // 产生式体ID（ID不关联到产生式对象，在生成配置后产生式对象便没有价值）
  using ProductionNodeId = LexicalGenerator::ProductionNodeId;
  // 节点类型
  using ProductionNodeType = LexicalGenerator::ProductionNodeType;
  // 用户定义的分析用函数、数据对象的管理器
  using ProcessFunctionClassManagerType =
      LexicalGenerator::ProcessFunctionClassManagerType;
  // 管理器中的对象的ID
  using ProcessFunctionClassId = LexicalGenerator::ProcessFunctionClassId;
  // 管理器中的对象的基类
  using ProcessFunctionInterface = frontend::generator::lexicalgenerator::ProcessFunctionInterface;
  // 语法分析表条目
  using ParsingTableEntry = LexicalGenerator::ParsingTableEntry;
  // 动作和目标
  using ActionAndAttachedData =
      LexicalGenerator::ParsingTableEntry::ActionAndAttachedData;
  // 具体动作
  using ActionType = LexicalGenerator::ActionType;
  // 移入时使用的数据
  using ShiftAttachedData = LexicalGenerator::ParsingTableEntry::ShiftAttachedData;
  // 归并时使用的数据
  using ReductAttachedData = LexicalGenerator::ParsingTableEntry::ReductAttachedData;

#ifdef USE_AMBIGUOUS_GRAMMAR
  // 运算符优先级，根据当前最高优先级运算符确定，0保留为非运算符优先级
  using OperatorPriority = LexicalGenerator::OperatorPriority;
#endif  // USE_AMBIGUOUS_GRAMMAR

  // 存储解析过程使用的数据
  struct ParsingData {
    // 当前语法分析表条目ID
    ParsingTableEntryId parsing_table_entry_id;
    // 在parsing_table_entry_id条目的基础上移入的单词的ID
    ProductionNodeId shift_node_id;

#ifdef USE_AMBIGUOUS_GRAMMAR
    // 非运算符优先级为0
    OperatorPriority operator_priority = OperatorPriority(0);
#endif  // USE_AMBIGUOUS_GRAMMAR
  };

  LexicalMachine();
  LexicalMachine(const LexicalMachine&) = delete;
  LexicalMachine& operator=(LexicalMachine&&) = delete;

  // 加载配置
  void LoadConfig(const std::string& filename);
  // 设置DFA返回的数据
  void SetDfaReturnData(DfaReturnData&& dfa_return_data) {
    dfa_return_data_ = std::move(dfa_return_data);
  }
  // 获取DFA返回的待移入的节点的数据
  DfaReturnData& GetWaitingShiftWordData() { return dfa_return_data_; }
  // 获取下一个单词的数据并存在dfa_return_data_中
  void GetNextWord() { SetDfaReturnData(dfa_machine_.GetNextWord()); }
  // 设置根分析表ID
  void SetRootParsingEntryId(ParsingTableEntryId root_parsing_entry_id) {
    root_parsing_entry_id_ = root_parsing_entry_id;
  }
  // 获取根分析表ID
  ParsingTableEntryId GetRootParsingEntryId() const {
    return root_parsing_entry_id_;
  }
  // 获取用户定义的处理操作的对象
  ProcessFunctionInterface& GetProcessFunctionClassId(
    ProcessFunctionClassId class_id) {
    return manager_process_function_class_[class_id];
  }
  // 获取在该终结节点条件下的动作（移入/规约）和附属数据
  // 不存在该转移条件则返回空指针
  const ActionAndAttachedData* GetActionAndTarget(ParsingTableEntryId src_entry_id,
                                            ProductionNodeId node_id) const {
    assert(src_entry_id.IsValid());
    return parsing_table_[src_entry_id].AtTerminalNode(node_id);
  }
  // 获取移入该非终结节点后到达的产生式条目
  ParsingTableEntryId GetNextEntryId(ParsingTableEntryId src_entry_id,
                                     ProductionNodeId node_id) const {
    assert(src_entry_id.IsValid());
    return parsing_table_[src_entry_id].AtNonTerminalNode(node_id);
  }
  // 处理待移入单词是终结节点的情况
  // 自动处理移入和归并，归并后自动执行一次移入非终结节点和GetNextWord()
  void TerminalWordWaitingShift(std::queue<ParsingData>* parsing_data,
                                ParsingData* parsing_data_now);
  // TerminalWordWaitingShift子过程，处理移入的情况，处理后自动GetNextWord()
  // 需要调用者设置parsing_data_now中移入的节点号
  void TerminalWordShift(const ActionAndAttachedData& action_and_target,
                         std::queue<ParsingData>* parsing_data, ParsingData* parsing_data_now);
  // TerminalWordWaitingShifg子过程，处理规约的情况
  // 处理后自动执行一次移入非终结节点的操作并GetNextWord()
  void TerminalWordReduct(const ActionAndAttachedData& action_and_target,
                          std::queue<ParsingData>* parsing_data, ParsingData* parsing_data_now);
  // 处理待移入单词是运算符的情况
  void OperatorWordWaitingShift();
  // 分析代码文件
  bool Parse(const std::string& filename);
 private:
  // DFA分析机
  frontend::parser::dfamachine::DfaMachine dfa_machine_;
  // 根分析表条目ID
  ParsingTableEntryId root_parsing_entry_id_;
  // 用户定义的分析用函数、数据对象的管理器
  ProcessFunctionClassManagerType manager_process_function_class_;
  // 语法分析表，只有加载配置时可以修改
  const LexicalGenerator::ParsingTableType parsing_table_;

  // DFA返回的数据
  DfaReturnData dfa_return_data_;
};

}  // namespace frontend::parser::lexicalmachine
#endif  // !PARSER_LEXICALMACHINE_LEXICALMACHINE_H_