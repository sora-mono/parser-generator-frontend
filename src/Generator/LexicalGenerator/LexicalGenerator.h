#include "Common/NodeManager.h"
#ifndef GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_
#define GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_

namespace generator::lexicalgenerator {
using common::NodeManager;

class LexicalGenerator {
 private:
  struct BaseNode;

  using StringIdType = NodeManager<std::string>::NodeId;
  using NodeIdType = NodeManager<BaseNode>::NodeId;
  //节点类型，分为终结符号与非终结符号
  enum SymbolType : bool { kTerminalSymbol, kNonTerminalSymbol };
  //节点标记，普通节点、运算符节点
  enum SymbolTag : bool { kUsual, kOperator };

  struct BaseNode {
    SymbolTag node_tag;           //节点标记
    NodeIdType node_id;           //该节点ID
    StringIdType node_symbol_id;  //该节点符号的ID
    virtual ~BaseNode() {}
  };
  struct TerminalNode : public BaseNode {
    bool is_opreator;  //是否为运算符
  };
  struct NonTerminalNode : public BaseNode {
    //表达式的体
    std::vector<std::vector<NodeIdType>> body;
  };
};
}  // namespace generator::lexicalgenerator
#endif  // !GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_
