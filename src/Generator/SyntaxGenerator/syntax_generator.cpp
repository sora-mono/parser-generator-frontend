#include "syntax_generator.h"

#include <queue>

namespace frontend::generator::syntax_generator {
SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddTerminalNode(
    const std::string& node_symbol, const std::string& body_symbol,
    WordPriority node_priority) {
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
    bool result = dfa_generator_.AddRegexpression(
        body_symbol, std::move(word_attached_data), node_priority);
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

inline SyntaxGenerator::ProductionNodeId SyntaxGenerator::SubAddTerminalNode(
    SymbolId node_symbol_id, SymbolId body_symbol_id) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<TerminalProductionNode>(node_symbol_id,
                                                           body_symbol_id);
  manager_nodes_.GetObject(node_id).SetThisNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  SetBodySymbolIdToProductionNodeIdMapping(body_symbol_id, node_id);
  return node_id;
}

SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddBinaryOperatorNode(
    const std::string& operator_symbol,
    OperatorAssociatityType binary_operator_associatity_type,
    OperatorPriority binary_operator_priority_level) {
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
  ProductionNodeId operator_node_id = SubAddOperatorNode(
      operator_node_symbol_id, binary_operator_associatity_type,
      binary_operator_priority_level);
  assert(operator_node_id.IsValid());
  frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
      word_attached_data;
  word_attached_data.production_node_id = operator_node_id;
  word_attached_data.node_type = ProductionNodeType::kOperatorNode;
  word_attached_data.binary_operator_associate_type =
      binary_operator_associatity_type;
  word_attached_data.binary_operator_priority = binary_operator_priority_level;
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
      binary_operator_priority_level.GetRawValue()));
  return operator_node_id;
}

SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddLeftUnaryOperatorNode(
    const std::string& operator_symbol,
    OperatorAssociatityType unary_operator_associatity_type,
    OperatorPriority unary_operator_priority_level) {
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
  ProductionNodeId operator_node_id = SubAddOperatorNode(
      operator_node_symbol_id, unary_operator_associatity_type,
      unary_operator_priority_level);
  assert(operator_node_id.IsValid());
  frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
      word_attached_data;
  word_attached_data.production_node_id = operator_node_id;
  word_attached_data.node_type = ProductionNodeType::kOperatorNode;
  word_attached_data.unary_operator_associate_type =
      unary_operator_associatity_type;
  word_attached_data.unary_operator_priority = unary_operator_priority_level;
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
      unary_operator_priority_level.GetRawValue()));
  return operator_node_id;
}

SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddBinaryUnaryOperatorNode(
    const std::string& operator_symbol,
    OperatorAssociatityType binary_operator_associatity_type,
    OperatorPriority binary_operator_priority_level,
    OperatorAssociatityType unary_operator_associatity_type,
    OperatorPriority unary_operator_priority_level) {
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
  ProductionNodeId operator_node_id = SubAddOperatorNode(
      operator_node_symbol_id, unary_operator_associatity_type,
      unary_operator_priority_level);
  assert(operator_node_id.IsValid());
  frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
      word_attached_data;
  word_attached_data.production_node_id = operator_node_id;
  word_attached_data.node_type = ProductionNodeType::kOperatorNode;
  word_attached_data.binary_operator_associate_type =
      binary_operator_associatity_type;
  word_attached_data.binary_operator_priority = binary_operator_priority_level;
  word_attached_data.unary_operator_associate_type =
      unary_operator_associatity_type;
  word_attached_data.unary_operator_priority = unary_operator_priority_level;
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
      binary_operator_priority_level.GetRawValue(),
      unary_operator_priority_level.GetRawValue()));
  return operator_node_id;
}

inline SyntaxGenerator::ProductionNodeId SyntaxGenerator::SubAddOperatorNode(
    SymbolId node_symbol_id, OperatorAssociatityType associatity_type,
    OperatorPriority priority_level) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<OperatorProductionNode>(
          node_symbol_id, associatity_type, priority_level);
  manager_nodes_.GetObject(node_id).SetThisNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  SetBodySymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  return node_id;
}

SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddNonTerminalNode(
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
  OutPutInfo(std::format("非终结节点ID:：{:} 当前产生式体ID：{:}",
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
  nonterminal_production_node.SetProductionCouldBeEmptyRedut();
}

inline SyntaxGenerator::ProductionNodeId SyntaxGenerator::SubAddNonTerminalNode(
    SymbolId node_symbol_id) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<NonTerminalProductionNode>(node_symbol_id);
  manager_nodes_.GetObject(node_id).SetThisNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  return node_id;
}

inline SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddEndNode() {
  ProductionNodeId node_id = manager_nodes_.EmplaceObject<EndNode>();
  manager_nodes_.GetObject(node_id).SetThisNodeId(node_id);
  return node_id;
}

void SyntaxGenerator::SetRootProduction(
    const std::string& production_node_symbol) {
  ProductionNodeId production_node_id =
      GetProductionNodeIdFromNodeSymbol(production_node_symbol);
  if (GetRootProductionNodeId().IsValid()) [[unlikely]] {
    OutPutError(std::format("仅且必须声明一个根产生式"));
    exit(-1);
  }
  assert(production_node_id.IsValid());
  // 生成根产生式
  // 根产生式名使用了无法通过宏定义的字符@以防止与用户定义的节点名冲突
  ProductionNodeId root_production_node_id =
      AddNonTerminalNode<RootReductClass>("@RootNode",
                                          {production_node_symbol});
  SetRootProductionNodeId(root_production_node_id);
}

// 获取节点

inline SyntaxGenerator::BaseProductionNode& SyntaxGenerator::GetProductionNode(
    ProductionNodeId production_node_id) {
  return const_cast<BaseProductionNode&>(
      static_cast<const SyntaxGenerator&>(*this).GetProductionNode(
          production_node_id));
}

inline const SyntaxGenerator::BaseProductionNode&
SyntaxGenerator::GetProductionNode(ProductionNodeId production_node_id) const {
  return manager_nodes_[production_node_id];
}

inline SyntaxGenerator::BaseProductionNode&
SyntaxGenerator::GetProductionNodeFromNodeSymbolId(SymbolId symbol_id) {
  ProductionNodeId production_node_id =
      GetProductionNodeIdFromNodeSymbolId(symbol_id);
  assert(symbol_id.IsValid());
  return GetProductionNode(production_node_id);
}

inline SyntaxGenerator::BaseProductionNode&
SyntaxGenerator::GetProductionNodeBodyFromSymbolId(SymbolId symbol_id) {
  ProductionNodeId production_node_id =
      GetProductionNodeIdFromBodySymbolId(symbol_id);
  assert(symbol_id.IsValid());
  return GetProductionNode(production_node_id);
}

// 获取非终结节点中的一个产生式体

inline const std::vector<SyntaxGenerator::ProductionNodeId>&
SyntaxGenerator::GetProductionBody(ProductionNodeId production_node_id,
                                   ProductionBodyId production_body_id) {
  NonTerminalProductionNode& nonterminal_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  // 只有非终结节点才有多个产生式体，对其它节点调用该函数无意义
  assert(nonterminal_node.Type() == ProductionNodeType::kNonTerminalNode);
  return nonterminal_node.GetBody(production_body_id).production_body;
}

// 记录核心项所属的核心ID

inline void SyntaxGenerator::AddCoreItemBelongToCoreId(
    const CoreItem& core_item, CoreId core_id) {
  auto& [production_node_id, production_body_id, point_index] = core_item;
  NonTerminalProductionNode& production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  assert(production_node.Type() == ProductionNodeType::kNonTerminalNode);
  production_node.AddCoreItemBelongToCoreId(production_body_id, point_index,
                                            core_id);
}

inline const std::list<SyntaxGenerator::CoreId>&
SyntaxGenerator::GetCoreIdFromCoreItem(ProductionNodeId production_node_id,
                                       ProductionBodyId body_id,
                                       PointIndex point_index) {
  NonTerminalProductionNode& production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  assert(production_node.Type() == ProductionNodeType::kNonTerminalNode);
  return production_node.GetCoreIdFromCoreItem(body_id, point_index);
}

void SyntaxGenerator::GetNonTerminalNodeFirstNodeIds(
    ProductionNodeId production_node_id,
    std::unordered_set<ProductionNodeId>* result,
    std::unordered_set<ProductionNodeId>&& processed_nodes) {
  bool inserted = processed_nodes.insert(production_node_id).second;
  assert(inserted);
  if (!production_node_id.IsValid()) [[unlikely]] {
    return;
  }
  NonTerminalProductionNode& production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  assert(production_node.Type() == ProductionNodeType::kNonTerminalNode);
  for (auto& body : production_node.GetAllBody()) {
    ProductionBodyId node_id_index(0);
    // 正在处理的产生式节点的ID
    ProductionNodeId node_id;
    while (node_id_index < body.production_body.size()) {
      node_id = body.production_body[node_id_index];
      if (GetProductionNode(node_id).Type() !=
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

std::unordered_set<SyntaxGenerator::ProductionNodeId> SyntaxGenerator::First(
    ProductionNodeId production_node_id, ProductionBodyId production_body_id,
    PointIndex point_index, const ForwardNodesContainerType& next_node_ids) {
  ProductionNodeId node_id = GetProductionNodeIdInBody(
      production_node_id, production_body_id, point_index);
  if (node_id.IsValid()) {
    std::unordered_set<ProductionNodeId> return_set;
    switch (GetProductionNode(node_id).Type()) {
      case ProductionNodeType::kTerminalNode:
      case ProductionNodeType::kOperatorNode:
        return_set.insert(node_id);
        break;
      case ProductionNodeType::kNonTerminalNode:
        // 合并当前·右侧非终结节点可能的所有单词ID
        GetNonTerminalNodeFirstNodeIds(node_id, &return_set);
        if (static_cast<NonTerminalProductionNode&>(GetProductionNode(node_id))
                .CouldBeEmptyReduct()) {
          // 当前·右侧的非终结节点可以空规约，需要考虑它下一个节点的情况
          return_set.merge(First(production_node_id, production_body_id,
                                 PointIndex(point_index + 1), next_node_ids));
        }
        break;
      default:
        assert(false);
        break;
    }
    return return_set;
  } else {
    return next_node_ids;
  }
}

bool SyntaxGenerator::CoreClosure(CoreId core_id) {
  OutPutInfo(
      std::format("对核心（ID = {:}）执行闭包操作", core_id.GetRawValue()));
  Core& core = GetCore(core_id);
  if (core.IsClosureAvailable()) {
    // 闭包有效，无需重求
    return false;
  }
  ParsingTableEntry& parsing_table_entry =
      GetParsingTableEntry(core.GetParsingTableEntryId());
  // 存储当前待展开的项
  // 存储指向待展开项的迭代器
  std::queue<std::map<CoreItem, ForwardNodesContainerType>::const_iterator>
      items_waiting_process;
  const auto& core_items_and_forward_nodes =
      GetCoreItemsAndForwardNodes(core_id);
  for (auto& iter : GetCoreMainItems(core_id)) {
    // 将项集中所有核心项压入待处理队列
    items_waiting_process.push(iter);
  }
  while (!items_waiting_process.empty()) {
    auto item_now = items_waiting_process.front();
    items_waiting_process.pop();
    const auto [production_node_id, production_body_id, point_index] =
        item_now->first;
    NonTerminalProductionNode& production_node_now =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(production_node_id));
    assert(production_node_now.Type() == ProductionNodeType::kNonTerminalNode);
    ProductionNodeId next_production_node_id = GetProductionNodeIdInBody(
        production_node_id, production_body_id, point_index);
    if (!next_production_node_id.IsValid()) {
      // 无后继节点，设置规约条目

      // 组装规约使用的数据
      ParsingTableEntry::ReductAttachedData reduct_attached_data(
          production_node_id,
          GetProcessFunctionClass(production_node_id, production_body_id),
          GetProductionBody(production_node_id, production_body_id));
      OutPutInfo(std::format("项：") + FormatItem(item_now->first));
      OutPutInfo(std::format("在向前看符号：") +
                 FormatLookForwardSymbols(item_now->second) + " 下执行规约");
      const auto& production_body =
          production_node_now.GetBody(production_body_id).production_body;
      for (auto node_id : item_now->second) {
        // 对每个向前看符号设置规约操作
        parsing_table_entry.SetTerminalNodeActionAndAttachedData(
            node_id, reduct_attached_data);
      }
      continue;
    }
    NonTerminalProductionNode& next_production_node =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(next_production_node_id));
    if (next_production_node.Type() != ProductionNodeType::kNonTerminalNode) {
      continue;
    }
    // 展开非终结节点，并为其创建向前看符号集
    ForwardNodesContainerType forward_node_ids =
        First(production_node_id, production_body_id,
              PointIndex(point_index + 1), item_now->second);
    for (auto body_id : next_production_node.GetAllBodyIds()) {
      // 将该非终结节点中的每个产生式体加入到core中，点在最左侧
      auto [iter, inserted] = AddItemAndForwardNodeIdsToCore(
          core_id, CoreItem(next_production_node_id, body_id, PointIndex(0)),
          forward_node_ids);
      if (inserted) {
        // 如果插入新的项则添加到队列中等待处理，并记录该项属于的新核心ID
        items_waiting_process.push(iter);
      }
    }
    if (next_production_node.CouldBeEmptyReduct()) {
      // ·右侧的非终结节点可以空规约
      // 向项集中添加空规约后得到的项，向前看符号复制原来的项的向前看符号集
      auto [iter, inserted] = AddItemAndForwardNodeIdsToCore(
          core_id,
          CoreItem(production_node_id, production_body_id,
                   PointIndex(point_index + 1)),
          item_now->second);
      if (inserted) {
        // 如果插入新的项则添加到队列中等待处理
        items_waiting_process.push(iter);
      }
    }
    OutPutInfo(std::format("项：") + FormatItem(item_now->first));
    const auto& production_node_waiting_spread_body =
        production_node_now.GetBody(production_body_id).production_body;
    OutPutInfo(std::format(" 已展开"));
    OutPutInfo(std::format("该节点当前拥有的向前看符号： ") +
               FormatLookForwardSymbols(item_now->second));
  }
  SetCoreClosureAvailable(core_id);
  return true;
}

SyntaxGenerator::CoreId SyntaxGenerator::GetCoreIdFromCoreItems(
    const std::list<SyntaxGenerator::CoreItem>& items) {
#ifdef _DEBUG
  // 检查参数中不存在重复项
  for (auto iter = items.cbegin(); iter != items.cend(); ++iter) {
    auto compare_iter = iter;
    while (++compare_iter != items.cend()) {
      assert(*iter != *compare_iter);
    }
  }
#endif  // _DEBUG

  // 存储核心ID包含几个给定项的个数，只有包含全部给定项才可能成为这些项属于的核心
  std::unordered_map<CoreId, size_t> core_includes_item_size;
  // 统计核心包含给定项的个数
  for (const auto& item : items) {
    auto [production_node_id, production_body_id, point_index] = item;
    for (auto core_id : GetCoreIdFromCoreItem(
             production_node_id, production_body_id, point_index)) {
      auto [iter, inserted] = core_includes_item_size.emplace(core_id, 1);
      if (!inserted) {
        // 该核心ID已经记录，增加该核心ID出现的次数
        ++iter->second;
      }
    }
  }
  // 检查所有出现过的核心ID，找出包含所有给定项的核心ID
  auto iter = core_includes_item_size.begin();
  while (iter != core_includes_item_size.end()) {
    if (iter->second != items.size()) [[likely]] {
      // 该核心没包含所有项
      continue;
    }
    // 检查该核心核心项数目是否等于给定项数目
    // 因为二者都不包含重复项，所以只要数目相等二者就包含相同项
    if (GetCoreMainItems(iter->first).size() == items.size()) {
      return iter->first;
    }
  }
  return CoreId::InvalidId();
}

bool SyntaxGenerator::SpreadLookForwardSymbolAndConstructParsingTableEntry(
    CoreId core_id) {
  // 如果未执行闭包操作则无需执行传播步骤
  if (!CoreClosure(core_id)) [[unlikely]] {
    return false;
  }
  // 执行闭包操作后可以空规约达到的每一项都在core内
  // 所以处理时无需考虑某项空规约可能达到的项，因为这些项都存在于core内

  // 需要传播向前看符号的core的ID，里面的ID可能重复
  std::list<CoreId> core_waiting_spread_ids;
  ParsingTableEntryId parsing_table_entry_id =
      GetCore(core_id).GetParsingTableEntryId();
  // 根据转移条件分类项，以便之后寻找goto后的项所属项集
  // 键是转移条件，值是移入键值后得到的项
  std::unordered_map<ProductionNodeId, std::list<CoreItem>> goto_table;
  // 收集所有转移条件
  for (const auto& item_and_forward_nodes :
       GetCoreItemsAndForwardNodes(core_id)) {
    auto [production_node_id, production_body_id, point_index] =
        item_and_forward_nodes.first;
    NonTerminalProductionNode& production_node_now =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(production_node_id));
    ProductionNodeId next_production_node_id =
        production_node_now.GetProductionNodeInBody(production_body_id,
                                                    point_index);
    if (next_production_node_id.IsValid()) [[unlikely]] {
      // 该项可以继续移入符号
      goto_table[next_production_node_id].emplace_back(
          production_node_id, production_body_id, PointIndex(point_index + 1));
    }
  }
  // 找到每个转移条件下的所有项是否存在共同所属项集，如果存在则转移到该项集
  // 否则建立新项集
  for (const auto& transform_condition_and_transformed_items : goto_table) {
    // 获取转移后项集属于的核心的ID
    CoreId core_after_transform_id = GetCoreIdFromCoreItems(
        transform_condition_and_transformed_items.second);
    if (core_after_transform_id.IsValid()) [[unlikely]] {
      // 转移到的项集已存在
      // 转移后到达的语法分析表条目
      ParsingTableEntryId parsing_table_entry_id_after_transform =
          GetCore(core_after_transform_id).GetParsingTableEntryId();
      ParsingTableEntry& parsing_table_entry =
          GetParsingTableEntry(parsing_table_entry_id);
      // 设置转移条件下转移到已有的项集
      switch (GetProductionNode(transform_condition_and_transformed_items.first)
                  .Type()) {
        case ProductionNodeType::kTerminalNode:
        case ProductionNodeType::kOperatorNode:
          parsing_table_entry.SetTerminalNodeActionAndAttachedData(
              transform_condition_and_transformed_items.first,
              ParsingTableEntry::ShiftAttachedData(
                  parsing_table_entry_id_after_transform));
          break;
        case ProductionNodeType::kNonTerminalNode:
          parsing_table_entry.SetNonTerminalNodeTransformId(
              transform_condition_and_transformed_items.first,
              parsing_table_entry_id_after_transform);
          break;
        default:
          assert(false);
          break;
      }
      bool new_forward_node_inserted = false;
      // 添加转移条件下转移到的这些项的向前看符号
      for (auto& item : transform_condition_and_transformed_items.second) {
        new_forward_node_inserted |= AddForwardNodes(
            core_after_transform_id, item, GetForwardNodeIds(core_id, item));
      }
      // 如果向已有项集的项中添加了新的向前看节点则重新传播该项
      if (new_forward_node_inserted) {
        core_waiting_spread_ids.push_back(core_after_transform_id);
        OutPutInfo(std::format(
            "Core ID:{:} "
            "的项集由于添加了新项/新向前看符号，重新传播该项集的向前看符号"));
      }
    } else {
      // 不存在这些项构成的核心，需要新建
      CoreId new_core_id = EmplaceCore();
      ParsingTableEntryId parsing_table_entry_id_after_transform =
          GetCore(new_core_id).GetParsingTableEntryId();
      ParsingTableEntry& parsing_table_entry =
          GetParsingTableEntry(parsing_table_entry_id);
      // 设置转移条件下转移到已有的项集
      switch (GetProductionNode(transform_condition_and_transformed_items.first)
                  .Type()) {
        case ProductionNodeType::kTerminalNode:
        case ProductionNodeType::kOperatorNode:
          parsing_table_entry.SetTerminalNodeActionAndAttachedData(
              transform_condition_and_transformed_items.first,
              ParsingTableEntry::ShiftAttachedData(
                  parsing_table_entry_id_after_transform));
          break;
        case ProductionNodeType::kNonTerminalNode:
          parsing_table_entry.SetNonTerminalNodeTransformId(
              transform_condition_and_transformed_items.first,
              parsing_table_entry_id_after_transform);
          break;
        default:
          assert(false);
          break;
      }
      // 填入所有核心项
      for (auto& item : transform_condition_and_transformed_items.second) {
        auto result = AddMainItemAndForwardNodeIdsToCore(
            new_core_id, item, GetForwardNodeIds(core_id, item));
        assert(result.second);
      }
      core_waiting_spread_ids.push_back(new_core_id);
    }
  }
  for (auto core_waiting_spread_id : core_waiting_spread_ids) {
    // 对新生成的每个项集都传播向前看符号
    SpreadLookForwardSymbolAndConstructParsingTableEntry(
        core_waiting_spread_id);
  }
  return true;
}

std::array<std::vector<SyntaxGenerator::ProductionNodeId>,
           sizeof(SyntaxGenerator::ProductionNodeType)>
SyntaxGenerator::ClassifyProductionNodes() {
  std::array<std::vector<ProductionNodeId>, sizeof(ProductionNodeType)>
      production_nodes;
  ObjectManager<BaseProductionNode>::Iterator iter = manager_nodes_.Begin();
  while (iter != manager_nodes_.End()) {
    production_nodes[static_cast<size_t>(iter->Type())].push_back(iter->Id());
    ++iter;
  }
  return production_nodes;
}

std::vector<std::vector<SyntaxGenerator::ParsingTableEntryId>>
SyntaxGenerator::ParsingTableEntryClassify(
    const std::vector<ProductionNodeId>& terminal_node_ids,
    const std::vector<ProductionNodeId>& nonterminal_node_ids) {
  // 存储相同终结节点转移表的条目
  std::vector<std::vector<ParsingTableEntryId>> terminal_classify_result,
      final_classify_result;
  std::vector<ParsingTableEntryId> entry_ids;
  entry_ids.reserve(syntax_parsing_table_.size());
  for (size_t i = 0; i < syntax_parsing_table_.size(); i++) {
    // 添加所有待分类的语法分析表条目ID
    entry_ids.push_back(ParsingTableEntryId(i));
  }
  ParsingTableTerminalNodeClassify(terminal_node_ids, 0, entry_ids,
                                   &terminal_classify_result);
  for (auto vec_entry_ids : terminal_classify_result) {
    ParsingTableNonTerminalNodeClassify(nonterminal_node_ids, 0, vec_entry_ids,
                                        &final_classify_result);
  }
  return final_classify_result;
}

inline void SyntaxGenerator::RemapParsingTableEntryId(
    const std::unordered_map<ParsingTableEntryId, ParsingTableEntryId>&
        moved_entry_id_to_new_entry_id) {
  for (auto& entry : syntax_parsing_table_) {
    entry.ResetEntryId(moved_entry_id_to_new_entry_id);
  }
}

void SyntaxGenerator::ParsingTableMergeOptimize() {
  std::array<std::vector<ProductionNodeId>, sizeof(ProductionNodeType)>
      classified_production_node_ids;
  size_t terminal_index =
      static_cast<size_t>(ProductionNodeType::kTerminalNode);
  size_t operator_index =
      static_cast<size_t>(ProductionNodeType::kOperatorNode);
  size_t nonterminal_index =
      static_cast<size_t>(ProductionNodeType::kNonTerminalNode);
  for (auto production_node_id :
       classified_production_node_ids[operator_index]) {
    // 将所有运算符类节点添加到终结节点表里，构成完整的终结节点表
    classified_production_node_ids[terminal_index].push_back(
        production_node_id);
  }
  std::vector<std::vector<ParsingTableEntryId>> classified_ids =
      ParsingTableEntryClassify(
          classified_production_node_ids[terminal_index],
          classified_production_node_ids[nonterminal_index]);
  // 存储所有旧条目到新条目的映射
  std::unordered_map<ParsingTableEntryId, ParsingTableEntryId>
      old_entry_id_to_new_entry_id;
  for (auto& entry_ids : classified_ids) {
    // 添加被合并的旧条目到相同条目的映射
    // 重复的条目只保留返回数组中的第一条，其余的全部映射到该条
    ParsingTableEntryId new_id = entry_ids.front();
    for (auto old_id : entry_ids) {
      assert(old_entry_id_to_new_entry_id.find(old_id) ==
             old_entry_id_to_new_entry_id.end());
      old_entry_id_to_new_entry_id[old_id] = new_id;
    }
  }
  // 存储移动后的条目的新条目ID
  std::unordered_map<ParsingTableEntryId, ParsingTableEntryId>
      moved_entry_to_new_entry_id;
  // 开始合并
  assert(syntax_parsing_table_.size() > 0);
  // 下一个要处理的条目
  ParsingTableEntryId next_process_entry_index(0);
  // 下一个插入位置
  ParsingTableEntryId next_insert_position_index(0);
  // 类似快排分类的算法
  // 寻找没有被合并的条目并紧凑排列在vector低下标一侧
  while (next_process_entry_index < syntax_parsing_table_.size()) {
    if (old_entry_id_to_new_entry_id.find(next_process_entry_index) ==
        old_entry_id_to_new_entry_id.end()) {
      // 该条目保留
      if (next_insert_position_index != next_process_entry_index) {
        // 需要移动到新位置保持vector紧凑
        syntax_parsing_table_[next_insert_position_index] =
            std::move(syntax_parsing_table_[next_process_entry_index]);
      }
      // 重映射保留条目的新位置
      moved_entry_to_new_entry_id[next_process_entry_index] =
          next_insert_position_index;
      ++next_insert_position_index;
    }
    ++next_process_entry_index;
  }
  for (auto& item : old_entry_id_to_new_entry_id) {
    // 重映射已被合并条目到保留的唯一条目
    // 与重映射保留条目的新位置的操作一起构成完整的条目重映射表
    // 所有旧条目都拥有自己对应的新条目，被合并的条目映射到
    assert(moved_entry_to_new_entry_id.find(item.second) !=
           moved_entry_to_new_entry_id.end());
    // item_and_forward_node_ids.first是旧entry的ID，item.second是保留下来的entry的ID
    moved_entry_to_new_entry_id[item.first] =
        moved_entry_to_new_entry_id[item.second];
  }
  // 至此每一个条目都有了新条目的映射
  // 释放多余空间
  old_entry_id_to_new_entry_id.clear();
  syntax_parsing_table_.resize(next_insert_position_index);
  // 将所有旧ID更新为新ID
  RemapParsingTableEntryId(moved_entry_to_new_entry_id);
}

std::string SyntaxGenerator::FormatSingleProductionBody(
    ProductionNodeId nonterminal_node_id,
    ProductionBodyId production_body_id) const {
  std::string format_result;
  const NonTerminalProductionNode& production_node_now =
      static_cast<const NonTerminalProductionNode&>(
          GetProductionNode(nonterminal_node_id));
  assert(production_node_now.Type() == ProductionNodeType::kNonTerminalNode);
  format_result += std::format(
      "{:} ->",
      GetNodeSymbolStringFromId(production_node_now.GetNodeSymbolId()));
  for (auto id :
       production_node_now.GetBody(production_body_id).production_body) {
    std::cout << std::format(
        " {:}",
        GetNodeSymbolStringFromId(GetProductionNode(id).GetNodeSymbolId()));
  }
  return format_result;
}

std::string SyntaxGenerator::FormatProductionBodys(
    ProductionNodeId nonterminal_node_id) {
  std::string format_result;
  const NonTerminalProductionNode& production_node_now =
      static_cast<const NonTerminalProductionNode&>(
          GetProductionNode(nonterminal_node_id));
  assert(production_node_now.Type() == ProductionNodeType::kNonTerminalNode);
  for (auto& body_id : production_node_now.GetAllBodyIds()) {
    format_result += FormatSingleProductionBody(nonterminal_node_id, body_id);
    format_result += '\n';
  }
  return format_result;
}

std::string SyntaxGenerator::FormatItem(const CoreItem& core_item) const {
  std::string format_result;
  auto& [production_node_id, production_body_id, point_index] = core_item;
  const NonTerminalProductionNode& production_node_now =
      static_cast<const NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  format_result += std::format(
      "{:} ->",
      GetNodeSymbolStringFromId(production_node_now.GetNodeSymbolId()));
  const auto& production_node_waiting_spread_body =
      production_node_now.GetBody(production_body_id).production_body;
  for (size_t i = 0; i < point_index; ++i) {
    format_result += std::format(
        " {:}", GetNodeSymbolStringFromId(
                    GetProductionNode(production_node_waiting_spread_body[i])
                        .GetNodeSymbolId()));
  }
  format_result += std::format(" ·");
  for (size_t i = point_index; i < production_node_waiting_spread_body.size();
       i++) {
    format_result += std::format(
        " {:}", GetNodeSymbolStringFromId(
                    GetProductionNode(production_node_waiting_spread_body[i])
                        .GetNodeSymbolId()));
  }
  return format_result;
}

std::string SyntaxGenerator::FormatLookForwardSymbols(
    const std::unordered_set<ProductionNodeId>& look_forward_node_ids) const {
  if (look_forward_node_ids.empty()) [[unlikely]] {
    return std::string();
  }
  std::string format_result;
  for (const auto& node_id : look_forward_node_ids) {
    format_result +=
        GetNodeSymbolStringFromId(GetProductionNode(node_id).GetNodeSymbolId());
    format_result += ' ';
  }
  // 弹出尾部空格
  format_result.pop_back();
  return format_result;
}

std::string SyntaxGenerator::FormatCoreItems(CoreId core_id) const {
  std::string format_result;
  const Core& core = GetCore(core_id);
  for (const auto& item_and_forward_nodes : core.GetItemsAndForwardNodeIds()) {
    format_result += FormatItem(item_and_forward_nodes.first);
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

SyntaxGenerator::SyntaxGenerator() {
  SyntaxGeneratorInit();
  ConfigConstruct();
  CheckUndefinedProductionRemained();
  dfa_generator_.DfaReconstrcut();
  ParsingTableConstruct();
  // 保存配置
  SaveConfig();
}

void SyntaxGenerator::ParsingTableConstruct() {
  // 创建输入到文件尾时返回的节点
  ProductionNodeId end_production_node_id = AddEndNode();
  // 设置遇到代码文件尾部时返回的数据
  frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
      end_of_file_saved_data;
  end_of_file_saved_data.production_node_id = end_production_node_id;
  GetProductionNode(end_production_node_id)
      .SetSymbolId(AddNodeSymbol("$").first);
  dfa_generator_.SetEndOfFileSavedData(std::move(end_of_file_saved_data));
  // 生成初始的项集，并将根产生式填入
  CoreId root_core_id = EmplaceCore();
  Core& root_core = GetCore(root_core_id);
  root_core.AddMainItemAndForwardNodeIds(
      CoreItem(GetRootProductionNodeId(), ProductionBodyId(0), PointIndex(0)),
      std::initializer_list<ProductionNodeId>{end_production_node_id});
  SetRootParsingTableEntryId(root_core.GetParsingTableEntryId());
  // 传播向前看符号同时构造语法分析表
  SpreadLookForwardSymbolAndConstructParsingTableEntry(root_core_id);
  // 合并等效项，压缩语法分析表
  ParsingTableMergeOptimize();
}

void SyntaxGenerator::SyntaxGeneratorInit() {
  NodeNumInit();
  undefined_productions_.clear();
  manager_nodes_.ObjectManagerInit();
  manager_node_symbol_.StructManagerInit();
  manager_terminal_body_symbol_.StructManagerInit();
  node_symbol_id_to_node_id_.clear();
  production_body_symbol_id_to_node_id_.clear();
  cores_.ObjectManagerInit();
  parsing_table_entry_id_to_core_id_.clear();
  root_production_node_id_ = ProductionNodeId::InvalidId();
  root_parsing_table_entry_id_ = ParsingTableEntryId::InvalidId();
  dfa_generator_.DfaInit();
  syntax_parsing_table_.clear();
  manager_process_function_class_.ObjectManagerInit();
}

void SyntaxGenerator::AddUnableContinueNonTerminalNode(
    const std::string& undefined_symbol, std::string&& node_symbol,
    std::vector<std::string>&& subnode_symbols,
    ProcessFunctionClassId class_id) {
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
    ProductionNodeId node_id =
        AddNonTerminalNode(std::move(node_could_continue_to_add_symbol),
                           std::move(node_body), process_function_class_id_);
    if (node_id.IsValid()) {
      // 成功添加非终结节点，删除该条记录
      iter_begin = undefined_productions_.erase(iter_begin);
    } else {
      ++iter_begin;
    }
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

void SyntaxGenerator::AddKeyWord(const std::string& key_word) {
  // 关键字优先级默认为2
  // 自动生成同名终结节点
  AddTerminalNode(
      key_word, key_word,
      frontend::generator::dfa_generator::DfaGenerator::WordPriority(2));
}

SyntaxGenerator::ProductionNodeId
SyntaxGenerator::GetProductionNodeIdFromBodySymbolId(SymbolId body_symbol_id) {
  auto iter = production_body_symbol_id_to_node_id_.find(body_symbol_id);
  if (iter != production_body_symbol_id_to_node_id_.end()) {
    return iter->second;
  } else {
    return ProductionNodeId::InvalidId();
  }
}

SyntaxGenerator::ProductionNodeId
SyntaxGenerator::GetProductionNodeIdFromNodeSymbol(
    const std::string& body_symbol) {
  SymbolId node_symbol_id = GetNodeSymbolId(body_symbol);
  if (node_symbol_id.IsValid()) {
    return GetProductionNodeIdFromNodeSymbolId(node_symbol_id);
  } else {
    return ProductionNodeId::InvalidId();
  }
}

SyntaxGenerator::ProductionNodeId
SyntaxGenerator::GetProductionNodeIdFromBodySymbol(
    const std::string& body_symbol) {
  SymbolId body_symbol_id = GetBodySymbolId(body_symbol);
  if (body_symbol_id.IsValid()) {
    return GetProductionNodeIdFromBodySymbolId(body_symbol_id);
  } else {
    return ProductionNodeId::InvalidId();
  }
}

inline SyntaxGenerator::ProductionNodeId
SyntaxGenerator::TerminalProductionNode::GetProductionNodeInBody(
    ProductionBodyId production_body_id, PointIndex point_index) const {
  assert(production_body_id == 0);
  if (point_index == 0) {
    return Id();
  } else {
    return ProductionNodeId::InvalidId();
  }
}

std::vector<SyntaxGenerator::ProductionBodyId>
SyntaxGenerator::NonTerminalProductionNode::GetAllBodyIds() const {
  std::vector<ProductionBodyId> production_body_ids;
  for (size_t i = 0; i < nonterminal_bodys_.size(); i++) {
    production_body_ids.push_back(ProductionBodyId(i));
  }
  return production_body_ids;
}

// 添加项所属的核心ID
// 要求不与已有的核心ID重复

inline void
SyntaxGenerator::NonTerminalProductionNode::AddCoreItemBelongToCoreId(
    ProductionBodyId body_id, PointIndex point_index, CoreId core_id) {
#ifdef _DEBUG
  const auto& core_ids_already_in =
      nonterminal_bodys_[body_id].cores_items_in_[point_index];
  for (auto core_id_already_in : core_ids_already_in) {
    assert(core_id != core_id_already_in);
  }
#endif  // _DEBUG
  nonterminal_bodys_[body_id].cores_items_in_[point_index].push_back(core_id);
}

inline SyntaxGenerator::ProductionNodeId
SyntaxGenerator::NonTerminalProductionNode::GetProductionNodeInBody(
    ProductionBodyId production_body_id, PointIndex point_index) const {
  assert(production_body_id < nonterminal_bodys_.size());
  if (point_index <
      nonterminal_bodys_[production_body_id].production_body.size()) {
    return nonterminal_bodys_[production_body_id].production_body[point_index];
  } else {
    return ProductionNodeId::InvalidId();
  }
}

inline SyntaxGenerator::ParsingTableEntry&
SyntaxGenerator::ParsingTableEntry::operator=(
    ParsingTableEntry&& parsing_table_entry) {
  action_and_attached_data_ =
      std::move(parsing_table_entry.action_and_attached_data_);
  nonterminal_node_transform_table_ =
      std::move(parsing_table_entry.nonterminal_node_transform_table_);
  return *this;
}

void SyntaxGenerator::ParsingTableEntry::ResetEntryId(
    const std::unordered_map<ParsingTableEntryId, ParsingTableEntryId>&
        old_entry_id_to_new_entry_id) {
  //处理终结节点的动作
  for (auto& action_and_attached_data : GetAllActionAndAttachedData()) {
    // 获取原条目ID的引用
    ShiftAttachedData& shift_attached_data =
        action_and_attached_data.second->GetShiftAttachedData();
    // 更新为新的条目ID
    shift_attached_data.SetNextParsingTableEntryId(
        old_entry_id_to_new_entry_id
            .find(shift_attached_data.GetNextParsingTableEntryId())
            ->second);
  }
  //处理非终结节点的转移
  for (auto& target : GetAllNonTerminalNodeTransformTarget()) {
    ParsingTableEntryId old_entry_id = target.second;
    ParsingTableEntryId new_entry_id =
        old_entry_id_to_new_entry_id.find(old_entry_id)->second;
    SetNonTerminalNodeTransformId(target.first, new_entry_id);
  }
}

SyntaxGenerator::BaseProductionNode&
SyntaxGenerator::BaseProductionNode::operator=(
    BaseProductionNode&& base_production_node) {
  base_type_ = std::move(base_production_node.base_type_);
  base_id_ = std::move(base_production_node.base_id_);
  base_symbol_id_ = std::move(base_production_node.base_symbol_id_);
  return *this;
}

SyntaxGenerator::OperatorProductionNode&
SyntaxGenerator::OperatorProductionNode::operator=(
    OperatorProductionNode&& operator_production_node) {
  BaseProductionNode::operator=(std::move(operator_production_node));
  operator_associatity_type_ =
      std::move(operator_production_node.operator_associatity_type_);
  operator_priority_level_ =
      std::move(operator_production_node.operator_priority_level_);
  return *this;
}

SyntaxGenerator::Core& SyntaxGenerator::Core::operator=(Core&& core) {
  core_closure_available_ = std::move(core.core_closure_available_);
  core_id_ = std::move(core.core_id_);
  parsing_table_entry_id_ = std::move(core.parsing_table_entry_id_);
  item_and_forward_node_ids_ = std::move(core.item_and_forward_node_ids_);
  return *this;
}

// 添加核心项
// 要求该核心项未添加过

inline void SyntaxGenerator::Core::SetMainItem(
    std::map<CoreItem, std::unordered_set<ProductionNodeId>>::const_iterator&
        item_iter) {
#ifdef _DEBUG
  // 不允许重复设置已有的核心项为核心项
  for (const auto& main_item_already_in : GetMainItems()) {
    assert(item_iter->first != main_item_already_in->first);
  }
#endif  // _DEBUG
  main_items_.emplace_back(item_iter);
  SetClosureNotAvailable();
}

bool SyntaxGenerator::ParsingTableEntry::ShiftAttachedData::operator==(
    const ActionAndAttachedDataInterface& shift_attached_data) const {
  return ActionAndAttachedDataInterface::operator==(shift_attached_data) &&
         next_entry_id_ ==
             static_cast<const ShiftAttachedData&>(shift_attached_data)
                 .next_entry_id_;
}

bool SyntaxGenerator::ParsingTableEntry::ReductAttachedData::operator==(
    const ActionAndAttachedDataInterface& reduct_attached_data) const {
  if (ActionAndAttachedDataInterface::operator==(reduct_attached_data))
      [[likely]] {
    const ReductAttachedData& real_type_reduct_attached_data =
        static_cast<const ReductAttachedData&>(reduct_attached_data);
    return reducted_nonterminal_node_id_ ==
               real_type_reduct_attached_data.reducted_nonterminal_node_id_ &&
           process_function_class_id_ ==
               real_type_reduct_attached_data.process_function_class_id_ &&
           production_body_ == real_type_reduct_attached_data.production_body_;
  } else {
    return false;
  }
}

bool SyntaxGenerator::ParsingTableEntry::ShiftReductAttachedData::operator==(
    const ActionAndAttachedDataInterface& shift_reduct_attached_data) const {
  return ActionAndAttachedDataInterface::operator==(
             shift_reduct_attached_data) &&
         shift_attached_data_ == static_cast<const ShiftReductAttachedData&>(
                                     shift_reduct_attached_data)
                                     .shift_attached_data_ &&
         reduct_attached_data_ == static_cast<const ShiftReductAttachedData&>(
                                      shift_reduct_attached_data)
                                      .reduct_attached_data_;
}

inline const SyntaxGenerator::ParsingTableEntry::ShiftAttachedData&
SyntaxGenerator::ParsingTableEntry::ActionAndAttachedDataInterface::
    GetShiftAttachedData() const {
  assert(false);
  // 防止警告
  return reinterpret_cast<const ShiftAttachedData&>(*this);
}

inline const SyntaxGenerator::ParsingTableEntry::ReductAttachedData&
SyntaxGenerator::ParsingTableEntry::ActionAndAttachedDataInterface::
    GetReductAttachedData() const {
  assert(false);
  // 防止警告
  return reinterpret_cast<const ReductAttachedData&>(*this);
}

inline const SyntaxGenerator::ParsingTableEntry::ShiftReductAttachedData&
SyntaxGenerator::ParsingTableEntry::ActionAndAttachedDataInterface::
    GetShiftReductAttachedData() const {
  assert(false);
  // 防止警告
  return reinterpret_cast<const ShiftReductAttachedData&>(*this);
}

inline bool SyntaxGenerator::ActionAndAttachedDataPointerEqualTo::operator()(
    const ParsingTableEntry::ActionAndAttachedDataInterface* const& lhs,
    const ParsingTableEntry::ActionAndAttachedDataInterface* const& rhs) const {
  if (lhs == nullptr) {
    if (rhs == nullptr) {
      return true;
    } else {
      return false;
    }
  } else if (rhs == nullptr) {
    return false;
  } else {
    return lhs->operator==(*rhs);
  }
}

}  // namespace frontend::generator::syntax_generator