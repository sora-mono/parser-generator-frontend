#ifndef GENERATOR_SYNTAXGENERATOR_PRODUCTION_ITEM_SET_H_
#define GENERATOR_SYNTAXGENERATOR_PRODUCTION_ITEM_SET_H_

#include <unordered_set>

#include "Generator/export_types.h"
namespace frontend::generator::syntax_generator {
// 项集与向前看符号
class ProductionItemSet {
 public:
  // 项集内单个项，内容从左到右依次为
  // 产生式节点ID，产生式体ID，下一个移入的单词的位置
  using ProductionItem =
      std::tuple<ProductionNodeId, ProductionBodyId, NextWordToShiftIndex>;
  // 向前看符号容器
  using ForwardNodesContainer = std::unordered_set<ProductionNodeId>;
  // 用来哈希ProductionItem的类
  // 在SyntaxGenerator声明结束后用该类特化std::hash<ProductionItem>
  // 以允许ProductionItem可以作为std::unordered_map键值
  struct ProductionItemHasher {
    size_t operator()(const ProductionItem& production_item) const {
      auto& [production_node_id, production_body_id, point_index] =
          production_item;
      return production_node_id * production_body_id * point_index;
    }
  };
  // 存储项和向前看节点的容器
  using ProductionItemAndForwardNodesContainer =
      std::unordered_map<ProductionItem, ForwardNodesContainer,
                         ProductionItemHasher>;

  ProductionItemSet() {}
  ProductionItemSet(SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id)
      : syntax_analysis_table_entry_id_(syntax_analysis_table_entry_id) {}
  template <class ItemAndForwardNodes>
  ProductionItemSet(ItemAndForwardNodes&& item_and_forward_node_ids,
                    SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id)
      : production_item_set_closure_available_(false),
        syntax_analysis_table_entry_id_(syntax_analysis_table_entry_id),
        item_and_forward_node_ids_(
            std::forward<ItemAndForwardNodes>(item_and_forward_node_ids)) {}
  ProductionItemSet(const ProductionItemSet&) = delete;
  ProductionItemSet& operator=(const ProductionItemSet&) = delete;
  ProductionItemSet(ProductionItemSet&& production_item_set)
      : production_item_set_closure_available_(std::move(
            production_item_set.production_item_set_closure_available_)),
        production_item_set_id_(
            std::move(production_item_set.production_item_set_id_)),
        syntax_analysis_table_entry_id_(
            std::move(production_item_set.syntax_analysis_table_entry_id_)),
        item_and_forward_node_ids_(
            std::move(production_item_set.item_and_forward_node_ids_)) {}
  ProductionItemSet& operator=(ProductionItemSet&& production_item_set);

  // 返回给定Item插入后的iterator和是否成功插入bool标记
  // 如果Item已存在则仅添加向前看符号
  // bool在不存在给定item且插入成功时为true
  // 可以使用单个未包装ID
  // 如果添加了新项或向前看符号则设置闭包无效
  template <class ForwardNodeIdContainer>
  std::pair<ProductionItemAndForwardNodesContainer::iterator, bool>
  AddItemAndForwardNodeIds(const ProductionItem& item,
                           ForwardNodeIdContainer&& forward_node_ids);
  // 在AddItemAndForwardNodeIds基础上设置添加的项为核心项
  template <class ForwardNodeIdContainer>
  std::pair<ProductionItemAndForwardNodesContainer::iterator, bool>
  AddMainItemAndForwardNodeIds(const ProductionItem& item,
                               ForwardNodeIdContainer&& forward_node_ids) {
    auto result = AddItemAndForwardNodeIds(
        item, std::forward<ForwardNodeIdContainer>(forward_node_ids));
    SetMainItem(result.first);
    return result;
  }

  // 判断给定item是否在该项集内，在则返回true
  bool IsItemIn(const ProductionItem& item) const {
    return item_and_forward_node_ids_.find(item) !=
           item_and_forward_node_ids_.end();
  }
  // 判断该项集求的闭包是否有效
  bool IsClosureAvailable() const {
    return production_item_set_closure_available_;
  }
  // 设置production_item_set_id
  void SetProductionItemSetId(ProductionItemSetId production_item_set_id) {
    production_item_set_id_ = production_item_set_id;
  }
  // 获取production_item_set_id
  ProductionItemSetId GetProductionItemSetId() const {
    return production_item_set_id_;
  }

