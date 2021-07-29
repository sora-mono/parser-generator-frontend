#include "syntax_machine.h"

namespace frontend::parser::syntaxmachine {

bool SyntaxMachine::Parse(const std::string& filename) {
  bool result = dfa_machine_.SetInputFile(filename);
  if (result == false) [[unlikely]] {
    fprintf(stderr, "打开文件\"%s\"失败，请检查\n", filename.c_str());
    return false;
  }
  GetNextWord();
  // 初始化当前解析数据
  GetParsingDataNow().parsing_table_entry_id = GetRootParsingEntryId();

#ifdef USE_AMBIGUOUS_GRAMMAR
  GetOperatorPriorityNow() = OperatorPriority(0);
#endif  // USE_AMBIGUOUS_GRAMMAR

  while (!GetParsingStack().empty()) {
    const WordInfo& dfa_return_data = GetWaitingProcessWordInfo();
    switch (dfa_return_data.word_attached_data_.node_type) {
      case ProductionNodeType::kTerminalNode:
      case ProductionNodeType::kOperatorNode:
      case ProductionNodeType::kEndNode:
        TerminalWordWaitingProcess();
        // TODO 添加用户调用清空数据栈的功能
        break;
      // 非终结节点在Reduct函数的处理流程中最后移入，不会在这里出现
      case ProductionNodeType::kNonTerminalNode:
      default:
        assert(false);
        break;
    }
  }
  return true;
}

void SyntaxMachine::TerminalWordWaitingProcess() {
  const ActionAndAttachedData& action_and_attached_data = *GetActionAndTarget(
      GetParsingDataNow().parsing_table_entry_id,
      ProductionNodeId(
          GetWaitingProcessWordInfo().word_attached_data_.production_node_id));
  switch (action_and_attached_data.action_type_) {
    // TODO 添加接受时的后续处理
    [[unlikely]] case ActionType::kAccept : break;
    // TODO 添加错误处理功能
    [[unlikely]] case ActionType::kError : break;
    case ActionType::kReduct:
      Reduct(action_and_attached_data);
      break;
    case ActionType::kShift:
      ShiftTerminalWord(action_and_attached_data);
      break;

#ifdef USE_AMBIGUOUS_GRAMMAR
    case ActionType::kShiftReduct: {
      // 该项仅在待移入单词为运算符时有效
      // 运算符优先级必须不为0
      auto& terminal_node_info =
          GetWaitingProcessWordInfo().word_attached_data_;
      OperatorPriority& priority_now = GetOperatorPriorityNow();
      assert(terminal_node_info.node_type ==
                 ProductionNodeType::kOperatorNode &&
             terminal_node_info.operator_priority != 0);
      if (priority_now > terminal_node_info.operator_priority) {
        // 当前优先级高于待处理的运算符的优先级，执行规约操作
        Reduct(action_and_attached_data);
      } else if (priority_now == terminal_node_info.operator_priority) {
        // 当前优先级等于待处理的运算符的优先级，需要判定结合性
        if (terminal_node_info.associate_type ==
            AssociatityType::kLeftToRight) {
          // 运算符为从左到右结合，执行规约操作
          Reduct(action_and_attached_data);
        } else {
          // 运算符为从右到左结合，执行移入操作
          ShiftTerminalWord(action_and_attached_data);
        }
      } else {
        // 当前优先级低于待处理的运算符的优先级，执行移入操作
        ShiftTerminalWord(action_and_attached_data);
      }
    } break;
#endif  // USE_AMBIGUOUS_GRAMMAR

    default:
      assert(false);
      break;
  }
}

inline void SyntaxMachine::ShiftTerminalWord(
    const ActionAndAttachedData& action_and_target) {
  ParsingData& parsing_data_now = GetParsingDataNow();
  WordInfo& word_info = GetWaitingProcessWordInfo();
  // 构建当前单词的数据

#ifdef USE_AMBIGUOUS_GRAMMAR
  parsing_data_now.operator_priority = GetOperatorPriorityNow();
#endif  // USE_AMBIGUOUS_GRAMMAR

  parsing_data_now.shift_node_id =
      ProductionNodeId(word_info.word_attached_data_.production_node_id);
  parsing_data_now.word_data_to_user_.node_type =
      word_info.word_attached_data_.node_type;
  TerminalWordData terminal_word_data;
  terminal_word_data.word = word_info.symbol_;
  terminal_word_data.line = word_info.line_;
  parsing_data_now.word_data_to_user_.SetWordData(
      std::move(terminal_word_data));

#ifdef USE_AMBIGUOUS_GRAMMAR
  // 如果移入了运算符则更新优先级为新的优先级
  if (parsing_data_now.word_data_to_user_.node_type ==
      ProductionNodeType::kOperatorNode) {
    GetOperatorPriorityNow() =
        OperatorPriority(word_info.word_attached_data_.operator_priority);
  }
#endif  // USE_AMBIGUOUS_GRAMMAR

  // 将当前的状态压入栈，需要调用者设置移入的节点号
  GetParsingStack().emplace(std::move(parsing_data_now));
  // 更新状态为移入该节点后到达的条目
  parsing_data_now.parsing_table_entry_id =
      action_and_target.GetShiftAttachedData().next_entry_id_;

#ifdef USE_AMBIGUOUS_GRAMMAR
  parsing_data_now.operator_priority =
      GetParsingStack().top().operator_priority;
#endif  // USE_AMBIGUOUS_GRAMMAR

  GetNextWord();
}

void SyntaxMachine::Reduct(const ActionAndAttachedData& action_and_target) {
  const ReductAttachedData& reduct_attached_data =
      action_and_target.GetReductAttachedData();
  ProcessFunctionInterface& process_function_object =
      GetProcessFunctionClass(reduct_attached_data.process_function_class_id_);
  const auto& production_body = reduct_attached_data.production_body_;
  // 传递给用户定义函数的数据
  std::vector<WordDataToUser> word_data_to_user;
  auto& parsing_stack = GetParsingStack();
  for (size_t i = production_body.size() - 1; i != -1; i++) {
    if (parsing_stack.top().shift_node_id == production_body[i]) {
      // 该节点正常参与规约
      PopParsingStack();
      word_data_to_user.emplace_back(
          std::move(GetParsingDataNow().word_data_to_user_));
    } else {
      // 该节点空规约
      word_data_to_user.emplace_back();
      word_data_to_user.back().node_type = ProductionNodeType::kEndNode;
    }
  }
  ShiftNonTerminalWord(
      process_function_object.Reduct(std::move(word_data_to_user)),
      action_and_target.GetReductAttachedData().reducted_nonterminal_node_id_);
}

void SyntaxMachine::ShiftNonTerminalWord(
    NonTerminalWordData&& non_terminal_word_data,
    ProductionNodeId reducted_nonterminal_node_id) {
  ParsingData& parsing_data_now = GetParsingDataNow();
  // 获取移入非终结节点后转移到的语法分析表条目
  ParsingTableEntryId next_entry_id = GetNextEntryId(
      parsing_data_now.parsing_table_entry_id, reducted_nonterminal_node_id);
  parsing_data_now.shift_node_id = reducted_nonterminal_node_id;
  parsing_data_now.word_data_to_user_.SetWordData(
      std::move(non_terminal_word_data));
  GetParsingStack().emplace(std::move(parsing_data_now));
  parsing_data_now.parsing_table_entry_id = next_entry_id;

#ifdef USE_AMBIGUOUS_GRAMMAR
  parsing_data_now.operator_priority =
      GetParsingStack().top().operator_priority;
#endif  // USE_AMBIGUOUS_GRAMMAR
}

}  // namespace frontend::parser::syntaxmachine