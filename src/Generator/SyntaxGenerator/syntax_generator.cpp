#include "syntax_generator.h"

#include <queue>

namespace frontend::generator::syntax_generator {
ProductionNodeId SyntaxGenerator::AddTerminalProduction(
    std::string&& node_symbol, std::string&& body_symbol,
    WordPriority node_priority, bool is_key_word) {
  auto [node_symbol_id, node_symbol_inserted] = AddNodeSymbol(node_symbol);
  auto [body_symbol_id, body_symbol_inserted] = AddBodySymbol(body_symbol);
  ProductionNodeId production_node_id;
  if (body_symbol_inserted) {
    ProductionNodeId old_node_symbol_id =
        GetProductionNodeIdFromNodeSymbolId(node_symbol_id);
    if (old_node_symbol_id.IsValid()) {
      // 该终结节点名已被使用
      OutPutError(std::format("终结节点名已定义：{:}", node_symbol));
      return ProductionNodeId::InvalidId();
    }
    // 需要添加一个新的终结节点
    production_node_id = SubAddTerminalNode(node_symbol_id, body_symbol_id);
    frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
        word_attached_data;
    word_attached_data.production_node_id = production_node_id;
    word_attached_data.node_type = ProductionNodeType::kTerminalNode;
    // 向DFA生成器注册关键词
    bool result;
    if (!is_key_word) [[likely]] {
      assert(node_priority == 0 || node_priority == 1);
      result = dfa_generator_.AddRegexpression(
          body_symbol, std::move(word_attached_data), node_priority);
    } else {
      assert(node_priority == 2);
      result = dfa_generator_.AddWord(
          body_symbol, std::move(word_attached_data), node_priority);
    }
    if (!result) [[unlikely]] {
      OutPutError(
          std::format("内部错误：无法添加终结节点正则表达式 {:}", body_symbol));
      exit(-1);
    }
  } else {
    // 该终结节点的内容已存在，不应重复添加
    OutPutError(std::format("多次声明同一正则：{:}", body_symbol));
    return ProductionNodeId::InvalidId();
  }
  // 输出构建过程
  OutPutInfo(
      std::format("成功添加终结产生式： {:} -> {:}", node_symbol, body_symbol));
  OutPutInfo(std::format(
      "产生式名ID：{:} 产生式体ID：{:} 终结产生式ID：{:} 产生式优先级：{:}",
      node_symbol_id.GetRawValue(), body_symbol_id.GetRawValue(),
      production_node_id.GetRawValue(), node_priority.GetRawValue()));
  // 判断新添加的终结节点是否为某个未定义产生式
  CheckNonTerminalNodeCanContinue(node_symbol);
  return production_node_id;
}

inline ProductionNodeId SyntaxGenerator::SubAddTerminalNode(
    SymbolId node_symbol_id, SymbolId body_symbol_id) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<TerminalProductionNode>(node_symbol_id,
                                                           body_symbol_id);
  manager_nodes_.GetObject(node_id).SetNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  SetBodySymbolIdToProductionNodeIdMapping(body_symbol_id, node_id);
  return node_id;
}

ProductionNodeId SyntaxGenerator::AddBinaryOperator(
    std::string&& operator_symbol,
    OperatorAssociatityType binary_operator_associatity_type,
    OperatorPriority binary_operator_priority) {
  // 运算符产生式名与运算符相同
  auto [operator_node_symbol_id, operator_node_symbol_inserted] =
      AddNodeSymbol(operator_symbol);
  if (!operator_node_symbol_inserted) [[unlikely]] {
    OutPutError(std::format("运算符：{:} 已定义", operator_symbol));
    return ProductionNodeId::InvalidId();
  }
  auto [operator_body_symbol_id, operator_body_symbol_inserted] =
      AddBodySymbol(operator_symbol);
  if (!operator_body_symbol_inserted) {
    OutPutError(std::format("运算符：{:} 已在DFA中添加，无法定义为运算符",
                            operator_symbol));
    return ProductionNodeId::InvalidId();
  }
  ProductionNodeId operator_node_id = SubAddBinaryOperatorNode(
      operator_node_symbol_id, binary_operator_associatity_type,
      binary_operator_priority);
  assert(operator_node_id.IsValid());
  frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
      word_attached_data;
  word_attached_data.production_node_id = operator_node_id;
  word_attached_data.node_type = ProductionNodeType::kOperatorNode;
  word_attached_data.binary_operator_associate_type =
      binary_operator_associatity_type;
  word_attached_data.binary_operator_priority = binary_operator_priority;
  // 向DFA生成器注册关键词
  bool result = dfa_generator_.AddWord(
      operator_symbol, std::move(word_attached_data),
      frontend::generator::dfa_generator::DfaGenerator::WordPriority(1));
  if (!result) {
    OutPutError(std::format("内部错误：无法添加双目运算符正则表达式 {:}",
                            operator_symbol));
    exit(-1);
  }
  // 输出构建过程
  OutPutInfo(std::format("成功添加双目运算符 {:}",
                         GetNodeSymbolStringFromId(operator_node_symbol_id)));
  OutPutInfo(std::format(
      "运算符名ID：{:} 运算符体ID：{:} 运算符ID：{:} 双目运算符优先级：{:}",
      operator_node_symbol_id.GetRawValue(),
      operator_body_symbol_id.GetRawValue(), operator_node_id.GetRawValue(),
      binary_operator_priority.GetRawValue()));
  return operator_node_id;
}

ProductionNodeId SyntaxGenerator::AddLeftUnaryOperator(
    std::string&& operator_symbol,
    OperatorAssociatityType unary_operator_associatity_type,
    OperatorPriority unary_operator_priority) {
  // 运算符产生式名与运算符相同
  auto [operator_node_symbol_id, operator_node_symbol_inserted] =
      AddNodeSymbol(operator_symbol);
  if (!operator_node_symbol_inserted) [[unlikely]] {
    OutPutError(std::format("运算符：{:} 已定义\n", operator_symbol));
    return ProductionNodeId::InvalidId();
  }
  auto [operator_body_symbol_id, operator_body_symbol_inserted] =
      AddBodySymbol(operator_symbol);
  if (!operator_body_symbol_inserted) {
    OutPutError(std::format("运算符：{:} 已在DFA中添加，无法定义为运算符\n",
                            operator_symbol));
    return ProductionNodeId::InvalidId();
  }
  ProductionNodeId operator_node_id = SubAddUnaryOperatorNode(
      operator_node_symbol_id, unary_operator_associatity_type,
      unary_operator_priority);
  assert(operator_node_id.IsValid());
  frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
      word_attached_data;
  word_attached_data.production_node_id = operator_node_id;
  word_attached_data.node_type = ProductionNodeType::kOperatorNode;
  word_attached_data.unary_operator_associate_type =
      unary_operator_associatity_type;
  word_attached_data.unary_operator_priority = unary_operator_priority;
  // 向DFA生成器注册关键词
  bool result = dfa_generator_.AddWord(
      operator_symbol, std::move(word_attached_data),
      frontend::generator::dfa_generator::DfaGenerator::WordPriority(1));
  if (!result) [[unlikely]] {
    OutPutError(std::format("内部错误：无法添加左侧单目运算符正则表达式 {:}",
                            operator_symbol));
    exit(-1);
  }
  // 输出构建过程
  OutPutInfo(std::format("成功添加左侧单目运算符 {:}",
                         GetNodeSymbolStringFromId(operator_node_symbol_id)));
  OutPutInfo(std::format(
      "运算符名ID：{:} 运算符体ID：{:} 运算符ID：{:} 左侧单目运算符优先级：{:}",
      operator_node_symbol_id.GetRawValue(),
      operator_body_symbol_id.GetRawValue(), operator_node_id.GetRawValue(),
      unary_operator_priority.GetRawValue()));
  return operator_node_id;
}

