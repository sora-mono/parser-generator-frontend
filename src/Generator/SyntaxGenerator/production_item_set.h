/// @file production_item_set.h
/// @brief 存储项集与向前看符号的类
/// @details
/// 该类在构造语法分析配置中使用
#ifndef GENERATOR_SYNTAXGENERATOR_PRODUCTION_ITEM_SET_H_
#define GENERATOR_SYNTAXGENERATOR_PRODUCTION_ITEM_SET_H_

#include <unordered_set>

#include "Generator/export_types.h"
namespace frontend::generator::syntax_generator {
/// @class ProductionItemSet production_item_set.h
/// @brief 存储项集与向前看符号的类
class ProductionItemSet {
 public:
  /// @brief 项集内单个项，内容从左到右依次为
  /// 产生式节点ID，产生式体ID，下一个移入的单词的位置
  using ProductionItem =
      std::tuple<ProductionNodeId, ProductionBodyId, NextWordToShiftIndex>;
  /// @brief 向前看符号容器
  using ForwardNodesContainer = std::unordered_set<ProductionNodeId>;
  /// @brief 哈希ProductionItem的类
  /// 通过该类允许ProductionItem可以作为std::unordered_map键值
  struct ProductionItemHasher {
    size_t operator()(const ProductionItem& production_item) const {
      return std::get<ProductionNodeId>(production_item) *
             std::get<ProductionBodyId>(production_item) *
             std::get<NextWordToShiftIndex>(production_item);
    }
  };
  /// @brief 存储项和向前看节点的容器
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

  /// @brief 向项集中插入项和对应的向前看符号
  /// @param[in] item ：待插入的项
  /// @param[in] forward_node_ids ：待插入的项的向前看符号
  /// @return 返回指向给定Item的iterator和是否插入了新项
  /// @note
  /// 如果Item已存在则仅添加向前看符号且第二个返回参数为false
  /// 可以使用单个未包装ID
  /// @attention 插入时该项集必须没求过闭包，因为求过闭包的项集可能在之后的构建
  /// 过程中被再次引用
  /// forward_node_ids必须不为空，所有项都一定有向前看符号
  template <class ForwardNodeIdContainer>
  std::pair<ProductionItemAndForwardNodesContainer::iterator, bool>
  AddItemAndForwardNodeIds(const ProductionItem& item,
                           ForwardNodeIdContainer&& forward_node_ids);
  /// @brief 向项集中插入核心项和对应的向前看符号
  /// @param[in] item ：待插入的项
  /// @param[in] forward_node_ids ：待插入的项的向前看符号
  /// @return 返回指向给定Item的iterator和是否插入了新项
  /// @note
  /// 如果Item已存在则仅添加向前看符号且第二个返回参数为false
  /// 可以使用单个未包装ID
  /// 在求闭包时从核心项开始展开
  /// @attention 插入时该项集必须没求过闭包，因为求过闭包的项集可能在之后的构建
  /// 过程中被再次引用
  /// forward_node_ids必须不为空，所有项都一定有向前看符号
  template <class ForwardNodeIdContainer>
  std::pair<ProductionItemAndForwardNodesContainer::iterator, bool>
  AddMainItemAndForwardNodeIds(const ProductionItem& item,
                               ForwardNodeIdContainer&& forward_node_ids) {
    auto result = AddItemAndForwardNodeIds(
        item, std::forward<ForwardNodeIdContainer>(forward_node_ids));
    SetMainItem(result.first);
    return result;
  }

  /// @brief 判断给定项是否在该项集内
  /// @return 返回给定项是否在该项集内
  /// @retval true ：给定项在该项集内
  /// @retval false ：给定项不在该项集内
  bool IsItemIn(const ProductionItem& item) const {
    return item_and_forward_node_ids_.find(item) !=
           item_and_forward_node_ids_.end();
  }
  /// @brief 获取该项集求的闭包是否有效
  /// @return 返回该项集求的闭包是否有效
  /// @retval true ：该项集闭包有效，无需重求
  /// @retval false ：该项集闭包无效，需要求闭包才能使用
  bool IsClosureAvailable() const {
    return production_item_set_closure_available_;
  }
  /// @brief 设置项集ID
  /// @param[in] production_item_set_id ：项集ID
  void SetProductionItemSetId(ProductionItemSetId production_item_set_id) {
    production_item_set_id_ = production_item_set_id;
  }
  /// @brief 获取项集ID
  /// @return 返回项集ID
  ProductionItemSetId GetProductionItemSetId() const {
    return production_item_set_id_;
  }

