#include "Generator/LexicalGenerator/lexical_generator.h"
#include"Parser/DfaMachine/dfa_machine.h"
#include"Common/object_manager.h"

#ifndef PARSER_LEXICALMACHINE_LEXICALMACHINE_H_
#define PARSER_LEXICALMACHINE_LEXICALMACHINE_H_

namespace frontend::parser::lexicalmachine {

class LexicalMachine {
 public:
  using LexicalConfig =
      frontend::generator::lexicalgenerator::LexicalGenerator::LexicalConfig;

  LexicalMachine();
  LexicalMachine(const LexicalMachine&) = delete;
  LexicalMachine& operator=(LexicalMachine&&) = delete;

 private:
  LexicalConfig lexical_config_;
  frontend::parser::dfamachine::DfaMachine dfa_machine_;
};

}  // namespace frontend::parser::lexicalmachine
#endif  // !PARSER_LEXICALMACHINE_LEXICALMACHINE_H_