ProductionNodeId SyntaxGenerator::AddBinaryLeftUnaryOperator(
    std::string&& operator_symbol,
    OperatorAssociatityType binary_operator_associatity_type,
    OperatorPriority binary_operator_priority,
    OperatorAssociatityType unary_operator_associatity_type,
    OperatorPriority unary_operator_priority) {
  // 运算符产生式名与运算符相同
  auto [operator_node_symbol_id, operator_node_symbol_inserted] =
      AddNodeSymbol(operator_symbol);
  if (!operator_node_symbol_inserted) [[unlikely]] {
    OutPutError(std::format("运算符：{:} 已定义\n", operator_symbol));
    return ProductionNodeId::InvalidId();
  }
  auto [operator_body_symbol_id, operator_body_symbol_inserted] =
      AddBodySymbol(operator_symbol);
  if (!operator_body_symbol_inserted) {
    OutPutError(std::format("运算符：{:} 已在DFA中添加，无法定义为运算符\n",
                            operator_symbol));
    return ProductionNodeId::InvalidId();
  }
  ProductionNodeId operator_node_id = SubAddBinaryUnaryOperatorNode(
      operator_node_symbol_id, binary_operator_associatity_type,
      binary_operator_priority, unary_operator_associatity_type,
      unary_operator_priority);
  assert(operator_node_id.IsValid());
  frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
      word_attached_data;
  word_attached_data.production_node_id = operator_node_id;
  word_attached_data.node_type = ProductionNodeType::kOperatorNode;
  word_attached_data.binary_operator_associate_type =
      binary_operator_associatity_type;
  word_attached_data.binary_operator_priority = binary_operator_priority;
  word_attached_data.unary_operator_associate_type =
      unary_operator_associatity_type;
  word_attached_data.unary_operator_priority = unary_operator_priority;
  // 向DFA生成器注册关键词
  bool result = dfa_generator_.AddWord(
      operator_symbol, std::move(word_attached_data),
      frontend::generator::dfa_generator::DfaGenerator::WordPriority(1));
  if (!result) [[unlikely]] {
    OutPutError(
        std::format("内部错误：无法添加双目和左侧单目运算符正则表达式 {:}",
                    operator_symbol));
    exit(-1);
  }
  // 输出构建过程
  OutPutInfo(std::format("成功添加双目和左侧单目运算符 {:}",
                         GetNodeSymbolStringFromId(operator_node_symbol_id)));
  OutPutInfo(std::format(
      "运算符名ID：{:} 运算符体ID：{:} 运算符ID：{:} 双目运算符优先级：{:} "
      "左侧单目运算符优先级：{:}",
      operator_node_symbol_id.GetRawValue(),
      operator_body_symbol_id.GetRawValue(), operator_node_id.GetRawValue(),
      binary_operator_priority.GetRawValue(),
      unary_operator_priority.GetRawValue()));
  return operator_node_id;
}

ProductionNodeId SyntaxGenerator::SubAddBinaryOperatorNode(
    SymbolId node_symbol_id, OperatorAssociatityType binary_associatity_type,
    OperatorPriority binary_priority) {
  ProductionNodeId node_id = manager_nodes_.EmplaceObject<BinaryOperatorNode>(
      node_symbol_id, binary_associatity_type, binary_priority);
  manager_nodes_.GetObject(node_id).SetNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  SetBodySymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  return node_id;
}

ProductionNodeId SyntaxGenerator::SubAddUnaryOperatorNode(
    SymbolId node_symbol_id, OperatorAssociatityType unary_associatity_type,
    OperatorPriority unary_priority) {
  ProductionNodeId node_id = manager_nodes_.EmplaceObject<UnaryOperatorNode>(
      node_symbol_id, unary_associatity_type, unary_priority);
  manager_nodes_.GetObject(node_id).SetNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  SetBodySymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  return node_id;
}

ProductionNodeId SyntaxGenerator::SubAddBinaryUnaryOperatorNode(
    SymbolId node_symbol_id, OperatorAssociatityType binary_associatity_type,
    OperatorPriority binary_priority,
    OperatorAssociatityType unary_associatity_type,
    OperatorPriority unary_priority) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<BinaryUnaryOperatorNode>(
          node_symbol_id, binary_associatity_type, binary_priority,
          unary_associatity_type, unary_priority);
  manager_nodes_.GetObject(node_id).SetNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  SetBodySymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  return node_id;
}

ProductionNodeId SyntaxGenerator::AddNonTerminalProduction(
    std::string&& node_symbol, std::vector<std::string>&& subnode_symbols,
    ProcessFunctionClassId class_id) {
  assert(!node_symbol.empty() && !subnode_symbols.empty() &&
         class_id.IsValid());
  std::vector<ProductionNodeId> node_ids;
  // 非终结节点与节点名一一对应
  ProductionNodeId production_node_id =
      GetProductionNodeIdFromNodeSymbol(node_symbol);
  if (!production_node_id.IsValid()) {
    // 该非终结节点名未注册
    SymbolId symbol_id = AddNodeSymbol(node_symbol).first;
    assert(symbol_id.IsValid());
    production_node_id = SubAddNonTerminalNode(symbol_id);
    // 检查添加的节点是否被前向依赖
    CheckNonTerminalNodeCanContinue(node_symbol);
  }
  NonTerminalProductionNode& production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  for (auto& subnode_symbol : subnode_symbols) {
    // 将产生式体的所有产生式名转换为节点ID
    ProductionNodeId subproduction_node_id =
        GetProductionNodeIdFromNodeSymbol(subnode_symbol);
    // 产生式名
    if (!subproduction_node_id.IsValid()) {
      // 产生式节点未定义
      // 添加待处理记录
      AddUnableContinueNonTerminalNode(subnode_symbol, std::move(node_symbol),
                                       std::move(subnode_symbols), class_id);
      return ProductionNodeId::InvalidId();
    }
    assert(subproduction_node_id.IsValid());
    node_ids.push_back(subproduction_node_id);
  }
  ProductionBodyId body_id =
      production_node.AddBody(std::move(node_ids), class_id);
  OutPutInfo(std::format("成功添加非终结节点 ") +
             FormatSingleProductionBody(production_node_id, body_id));
  OutPutInfo(std::format("非终结节点ID：{:} 当前产生式体ID：{:}",
                         production_node_id.GetRawValue(),
                         body_id.GetRawValue()));
  return production_node_id;
}

void SyntaxGenerator::SetNonTerminalNodeCouldEmptyReduct(
    const std::string& nonterminal_node_symbol) {
  ProductionNodeId nonterminal_node_id =
      GetProductionNodeIdFromNodeSymbol(nonterminal_node_symbol);
  assert(nonterminal_node_id.IsValid());
  NonTerminalProductionNode& nonterminal_production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(nonterminal_node_id));
  if (nonterminal_production_node.GetType() !=
      ProductionNodeType::kNonTerminalNode) [[unlikely]] {
    OutPutError(std::format("无法设置非终结产生式以外的产生式：{:} 允许空规约",
                            nonterminal_node_symbol));
    exit(-1);
  }
  nonterminal_production_node.SetProductionCouldBeEmptyRedut();
}

inline ProductionNodeId SyntaxGenerator::SubAddNonTerminalNode(
    SymbolId node_symbol_id) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<NonTerminalProductionNode>(node_symbol_id);
  manager_nodes_.GetObject(node_id).SetNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  return node_id;
}

inline ProductionNodeId SyntaxGenerator::AddEndNode() {
  ProductionNodeId node_id = manager_nodes_.EmplaceObject<EndNode>();
  GetProductionNode(node_id).SetSymbolId(AddNodeSymbol("$").first);
  GetProductionNode(node_id).SetNodeId(node_id);
  return node_id;
}

void SyntaxGenerator::SetRootProduction(
    const std::string& production_node_symbol) {
  if (GetRootProductionNodeId().IsValid()) [[unlikely]] {
    OutPutError(std::format("仅且必须声明一个根产生式"));
    exit(-1);
  }
  ProductionNodeId production_node_id =
      GetProductionNodeIdFromNodeSymbol(production_node_symbol);
  if (!production_node_id.IsValid()) [[unlikely]] {
    OutPutError(std::format("不存在非终结产生式 {:}", production_node_symbol));
    exit(-1);
  }
  SetRootProductionNodeId(production_node_id);
}