  /// @brief 设置该项集求的闭包有效
  /// @attention 该函数仅应由闭包函数调用
  void SetClosureAvailable() { production_item_set_closure_available_ = true; }
  /// @brief 获取全部指向核心项的迭代器
  /// @return 返回全部指向核心项的迭代器的const引用
  const std::list<ProductionItemAndForwardNodesContainer::const_iterator>&
  GetMainItemIters() const {
    return main_items_;
  }
  /// @brief 获取全部项和对应的向前看节点
  /// @return 返回全部项和对应的向前看节点的const引用
  const ProductionItemAndForwardNodesContainer& GetItemsAndForwardNodeIds()
      const {
    return item_and_forward_node_ids_;
  }
  /// @brief 获取该项集对应的语法分析表条目ID
  /// @return 该项集对应的语法分析表条目ID
  SyntaxAnalysisTableEntryId GetSyntaxAnalysisTableEntryId() const {
    return syntax_analysis_table_entry_id_;
  }
  /// @brief 设置该项集对应的语法分析表条目ID
  /// @param[in] syntax_analysis_table_entry_id ：该项集对应的语法分析表条目ID
  void SetSyntaxAnalysisTableEntryId(
      SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id) {
    syntax_analysis_table_entry_id_ = syntax_analysis_table_entry_id;
  }
  /// @brief 向给定项中添加向前看符号
  /// @param[in] item ：待添加向前看符号的项
  /// @param[in] forward_node_id_container ：待添加的向前看符号
  /// @return 返回是否添加了向前看符号
  /// @retval true ：添加了向前看符号
  /// @retval false ：forward_node_id_container中全部向前看符号均已存在
  /// @note
  /// 1.forward_node_id_container支持输入单个向前看符号和存储向前看符号的容器
  /// 2.item必须为核心项，否则请调用AddItemAndForwardNodeIds及类似函数
  /// 3.如果添加了新的向前看符号则设置闭包无效
  /// @attention item必须已经添加到该项集中
  template <class ForwardNodeIdContainer>
  bool AddForwardNodes(const ProductionItem& item,
                       ForwardNodeIdContainer&& forward_node_id_container);
  /// @brief 清空所有非核心项
  void ClearNotMainItem();
  /// @brief 判断给定项是否为该项集的核心项
  /// @param[in] item ：待判断的项
  /// @return 返回给定项是否为该项集的核心项的判断结果
  /// @retval true 给定项是该项集的核心项
  /// @retval false 给定项不是该项集的核心项或不存在于该项集
  /// @warning 时间复杂度O(n)，n为该项集核心项数
  bool IsMainItem(const ProductionItem& item);
  /// @brief 获取项集中项的个数
  /// @return 返回项的个数
  size_t Size() const { return item_and_forward_node_ids_.size(); }
  /// @brief 获取核心项集个数
  size_t MainItemSize() const { return GetMainItemIters().size(); }

 private:
  /// @brief 设置一项为核心项
  /// @param[in] item_iter ：指向要被设置为核心项的项的迭代器
  /// @note
  /// 要求指定的项未被设置成核心项过
  /// 自动设置闭包无效
  void SetMainItem(
      ProductionItemAndForwardNodesContainer ::const_iterator& item_iter);
  /// @brief 向给定项中添加向前看符号
  /// @param[in] iter ：待添加向前看符号的项
  /// @param[in] forward_node_id_container ：待添加的向前看符号
  /// @return 返回是否添加了新的向前看符号
  /// @retval true ：添加了向前看符号
  /// @retval false ：forward_node_id_container中全部向前看符号均已存在
  /// @note
  /// 1.向已有的项集中添加向前看符号最终都应走该接口
  /// 2.forward_node_id_container支持输入单个向前看符号和存储向前看符号的容器
  /// 3.如果添加了新的向前看符号则设置闭包无效
  template <class ForwardNodeIdContainer>
  bool AddForwardNodes(
      const ProductionItemAndForwardNodesContainer::iterator& iter,
      ForwardNodeIdContainer&& forward_node_id_container);
  /// @brief 设置闭包无效
  /// @note 每个修改了项/项的向前看符号的函数都应设置闭包无效
  void SetClosureNotAvailable() {
    production_item_set_closure_available_ = false;
  }
  /// @brief 获取全部指向核心项的迭代器
  /// @return 返回全部指向核心项的迭代器的引用
  std::list<ProductionItemAndForwardNodesContainer::const_iterator>&
  GetMainItemIters() {
    return main_items_;
  }

  /// @brief 存储指向核心项的迭代器
  std::list<ProductionItemAndForwardNodesContainer::const_iterator> main_items_;
  /// @brief 该项集求的闭包是否有效（求过闭包且未更改过则为true）
  bool production_item_set_closure_available_ = false;
  /// @brief 项集ID
  ProductionItemSetId production_item_set_id_ =
      ProductionItemSetId::InvalidId();
  /// @brief 项集对应的语法分析表条目ID
  SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id_ =
      SyntaxAnalysisTableEntryId::InvalidId();
  /// @brief 项和对应的向前看符号
  ProductionItemAndForwardNodesContainer item_and_forward_node_ids_;
};

template <class ForwardNodeIdContainer>
std::pair<ProductionItemSet::ProductionItemAndForwardNodesContainer::iterator,
          bool>
ProductionItemSet::AddItemAndForwardNodeIds(
    const ProductionItem& item, ForwardNodeIdContainer&& forward_node_ids) {
  // 已经求过闭包的项集不能添加新项
  assert(!IsClosureAvailable());
  // 任何项都必须携带向前看符号
  assert(forward_node_ids.size() != 0);
  auto iter = item_and_forward_node_ids_.find(item);
  if (iter == item_and_forward_node_ids_.end()) {
    return item_and_forward_node_ids_.emplace(
        item, std::unordered_set<ProductionNodeId>(
                  std::forward<ForwardNodeIdContainer>(forward_node_ids)));
  } else {
    AddForwardNodes(iter,
                    std::forward<ForwardNodeIdContainer>(forward_node_ids));
    return std::make_pair(iter, false);
  }
}

template <class ForwardNodeIdContainer>
inline bool ProductionItemSet::AddForwardNodes(
    const ProductionItem& item,
    ForwardNodeIdContainer&& forward_node_id_container) {
#ifdef _DEBUG
  // 检查给定项是核心项
  bool is_main_item = false;
  for (const auto& main_item_iter : GetMainItemIters()) {
    if (main_item_iter->first == item) [[unlikely]] {
      is_main_item = true;
      break;
    }
  }
  assert(is_main_item);
#endif  // _DEBUG
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