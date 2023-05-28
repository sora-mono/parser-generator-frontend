#include "syntax_parser.h"

#include <format>
namespace frontend::parser::syntax_parser {
void SyntaxParser::LoadConfig() {
  std::ifstream config_file(frontend::common::kSyntaxConfigFileName,
                            std::ios_base::binary);
  boost::archive::binary_iarchive iarchive(config_file);
  dfa_parser_.LoadConfig();
  iarchive >> *this;
}
bool SyntaxParser::Parse(const std::string& filename) {
  bool result = dfa_parser_.SetInputFile(filename);
  if (result == false) [[unlikely]] {
    LOG_ERROR("Parser",
              std::format("打开文件\"{:}\"失败，请检查\n", filename));
    return false;
  }
  GetNextWord();
  // 清空解析数据栈
  auto old_parsing_stack = std::stack<ParsingData>();
  parsing_stack_.swap(old_parsing_stack);
  // 压入哨兵，避免Reduct函数中每次弹出栈中数据都需要判断是否栈空
  PushParsingData(ParsingData());
  // 初始化解析数据栈，压入当前解析数据
  PushParsingData(
      ParsingData{.syntax_analysis_table_entry_id = GetRootParsingEntryId(),
                  .operator_priority = OperatorPriority(0)});
  while (GetParsingStackSize() != 1) {
    // TODO 添加用户调用清空数据栈的功能
    TerminalWordWaitingProcess();
  }
  return true;
}

void SyntaxParser::TerminalWordWaitingProcess() {
  assert(GetWaitingProcessWordInfo().word_attached_data_.node_type ==
             ProductionNodeType::kTerminalNode ||
         GetWaitingProcessWordInfo().word_attached_data_.node_type ==
             ProductionNodeType::kOperatorNode ||
         GetWaitingProcessWordInfo().word_attached_data_.node_type ==
             ProductionNodeType::kEndNode);

  ProductionNodeId production_node_to_shift_id =
      GetWaitingProcessWordInfo().word_attached_data_.production_node_id;
  const ActionAndAttachedDataInterface& action_and_attached_data =
      GetActionAndTarget(GetParsingDataNow().syntax_analysis_table_entry_id,
                         production_node_to_shift_id);

  switch (action_and_attached_data.GetActionType()) {
    // TODO 添加接受时的后续处理
    [[unlikely]] case ActionType::kAccept:
      exit(0);
      break;
    // TODO 添加错误处理功能
    [[unlikely]] case ActionType::kError:
      LOG_ERROR("Parser", std::format("Line: {:} Column: {:}: Syntax Error!",
                                      GetLine() + 1, GetColumn() + 1))
      exit(-1);
      break;
    case ActionType::kReduct:
      Reduct(action_and_attached_data.GetReductAttachedData());
      break;
    case ActionType::kShift:
      ShiftTerminalWord(action_and_attached_data.GetShiftAttachedData());
      break;
    case ActionType::kShiftReduct: {
      // 需要根据实际情况判断移入还是规约
      auto& terminal_node_info =
          GetWaitingProcessWordInfo().word_attached_data_;
      switch (terminal_node_info.node_type) {
        case ProductionNodeType::kTerminalNode:
          // 非运算符节点使用贪心策略，防止有公共前缀的产生式只有最短的那条有效
          ShiftTerminalWord(action_and_attached_data.GetShiftAttachedData());
          break;
        case ProductionNodeType::kOperatorNode: {
          // 运算符优先级必须不为0
          OperatorPriority priority_now = GetParsingDataNow().operator_priority;
          auto [operator_associate_type, operator_priority] =
              terminal_node_info.GetAssociatityTypeAndPriority(
                  LastOperateIsReduct());
          if (priority_now > operator_priority) {
            // 当前优先级高于待处理的运算符的优先级，执行规约操作
            Reduct(action_and_attached_data.GetReductAttachedData());
          } else if (priority_now == operator_priority) {
            // 当前优先级等于待处理的运算符的优先级，需要判定结合性
            if (operator_associate_type ==
                OperatorAssociatityType::kLeftToRight) {
              // 运算符为从左到右结合，执行规约操作
              Reduct(action_and_attached_data.GetReductAttachedData());
            } else {
              // 运算符为从右到左结合，执行移入操作
              ShiftTerminalWord(
                  action_and_attached_data.GetShiftAttachedData());
            }
          } else {
            // 当前优先级低于待处理的运算符的优先级，执行移入操作
            ShiftTerminalWord(action_and_attached_data.GetShiftAttachedData());
          }
        } break;
        case ProductionNodeType::kEndNode:
          // EndNode在转移表里的动作只能规约不能移入
        case ProductionNodeType::kNonTerminalNode:
          // NonTerminalNode在产生后就被Reduct函数调用ShiftNonTerminalNode消耗
        default:
          assert(false);
          break;
      }
    } break;
    default:
      assert(false);
      break;
  }
}

inline void SyntaxParser::ShiftTerminalWord(
    const ShiftAttachedData& action_and_target) {
  ParsingData& parsing_data_now = GetParsingDataNow();
  WordInfo& word_info = GetWaitingProcessWordInfo();
  // 构建当前单词的数据
  // 待移入节点的ID
  parsing_data_now.shift_node_id =
      word_info.word_attached_data_.production_node_id;
  // 添加待移入节点信息到当前解析用信息
  parsing_data_now.word_data_to_user = std::move(word_info.symbol_);
  // 如果移入了运算符则更新优先级为新的优先级
  OperatorPriority new_parsing_data_priority;
  if (word_info.word_attached_data_.node_type ==
      ProductionNodeType::kOperatorNode) {
    new_parsing_data_priority =
        word_info.word_attached_data_
            .GetAssociatityTypeAndPriority(LastOperateIsReduct())
            .second;
  } else {
    // 否则使用原来的优先级
    new_parsing_data_priority = parsing_data_now.operator_priority;
  }
  // 压入移入该单词后得到的解析数据
  PushParsingData(
      ParsingData{.syntax_analysis_table_entry_id =
                      action_and_target.GetNextSyntaxAnalysisTableEntryId(),
                  .operator_priority = new_parsing_data_priority});
  // 获取下一个单词
  GetNextWord();
  // 执行了移入操作，需要设置上一步为非规约操作
  SetLastOperateIsNotReduct();
}

void SyntaxParser::Reduct(const ReductAttachedData& action_and_target) {
  const ReductAttachedData& reduct_attached_data =
      action_and_target.GetReductAttachedData();
  const ProcessFunctionInterface& process_function_object =
      GetProcessFunctionClass(reduct_attached_data.GetProcessFunctionClassId());
  const auto& production_body = reduct_attached_data.GetProductionBody();
  // 传递给用户定义函数的数据
  std::vector<std::any> word_data_to_user(production_body.size());

  // 弹出栈顶数据，露出word_data_to_user存储有效数据的部分
  // 除了这里任何时候栈顶数据的word_data_to_user都不包含有效数据
  ParsingData old_parsing_data = std::move(GetParsingDataNow());
  PopTopParsingData();
  // 生成规约数据，数组内数据顺序为产生式书写顺序
  // 从后向前添加数据
  auto production_node_id_iter = production_body.rbegin();
  for (auto user_data_iter = word_data_to_user.rbegin();
       user_data_iter != word_data_to_user.rend(); ++production_node_id_iter) {
    if (GetParsingDataNow().shift_node_id == *production_node_id_iter)
        [[likely]] {
      // 当前产生式正常参与规约，获取之前保存下来提供给用户规约使用的数据
      *user_data_iter = std::move(GetParsingDataNow().word_data_to_user);
      // 弹出解析数据栈顶部无附属数据的部分，露出之前存入的解析数据
      old_parsing_data = std::move(GetParsingDataNow());
      PopTopParsingData();
      // 由于有哨兵，解析数据栈永远不空
      assert(GetParsingStackSize() != 0);
    }
    ++user_data_iter;
  }
  // 恢复栈顶数据
  // 栈顶数据储存有效信息：当前语法分析表条目和运算符优先级
  PushParsingData(std::move(old_parsing_data));
  ShiftNonTerminalWord(
      process_function_object.Reduct(std::move(word_data_to_user)),
      reduct_attached_data.GetReductedNonTerminalNodeId());
  // 执行了一次完整的规约操作，需要设置上一步执行了规约操作的标记
  SetLastOperateIsReduct();

  LOG_INFO("Parser",
           std::format("Reduct Function Called: {:}",
                       process_function_object.GetReductFunctionName()))
}

void SyntaxParser::ShiftNonTerminalWord(
    std::any&& non_terminal_word_data,
    ProductionNodeId reducted_nonterminal_node_id) {
  ParsingData& parsing_data_now = GetParsingDataNow();
  // 获取移入非终结节点后转移到的语法分析表条目
  SyntaxAnalysisTableEntryId next_entry_id =
      GetNextEntryId(parsing_data_now.syntax_analysis_table_entry_id,
                     reducted_nonterminal_node_id);
  // 更新移入节点的ID和附属数据
  parsing_data_now.shift_node_id = reducted_nonterminal_node_id;
  parsing_data_now.word_data_to_user = std::move(non_terminal_word_data);
  // 移入非终结节点不改变运算符优先级
  PushParsingData(
      ParsingData{.syntax_analysis_table_entry_id = next_entry_id,
                  .operator_priority = parsing_data_now.operator_priority});
}

}  // namespace frontend::parser::syntax_parser