inline BaseProductionNode& SyntaxGenerator::GetProductionNode(
    ProductionNodeId production_node_id) {
  return const_cast<BaseProductionNode&>(
      static_cast<const SyntaxGenerator&>(*this).GetProductionNode(
          production_node_id));
}

inline const BaseProductionNode& SyntaxGenerator::GetProductionNode(
    ProductionNodeId production_node_id) const {
  return manager_nodes_[production_node_id];
}

inline BaseProductionNode& SyntaxGenerator::GetProductionNodeFromNodeSymbolId(
    SymbolId symbol_id) {
  ProductionNodeId production_node_id =
      GetProductionNodeIdFromNodeSymbolId(symbol_id);
  assert(symbol_id.IsValid());
  return GetProductionNode(production_node_id);
}

inline BaseProductionNode& SyntaxGenerator::GetProductionNodeFromBodySymbolId(
    SymbolId symbol_id) {
  ProductionNodeId production_node_id =
      GetProductionNodeIdFromBodySymbolId(symbol_id);
  assert(symbol_id.IsValid());
  return GetProductionNode(production_node_id);
}

inline const std::vector<ProductionNodeId>& SyntaxGenerator::GetProductionBody(
    ProductionNodeId production_node_id, ProductionBodyId production_body_id) {
  NonTerminalProductionNode& nonterminal_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  // 只有非终结节点才有多个产生式体，对其它节点调用该函数无意义
  assert(nonterminal_node.GetType() == ProductionNodeType::kNonTerminalNode);
  return nonterminal_node.GetBody(production_body_id).production_body;
}

inline ProductionItemSetId
SyntaxGenerator::GetProductionItemSetIdFromSyntaxAnalysisTableEntryId(
    SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id) {
  auto iter = syntax_analysis_table_entry_id_to_production_item_set_id_.find(
      syntax_analysis_table_entry_id);
  assert(iter !=
         syntax_analysis_table_entry_id_to_production_item_set_id_.end());
  return iter->second;
}

inline ProductionItemSetId SyntaxGenerator::EmplaceProductionItemSet() {
  SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id =
      AddSyntaxAnalysisTableEntry();
  ProductionItemSetId production_item_set_id =
      production_item_sets_.EmplaceObject(syntax_analysis_table_entry_id);
  production_item_sets_[production_item_set_id].SetProductionItemSetId(
      production_item_set_id);
  SetSyntaxAnalysisTableEntryIdToProductionItemSetIdMapping(
      syntax_analysis_table_entry_id, production_item_set_id);
  return production_item_set_id;
}

inline void SyntaxGenerator::AddProductionItemBelongToProductionItemSetId(
    const ProductionItem& production_item,
    ProductionItemSetId production_item_set_id) {
  auto& [production_node_id, production_body_id, next_word_to_shift_index] =
      production_item;
  NonTerminalProductionNode& production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  assert(production_node.GetType() == ProductionNodeType::kNonTerminalNode);
  production_node.AddProductionItemBelongToProductionItemSetId(
      production_body_id, next_word_to_shift_index, production_item_set_id);
}

inline const std::list<ProductionItemSetId>&
SyntaxGenerator::GetProductionItemSetIdFromProductionItem(
    ProductionNodeId production_node_id, ProductionBodyId body_id,
    NextWordToShiftIndex next_word_to_shift_index) {
  NonTerminalProductionNode& production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  assert(production_node.GetType() == ProductionNodeType::kNonTerminalNode);
  return production_node.GetProductionItemSetIdFromProductionItem(
      body_id, next_word_to_shift_index);
}

void SyntaxGenerator::GetNonTerminalNodeFirstNodeIds(
    ProductionNodeId production_node_id, ForwardNodesContainer* result,
    std::unordered_set<ProductionNodeId>&& processed_nodes) {
  bool inserted = processed_nodes.insert(production_node_id).second;
  assert(inserted);
  assert(production_node_id.IsValid());
  assert(result);
  NonTerminalProductionNode& production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  assert(production_node.GetType() == ProductionNodeType::kNonTerminalNode);
  for (auto& body : production_node.GetAllBody()) {
    NextWordToShiftIndex node_id_index(0);
    // 正在处理的产生式节点的ID
    ProductionNodeId node_id;
    while (node_id_index < body.production_body.size()) {
      node_id = body.production_body[node_id_index];
      if (GetProductionNode(node_id).GetType() !=
          ProductionNodeType::kNonTerminalNode) {
        result->insert(node_id);
        break;
      }
      assert(node_id.IsValid());
      // 检查该节点是否已经处理过
      if (processed_nodes.find(node_id) == processed_nodes.end()) {
        // 如果该节点未处理则获取该节点的first节点ID并添加到主first节点ID集合中
        GetNonTerminalNodeFirstNodeIds(node_id, result,
                                       std::move(processed_nodes));
      }
      if (static_cast<NonTerminalProductionNode&>(GetProductionNode(node_id))
              .CouldBeEmptyReduct()) {
        // 该非终结节点可以空规约，需要考虑后面的节点
        ++node_id_index;
      } else {
        // 已处理所有节点，跳出循环
        break;
      }
    }
  }
}

SyntaxGenerator::ForwardNodesContainer SyntaxGenerator::First(
    ProductionNodeId production_node_id, ProductionBodyId production_body_id,
    NextWordToShiftIndex next_word_to_shift_index,
    const ForwardNodesContainer& next_node_ids) {
  ProductionNodeId node_id = GetProductionNodeIdInBody(
      production_node_id, production_body_id, next_word_to_shift_index);
  if (node_id.IsValid()) {
    ForwardNodesContainer forward_nodes;
    switch (GetProductionNode(node_id).GetType()) {
      case ProductionNodeType::kTerminalNode:
      case ProductionNodeType::kOperatorNode:
        forward_nodes.insert(node_id);
        break;
      case ProductionNodeType::kNonTerminalNode:
        // 合并当前·右侧非终结节点可能的所有单词ID
        GetNonTerminalNodeFirstNodeIds(node_id, &forward_nodes);
        if (static_cast<NonTerminalProductionNode&>(GetProductionNode(node_id))
                .CouldBeEmptyReduct()) {
          // 当前·右侧的非终结节点可以空规约，需要考虑它下一个节点的情况
          forward_nodes.merge(
              First(production_node_id, production_body_id,
                    NextWordToShiftIndex(next_word_to_shift_index + 1),
                    next_node_ids));
        }
        break;
      default:
        assert(false);
        break;
    }
    return forward_nodes;
  } else {
    return next_node_ids;
  }
}

