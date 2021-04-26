#include "Generator/LexicalGenerator/lexical_generator.h"
namespace frontend::generator::lexicalgenerator {
inline void LexicalGenerator::AddSymbolIdToNodeIdMapping(SymbolId symbol_id,
                                                         NodeId node_id) {
  symbol_id_to_node_id_.insert(std::make_pair(symbol_id, node_id));
}

inline LexicalGenerator::NodeId LexicalGenerator::AddTerminalNode(
    SymbolId symbol_id) {
  NodeId node_id = manager_nodes_.EmplaceNode<TerminalNode>(
      NodeType::kTerminalNode, symbol_id);
  manager_nodes_.GetNode(node_id)->SetId(node_id);
  return node_id;
}

inline LexicalGenerator::NodeId LexicalGenerator::AddOperatorNode(
    SymbolId symbol_id, AssociatityType associatity_type,
    PriorityLevel priority_level) {
  NodeId node_id = manager_nodes_.EmplaceNode<OperatorNode>(
      NodeType::kOperatorNode, symbol_id, associatity_type, priority_level);
  manager_nodes_.GetNode(node_id)->SetId(node_id);
  return node_id;
}

std::vector<LexicalGenerator::NodeId> LexicalGenerator::GetNodeIds(
    SymbolId symbol_id) {
  std::vector<LexicalGenerator::NodeId> node_ids;
  auto [iter_begin, iter_end] = symbol_id_to_node_id_.equal_range(symbol_id);
  assert(iter_begin != symbol_id_to_node_id_.end());
  for (; iter_begin != iter_end; ++iter_begin) {
    node_ids.push_back(iter_begin->second);
  }
  return node_ids;
}

}  // namespace frontend::generator::lexicalgenerator