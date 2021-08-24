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
IdOrEquivenceReturnData IdOrEquivenceConstTagId(
    std::vector<WordDataToUser>&& word_data) {
  // 无论Consttag是否空规约都应该是2个节点
  assert(word_data.size() == 2);
  auto& const_tag_data = word_data[0].GetNonTerminalWordData().user_data_;
  ConstTag const_tag;
  if (!const_tag_data.has_value()) {
    // Const标记空规约，使用非const标记
    const_tag = ConstTag::kNonConst;
  } else {
    // 设置const_tag为之前规约得到的Tag
    const_tag = std::any_cast<ConstTag>(const_tag_data);
  }
  auto iter = parser_front_end.AnnounceVariety(
      std::string(word_data[1].GetTerminalWordData().word));
  // 构建变量节点
  std::shared_ptr<VarietyOperatorNode> variety_node =
      std::make_shared<VarietyOperatorNode>(&iter->first, const_tag,
                                            LeftRightValueTag::kLeftValue);
  // 创建哨兵节点，避免判断type_chain是否为空
  std::shared_ptr<TypeInterface> type_chain = std::make_shared<EndType>();
  return IdOrEquivenceReturnData(variety_node, type_chain, type_chain);
}
IdOrEquivenceReturnData IdOrEquivenceConstTag(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  auto& const_tag_data = word_data.front().GetNonTerminalWordData().user_data_;
  ConstTag const_tag;
  if (!const_tag_data.has_value()) {
    // Const标记空规约，使用非const标记
    const_tag = ConstTag::kNonConst;
  } else {
    // 设置const_tag为之前规约得到的Tag
    const_tag = std::any_cast<ConstTag>(const_tag_data);
  }
  // 构建变量节点
  std::shared_ptr<VarietyOperatorNode> variety_node =
      std::make_shared<VarietyOperatorNode>(nullptr, const_tag,
                                            LeftRightValueTag::kLeftValue);
  // 创建哨兵节点，避免判断type_chain是否为空
  std::shared_ptr<TypeInterface> type_chain = std::make_shared<EndType>();
  return IdOrEquivenceReturnData(variety_node, type_chain, type_chain);
}
std::any IdOrEquivenceNumAddressing(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 4);
  // 获取之前解析到的信息
  IdOrEquivenceReturnData&& data_before =
      std::any_cast<IdOrEquivenceReturnData>(
          word_data[0].GetNonTerminalWordData().user_data_);
  // 转换为long long的字符数
  size_t chars_converted_to_longlong;
  std::string& array_size_or_index_string =
      word_data[2].GetTerminalWordData().word;
  // 添加一层指针和指针指向的数组大小
  data_before.GetTypeChainTailReference().SetNextNodePointer(
      std::make_shared<PointerType>(ConstTag::kNonConst,
                                    std::stoll(array_size_or_index_string,
                                               &chars_converted_to_longlong)));
  if (chars_converted_to_longlong != array_size_or_index_string.size())
      [[unlikely]] {
    // 不是所有的数字都参与了转换，说明使用了浮点数
    std::cerr
        << std::format(
               "行数：{:} "
               "{:}无法作为数组大小或偏移量，原因可能为浮点数或者数值过大",
               word_data[2].GetTerminalWordData().line,
               array_size_or_index_string)
        << std::endl;
    exit(-1);
  }
  // 设置新的尾节点指针
  data_before.SetTypeChainTail(
      data_before.GetTypeChainTailReference().GetNextNodePointer());
  return std::move(word_data[0].GetNonTerminalWordData().user_data_);
}
std::any IdOrEquivenceAnonymousAddressing(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 3);
  // 获取之前解析到的信息
  IdOrEquivenceReturnData& data_before =
      *std::any_cast<IdOrEquivenceReturnData>(
          &word_data[0].GetNonTerminalWordData().user_data_);
  // 添加一层指针和指针指向的数组大小
  // -1代表待推断大小
  data_before.GetTypeChainTailReference().SetNextNodePointer(
      std::make_shared<PointerType>(ConstTag::kNonConst, -1));
  // 设置新的尾节点指针
  data_before.SetTypeChainTail(
      data_before.GetTypeChainTailReference().GetNextNodePointer());
  return std::move(word_data[0].GetNonTerminalWordData().user_data_);
}
std::any IdOrEquivencePointerAnnounce(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 3);
  // 获取之前解析到的信息
  IdOrEquivenceReturnData& data_before =
      *std::any_cast<IdOrEquivenceReturnData>(
          &word_data[2].GetNonTerminalWordData().user_data_);
  // 添加一层指针和指针指向的数组大小
  // 0代表纯指针
  data_before.GetTypeChainTailReference().SetNextNodePointer(
      std::make_shared<PointerType>(ConstTag::kNonConst, 0));
  // 设置新的尾节点指针
  data_before.SetTypeChainTail(
      data_before.GetTypeChainTailReference().GetNextNodePointer());
  return std::move(word_data[2].GetNonTerminalWordData());
}
std::any IdOrEquivenceInBrackets(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 3);
  return std::move(word_data[1].GetNonTerminalWordData().user_data_);
}
EnumReturnData NotEmptyEnumArgumentsIdInit(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 1);
  std::string& id = word_data.front().GetTerminalWordData().word;
  EnumReturnData enum_return_data;
  // 第一个枚举值默认从0开始
  enum_return_data.GetContainer().emplace(std::move(id), 0);
  // 更新相关信息
  enum_return_data.SetLastValue(0);
  enum_return_data.SetMaxValue(0);
  enum_return_data.SetMinValue(0);
  return enum_return_data;
}
EnumReturnData NotEmptyEnumArgumentsIdAssignNumInit(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 3);
  std::string& id = word_data[0].GetTerminalWordData().word;
  std::string& num = word_data[2].GetTerminalWordData().word;
  EnumReturnData enum_return_data;
  size_t chars_converted_to_longlong;
  // 枚举项的值
  long long id_value = std::stoll(num, &chars_converted_to_longlong);
  if (chars_converted_to_longlong != num.size()) [[unlikely]] {
    // 存在浮点数，不是所有的值都可以作为枚举项的值
    std::cerr << std::format(
                     "行数：{:} {:}不能作为枚举项的值，原因可能为浮点数",
                     word_data[2].GetTerminalWordData().line, num)
              << std::endl;
    exit(-1);
  }
  // 添加给定值的枚举值
  enum_return_data.GetContainer().emplace(std::move(id), id_value);
  // 设定相关信息
  enum_return_data.SetLastValue(id_value);
  enum_return_data.SetMaxValue(id_value);
  enum_return_data.SetMinValue(id_value);
  return enum_return_data;
}
std::any NotEmptyEnumArgumentsId(std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 3);
  EnumReturnData& data_before = *std::any_cast<EnumReturnData>(
      &word_data[0].GetNonTerminalWordData().user_data_);
  std::string& id = word_data[2].GetTerminalWordData().word;
  // 未指定项的值则顺延上一个定义的项的值
  long long id_value = data_before.GetLastValue() + 1;
  auto [iter, inserted] =
      data_before.GetContainer().emplace(std::move(id), id_value);
  if (!inserted) [[unlikely]] {
    // 待添加的项的ID已经存在
    std::cerr << std::format("行数：{:} 枚举项名：{:}已定义",
                             word_data[2].GetTerminalWordData().line, id)
              << std::endl;
    exit(-1);
  }
  // 更新相关信息
  data_before.SetLastValue(id_value);
  if (data_before.GetMaxValue() < id_value) {
    data_before.SetMaxValue(id_value);
  } else if (data_before.GetMinValue() > id_value) {
    data_before.SetMinValue(id_value);
  }
  return std::move(word_data[0].GetNonTerminalWordData().user_data_);
}
std::any NotEmptyEnumArgumentsIdAssignNum(
    std::vector<WordDataToUser>&& word_data) {
  assert(word_data.size() == 5);
  EnumReturnData& data_before = *std::any_cast<EnumReturnData>(
      &word_data[0].GetNonTerminalWordData().user_data_);
  std::string& id = word_data[2].GetTerminalWordData().word;
  std::string& num = word_data[4].GetTerminalWordData().word;
  size_t chars_converted_to_longlong;
  // 未指定项的值则顺延上一个定义的项的值
  long long id_value = std::stoll(num, &chars_converted_to_longlong);
  if (chars_converted_to_longlong != num.size()) [[unlikely]] {
    // 给定项的值不能完全转化为long long
    std::cerr << std::format(
                     "行数：{0:} 枚举项{1:} = {2:}中 "
                     "{2:}不能作为枚举项的值，可能原因为浮点数",
                     word_data[4].GetTerminalWordData().line, id, num)
              << std::endl;
    exit(-1);
  }
  auto [iter, inserted] =
      data_before.GetContainer().emplace(std::move(id), id_value);
  if (!inserted) [[unlikely]] {
    // 待添加的项的ID已经存在
    std::cerr << std::format(
                     "行数：{0:} 枚举项{1:} = {2:}中 枚举项名：{1:}已定义",
                     word_data[2].GetTerminalWordData().line, id, num)
              << std::endl;
    exit(-1);
  }
  // 更新相关信息
  data_before.SetLastValue(id_value);
  if (data_before.GetMaxValue() < id_value) {
    data_before.SetMaxValue(id_value);
  } else if (data_before.GetMinValue() > id_value) {
    data_before.SetMinValue(id_value);
  }
  return std::move(word_data[0].GetNonTerminalWordData().user_data_);
}
}  // namespace c_parser_frontend::parse_functions