bool SyntaxGenerator::ProductionItemSetClosure(
    ProductionItemSetId production_item_set_id) {
  OutPutInfo(std::format("对ID = {:}的项集执行闭包操作",
                         production_item_set_id.GetRawValue()));
  OutPutInfo(std::format("该项集具有的项和向前看符号如下：\n") +
             FormatProductionItems(production_item_set_id));
  ProductionItemSet& production_item_set =
      GetProductionItemSet(production_item_set_id);
  if (production_item_set.IsClosureAvailable()) {
    // 闭包有效，无需重求
    return false;
  }
  SyntaxAnalysisTableEntry& syntax_analysis_table_entry =
      GetSyntaxAnalysisTableEntry(
          production_item_set.GetSyntaxAnalysisTableEntryId());
  // 清空语法分析表
  syntax_analysis_table_entry.Clear();
  // 清空非核心项
  production_item_set.ClearNotMainItem();
  // 存储当前待展开的项
  // 存储指向待展开项的迭代器
  std::queue<ProductionItemAndForwardNodesContainer::const_iterator>
      items_waiting_process;
  const auto& production_items_and_forward_nodes =
      GetProductionItemsAndForwardNodes(production_item_set_id);
  for (auto& iter : GetProductionItemSetMainItems(production_item_set_id)) {
    // 将项集中所有核心项压入待处理队列
    items_waiting_process.push(iter);
  }
  while (!items_waiting_process.empty()) {
    auto item_now = items_waiting_process.front();
    items_waiting_process.pop();
    const auto [production_node_id, production_body_id,
                next_word_to_shift_index] = item_now->first;
    NonTerminalProductionNode& production_node_now =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(production_node_id));
    assert(production_node_now.GetType() ==
           ProductionNodeType::kNonTerminalNode);
    ProductionNodeId next_production_node_id = GetProductionNodeIdInBody(
        production_node_id, production_body_id, next_word_to_shift_index);
    if (!next_production_node_id.IsValid()) {
      // 无后继节点，设置规约条目

      // 组装规约使用的数据
      SyntaxAnalysisTableEntry::ReductAttachedData reduct_attached_data(
          production_node_id,
          GetProcessFunctionClass(production_node_id, production_body_id),
          GetProductionBody(production_node_id, production_body_id));
      OutPutInfo(std::format("项：") + FormatProductionItem(item_now->first));
      OutPutInfo(std::format("在向前看符号：") +
                 FormatLookForwardSymbols(item_now->second) + " 下执行规约");
      const auto& production_body =
          production_node_now.GetBody(production_body_id).production_body;
      for (auto node_id : item_now->second) {
        // 对每个向前看符号设置规约操作
        syntax_analysis_table_entry.SetTerminalNodeActionAndAttachedData(
            node_id, reduct_attached_data);
      }
      continue;
    }
    NonTerminalProductionNode& next_production_node =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(next_production_node_id));
    if (next_production_node.GetType() !=
        ProductionNodeType::kNonTerminalNode) {
      OutPutInfo(std::format("项：") + FormatProductionItem(item_now->first) +
                 std::format(" 由于下一个移入的符号为终结节点而终止展开"));
      continue;
    }
    // 展开非终结节点，并为其创建向前看符号集
    ForwardNodesContainer&& forward_node_ids = First(
        production_node_id, production_body_id,
        NextWordToShiftIndex(next_word_to_shift_index + 1), item_now->second);
    OutPutInfo(std::format("项：") + FormatProductionItem(item_now->first) +
               std::format(" 正在展开非终结节点 {:}",
                           GetNextNodeToShiftSymbolString(
                               production_node_id, production_body_id,
                               next_word_to_shift_index)));
    OutPutInfo(std::format("该非终结节点具有的全部向前看符号：") +
               FormatLookForwardSymbols(forward_node_ids));
    for (auto body_id : next_production_node.GetAllBodyIds()) {
      // 将该非终结节点中的每个产生式体加入到production_item_set中，点在最左侧
      auto [iter, inserted] = AddItemAndForwardNodeIdsToProductionItem(
          production_item_set_id,
          ProductionItem(next_production_node_id, body_id,
                         NextWordToShiftIndex(0)),
          forward_node_ids);
      if (inserted) {
        // 如果插入新的项则添加到队列中等待处理，并记录该项属于的新项集ID
        items_waiting_process.push(iter);
        OutPutInfo(std::format("向项集中插入新项：") +
                   FormatProductionItem(iter->first) +
                   std::format(" 向前看符号：") +
                   FormatLookForwardSymbols(forward_node_ids));
      }
    }
    if (next_production_node.CouldBeEmptyReduct()) {
      // ·右侧的非终结节点可以空规约
      // 向项集中添加空规约后得到的项，向前看符号复制原来的项的向前看符号集
      auto [iter, inserted] = AddItemAndForwardNodeIdsToProductionItem(
          production_item_set_id,
          ProductionItem(production_node_id, production_body_id,
                         NextWordToShiftIndex(next_word_to_shift_index + 1)),
          item_now->second);
      if (inserted) {
        // 如果插入新的项则添加到队列中等待处理
        items_waiting_process.push(iter);
        OutPutInfo(
            std::format("由于非终结产生式 {:} 可以空规约，向项集中插入新项：",
                        GetNextNodeToShiftSymbolString(
                            production_node_id, production_body_id,
                            next_word_to_shift_index)) +
            FormatProductionItemAndLookForwardSymbols(iter->first,
                                                      item_now->second));
      }
    }
    OutPutInfo(std::format("项：") +
               FormatProductionItemAndLookForwardSymbols(item_now->first,
                                                         item_now->second) +
               std::format(" 已展开"));
  }
  SetProductionItemSetClosureAvailable(production_item_set_id);
  OutPutInfo(std::format("完成对ProductionItemID = {:}的项集的闭包操作",
                         production_item_set_id.GetRawValue()));
  OutPutInfo(std::format("该项集具有的项和向前看符号如下：\n") +
             FormatProductionItems(production_item_set_id));
  return true;
}

ProductionItemSetId SyntaxGenerator::GetProductionItemSetIdFromProductionItems(
    const std::list<SyntaxGenerator::ProductionItemAndForwardNodesContainer::
                        const_iterator>& items) {
#ifdef _DEBUG
  // 检查参数中不存在重复项
  // 所有的参数都是移入相同的节点前的项，移入相同节点后的项相同的充分必要条件为
  // 移入相同节点前的项相同
  // 所以只需检查移入相同节点前的项是否相同
  for (auto iter = items.cbegin(); iter != items.cend(); ++iter) {
    auto compare_iter = iter;
    while (++compare_iter != items.cend()) {
      assert((*iter)->first != (*compare_iter)->first);
    }
  }
#endif  // _DEBUG

  // 存储项集ID包含几个给定项的个数，只有包含全部给定项才可能成为这些项属于的项集
  std::unordered_map<ProductionItemSetId, size_t>
      production_item_set_includes_item_size;
  // 统计项集包含给定项的个数
  for (const auto& item : items) {
    // 拆分移入节点前的项数据
    auto [production_node_id, production_body_id, next_word_to_shift_index] =
        item->first;
    // 获取移入节点后的项数据
    ++next_word_to_shift_index;
    const auto& production_item_set_ids_item_belong_to =
        GetProductionItemSetIdFromProductionItem(
            production_node_id, production_body_id, next_word_to_shift_index);
    // 优化手段，任意给定项不属于任何项集则不存在包含所有给定项的项集
    if (production_item_set_ids_item_belong_to.empty()) [[likely]] {
      return ProductionItemSetId::InvalidId();
    }
    for (auto production_item_set_id : production_item_set_ids_item_belong_to) {
      auto [iter, inserted] = production_item_set_includes_item_size.emplace(
          production_item_set_id, 1);
      if (!inserted) {
        // 该项集ID已经记录，增加该项集ID出现的次数
        ++iter->second;
      }
    }
  }
  // 检查所有出现过的项集ID，找出包含所有给定项的项集ID
  auto iter = production_item_set_includes_item_size.begin();
  for (; iter != production_item_set_includes_item_size.end(); ++iter) {
    if (iter->second != items.size()) [[likely]] {
      // 该项集没包含所有项
      continue;
    }
    // 检查是否所有项都是该项集的核心项
    bool all_item_is_main_item = true;
    ProductionItemSet& production_item_set_may_belong_to =
        GetProductionItemSet(iter->first);
    for (const auto& item : items) {
      // 构造移入节点后得到的项
      auto item_after_shift = item->first;
      ++std::get<NextWordToShiftIndex>(item_after_shift);
      if (!production_item_set_may_belong_to.IsMainItem(item_after_shift)) {
        all_item_is_main_item = false;
        break;
      }
    }
    if (all_item_is_main_item) {
      return iter->first;
    }
  }
  return ProductionItemSetId::InvalidId();
}

