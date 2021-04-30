#include "Generator/LexicalGenerator/lexical_generator.h"
namespace frontend::generator::lexicalgenerator {
inline void LexicalGenerator::AddSymbolIdToNodeIdMapping(SymbolId symbol_id,
                                                         ObjectId node_id) {
  symbol_id_to_node_id_.insert(std::make_pair(symbol_id, node_id));
}

inline LexicalGenerator::ObjectId LexicalGenerator::AddTerminalNode(
    SymbolId symbol_id) {
  ObjectId node_id = manager_nodes_.EmplaceObject<TerminalNode>(
      NodeType::kTerminalNode, symbol_id);
  manager_nodes_.GetObject(node_id).SetId(node_id);
  return node_id;
}

inline LexicalGenerator::ObjectId LexicalGenerator::AddOperatorNode(
    SymbolId symbol_id, AssociatityType associatity_type,
    PriorityLevel priority_level) {
  ObjectId node_id = manager_nodes_.EmplaceObject<OperatorNode>(
      NodeType::kOperatorNode, symbol_id, associatity_type, priority_level);
  manager_nodes_.GetObject(node_id).SetId(node_id);
  return node_id;
}

std::vector<LexicalGenerator::ObjectId> LexicalGenerator::GetSymbolToNodeIds(
    SymbolId symbol_id) {
  std::vector<LexicalGenerator::ObjectId> node_ids;
  auto [iter_begin, iter_end] = symbol_id_to_node_id_.equal_range(symbol_id);
  assert(iter_begin != symbol_id_to_node_id_.end());
  for (; iter_begin != iter_end; ++iter_begin) {
    node_ids.push_back(iter_begin->second);
  }
  return node_ids;
}

LexicalGenerator::CoreId LexicalGenerator::GetItemCoreIdOrInsert(
    const CoreItem& core_item) {
  auto iter = item_to_core_id_.find(core_item);
  CoreId core_id = -1;
  if (iter == item_to_core_id_.end()) {
    core_id = AddNewCore();
    AddItemToCore(core_item, core_id);
    SetItemToCoreIdMapping(core_item, core_id);
  } else {
    core_id = iter->second;
  }
  return core_id;
}

}  // namespace frontend::generator::lexicalgenerator