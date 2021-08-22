#include "reduct_functions.h"
namespace c_parser_frontend::parse_functions {
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueChar(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  std::string& word = word_data.front().GetTerminalWordData().word;
  // ''中夹一个字符
  assert(word.size() == 3);
  // 提取单个字符作为值
  auto initialize_node = std::make_shared<BasicTypeInitializeOperatorNode>(
      InitializeType::kBasic, std::to_string(word[1]));
  initialize_node->SetInitDataType(
      CommonlyUsedTypeGenerator::GetBasicType<BuiltInType::kChar,
                                              SignTag::kSigned>());
  return initialize_node;
}
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueNum(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  std::string& word = word_data.front().GetTerminalWordData().word;
  auto initialize_node = std::make_shared<BasicTypeInitializeOperatorNode>(
      InitializeType::kBasic, std::move(word));
  initialize_node->SetInitDataType(
      CommonlyUsedTypeGenerator::GetBasicType<BuiltInType::kChar,
                                              SignTag::kSigned>());
  return initialize_node;
}
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueString(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  std::string& word = word_data.front().GetTerminalWordData().word;
  // 字符串使用""作为分隔标记
  assert(word.front() == '"');
  assert(word.back() == '"');
  // 删除尾部的"
  word.pop_back();
  // 删除头部的"
  word.erase(word.begin());
  auto initialize_node = std::make_shared<BasicTypeInitializeOperatorNode>(
      InitializeType::kString, std::move(word));
  initialize_node->SetInitDataType(
      CommonlyUsedTypeGenerator::GetConstExprStringType());
  return initialize_node;
}
std::shared_ptr<VarietyOperatorNode> IdOrEquivenceConstTagId(
    std::vector<WordDataToUser>&& word_data) {
  // 无论Consttag是否空规约都应该是2个节点
  assert(word_data.size() == 2);

}
}  // namespace c_parser_frontend::parse_functions