bool SyntaxGenerator::
    SpreadLookForwardSymbolAndConstructSyntaxAnalysisTableEntry(
        ProductionItemSetId production_item_set_id) {
  // 如果未执行闭包操作则无需执行传播步骤
  if (!ProductionItemSetClosure(production_item_set_id)) [[unlikely]] {
    OutPutInfo(std::format(
        "ID = {:}的项集在求闭包后没有任何更改，无需重新传播向前看符号",
        production_item_set_id.GetRawValue()));
    return false;
  }
  OutPutInfo(std::format("对ID = {:}的项集传播向前看符号",
                         production_item_set_id.GetRawValue()));
  // 执行闭包操作后可以空规约达到的每一项都在production_item_set内
  // 所以处理时无需考虑某项空规约可能达到的项，因为这些项都存在于production_item_set内

  // 需要传播向前看符号的production_item_set的ID，里面的ID可能重复
  std::list<ProductionItemSetId> production_item_set_waiting_spread_ids;
  SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id =
      GetProductionItemSet(production_item_set_id)
          .GetSyntaxAnalysisTableEntryId();
  // 根据转移条件分类项，以便之后寻找goto后的项所属项集
  // 键是转移条件，值是移入键值前的项
  std::unordered_map<
      ProductionNodeId,
      std::list<ProductionItemAndForwardNodesContainer::const_iterator>>
      goto_table;
  // 收集所有转移条件
  const auto& items_and_forward_nodes =
      GetProductionItemsAndForwardNodes(production_item_set_id);
  for (auto iter = items_and_forward_nodes.cbegin();
       iter != items_and_forward_nodes.cend(); iter++) {
    auto [production_node_id, production_body_id, next_word_to_shift_index] =
        iter->first;
    NonTerminalProductionNode& production_node_now =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(production_node_id));
    ProductionNodeId next_production_node_id =
        production_node_now.GetProductionNodeInBody(production_body_id,
                                                    next_word_to_shift_index);
    if (next_production_node_id.IsValid()) [[unlikely]] {
      // 该项可以继续移入符号
      goto_table[next_production_node_id].emplace_back(iter);
    }
  }
  // 找到每个转移条件下的所有项是否存在共同所属项集
  // 如果存在则设置转移到该项集并传播向前看符号，否则建立新项集并添加所有项
  for (const auto& transform_condition_and_transformed_data : goto_table) {
    // 获取转移后项集属于的项集的ID
    ProductionItemSetId production_item_set_after_transform_id =
        GetProductionItemSetIdFromProductionItems(
            transform_condition_and_transformed_data.second);
    SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id_after_transform;
    if (production_item_set_after_transform_id.IsValid()) [[unlikely]] {
      // 转移到的项集已存在
      // 转移后到达的语法分析表条目
      syntax_analysis_table_entry_id_after_transform =
          GetProductionItemSet(production_item_set_after_transform_id)
              .GetSyntaxAnalysisTableEntryId();
      bool new_forward_node_inserted = false;
      // 添加转移条件下转移到的这些项的向前看符号
      for (auto& item_and_forward_nodes_iter :
           transform_condition_and_transformed_data.second) {
        // 构建移入符号后的项
        ProductionItem shifted_item = item_and_forward_nodes_iter->first;
        // 获取移入符号后的项数据
        ++std::get<NextWordToShiftIndex>(shifted_item);
        new_forward_node_inserted |=
            AddForwardNodes(production_item_set_after_transform_id,
                            shifted_item, item_and_forward_nodes_iter->second);
        OutPutInfo(std::format(
            "设置项：{:} 在向前看符号 {:} 下执行移入操作",
            FormatProductionItem(item_and_forward_nodes_iter->first),
            GetNodeSymbolStringFromProductionNodeId(
                transform_condition_and_transformed_data.first)));
      }
      // 如果向已有项集的项中添加了新的向前看符号则重新传播该项
      if (new_forward_node_inserted) {
        production_item_set_waiting_spread_ids.push_back(
            production_item_set_after_transform_id);
        OutPutInfo(std::format(
            "ProductionItemSet ID:{:} "
            "的项集由于添加了新项/新向前看符号，重新传播该项集的向前看符号",
            production_item_set_after_transform_id.GetRawValue()));
      }
    } else {
      // 不存在这些项构成的项集，需要新建
      production_item_set_after_transform_id = EmplaceProductionItemSet();
      syntax_analysis_table_entry_id_after_transform =
          GetProductionItemSet(production_item_set_after_transform_id)
              .GetSyntaxAnalysisTableEntryId();
      // 填入所有核心项
      for (auto& item_and_forward_nodes_iter :
           transform_condition_and_transformed_data.second) {
        // 构建移入符号后的项
        ProductionItem shifted_item = item_and_forward_nodes_iter->first;
        // 获取移入符号后的项数据
        ++std::get<NextWordToShiftIndex>(shifted_item);
        auto result = AddMainItemAndForwardNodeIdsToProductionItem(
            production_item_set_after_transform_id, shifted_item,
            item_and_forward_nodes_iter->second);
        assert(result.second);
      }
      production_item_set_waiting_spread_ids.push_back(
          production_item_set_after_transform_id);
    }
    SyntaxAnalysisTableEntry& syntax_analysis_table_entry =
        GetSyntaxAnalysisTableEntry(syntax_analysis_table_entry_id);
    // 设置转移条件下转移到已有的项集
    switch (GetProductionNode(transform_condition_and_transformed_data.first)
                .GetType()) {
      case ProductionNodeType::kTerminalNode:
      case ProductionNodeType::kOperatorNode:
        syntax_analysis_table_entry.SetTerminalNodeActionAndAttachedData(
            transform_condition_and_transformed_data.first,
            SyntaxAnalysisTableEntry::ShiftAttachedData(
                syntax_analysis_table_entry_id_after_transform));
        OutPutInfo(std::format(
            "ID = {:}的项集在移入终结符号 {:} 后转移到ID = {:}的项集",
            production_item_set_id.GetRawValue(),
            GetNodeSymbolStringFromProductionNodeId(
                transform_condition_and_transformed_data.first),
            production_item_set_after_transform_id.GetRawValue()));
        break;
      case ProductionNodeType::kNonTerminalNode:
        syntax_analysis_table_entry.SetNonTerminalNodeTransformId(
            transform_condition_and_transformed_data.first,
            syntax_analysis_table_entry_id_after_transform);
        OutPutInfo(std::format(
            "ID = {:}的项集在移入非终结符号 {:} 后转移到ID = {:}的项集",
            production_item_set_id.GetRawValue(),
            GetNodeSymbolStringFromProductionNodeId(
                transform_condition_and_transformed_data.first),
            production_item_set_after_transform_id.GetRawValue()));
        break;
      default:
        assert(false);
        break;
    }
  }
  OutPutInfo(std::format("ID = {:}的项集传播向前看符号操作已完成",
                         production_item_set_id.GetRawValue()));
  for (auto production_item_set_waiting_spread_id :
       production_item_set_waiting_spread_ids) {
    // 对新生成的每个项集都传播向前看符号
    SpreadLookForwardSymbolAndConstructSyntaxAnalysisTableEntry(
        production_item_set_waiting_spread_id);
  }
  return true;
}

std::array<std::vector<ProductionNodeId>, 4>
SyntaxGenerator::ClassifyProductionNodes() const {
  std::array<std::vector<ProductionNodeId>, 4> production_nodes;
  ObjectManager<BaseProductionNode>::ConstIterator iter =
      manager_nodes_.ConstBegin();
  while (iter != manager_nodes_.ConstEnd()) {
    production_nodes[static_cast<size_t>(iter->GetType())].push_back(
        iter->GetNodeId());
    ++iter;
  }
  return production_nodes;
}

