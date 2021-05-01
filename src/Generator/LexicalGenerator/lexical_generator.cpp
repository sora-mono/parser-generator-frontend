#include "Generator/LexicalGenerator/lexical_generator.h"

#include <queue>

namespace frontend::generator::lexicalgenerator {
inline void LexicalGenerator::AddSymbolIdToProductionNodeIdMapping(
    SymbolId symbol_id, ProductionNodeId node_id) {
  symbol_id_to_node_id_.insert(std::make_pair(symbol_id, node_id));
}

inline LexicalGenerator::ProductionNodeId LexicalGenerator::AddTerminalNode(
    SymbolId symbol_id) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<TerminalProductionNode>(
          NodeType::kTerminalNode, symbol_id);
  manager_nodes_.GetObject(node_id).SetId(node_id);
  return node_id;
}

inline LexicalGenerator::ProductionNodeId LexicalGenerator::AddOperatorNode(
    SymbolId symbol_id, AssociatityType associatity_type,
    PriorityLevel priority_level) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<OperatorProductionNode>(
          NodeType::kOperatorNode, symbol_id, associatity_type, priority_level);
  manager_nodes_.GetObject(node_id).SetId(node_id);
  return node_id;
}

std::vector<LexicalGenerator::ProductionNodeId>
LexicalGenerator::GetSymbolToProductionNodeIds(SymbolId symbol_id) {
  std::vector<LexicalGenerator::ProductionNodeId> node_ids;
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
  CoreId core_id = CoreId::InvalidId();
  if (iter == item_to_core_id_.end()) {
    core_id = AddNewCore();
    AddItemToCore(core_item, core_id);
    SetItemToCoreIdMapping(core_item, core_id);
  } else {
    core_id = iter->second;
  }
  return core_id;
}

void LexicalGenerator::CoreClosure(CoreId core_id) {
  Core& core_now = GetCore(core_id);
  std::queue<const CoreItem*> items_waiting_process;
  for (auto& item : core_now.GetItems()) {
    items_waiting_process.push(&item);
  }
  while (!items_waiting_process.empty()) {
    const CoreItem* item_now = items_waiting_process.front();
    items_waiting_process.pop();
    // NonTerminalProductionNode* production_node =
    //    static_cast<NonTerminalProductionNode*>(
    //        GetProductionNode(item_now->first));
    // production_node->
  }
}
}  // namespace frontend::generator::lexicalgenerator