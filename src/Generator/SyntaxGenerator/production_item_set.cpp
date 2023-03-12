#include "production_item_set.h"

namespace frontend::generator::syntax_generator {

ProductionItemSet& ProductionItemSet::operator=(
    ProductionItemSet&& production_item_set) {
  production_item_set_closure_available_ =
      std::move(production_item_set.production_item_set_closure_available_);
  production_item_set_id_ =
      std::move(production_item_set.production_item_set_id_);
  syntax_analysis_table_entry_id_ =
      std::move(production_item_set.syntax_analysis_table_entry_id_);
  item_and_forward_node_ids_ =
      std::move(production_item_set.item_and_forward_node_ids_);
  return *this;
}

void ProductionItemSet::ClearNotMainItem() {
  ProductionItemAndForwardNodesContainer new_container;
  for (auto& main_item_iter : GetMainItemIters()) {
    main_item_iter = new_container.insert(*main_item_iter).first;
  }
  item_and_forward_node_ids_.swap(new_container);
}

bool ProductionItemSet::IsMainItem(const ProductionItem& item) {
  for (const auto& main_item_iter : GetMainItemIters()) {
    if (item == main_item_iter->first) [[unlikely]] {
      return true;
    }
  }
  return false;
}

void ProductionItemSet::SetMainItem(
    ProductionItemAndForwardNodesContainer::const_iterator& item_iter) {
#ifdef _DEBUG
  // 不允许重复设置已有的核心项为核心项
  for (const auto& main_item_already_in : GetMainItemIters()) {
    assert(item_iter->first != main_item_already_in->first);
  }
#endif  // _DEBUG
  main_items_.emplace_back(item_iter);
  SetClosureNotAvailable();
}
}  // namespace frontend::generator::syntax_generator