void SyntaxGenerator::SyntaxAnalysisTableTerminalNodeClassify(
    const std::vector<ProductionNodeId>& terminal_node_ids, size_t index,
    std::list<SyntaxAnalysisTableEntryId>&& syntax_analysis_table_entry_ids,
    std::vector<std::list<SyntaxAnalysisTableEntryId>>* equivalent_ids) {
  if (index >= terminal_node_ids.size()) [[unlikely]] {
    // 分类完成，将可以合并的组添加到结果
    assert(syntax_analysis_table_entry_ids.size() > 1);
    equivalent_ids->emplace_back(std::move(syntax_analysis_table_entry_ids));
    return;
  }
  assert(GetProductionNode(terminal_node_ids[index]).GetType() ==
             ProductionNodeType::kTerminalNode ||
         GetProductionNode(terminal_node_ids[index]).GetType() ==
             ProductionNodeType::kOperatorNode);
  // 以下三个分类表，根据转移条件下的转移结果分类
  // 使用ActionAndAttachedDataPointerEqualTo来判断两个转移结果是否相同
  // 键值为ShiftAttachedData移入符号后转移到的语法分析表条目ID
  std::unordered_map<SyntaxAnalysisTableEntryId,
                     std::list<SyntaxAnalysisTableEntryId>>
      shift_classify_table;
  // 键值为ReductAttachedData规约时使用的包装Reduct函数的类的对象的ID
  // 这个ID是唯一的规约数据，所以可以用来标识规约数据
  std::unordered_map<ProcessFunctionClassId,
                     std::list<SyntaxAnalysisTableEntryId>>
      reduct_classify_table;
  // 键值前半部分为ShiftReductAttachedData移入符号后转移到的语法分析表条目ID
  // 键值后半部分为ReductAttachedData规约时使用的包装Reduct函数的类的对象ID
  std::unordered_map<
      std::pair<SyntaxAnalysisTableEntryId, ProcessFunctionClassId>,
      std::list<SyntaxAnalysisTableEntryId>,
      SyntaxAnalysisTableEntryIdAndProcessFunctionClassIdHasher>
      shift_reduct_classify_table;
  // 在该条件下不能规约也不能移入的语法分析表条目ID
  std::list<SyntaxAnalysisTableEntryId> nothing_to_do_entry_ids;
  ProductionNodeId transform_id = terminal_node_ids[index];
  for (auto syntax_analysis_table_entry_id : syntax_analysis_table_entry_ids) {
    // 利用unordered_map进行分类
    auto action_and_attahced_data =
        GetSyntaxAnalysisTableEntry(syntax_analysis_table_entry_id)
            .AtTerminalNode(transform_id);
    if (action_and_attahced_data) [[unlikely]] {
      // 在该向前看节点处存在
      switch (action_and_attahced_data->GetActionType()) {
        case ActionType::kShift:
          shift_classify_table[action_and_attahced_data->GetShiftAttachedData()
                                   .GetNextSyntaxAnalysisTableEntryId()]
              .push_back(syntax_analysis_table_entry_id);
          break;
        case ActionType::kReduct:
          reduct_classify_table[action_and_attahced_data
                                    ->GetReductAttachedData()
                                    .GetProcessFunctionClassId()]
              .push_back(syntax_analysis_table_entry_id);
          break;
        case ActionType::kShiftReduct:
          shift_reduct_classify_table
              [std::make_pair(action_and_attahced_data->GetShiftAttachedData()
                                  .GetNextSyntaxAnalysisTableEntryId(),
                              action_and_attahced_data->GetReductAttachedData()
                                  .GetProcessFunctionClassId())]
                  .push_back(syntax_analysis_table_entry_id);
          break;
        default:
          assert(false);
          break;
      }
    } else {
      nothing_to_do_entry_ids.push_back(syntax_analysis_table_entry_id);
    }
  }

  size_t next_index = index + 1;
  // 处理在当前条件下移入的节点
  for (auto& syntax_analysis_table_entry_ids : shift_classify_table) {
    if (syntax_analysis_table_entry_ids.second.size() > 1) {
      // 含有多个条目且有剩余未比较的转移条件，需要继续分类
      SyntaxAnalysisTableTerminalNodeClassify(
          terminal_node_ids, next_index,
          std::move(syntax_analysis_table_entry_ids.second), equivalent_ids);
    }
  }
  // 处理在当前条件下规约的节点
  for (auto& syntax_analysis_table_entry_ids : reduct_classify_table) {
    if (syntax_analysis_table_entry_ids.second.size() > 1) {
      // 含有多个条目且有剩余未比较的转移条件，需要继续分类
      SyntaxAnalysisTableTerminalNodeClassify(
          terminal_node_ids, next_index,
          std::move(syntax_analysis_table_entry_ids.second), equivalent_ids);
    }
  }
  // 处理在当前条件下既可以移入也可以规约的节点
  for (auto& syntax_analysis_table_entry_ids : shift_reduct_classify_table) {
    if (syntax_analysis_table_entry_ids.second.size() > 1) {
      // 含有多个条目且有剩余未比较的转移条件，需要继续分类
      SyntaxAnalysisTableTerminalNodeClassify(
          terminal_node_ids, next_index,
          std::move(syntax_analysis_table_entry_ids.second), equivalent_ids);
    }
  }
  // 处理在当前条件下既可以移入也可以规约的节点
  if (nothing_to_do_entry_ids.size() > 1) [[likely]] {
    // 含有多个条目且有剩余未比较的转移条件，需要继续分类
    SyntaxAnalysisTableTerminalNodeClassify(terminal_node_ids, next_index,
                                            std::move(nothing_to_do_entry_ids),
                                            equivalent_ids);
  }
}

void SyntaxGenerator::SyntaxAnalysisTableNonTerminalNodeClassify(
    const std::vector<ProductionNodeId>& nonterminal_node_ids, size_t index,
    std::list<SyntaxAnalysisTableEntryId>&& syntax_analysis_table_entry_ids,
    std::vector<std::list<SyntaxAnalysisTableEntryId>>* equivalent_ids) {
  if (index >= nonterminal_node_ids.size()) {
    // 分类完成，将可以合并的条目添加到结果
    assert(syntax_analysis_table_entry_ids.size() > 1);
    equivalent_ids->emplace_back(std::move(syntax_analysis_table_entry_ids));
  } else {
    assert(GetProductionNode(nonterminal_node_ids[index]).GetType() ==
           ProductionNodeType::kNonTerminalNode);
    // 分类表，根据转移条件下的转移结果分类
    std::unordered_map<SyntaxAnalysisTableEntryId,
                       std::list<SyntaxAnalysisTableEntryId>>
        classify_table;
    ProductionNodeId transform_id = nonterminal_node_ids[index];
    for (auto production_node_id : syntax_analysis_table_entry_ids) {
      // 利用unordered_map进行分类
      classify_table[GetSyntaxAnalysisTableEntry(production_node_id)
                         .AtNonTerminalNode(transform_id)]
          .push_back(production_node_id);
    }
    size_t next_index = index + 1;
    for (auto syntax_analysis_table_entry_ids : classify_table) {
      if (syntax_analysis_table_entry_ids.second.size() > 1) {
        // 该类含有多个条目且有剩余未比较的转移条件，需要继续分类
        SyntaxAnalysisTableNonTerminalNodeClassify(
            nonterminal_node_ids, next_index,
            std::move(syntax_analysis_table_entry_ids.second), equivalent_ids);
      }
    }
  }
}

std::vector<std::list<SyntaxAnalysisTableEntryId>>
SyntaxGenerator::SyntaxAnalysisTableEntryClassify(
    std::vector<ProductionNodeId>&& operator_node_ids,
    std::vector<ProductionNodeId>&& terminal_node_ids,
    std::vector<ProductionNodeId>&& nonterminal_node_ids) {
  // 存储相同转移条件的转移表条目ID
  std::vector<std::list<SyntaxAnalysisTableEntryId>> operator_classify_result,
      terminal_classify_result, final_classify_result;
  std::list<SyntaxAnalysisTableEntryId> entry_ids;
  for (size_t i = 0; i < syntax_analysis_table_.size(); i++) {
    // 添加所有待分类的语法分析表条目ID
    entry_ids.push_back(SyntaxAnalysisTableEntryId(i));
  }
  SyntaxAnalysisTableTerminalNodeClassify(
      operator_node_ids, 0, std::move(entry_ids), &operator_classify_result);
  for (auto& syntax_analysis_table_entry_ids : operator_classify_result) {
    if (syntax_analysis_table_entry_ids.size() > 1) {
      SyntaxAnalysisTableTerminalNodeClassify(
          terminal_node_ids, 0, std::move(syntax_analysis_table_entry_ids),
          &terminal_classify_result);
    }
  }
  for (auto& syntax_analysis_table_entry_ids : terminal_classify_result) {
    if (syntax_analysis_table_entry_ids.size() > 1) {
      SyntaxAnalysisTableNonTerminalNodeClassify(
          nonterminal_node_ids, 0, std::move(syntax_analysis_table_entry_ids),
          &final_classify_result);
    }
  }
  return final_classify_result;
}

inline void SyntaxGenerator::RemapSyntaxAnalysisTableEntryId(
    const std::unordered_map<SyntaxAnalysisTableEntryId,
                             SyntaxAnalysisTableEntryId>& old_id_to_new_id) {
  for (auto& entry : syntax_analysis_table_) {
    entry.ResetEntryId(old_id_to_new_id);
  }
}

