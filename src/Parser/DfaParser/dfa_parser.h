/// @file dfa_parser.h
/// @brief DFA解析器
#ifndef PARSER_DFAPARSER_DFAPARSER_H_
#define PARSER_DFAPARSER_DFAPARSER_H_

#include <fstream>
#include <iostream>
#include <list>

#include "Common/common.h"
#include "Generator/export_types.h"
#include "Parser/line_and_column.h"
#include "boost/archive/binary_iarchive.hpp"

namespace frontend::parser::dfa_parser {

/// @class DfaParser dfa_parser.h
/// @brief DFA解析器
class DfaParser {
  using DfaConfigType = frontend::generator::dfa_generator::DfaConfigType;
  using WordAttachedData = frontend::generator::dfa_generator::WordAttachedData;
  using TransformArrayId = frontend::generator::dfa_generator::TransformArrayId;

 public:
  DfaParser() {}
  DfaParser(const DfaParser&) = delete;
  ~DfaParser() {
    if (file_ != nullptr) {
      fclose(file_);
    }
  }
  DfaParser& operator=(const DfaParser&) = delete;

  /// @class WordInfo dfa_parser.h
  /// @brief 单词数据
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
    /// @brief 单词的附属数据（添加单词时存储）
    WordAttachedData word_attached_data_;
    /// @brief 获取到的单词
    std::string symbol_;
  };

  /// @brief 设置输入文件
  /// @param[in] filename ：输入文件名
  /// @return 返回打开文件是否成功
  /// @retval true 成功打开文件
  /// @retval false 打开文件失败
  /// @note 自动读取输入文件的第一个字符到character_now
  bool SetInputFile(const std::string filename);
  /// @brief 获取下一个单词
  /// @return 返回获取到的单词数据
  /// @retval WordInfo(GetEndOfFileSavedData,std::string())
  /// 达到文件尾且未获取到单词
  /// @note
  /// 如果获取单词时达到文件尾则返回获取到的单词和附属数据
  WordInfo GetNextWord();

  /// @brief 重置状态
  void Reset() {
    file_ = nullptr;
    SetLine(0);
    SetColumn(0);
  }
  /// @brief 设置达到文件尾且未获取到任何单词时返回的单词数据
  /// @param[in] word_attached_data ：单词数据
  void SetEndOfFileSavedData(const WordAttachedData& word_attached_data) {
    file_end_saved_data_ = word_attached_data;
  }
  /// @brief 获取达到文件尾且未获取到任何单词时返回的单词数据
  /// @return 返回达到文件尾且未获取到任何单词时返回的单词数据
  const WordAttachedData& GetEndOfFileSavedData() const {
    return file_end_saved_data_;
  }
  /// @brief 加载配置
  /// @note 配置文件名为frontend::common::kDfaConfigFileName
  void LoadConfig() {
    std::ifstream config_file(frontend::common::kDfaConfigFileName,
                              std::ios_base::binary);
    boost::archive::binary_iarchive iarchive(config_file);
    iarchive >> *this;
  }
  /// @brief 设置当前待处理字符
  /// @param[in] character_now ：当前待处理字符
  void SetCharacterNow(char character_now) { character_now_ = character_now; }
  /// @brief 获取当前待处理字符
  /// @return 返回当前待处理字符
  char GetCharacterNow() const { return character_now_; }

 private:
  /// @brief 允许序列化类访问成员
  friend class boost::serialization::access;

  /// @brief boost-serialization加载DFA配置的函数
  /// @param[in,out] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  template <class Archive>
  void load(Archive& ar, const unsigned int version) {
    ar >> dfa_config_;
    ar >> root_transform_array_id_;
    ar >> file_end_saved_data_;
  }
  /// 将序列化分为保存与加载，Parser仅加载配置，不保存
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  /// @brief 起始DFA分析表ID
  TransformArrayId root_transform_array_id_;
  /// @brief DFA配置
  DfaConfigType dfa_config_;
  /// @brief 遇到文件尾且未获取到单词时返回的数据
  WordAttachedData file_end_saved_data_;
  /// @brief 当前输入文件
  FILE* file_ = nullptr;
  /// @brief 当前待处理字符
  char character_now_;
};

}  // namespace frontend::parser::dfa_parser
#endif  /// !PARSER_DFAMACHINE_DFAMACHINE_H_