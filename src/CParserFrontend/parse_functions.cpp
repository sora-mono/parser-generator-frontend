#include "parse_functions.h"
namespace c_parser_frontend::parse_functions {
std::any SingleConstexprValueChar(std::vector<WordDataToUser>&& word_data) {
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
std::any SingleConstexprValueNum(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  std::string& word = word_data.front().GetTerminalWordData().word;
  auto initialize_node = std::make_shared<BasicTypeInitializeOperatorNode>(
      InitializeType::kBasic, std::move(word));
  initialize_node->SetInitDataType(
      CommonlyUsedTypeGenerator::GetBasicType<BuiltInType::kChar,
                                              SignTag::kSigned>());
  return initialize_node;
}
}  // namespace c_parser_frontend::parse_functions