void SyntaxGenerator::SyntaxAnalysisTableMergeOptimize() {
  std::array<std::vector<ProductionNodeId>, 4>&&
      classified_production_node_ids = ClassifyProductionNodes();
  constexpr size_t terminal_index =
      static_cast<size_t>(ProductionNodeType::kTerminalNode);
  constexpr size_t operator_index =
      static_cast<size_t>(ProductionNodeType::kOperatorNode);
  constexpr size_t nonterminal_index =
      static_cast<size_t>(ProductionNodeType::kNonTerminalNode);
  std::vector<std::list<SyntaxAnalysisTableEntryId>> classified_ids =
      SyntaxAnalysisTableEntryClassify(
          std::move(classified_production_node_ids[operator_index]),
          std::move(classified_production_node_ids[terminal_index]),
          std::move(classified_production_node_ids[nonterminal_index]));
  // 存储需要重映射的条目ID和新条目ID
  std::unordered_map<SyntaxAnalysisTableEntryId, SyntaxAnalysisTableEntryId>
      remapped_entry_id_to_new_entry_id;
  for (auto& entry_ids : classified_ids) {
    // 添加被合并的旧条目到相同条目的映射
    // 重复的条目只保留返回数组中的第一条，其余的全部映射到该条
    auto iter = entry_ids.begin();
    SyntaxAnalysisTableEntryId new_id = *iter;
    ++iter;
    while (iter != entry_ids.end()) {
      assert(remapped_entry_id_to_new_entry_id.find(*iter) ==
             remapped_entry_id_to_new_entry_id.end());
      remapped_entry_id_to_new_entry_id[*iter] = new_id;
      ++iter;
    }
  }
  // 存储移动后的条目的新条目ID
  std::unordered_map<SyntaxAnalysisTableEntryId, SyntaxAnalysisTableEntryId>
      moved_entry_to_new_entry_id;
  // 开始合并
  assert(syntax_analysis_table_.size() > 0);
  // 类似快排分类的算法
  // 寻找没有被合并的条目并紧凑排列在vector低下标一侧

  // 下一个要处理的条目
  SyntaxAnalysisTableEntryId next_process_entry_index(0);
  // 跳过前面维持原位的语法分析表条目
  while (next_process_entry_index < syntax_analysis_table_.size() &&
         remapped_entry_id_to_new_entry_id.find(next_process_entry_index) ==
             remapped_entry_id_to_new_entry_id.end())
    [[likely]] {
      // 保留的条目仍处于原位
      ++next_process_entry_index;
    }
  // 下一个插入位置
  SyntaxAnalysisTableEntryId next_insert_position_index(
      next_process_entry_index);
  ++next_process_entry_index;
  while (next_process_entry_index < syntax_analysis_table_.size()) {
    if (remapped_entry_id_to_new_entry_id.find(next_process_entry_index) ==
        remapped_entry_id_to_new_entry_id.end()) {
      // 该条目保留
      assert(next_insert_position_index != next_process_entry_index);
      // 需要移动到新位置保持vector紧凑
      syntax_analysis_table_[next_insert_position_index] =
          std::move(syntax_analysis_table_[next_process_entry_index]);
      // 重映射保留条目的新位置
      moved_entry_to_new_entry_id[next_process_entry_index] =
          next_insert_position_index;
      ++next_insert_position_index;
    }
    ++next_process_entry_index;
  }
  // 释放多余空间
  syntax_analysis_table_.resize(next_insert_position_index);
  // 将所有旧ID更新为新ID
  RemapSyntaxAnalysisTableEntryId(moved_entry_to_new_entry_id);
}

inline void SyntaxGenerator::SaveConfig() const {
  dfa_generator_.SaveConfig();
  std::ofstream config_file(frontend::common::kSyntaxConfigFileName,
                            std::ios_base::binary | std::ios_base::out);
  // oarchive要在config_file析构前析构，否则文件不完整在反序列化时会抛异常
  {
    boost::archive::binary_oarchive oarchive(config_file);
    oarchive << *this;
  }
}

std::string SyntaxGenerator::FormatSingleProductionBody(
    ProductionNodeId nonterminal_node_id,
    ProductionBodyId production_body_id) const {
  std::string format_result;
  const NonTerminalProductionNode& production_node_now =
      static_cast<const NonTerminalProductionNode&>(
          GetProductionNode(nonterminal_node_id));
  assert(production_node_now.GetType() == ProductionNodeType::kNonTerminalNode);
  format_result += std::format(
      "{:} ->",
      GetNodeSymbolStringFromId(production_node_now.GetNodeSymbolId()));
  for (auto id :
       production_node_now.GetBody(production_body_id).production_body) {
    format_result +=
        std::format(" {:}", GetNodeSymbolStringFromProductionNodeId(id));
  }
  return format_result;
}

std::string SyntaxGenerator::FormatProductionBodys(
    ProductionNodeId nonterminal_node_id) {
  std::string format_result;
  const NonTerminalProductionNode& production_node_now =
      static_cast<const NonTerminalProductionNode&>(
          GetProductionNode(nonterminal_node_id));
  assert(production_node_now.GetType() == ProductionNodeType::kNonTerminalNode);
  for (auto& body_id : production_node_now.GetAllBodyIds()) {
    format_result += FormatSingleProductionBody(nonterminal_node_id, body_id);
    format_result += '\n';
  }
  return format_result;
}

std::string SyntaxGenerator::FormatProductionItem(
    const ProductionItem& production_item) const {
  std::string format_result;
  auto& [production_node_id, production_body_id, next_word_to_shift_index] =
      production_item;
  const NonTerminalProductionNode& production_node_now =
      static_cast<const NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  format_result += std::format(
      "{:} ->",
      GetNodeSymbolStringFromId(production_node_now.GetNodeSymbolId()));
  const auto& production_node_waiting_spread_body =
      production_node_now.GetBody(production_body_id).production_body;
  for (size_t i = 0; i < next_word_to_shift_index; ++i) {
    format_result +=
        std::format(" {:}", GetNodeSymbolStringFromProductionNodeId(
                                production_node_waiting_spread_body[i]));
  }
  format_result += std::format(" ·");
  for (size_t i = next_word_to_shift_index;
       i < production_node_waiting_spread_body.size(); i++) {
    format_result +=
        std::format(" {:}", GetNodeSymbolStringFromProductionNodeId(
                                production_node_waiting_spread_body[i]));
  }
  return format_result;
}

std::string SyntaxGenerator::FormatProductionItemAndLookForwardSymbols(
    const ProductionItem& production_item,
    const ForwardNodesContainer& look_forward_node_ids) const {
  return FormatProductionItem(production_item) + std::format(" 向前看符号：") +
         FormatLookForwardSymbols(look_forward_node_ids);
}

std::string SyntaxGenerator::FormatLookForwardSymbols(
    const ForwardNodesContainer& look_forward_node_ids) const {
  if (look_forward_node_ids.empty()) [[unlikely]] {
    return std::string();
  }
  std::string format_result;
  for (const auto& node_id : look_forward_node_ids) {
    format_result += GetNodeSymbolStringFromProductionNodeId(node_id);
    format_result += ' ';
  }
  // 弹出尾部空格
  format_result.pop_back();
  return format_result;
}

std::string SyntaxGenerator::FormatProductionItems(
    ProductionItemSetId production_item_set_id) const {
  std::string format_result;
  const ProductionItemSet& production_item_set =
      GetProductionItemSet(production_item_set_id);
  for (const auto& item_and_forward_nodes :
       production_item_set.GetItemsAndForwardNodeIds()) {
    format_result += FormatProductionItem(item_and_forward_nodes.first);
    format_result += std::format(" 向前看符号：");
    format_result += FormatLookForwardSymbols(item_and_forward_nodes.second);
    format_result += '\n';
  }
  if (!format_result.empty()) [[likely]] {
    // 弹出最后的换行符
    format_result.pop_back();
  }
  return format_result;
}

