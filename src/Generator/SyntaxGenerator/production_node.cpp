#include "production_node.h"

namespace frontend::generator::syntax_generator {
BaseProductionNode& BaseProductionNode::operator=(
    BaseProductionNode&& base_production_node) {
  base_type_ = std::move(base_production_node.base_type_);
  base_id_ = std::move(base_production_node.base_id_);
  base_symbol_id_ = std::move(base_production_node.base_symbol_id_);
  return *this;
}

OperatorNodeInterface& OperatorNodeInterface::operator=(
    OperatorNodeInterface&& operator_production_node) {
  BaseProductionNode::operator=(std::move(operator_production_node));
  operator_type_ = std::move(operator_production_node.operator_type_);
  return *this;
}

ProductionNodeId TerminalProductionNode::GetProductionNodeInBody(
    ProductionBodyId production_body_id,
    NextWordToShiftIndex next_word_to_shift_index) const {
  assert(production_body_id == 0);
  if (next_word_to_shift_index == 0) {
    return GetNodeId();
  } else {
    return ProductionNodeId::InvalidId();
  }
}
ProductionNodeId OperatorNodeInterface::GetProductionNodeInBody(
    ProductionBodyId production_body_id,
    NextWordToShiftIndex next_word_to_shift_index) const {
  assert(production_body_id == 0);
  if (next_word_to_shift_index != 0) [[unlikely]] {
    return ProductionNodeId::InvalidId();
  } else {
    return GetNodeId();
  }
}

std::vector<ProductionBodyId> NonTerminalProductionNode::GetAllBodyIds() const {
  std::vector<ProductionBodyId> production_body_ids;
  for (size_t i = 0; i < nonterminal_bodys_.size(); i++) {
    production_body_ids.push_back(ProductionBodyId(i));
  }
  return production_body_ids;
}

void NonTerminalProductionNode::AddProductionItemBelongToProductionItemSetId(
    ProductionBodyId body_id, NextWordToShiftIndex next_word_to_shift_index,
    ProductionItemSetId production_item_set_id) {
#ifdef _DEBUG
  const auto& core_ids_already_in =
      nonterminal_bodys_[body_id].cores_items_in_[next_word_to_shift_index];
  for (auto core_id_already_in : core_ids_already_in) {
    assert(production_item_set_id != core_id_already_in);
  }
#endif  // _DEBUG
  nonterminal_bodys_[body_id]
      .cores_items_in_[next_word_to_shift_index]
      .push_back(production_item_set_id);
}

ProductionNodeId NonTerminalProductionNode::GetProductionNodeInBody(
    ProductionBodyId production_body_id,
    NextWordToShiftIndex next_word_to_shift_index) const {
  assert(production_body_id < nonterminal_bodys_.size());
  if (next_word_to_shift_index <
      nonterminal_bodys_[production_body_id].production_body.size()) {
    return nonterminal_bodys_[production_body_id]
        .production_body[next_word_to_shift_index];
  } else {
    return ProductionNodeId::InvalidId();
  }
}
}  // namespace frontend::generator::syntax_generator