#include "Common/object_manager.h"
#include "Generator/LexicalGenerator/lexical_generator.h"
#include "Parser/DfaMachine/dfa_machine.h"

#ifndef PARSER_LEXICALMACHINE_LEXICALMACHINE_H_
#define PARSER_LEXICALMACHINE_LEXICALMACHINE_H_

namespace frontend::parser::lexicalmachine {

class LexicalMachine {
 public:
  // 语法分析表ID
  using ParsingTableEntryId = frontend::generator::lexicalgenerator::
      LexicalGenerator::ParsingTableEntryId;
  // 产生式体ID（ID不关联到产生式对象，在生成配置后产生式对象便没有价值）
  using ProductionNodeId =
      frontend::generator::lexicalgenerator::LexicalGenerator::ProductionNodeId;
  // DFA引擎返回的数据
  using DfaReturnData = frontend::parser::dfamachine::DfaMachine::ReturnData;
  // 管理包装用户自定义函数和数据的类的已分配对象的容器
  using ProcessFunctionClassManagerType = frontend::generator::
      lexicalgenerator::LexicalGenerator::ProcessFunctionClassManagerType;
  LexicalMachine();
  LexicalMachine(const LexicalMachine&) = delete;
  LexicalMachine& operator=(LexicalMachine&&) = delete;
  // 获取下一个单词的数据
  DfaReturnData GetNextNode() { return dfa_machine_.GetNextNode(); }
  //分析代码文件
  bool Parse(const std::string& filename);

 private:
  frontend::parser::dfamachine::DfaMachine dfa_machine_;
  ProcessFunctionClassManagerType manager_process_function_class_;
};

}  // namespace frontend::parser::lexicalmachine
#endif  // !PARSER_LEXICALMACHINE_LEXICALMACHINE_H_