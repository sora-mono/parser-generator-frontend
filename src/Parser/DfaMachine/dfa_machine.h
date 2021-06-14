#include "Generator/DfaGenerator/dfa_generator.h"

#ifndef PARSER_DFAMACHINE_DFAMACHINE_H_
#define PARSER_DFAMACHINE_DFAMACHINE_H_

namespace frontend::parser::dfamachine {

class DfaMachine {
  using DfaConfigType =
      frontend::generator::dfa_generator::DfaGenerator::DfaConfigType;
  using SavedData = frontend::generator::dfa_generator::DfaGenerator::SavedData;
  using TransformArrayId =
      frontend::generator::dfa_generator::DfaGenerator::TransformArrayId;

 public:
  DfaMachine() { LoadConfig(); }
  DfaMachine(const DfaMachine&) = delete;
  DfaMachine& operator=(const DfaMachine&) = delete;

  struct ReturnData {
    SavedData saved_data;
    // 单词所在行
    size_t line;
    // 获取到的单词
    std::string symbol;
  };
  // 设置输入文件
  bool SetInputFile(const std::string filename);
  // 获取下一个词
  // 如果遇到了文件结尾则返回tail_node_id = SavedData::InvalidId()
  ReturnData GetNextNode();
  // 获取当前行数
  size_t GetLine() { return line_; }
  // 设置当前行数
  void SetLine(size_t line) { line_ = line; }
  //重置状态
  void Reset() {
    file_ = nullptr;
    line_ = 0;
  }
  const SavedData& GetSavedDataEndOfFile() { return file_end_saved_data_; }

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