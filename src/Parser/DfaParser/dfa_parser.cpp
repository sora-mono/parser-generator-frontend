#include "Parser/DfaParser/dfa_parser.h"

#include <format>

namespace frontend::parser::dfa_parser {
bool DfaParser::SetInputFile(const std::string filename) {
  FILE* file;
  fopen_s(&file, filename.c_str(), "r");
  if (file == nullptr) {
    return false;
  } else {
    file_ = file;
    SetCharacterNow(fgetc(file_));
    return true;
  }
}

DfaParser::WordInfo DfaParser::GetNextWord() {
  std::string symbol;
  WordInfo return_data;
  // 当前状态转移表ID
  TransformArrayId transform_array_id = root_transform_array_id_;
  // 跳过空白字符
  while (std::isspace(GetCharacterNow())) {
    if (GetCharacterNow() == '\n') [[unlikely]] {
      // 行数+1
      SetLine(GetLine() + 1);
      // 重置列数为0
      SetColumn(0);
    }
    SetCharacterNow(fgetc(file_));
    SetColumn(GetColumn() + 1);
  }
  while (true) {
    // 判断特殊情况
    switch (GetCharacterNow()) {
      case EOF:
        if (feof(file_)) {
          if (!symbol.empty()) {
            // 如果已经获取到了单词则返回单词携带的数据而不是直接返回文件尾数据
            return WordInfo(dfa_config_[transform_array_id].second,
                            std::move(symbol));
          } else {
            // 没有获取到单词，直接返回文件尾数据
            return WordInfo(GetEndOfFileSavedData(), std::string());
          }
        }
        break;
      case '\n':
        // 行数+1
        SetLine(GetLine() + 1);
        // 重置列数为0
        SetColumn(0);
        SetCharacterNow(fgetc(file_));
        continue;
        break;
      default:
        break;
    }
    TransformArrayId next_array_id =
        dfa_config_[transform_array_id].first[GetCharacterNow()];
    if (!next_array_id.IsValid()) {
      // 无法移入当前字符
      break;
    } else {
      // 可以移入
      symbol += GetCharacterNow();
      transform_array_id = next_array_id;
      SetCharacterNow(fgetc(file_));
      SetColumn(GetColumn() + 1);
    }
  }
  std::cout << symbol << std::endl;
  return WordInfo(dfa_config_[transform_array_id].second, std::move(symbol));
}

}  // namespace frontend::parser::dfa_parser