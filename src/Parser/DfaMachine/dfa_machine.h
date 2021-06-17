#include "Generator/DfaGenerator/dfa_generator.h"

#ifndef PARSER_DFAMACHINE_DFAMACHINE_H_
#define PARSER_DFAMACHINE_DFAMACHINE_H_

namespace frontend::parser::dfamachine {

class DfaMachine {
  using DfaGenerator = frontend::generator::dfa_generator::DfaGenerator;
  using DfaConfigType = DfaGenerator ::DfaConfigType;
  using SavedData = DfaGenerator::SavedData;
  using TransformArrayId = DfaGenerator::TransformArrayId;

 public:
  DfaMachine() { LoadConfig(); }
  DfaMachine(const DfaMachine&) = delete;
  DfaMachine& operator=(const DfaMachine&) = delete;

  struct ReturnData {
    ReturnData() = default;
    template <class SavedDataType>
    ReturnData(SavedDataType&& saved_data, size_t line, std::string&& symbol_)
        : saved_data_(std::forward<SavedDataType>(saved_data)),
          line_(line),
          symbol_(std::move(symbol_)) {}
    ReturnData(ReturnData&& return_data)
        : saved_data_(std::move(return_data.saved_data_)),
          line_(std::move(return_data.line_)),
          symbol_(std::move(return_data.symbol_)) {}
    ReturnData& operator=(ReturnData&& return_data) {
      saved_data_ = std::move(return_data.saved_data_);
      line_ = std::move(return_data.line_);
      symbol_ = std::move(return_data.symbol_);
      return *this;
    }
    SavedData saved_data_;
    // 单词所在行
    size_t line_;
    // 获取到的单词
    std::string symbol_;
  };
  // 设置输入文件
  bool SetInputFile(const std::string filename);
  // 获取下一个词
  // 如果遇到了文件结尾则返回tail_node_id = SavedData::InvalidId()
  ReturnData GetNextWord();
  // 获取当前行数
  size_t GetLine() { return line_; }
  // 设置当前行数
  void SetLine(size_t line_) { line_ = line_; }
  //重置状态
  void Reset() {
    file_ = nullptr;
    line_ = 0;
  }
  void SetEndOfFileSavedData(const SavedData& saved_data_) {
    file_end_saved_data_ = saved_data_;
  }
  const SavedData& GetEndOfFileSavedData() { return file_end_saved_data_; }

 private:
  // TODO 将加载配置功能单独提取出来放到一个类中
  // 加载配置
  void LoadConfig();

  // 起始DFA分析表ID
  TransformArrayId start_id_;
  // DFA配置
  DfaConfigType dfa_config_;
  // 遇到文件尾时返回的数据
  SavedData file_end_saved_data_;
  // 当前输入文件
  FILE* file_;
  //当前行，从0开始计数，获取到每个回车后+1
  size_t line_ = 0;
};

}  // namespace frontend::parser::dfamachine
#endif  // !PARSER_DFAMACHINE_DFAMACHINE_H_