  // 设置该项集求的闭包有效，仅应由闭包函数调用
  void SetClosureAvailable() { production_item_set_closure_available_ = true; }
  // 获取全部核心项
  const std::list<ProductionItemAndForwardNodesContainer::const_iterator>&
  GetMainItems() const {
    return main_items_;
  }
  // 设置一项为核心项
  // 要求该核心项未添加过
  // 设置闭包无效
  void SetMainItem(
      ProductionItemAndForwardNodesContainer ::const_iterator& item_iter);
  // 获取全部项和对应的向前看节点
  const ProductionItemAndForwardNodesContainer& GetItemsAndForwardNodeIds()
      const {
    return item_and_forward_node_ids_;
  }
  // 获取项对应的语法分析表条目ID
  SyntaxAnalysisTableEntryId GetSyntaxAnalysisTableEntryId() const {
    return syntax_analysis_table_entry_id_;
  }

  // 向给定项中添加向前看符号，同时支持输入单个符号和符号容器
  // 返回是否添加
  // 要求项已经存在，否则请调用AddItemAndForwardNodeIds
  // 如果添加了新的向前看符号则设置闭包无效
  template <class ForwardNodeIdContainer>
  bool AddForwardNodes(const ProductionItem& item,
                       ForwardNodeIdContainer&& forward_node_id_container);
  size_t Size() const { return item_and_forward_node_ids_.size(); }

 private:
  // 向给定项中添加向前看符号
  // 返回是否添加
  // 如果添加了新的向前看符号则设置闭包无效
  template <class ForwardNodeIdContainer>
  bool AddForwardNodes(
      const ProductionItemAndForwardNodesContainer::iterator& iter,
      ForwardNodeIdContainer&& forward_node_id_container);
  // 设置闭包无效
  // 应由每个修改了项/项的向前看符号的函数调用
  void SetClosureNotAvailable() {
    production_item_set_closure_available_ = false;
  }

  // 存储指向核心项的迭代器
  std::list<ProductionItemAndForwardNodesContainer::const_iterator> main_items_;
  // 该项集求的闭包是否有效（求过闭包则为true）
  bool production_item_set_closure_available_ = false;
  // 项集ID
  ProductionItemSetId production_item_set_id_ =
      ProductionItemSetId::InvalidId();
  // 项对应的语法分析表条目ID
  SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id_ =
      SyntaxAnalysisTableEntryId::InvalidId();
  // 项和对应的向前看符号
  ProductionItemAndForwardNodesContainer item_and_forward_node_ids_;
};

template <class ForwardNodeIdContainer>
std::pair<ProductionItemSet::ProductionItemAndForwardNodesContainer::iterator,
          bool>
ProductionItemSet::AddItemAndForwardNodeIds(
    const ProductionItem& item, ForwardNodeIdContainer&& forward_node_ids) {
  // 已经求过闭包的项集不能添加新项
  assert(!IsClosureAvailable());
  auto iter = item_and_forward_node_ids_.find(item);
  if (iter == item_and_forward_node_ids_.end()) {
    auto result = item_and_forward_node_ids_.emplace(
        item, std::unordered_set<ProductionNodeId>(
                  std::forward<ForwardNodeIdContainer>(forward_node_ids)));
    if (result.second) {
      SetClosureNotAvailable();
    }
    return result;
  } else {
    bool new_forward_node_inserted = AddForwardNodes(
        iter, std::forward<ForwardNodeIdContainer>(forward_node_ids));
    if (new_forward_node_inserted) {
      SetClosureNotAvailable();
    }
    return std::make_pair(iter, false);
  }
}

template <class ForwardNodeIdContainer>
inline bool ProductionItemSet::AddForwardNodes(
    const ProductionItem& item,
    ForwardNodeIdContainer&& forward_node_id_container) {
  auto iter = item_and_forward_node_ids_.find(item);
  assert(iter != item_and_forward_node_ids_.end());
  return AddForwardNodes(
      iter, std::forward<ForwardNodeIdContainer>(forward_node_id_container));
}

template <class ForwardNodeIdContainer>
inline bool ProductionItemSet::AddForwardNodes(
    const ProductionItemAndForwardNodesContainer::iterator& iter,
    ForwardNodeIdContainer&& forward_node_id_container) {
  assert(iter != item_and_forward_node_ids_.end());
  bool result;
  if constexpr (std::is_same_v<std::decay_t<ForwardNodeIdContainer>,
                               ProductionNodeId>) {
    // 对单个向前看符号特化
    result = iter->second
                 .emplace(std::forward<ForwardNodeIdContainer>(
                     forward_node_id_container))
                 .second;
  } else {
    // 对容器特化
    result = false;
    for (auto& forward_node_id : forward_node_id_container) {
      result |= iter->second.insert(forward_node_id).second;
    }
  }
  // 如果添加了新的向前看节点则设置闭包无效
  if (result) {
    SetClosureNotAvailable();
  }
  return result;
}

}  // namespace frontend::generator::syntax_generator

#endif  // !GENERATOR_SYNTAXGENERATOR_PRODUCTION_ITEM_SET_H_