void SyntaxGenerator::SyntaxAnalysisTableConstruct() {
  // 创建输入到文件尾时返回的节点
  ProductionNodeId end_production_node_id = AddEndNode();
  // 设置遇到代码文件尾部时返回的数据
  frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
      end_of_file_saved_data;
  end_of_file_saved_data.production_node_id = end_production_node_id;
  end_of_file_saved_data.node_type = ProductionNodeType::kEndNode;
  dfa_generator_.SetEndOfFileSavedData(std::move(end_of_file_saved_data));
  // 生成初始的项集，并将根产生式填入
  ProductionItemSetId root_production_item_set_id = EmplaceProductionItemSet();
  ProductionItemSet& root_production_item_set =
      GetProductionItemSet(root_production_item_set_id);
  // 生成内部根产生式，防止用户定义的根产生式具有多个产生式体导致一系列问题
  // 根产生式名使用了无法通过宏定义的字符@以防止与用户定义的节点名冲突
  ProductionNodeId inside_root_production_node_id =
      AddNonTerminalProduction<RootReductClass>(
          "@RootNode",
          {GetNodeSymbolStringFromProductionNodeId(GetRootProductionNodeId())});
  root_production_item_set.AddMainItemAndForwardNodeIds(
      ProductionItem(inside_root_production_node_id, ProductionBodyId(0),
                     NextWordToShiftIndex(0)),
      std::initializer_list<ProductionNodeId>{end_production_node_id});
  SetRootSyntaxAnalysisTableEntryId(
      root_production_item_set.GetSyntaxAnalysisTableEntryId());
  // 传播向前看符号同时构造语法分析表
  SpreadLookForwardSymbolAndConstructSyntaxAnalysisTableEntry(
      root_production_item_set_id);
  // 设置内部根产生式遇到文件尾时可以接受，用来应对空输入的情况
  SyntaxAnalysisTableEntryId inside_root_syntax_analysis_entry_id =
      root_production_item_set.GetSyntaxAnalysisTableEntryId();
  GetSyntaxAnalysisTableEntry(inside_root_syntax_analysis_entry_id)
      .SetAcceptInEofForwardNode(end_production_node_id);
  // 设置移入用户设置的root节点后在遇到文件尾返回的节点时使用Accept动作
  SyntaxAnalysisTableEntryId entry_after_shift_user_defined_root =
      GetSyntaxAnalysisTableEntry(
          root_production_item_set.GetSyntaxAnalysisTableEntryId())
          .AtNonTerminalNode(GetRootProductionNodeId());
  GetSyntaxAnalysisTableEntry(entry_after_shift_user_defined_root)
      .SetAcceptInEofForwardNode(end_production_node_id);
  // 合并等效项，压缩语法分析表
  SyntaxAnalysisTableMergeOptimize();
}

void SyntaxGenerator::ConstructSyntaxConfig() {
  SyntaxGeneratorInit();
  ConfigConstruct();
  CheckUndefinedProductionRemained();
  dfa_generator_.DfaConstruct();
  SyntaxAnalysisTableConstruct();
  // 保存配置
  SaveConfig();
}

void SyntaxGenerator::SyntaxGeneratorInit() {
  undefined_productions_.clear();
  manager_nodes_.ObjectManagerInit();
  manager_node_symbol_.StructManagerInit();
  manager_terminal_body_symbol_.StructManagerInit();
  node_symbol_id_to_node_id_.clear();
  production_body_symbol_id_to_node_id_.clear();
  production_item_sets_.ObjectManagerInit();
  syntax_analysis_table_entry_id_to_production_item_set_id_.clear();
  root_production_node_id_ = ProductionNodeId::InvalidId();
  root_syntax_analysis_table_entry_id_ =
      SyntaxAnalysisTableEntryId::InvalidId();
  dfa_generator_.DfaInit();
  syntax_analysis_table_.clear();
  manager_process_function_class_.ObjectManagerInit();
}

void SyntaxGenerator::AddUnableContinueNonTerminalNode(
    const std::string& undefined_symbol, std::string&& node_symbol,
    std::vector<std::string>&& subnode_symbols,
    ProcessFunctionClassId class_id) {
  assert(!undefined_symbol.empty() && !node_symbol.empty() &&
         !subnode_symbols.empty() && class_id.IsValid());
  std::string temp_output(std::format("非终结产生式 {:} ->", node_symbol));
  for (const auto& subnode_symbol : subnode_symbols) {
    temp_output += std::format(" {:}", subnode_symbol);
  }
  OutPutInfo(temp_output);
  OutPutInfo(
      std::format("由于尚未定义产生式{:}而被推迟添加", undefined_symbol));
  undefined_productions_.emplace(
      undefined_symbol, std::make_tuple(std::move(node_symbol),
                                        std::move(subnode_symbols), class_id));
}

void SyntaxGenerator::CheckNonTerminalNodeCanContinue(
    const std::string& added_node_symbol) {
  auto [iter_begin, iter_end] =
      undefined_productions_.equal_range(added_node_symbol);
  while (iter_begin != iter_end) {
    auto& [node_could_continue_to_add_symbol, node_body,
           process_function_class_id_] = iter_begin->second;
    std::string temp_output(
        std::format("非终结产生式 {:} ->", node_could_continue_to_add_symbol));
    for (const auto& subnode_symbol : node_body) {
      temp_output += std::format(" {:}", subnode_symbol);
    }
    temp_output += std::format("恢复添加");
    OutPutInfo(temp_output);
    // 如果无法继续添加则会再次调用AddUnableContinueNonTerminalNode函数添加
    AddNonTerminalProduction(std::move(node_could_continue_to_add_symbol),
                             std::move(node_body), process_function_class_id_);
    undefined_productions_.erase(iter_begin++);
  }
}

void SyntaxGenerator::CheckUndefinedProductionRemained() {
  if (!undefined_productions_.empty()) {
    // 仍存在未定义产生式
    for (auto& item : undefined_productions_) {
      auto& [node_symbol, node_bodys, class_id] = item.second;
      std::string temp_output(
          std::format("非终结产生式： {:} ->", node_symbol));
      for (auto& body : node_bodys) {
        temp_output += std::format(" {:}", body);
      }
      temp_output += "中";
      OutPutError(temp_output);
      OutPutError(std::format("产生式： {:} 未定义", item.first));
    }
    exit(-1);
  }
}

void SyntaxGenerator::AddKeyWord(std::string&& key_word) {
  // 关键字优先级默认为2
  // 自动生成同名终结节点
  // 关键字定义时将key_word视作单词而不是正则表达式
  AddTerminalProduction(
      std::string(key_word), std::move(key_word),
      frontend::generator::dfa_generator::DfaGenerator::WordPriority(2), true);
}
ProductionNodeId SyntaxGenerator::GetProductionNodeIdFromNodeSymbolId(
    SymbolId node_symbol_id) {
  auto iter = node_symbol_id_to_node_id_.find(node_symbol_id);
  if (iter == node_symbol_id_to_node_id_.end()) {
    return ProductionNodeId::InvalidId();
  } else {
    return iter->second;
  }
}
ProductionNodeId SyntaxGenerator::GetProductionNodeIdFromBodySymbolId(
    SymbolId body_symbol_id) {
  auto iter = production_body_symbol_id_to_node_id_.find(body_symbol_id);
  if (iter != production_body_symbol_id_to_node_id_.end()) {
    return iter->second;
  } else {
    return ProductionNodeId::InvalidId();
  }
}

inline ProductionNodeId SyntaxGenerator::GetProductionNodeIdFromNodeSymbol(
    const std::string& node_symbol) {
  SymbolId node_symbol_id = GetNodeSymbolId(node_symbol);
  if (node_symbol_id.IsValid()) {
    return GetProductionNodeIdFromNodeSymbolId(node_symbol_id);
  } else {
    return ProductionNodeId::InvalidId();
  }
}

ProductionNodeId SyntaxGenerator::GetProductionNodeIdFromBodySymbol(
    const std::string& body_symbol) {
  SymbolId body_symbol_id = GetBodySymbolId(body_symbol);
  if (body_symbol_id.IsValid()) {
    return GetProductionNodeIdFromBodySymbolId(body_symbol_id);
  } else {
    return ProductionNodeId::InvalidId();
  }
}

}  // namespace frontend::generator::syntax_generator