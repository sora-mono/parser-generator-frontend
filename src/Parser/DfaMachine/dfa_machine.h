#include "Generator/DfaGenerator/dfa_generator.h"

#ifndef PARSER_DFAMACHINE_DFAMACHINE_H_
#define PARSER_DFAMACHINE_DFAMACHINE_H_

namespace frontend::parser::dfamachine {

class DfaMachine {
  using DfaConfigType =
      frontend::generator::dfa_generator::DfaGenerator::DfaConfigType;
  using TailNodeId =
      frontend::generator::dfa_generator::DfaGenerator::TailNodeId;
  using TransformArrayId =
      frontend::generator::dfa_generator::DfaGenerator::TransformArrayId;

 public:
  DfaMachine() { LoadConfig(); }
  DfaMachine(const DfaMachine&) = delete;
  DfaMachine& operator=(const DfaMachine&) = delete;

  struct ReturnData {
    TailNodeId tail_node_id;
    size_t line;
    std::string symbol;
  };
  // 设置输入文件
  bool SetInputFile(const std::string filename);
  // 获取下一个词
  // 如果遇到了文件结尾则返回tail_node_id = TailNodeId::InvalidId()
  ReturnData GetNextNode();
  //重置状态
  void Reset() {
    file_ = nullptr;
    line_ = 0;
  }

 private:
  // TODO 将加载配置功能单独提取出来放到一个类中
  // 加载配置
  void LoadConfig();

  // 起始DFA分析表ID
  TransformArrayId start_id_;
  // DFA配置
  DfaConfigType dfa_config_;
  // 当前输入文件
  FILE* file_;
  //当前行，从0开始计数，获取到每个回车后+1
  size_t line_ = 0;
};

}  // namespace frontend::parser::dfamachine
#endif  // !PARSER_DFAMACHINE_DFAMACHINE_H_