#ifndef PARSER_DFAMACHINE_DFAMACHINE_H_
#define PARSER_DFAMACHINE_DFAMACHINE_H_

#include <fstream>
#include <iostream>
#include <list>

#include "Common/common.h"
#include "Generator/export_types.h"
#include "Parser/line_and_column.h"
#include "boost/archive/binary_iarchive.hpp"

namespace frontend::parser::dfamachine {

class DfaMachine {
  using DfaConfigType = frontend::generator::dfa_generator::DfaConfigType;
  using WordAttachedData = frontend::generator::dfa_generator::WordAttachedData;
  using TransformArrayId = frontend::generator::dfa_generator::TransformArrayId;

 public:
  DfaMachine() {}
  DfaMachine(const DfaMachine&) = delete;
  DfaMachine& operator=(const DfaMachine&) = delete;

  struct WordInfo {
    WordInfo() = default;
    template <class SavedDataType>
    WordInfo(SavedDataType&& saved_data, std::string&& symbol_)
        : word_attached_data_(std::forward<SavedDataType>(saved_data)),
          symbol_(std::move(symbol_)) {}
    WordInfo(WordInfo&& return_data)
        : word_attached_data_(std::move(return_data.word_attached_data_)),
          symbol_(std::move(return_data.symbol_)) {}
    WordInfo& operator=(WordInfo&& return_data) {
      word_attached_data_ = std::move(return_data.word_attached_data_);
      symbol_ = std::move(return_data.symbol_);
      return *this;
    }
    WordAttachedData word_attached_data_;
    // 获取到的单词
    std::string symbol_;
  };

  // 设置输入文件，自动读取输入文件的第一个字符到character_now
  bool SetInputFile(const std::string filename);
  // 获取下一个词
  // 如果遇到了文件结尾则返回tail_node_id = WordAttachedData::InvalidId()
  WordInfo GetNextWord();

  // 重置状态
  void Reset() {
    file_ = nullptr;
    SetLine(0);
    SetColumn(0);
  }
  void SetEndOfFileSavedData(const WordAttachedData& word_attached_data_) {
    file_end_saved_data_ = word_attached_data_;
  }
  const WordAttachedData& GetEndOfFileSavedData() {
    return file_end_saved_data_;
  }
  // 加载配置
  void LoadConfig() {
    std::ifstream config_file(frontend::common::kDfaConfigFileName,
                              std::ios_base::binary);
    boost::archive::binary_iarchive iarchive(config_file);
    iarchive >> *this;
  }
  void SetCharacterNow(char character_now) { character_now_ = character_now; }
  char GetCharacterNow() const { return character_now_; }

 private:
  // 允许序列化类访问成员
  friend class boost::serialization::access;

  template <class Archive>
  void load(Archive& ar, const unsigned int version) {
    ar >> dfa_config_;
    ar >> root_transform_array_id_;
    ar >> file_end_saved_data_;
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  // 起始DFA分析表ID
  TransformArrayId root_transform_array_id_;
  // DFA配置
  DfaConfigType dfa_config_;
  // 遇到文件尾时返回的数据
  WordAttachedData file_end_saved_data_;
  // 当前输入文件
  FILE* file_;
  // 当前读取到的字符
  char character_now_;
};

}  // namespace frontend::parser::dfamachine
#endif  // !PARSER_DFAMACHINE_DFAMACHINE_H_