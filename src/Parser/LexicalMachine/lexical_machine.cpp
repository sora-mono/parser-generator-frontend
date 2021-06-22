#include "lexical_machine.h"

#include <stack>
namespace frontend::parser::lexicalmachine {

bool LexicalMachine::Parse(const std::string& filename) {
  bool result = dfa_machine_.SetInputFile(filename);
  if (result == false) {
    [[unlikely]] fprintf(stderr, "打开文件失败，请检查\n");
    return false;
  }
  std::stack<ParsingData> parsing_data;
  GetNextWord();
  // 根产生式的分析数据
  ParsingData root_parsing_data;
  root_parsing_data.parsing_table_entry_id = GetRootParsingEntryId();
  // 将根节点压入栈
  parsing_data.push(std::move(root_parsing_data));

  while (!parsing_data.empty()) {
    const DfaReturnData& data = GetWaitingShiftWordData();
    switch (data.saved_data_.node_type) {
      case ProductionNodeType::kTerminalNode:
        break;
      case ProductionNodeType::kOperatorNode:
        break;
      case ProductionNodeType::kNonTerminalNode:
        break;
      default:
        assert(false);
        break;
    }
  }
}

void LexicalMachine::TerminalWordWaitingShift(
    std::queue<ParsingData>* parsing_data, ParsingData* parsing_data_now) {
  const ActionAndAttachedData& action_and_attached_data = *GetActionAndTarget(
      parsing_data_now->parsing_table_entry_id,
      ProductionNodeId(
          GetWaitingShiftWordData().saved_data_.production_node_id));
  switch (action_and_attached_data.action_type_) {
    [[unlikely]] case ActionType::kAccept : break;
    [[unlikely]] case ActionType::kError : break;
    case ActionType::kReduct:
      TerminalWordReduct(action_and_attached_data, parsing_data,
                         parsing_data_now);
      break;
    case ActionType::kShift:
      // 设置要移入的节点的ID
      parsing_data_now->shift_node_id = ProductionNodeId(
          GetWaitingShiftWordData().saved_data_.production_node_id);
      TerminalWordShift(action_and_attached_data, parsing_data,
                        parsing_data_now);
      break;
    case ActionType::kShiftReduct:
      // 待移入的单词必须是运算符，运算符优先级必须不为0
      assert(GetWaitingShiftWordData().saved_data_.priority != 0);

      break;
    default:
      assert(false);
      break;
  }
}

inline void LexicalMachine::TerminalWordShift(
    const ActionAndAttachedData& action_and_target,
    std::queue<ParsingData>* parsing_data, ParsingData* parsing_data_now) {
  // 将当前的状态压入栈，需要调用者设置移入的节点号
  parsing_data->push(*parsing_data_now);
  // 更新状态为移入该节点后到达的条目
  parsing_data_now->parsing_table_entry_id =
      action_and_target.GetShiftAttachedData().next_entry_id_;
  GetNextWord();
}

void LexicalMachine::TerminalWordReduct(
    const ActionAndAttachedData& action_and_target,
    std::queue<ParsingData>* parsing_data, ParsingData* parsing_data_now) {}

}  // namespace frontend::parser::lexicalmachine