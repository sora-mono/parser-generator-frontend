#include "Generator/LexicalGenerator/lexical_generator.h"

#include <fstream>
#include <queue>

#include "LexicalConfig/config_construct.cpp"
namespace frontend::generator::lexicalgenerator {
inline LexicalGenerator::ProductionNodeId LexicalGenerator::AddTerminalNode(
    const std::string& node_symbol, const std::string& body_symbol,
    frontend::generator::dfa_generator::DfaGenerator::TailNodePriority
        node_priority) {
  // 终结节点应与产生式体绑定，产生式名只是一个代号
  auto [node_symbol_id, node_symbol_inserted] = AddNodeSymbol(node_symbol);
  auto [body_symbol_id, body_symbol_inserted] = AddBodySymbol(body_symbol);
  ProductionNodeId production_node_id;
  if (body_symbol_inserted) {
    ProductionNodeId old_node_symbol_id =
        GetProductionNodeIdFromNodeSymbolId(node_symbol_id);
    if (old_node_symbol_id.IsValid()) {
      // 产生式体绑定的节点是即将添加的全新节点，产生式名绑定的节点一定与它不同
      printf("重定义终结产生式名：%s\n", node_symbol.c_str());
      return ProductionNodeId::InvalidId();
    }
    // 需要添加一个新的终结节点
    production_node_id = SubAddTerminalNode(node_symbol_id, body_symbol_id);
    frontend::generator::dfa_generator::DfaGenerator::SavedData saved_data;
    saved_data.id = production_node_id;
    saved_data.node_type = ProductionNodeType::kTerminalNode;
    saved_data.process_function_class_id = ProcessFunctionClassId::InvalidId();
    // 向DFA生成器注册关键词
    dfa_generator_.AddRegexpression(body_symbol, saved_data, node_priority);
  } else {
    // 该终结节点的内容已存在，不应重复添加
    production_node_id = GetProductionNodeIdFromBodySymbolId(body_symbol_id);
    assert(production_node_id.IsValid());
    if (node_symbol_inserted) {
      // 仅添加产生式名到节点的映射
      SetNodeSymbolIdToProductionNodeIdMapping(body_symbol_id,
                                               production_node_id);
    } else if (production_node_id != GetProductionNodeIdFromNodeSymbolId(
                                         node_symbol_id)) [[unlikely]] {
      // 节点名绑定的节点与产生式体绑定的节点不同，出现重定义错误
      fprintf(stderr, "重定义终结产生式名：%s\n", node_symbol.c_str());
    }
  }
  // 判断新添加的终结节点是否为某个未定义产生式
  CheckNonTerminalNodeCanContinue(node_symbol);
  return production_node_id;
}

inline LexicalGenerator::ProductionNodeId LexicalGenerator::SubAddTerminalNode(
    SymbolId node_symbol_id, SymbolId body_symbol_id) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<TerminalProductionNode>(node_symbol_id,
                                                           body_symbol_id);
  manager_nodes_.GetObject(node_id).SetThisNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  SetBodySymbolIdToProductionNodeIdMapping(body_symbol_id, node_id);
  return node_id;
}

#ifdef USE_AMBIGUOUS_GRAMMAR
inline LexicalGenerator::ProductionNodeId LexicalGenerator::AddOperatorNode(
    const std::string& operator_symbol, AssociatityType associatity_type,
    PriorityLevel priority_level, ProcessFunctionClassId class_id) {
  auto [operator_symbol_id, inserted] = AddBodySymbol(operator_symbol);
  assert(operator_symbol_id.IsValid());
  if (!inserted) [[unlikely]] {
    fprintf(stderr, "重定义运算符：%s\n", operator_symbol.c_str());
    return ProductionNodeId::InvalidId();
  }
  // 运算符产生式名与运算符相同
  SymbolId operator_node_symbol_id;
  std::pair(operator_node_symbol_id, inserted) = AddNodeSymbol(operator_symbol);
  if (!inserted) [[unlikely]] {
    fprintf(stderr, "运算符：%s的产生式名已被占用\n", operator_symbol.c_str());
    return ProductionNodeId::InvalidId();
  }
  assert(class_id.IsValid());
  ProductionNodeId operator_node_id = SubAddOperatorNode(
      operator_symbol_id, associatity_type, priority_level, class_id);
  assert(operator_node_id.IsValid());
  frontend::generator::dfa_generator::DfaGenerator::SavedData saved_data;
  saved_data.id = operator_node_id;
  saved_data.node_type = ProductionNodeType::kOperatorNode;
  saved_data.process_function_class_id = class_id;
  saved_data.associate_type = associatity_type;
  saved_data.priority = priority_level;
  // 向DFA生成器注册关键词
  dfa_generator_.AddKeyword(
      operator_symbol, saved_data,
      frontend::generator::dfa_generator::DfaGenerator::TailNodePriority(
          priority_level));
  return operator_node_id;
}

inline LexicalGenerator::ProductionNodeId LexicalGenerator::SubAddOperatorNode(
    SymbolId node_symbol_id, AssociatityType associatity_type,
    PriorityLevel priority_level, ProcessFunctionClassId class_id) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<OperatorProductionNode>(
          node_symbol_id, node_symbol_id, associatity_type, priority_level,
          class_id);
  manager_nodes_.GetObject(node_id).SetThisNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  SetBodySymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  return node_id;
}
#endif  // USE_AMBIGUOUS_GRAMMAR

inline LexicalGenerator::ProductionNodeId LexicalGenerator::AddNonTerminalNode(
    const std::string& node_symbol,
    const std::vector<std::pair<std::string, bool>>& subnode_symbols,
    ProcessFunctionClassId class_id) {
  assert(!node_symbol.empty() && !subnode_symbols.empty() &&
         class_id.IsValid());
  std::vector<ProductionNodeId> node_ids;
  // 非终结节点与节点名一一对应
  ProductionNodeId production_node_id =
      GetProductionNodeIdFromNodeSymbol(node_symbol);
  if (!production_node_id.IsValid()) {
    // 该终结节点名未注册
    SymbolId symbol_id = AddNodeSymbol(node_symbol).first;
    assert(symbol_id.IsValid());
    production_node_id = SubAddNonTerminalNode(symbol_id);
    // 检查添加的节点是否被前向依赖
    CheckNonTerminalNodeCanContinue(node_symbol);
  }
  for (auto& subnode_symbol : subnode_symbols) {
    // 将产生式体的所有产生式名转换为节点ID
    ProductionNodeId subproduction_node_id;
    if (subnode_symbol.second) [[unlikely]] {
      // 匿名终结节点
      SymbolId body_symbol_id = GetBodySymbolId(subnode_symbol.first);
      if (!body_symbol_id.IsValid()) {
        // 产生式体符号未注册，自动分配与产生式体符号同名的产生式名
        // 使用优先级0因为关键字的声明在另一个区域
        subproduction_node_id =
            AddTerminalNode(subnode_symbol.first, subnode_symbol.first,
                            frontend::generator::dfa_generator ::nfa_generator::
                                NfaGenerator::TailNodePriority(0));
        assert(subproduction_node_id.IsValid());
      } else {
        // 产生式体符号已注册
        subproduction_node_id =
            GetProductionNodeIdFromBodySymbolId(body_symbol_id);
        assert(subproduction_node_id.IsValid());
      }
    } else [[likely]] {
      if (subnode_symbol.first == "@") {
        // 空节点标记，设置该终结节点的允许空规约标记
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(production_node_id))
            .SetProductionCouldBeEmpty();
      } else {
        subproduction_node_id =
            GetProductionNodeIdFromNodeSymbol(subnode_symbol.first);
      }
      // 产生式名
      if (!subproduction_node_id.IsValid()) [[unlikely]] {
        // 产生式节点未定义
        // 添加待处理记录
        AddUnableContinueNonTerminalNode(subnode_symbol.first, node_symbol,
                                         subnode_symbols, class_id);
      }
    }
    node_ids.push_back(subproduction_node_id);
  }
  NonTerminalProductionNode& production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  ProductionBodyId body_id = production_node.AddBody(std::move(node_ids));
  production_node.SetBodyProcessFunctionClassId(body_id, class_id);
  return production_node_id;
}

inline LexicalGenerator::ProductionNodeId
LexicalGenerator::SubAddNonTerminalNode(SymbolId node_symbol_id) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<NonTerminalProductionNode>(node_symbol_id);
  manager_nodes_.GetObject(node_id).SetThisNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  return node_id;
}

// inline LexicalGenerator::ProductionNodeId LexicalGenerator::AddEndNode(
//    SymbolId symbol_id) {
//  ProductionNodeId node_id = manager_nodes_.EmplaceObject<EndNode>(symbol_id);
//  manager_nodes_.GetObject(node_id).SetThisNodeId(node_id);
//  return node_id;
//}

LexicalGenerator::ProductionNodeId
LexicalGenerator::GetProductionNodeIdFromNodeSymbol(SymbolId symbol_id) {
  assert(symbol_id.IsValid());
  auto iter = node_symbol_id_to_node_id_.find(symbol_id);
  assert(iter != node_symbol_id_to_node_id_.end());
  return iter->second;
}

inline void LexicalGenerator::SetItemCoreId(const CoreItem& core_item,
                                            CoreId core_id) {
  assert(core_id.IsValid());
  auto [production_node_id, production_body_id, point_index] = core_item;
  GetProductionNode(production_node_id)
      .SetCoreId(production_body_id, point_index, core_id);
}

inline std::pair<std::set<LexicalGenerator::CoreItem>::iterator, bool>
LexicalGenerator::AddItemToCore(const CoreItem& core_item, CoreId core_id) {
  assert(core_id.IsValid());
  auto iterator_and_state = GetCore(core_id).AddItem(core_item);
  if (iterator_and_state.second == true) {
    // 以前不存在则添加映射记录
    SetItemCoreId(core_item, core_id);
  }
  return iterator_and_state;
}

inline LexicalGenerator::CoreId LexicalGenerator::GetCoreId(
    ProductionNodeId production_node_id, ProductionBodyId production_body_id,
    PointIndex point_index) {
  assert(production_node_id.IsValid() && production_body_id.IsValid() &&
         point_index.IsValid());
  return GetProductionNode(production_node_id)
      .GetCoreId(production_body_id, point_index);
}

inline LexicalGenerator::CoreId LexicalGenerator::GetItemCoreId(
    const CoreItem& core_item) {
  auto [production_node_id, production_body_id, point_index] = core_item;
  return GetCoreId(production_node_id, production_body_id, point_index);
}

std::pair<LexicalGenerator::CoreId, bool>
LexicalGenerator::GetItemCoreIdOrInsert(const CoreItem& core_item,
                                        CoreId insert_core_id) {
  CoreId core_id = GetItemCoreId(core_item);
  if (!core_id.IsValid()) {
    if (insert_core_id.IsValid()) {
      core_id = insert_core_id;
    } else {
      core_id = AddNewCore();
    }
#ifdef _DEBUG
    bool result = AddItemToCore(core_item, core_id).second;
    // 必须插入
    assert(result == true);
#else
    AddItemToCore(core_item, core_id);
#endif  // _DEBUG
    return std::make_pair(core_id, true);
  } else {
    return std::make_pair(core_id, false);
  }
}

std::unordered_set<LexicalGenerator::ProductionNodeId>
LexicalGenerator::GetNonTerminalNodeFirstNodes(
    ProductionNodeId production_node_id,
    std::unordered_set<ProductionNodeId>&& processed_nodes) {
  if (!production_node_id.IsValid()) {
    return std::unordered_set<ProductionNodeId>();
  }
  NonTerminalProductionNode& production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  assert(production_node.Type() == ProductionNodeType::kNonTerminalNode);
  std::unordered_set<ProductionNodeId> return_set;
  for (auto& body : production_node.GetAllBody()) {
    // 正在处理的产生式节点的ID
    ProductionNodeId node_id = body.front();
    BaseProductionNode& node = GetProductionNode(node_id);
    if (node.Type() == ProductionNodeType::kNonTerminalNode) {
      return_set.merge(
          GetNonTerminalNodeFirstNodes(node_id, std::move(processed_nodes)));
    } else {
      return_set.insert(node_id);
    }
  }
  processed_nodes.insert(production_node_id);
  return return_set;
}

 std::unordered_set<LexicalGenerator::ProductionNodeId>
LexicalGenerator::First(
    ProductionNodeId production_node_id, ProductionBodyId production_body_id,
    PointIndex point_index,
    const std::unordered_set<ProductionNodeId>& next_node_ids) {
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
        node_id = GetProductionNodeIdInBody(production_node_id,
                                            production_body_id, point_index);
        // 合并非终结节点的所有向前看符号
        return_set.merge(GetNonTerminalNodeFirstNodes(node_id));
        if (static_cast<NonTerminalProductionNode&>(
                GetProductionNode(production_node_id))
                .CouldBeEmpty()) {
          // 该非终结节点可以空规约
          return_set.merge(First(production_node_id, production_body_id,
                                 ++point_index, next_node_ids));
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

void LexicalGenerator::CoreClosure(CoreId core_id) {
  if (GetCore(core_id).IsClosureAvailable()) {
    // 闭包有效，无需重求
    return;
  }
  std::queue<const CoreItem*> items_waiting_process;
  for (auto& item : GetCoreItems(core_id)) {
    items_waiting_process.push(&item);
  }
  while (!items_waiting_process.empty()) {
    const CoreItem* item_now = items_waiting_process.front();
    items_waiting_process.pop();
    auto [production_node_id, production_body_id, point_index] = *item_now;
    ProductionNodeId next_production_node_id = GetProductionBodyNextShiftNodeId(
        production_node_id, production_body_id, point_index);
    if (!next_production_node_id.IsValid()) {
      // 点已经在最后了，没有下一个节点
      continue;
    }
    NonTerminalProductionNode& next_production_node =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(next_production_node_id));
    if (next_production_node.Type() != ProductionNodeType::kNonTerminalNode) {
      // 不是非终结节点，无法展开
      continue;
    }
    const std::unordered_set<ProductionNodeId>& forward_nodes_second_part =
        GetForwardNodeIds(production_node_id, production_body_id, point_index);
    // 获取待生成节点的所有向前看节点
    std::unordered_set<ProductionNodeId> forward_nodes =
        First(production_node_id, production_body_id,
              point_index, forward_nodes_second_part);
    const BodyContainerType& bodys = next_production_node.GetAllBody();
    for (size_t i = 0; i < bodys.size(); i++) {
      // 将要展开的节点的每个产生式体加到该项集里，点在最左边
      ProductionBodyId body_id(i);
      PointIndex point_index(0);
      auto [iter_item, inserted] = AddItemToCore(
          CoreItem(next_production_node_id, body_id, point_index), core_id);
      AddForwardNodeContainer(next_production_node_id, body_id, point_index,
                              forward_nodes);
      if (inserted) {
        // 新插入了item，加入队列等待处理
        items_waiting_process.push(&*iter_item);
      }
    }
  }
  SetCoreClosureAvailable(core_id);
}

void LexicalGenerator::SetGotoCacheEntry(
    CoreId core_id_src, ProductionNodeId transform_production_node_id,
    CoreId core_id_dst) {
  assert(core_id_src.IsValid() && core_id_dst.IsValid());
  auto [iter_begin, iter_end] = core_id_goto_core_id_.equal_range(core_id_src);
  for (; iter_begin != iter_end; ++iter_end) {
    // 如果已有缓存项则修改
    if (iter_begin->second.first == transform_production_node_id) {
      iter_begin->second.second = core_id_dst;
      return;
    }
  }
  // 不存在该条缓存则插入，一条entry可以对应多个目标
  core_id_goto_core_id_.insert(std::make_pair(
      core_id_src, std::make_pair(transform_production_node_id, core_id_dst)));
}

LexicalGenerator::CoreId LexicalGenerator::GetGotoCacheEntry(
    CoreId core_id_src, ProductionNodeId transform_production_node_id) {
  assert(core_id_src.IsValid());
  CoreId core_id_dst = CoreId::InvalidId();
  auto [iter_begin, iter_end] = core_id_goto_core_id_.equal_range(core_id_src);
  for (; iter_begin != iter_end; ++iter_begin) {
    if (iter_begin->second.first == transform_production_node_id) {
      core_id_dst = iter_begin->second.second;
      break;
    }
  }
  return core_id_dst;
}

inline LexicalGenerator::ProductionNodeId
LexicalGenerator::GetProductionBodyNextShiftNodeId(
    ProductionNodeId production_node_id, ProductionBodyId production_body_id,
    PointIndex point_index) {
  return GetProductionNodeIdInBody(production_node_id, production_body_id,
                                   point_index);
}

inline LexicalGenerator::ProductionNodeId
LexicalGenerator::GetProductionBodyNextNextNodeId(
    ProductionNodeId production_node_id, ProductionBodyId production_body_id,
    PointIndex point_index) {
  return GetProductionNodeIdInBody(production_node_id, production_body_id,
                                   PointIndex(point_index + 1));
}

std::pair<LexicalGenerator::CoreId, bool> LexicalGenerator::ItemGoto(
    const CoreItem& item, ProductionNodeId transform_production_node_id,
    CoreId insert_core_id) {
  assert(transform_production_node_id.IsValid());
  auto [item_production_node_id, item_production_body_id, item_point_index] =
      item;
  ProductionNodeId next_production_node_id = GetProductionBodyNextShiftNodeId(
      item_production_node_id, item_production_body_id, item_point_index);
  while (next_production_node_id.IsValid()) {
    if (manager_nodes_.IsSame(next_production_node_id,
                              transform_production_node_id)) {
      // 有可移入节点且该节点是给定的节点
      // 返回移入后的项集ID和是否创建新项集标记
      return GetItemCoreIdOrInsert(
          CoreItem(item_production_node_id, item_production_body_id,
                   ++item_point_index),
          insert_core_id);
    } else {
      NonTerminalProductionNode& production_node =
          static_cast<NonTerminalProductionNode&>(
              GetProductionNode(next_production_node_id));
      if (production_node.Type() == ProductionNodeType::kNonTerminalNode &&
          production_node.CouldBeEmpty()) {
        // 待移入的节点可以空规约，考虑它的下一个节点
        next_production_node_id = GetProductionBodyNextShiftNodeId(
            item_production_node_id, item_production_body_id,
            ++item_point_index);
      } else {
        // 待移入节点不可以空规约且不是给定的节点，返回无效信息
        break;
      }
    }
  }
  return std::make_pair(CoreId::InvalidId(), false);
}

std::pair<LexicalGenerator::CoreId, bool> LexicalGenerator::Goto(
    CoreId core_id_src, ProductionNodeId transform_production_node_id) {
  // Goto缓存是否有效
  bool map_cache_available = IsGotoCacheAvailable(core_id_src);
  // 缓存有效
  if (map_cache_available) {
    return std::make_pair(
        GetGotoCacheEntry(core_id_src, transform_production_node_id), false);
  }
  // 缓存无效，清除该core_id对应所有的缓存
  RemoveGotoCacheEntry(core_id_src);
  // 是否新建core标志
  bool core_constructed = false;
  CoreId core_id_dst = CoreId::InvalidId();
  CoreClosure(core_id_src);
  const auto& items = GetCoreItems(core_id_src);
  auto iter = items.begin();
  // 找到第一个成功Goto的项并记录CoreId和是否新建了Core
  while (iter != items.end()) {
    std::pair(core_id_dst, core_constructed) =
        ItemGoto(*iter, transform_production_node_id);
    if (core_id_dst.IsValid()) {
      break;
    }
    ++iter;
  }
  if (core_constructed) {
    // 创建了新的项集，需要对剩下的项都做ItemGoto，将可以插入的项插入到新的项集
    assert(core_id_dst.IsValid());
    while (iter != items.end()) {
#ifdef _DEBUG
      // 测试是否每一个项Goto结果都是相同的
      auto [core_id, constructed] =
          ItemGoto(*iter, transform_production_node_id, core_id_dst);
      assert(!core_id.IsValid() ||
             core_id == core_id_dst && constructed == true);
#else
      ItemGoto(*iter, transform_production_node_id, core_id_dst);
#endif  // _DEBUG
      ++iter;
    }
  }
#ifdef _DEBUG
  // 测试是否每一个项Goto结果都是相同的
  else {
    while (iter != items.end()) {
      auto [core_id, constructed] =
          ItemGoto(*iter, transform_production_node_id, core_id_dst);
      assert(core_id == core_id_dst && constructed == false);
      ++iter;
    }
  }
#endif  // _DEBUG
  // 设置缓存
  SetGotoCacheEntry(core_id_src, transform_production_node_id, core_id_dst);
  return std::make_pair(core_id_dst, core_constructed);
}

void LexicalGenerator::SpreadLookForwardSymbol(CoreId core_id) {
  const auto& items = GetCoreItems(core_id);
  for (auto& item : items) {
    auto [production_node_id, production_body_id, point_index] = item;
    // 项集对每一个可能的下一个符号都执行一次Goto，使得在传播向前看符号前
    // 所有可能用到的的项都有某个对应的项集
    ProductionNodeId next_production_node_id = GetProductionBodyNextShiftNodeId(
        production_node_id, production_body_id, point_index);
    if (next_production_node_id.IsValid()) {
      Goto(core_id, next_production_node_id);
    }
  }
  for (auto& item : items) {
    auto [production_node_id, production_body_id, point_index] = item;
    ProductionNodeId next_production_node_id = GetProductionBodyNextShiftNodeId(
        production_node_id, production_body_id, point_index);
    if (next_production_node_id.IsValid()) {
      std::unordered_set<ProductionNodeId> forward_nodes = GetForwardNodeIds(
          production_node_id, production_body_id, point_index);
      // 传播向前看符号
      AddForwardNodeContainer(production_node_id, production_body_id,
                              ++point_index, forward_nodes);
    }
  }
}

std::array<std::vector<LexicalGenerator::ProductionNodeId>,
           sizeof(LexicalGenerator::ProductionNodeType)>
LexicalGenerator::ClassifyProductionNodes() {
  std::array<std::vector<ProductionNodeId>, sizeof(ProductionNodeType)>
      production_nodes;
  ObjectManager<BaseProductionNode>::Iterator iter = manager_nodes_.Begin();
  while (iter != manager_nodes_.End()) {
    production_nodes[static_cast<size_t>(iter->Type())].push_back(iter->Id());
    ++iter;
  }
  return production_nodes;
}

inline LexicalGenerator::ParsingTableEntryId
LexicalGenerator::GetParsingEntryIdCoreId(CoreId core_id) {
  auto iter = core_id_to_parsing_table_entry_id_.find(core_id);
  if (iter != core_id_to_parsing_table_entry_id_.end()) {
    return iter->second;
  } else {
    return ParsingTableEntryId::InvalidId();
  }
}

std::vector<std::vector<LexicalGenerator::ParsingTableEntryId>>
LexicalGenerator::ParsingTableEntryClassify(
    const std::vector<ProductionNodeId>& terminal_node_ids,
    const std::vector<ProductionNodeId>& nonterminal_node_ids) {
  // 存储相同终结节点转移表的条目
  std::vector<std::vector<ParsingTableEntryId>> terminal_classify_result,
      final_classify_result;
  std::vector<ParsingTableEntryId> entry_ids;
  entry_ids.reserve(lexical_config_parsing_table_.size());
  for (size_t i = 0; i < lexical_config_parsing_table_.size(); i++) {
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

void LexicalGenerator::RemapParsingTableEntryId(
    const std::unordered_map<ParsingTableEntryId, ParsingTableEntryId>&
        moved_entry_to_new_entry_id) {
  for (auto& entry : lexical_config_parsing_table_) {
    //处理终结节点的动作
    for (auto& action_and_target : entry.GetAllTerminalNodeActionAndTarget()) {
      ParsingTableEntryId old_entry_id, new_entry_id;
      switch (action_and_target.second.first) {
        case ActionType::kAccept:
        case ActionType::kError:
        case ActionType::kReduction:
          break;
        case ActionType::kShift:
          old_entry_id = *std::get_if<ParsingTableEntryId>(
              &action_and_target.second.second);
          new_entry_id = moved_entry_to_new_entry_id.find(old_entry_id)->second;
          // 更新为新的条目ID
          entry.SetTerminalNodeActionAndTarget(action_and_target.first,
                                               action_and_target.second.first,
                                               new_entry_id);
          break;
        default:
          assert(false);
          break;
      }
    }
    //处理非终结节点的转移
    for (auto& target : entry.GetAllNonTerminalNodeTransformTarget()) {
      ParsingTableEntryId old_entry_id = target.second;
      ParsingTableEntryId new_entry_id =
          moved_entry_to_new_entry_id.find(old_entry_id)->second;
      entry.SetNonTerminalNodeTransformId(target.first, new_entry_id);
    }
  }
}

void LexicalGenerator::ParsingTableMergeOptimize() {
  std::array<std::vector<ProductionNodeId>, sizeof(ProductionNodeType)>
      classified_production_node_ids;
  size_t terminal_index =
      static_cast<size_t>(ProductionNodeType::kTerminalNode);
  size_t operator_index =
      static_cast<size_t>(ProductionNodeType::kOperatorNode);
  size_t nonterminal_index =
      static_cast<size_t>(ProductionNodeType::kNonTerminalNode);
  for (auto id : classified_production_node_ids[operator_index]) {
    // 将所有运算符类节点添加到终结节点表里，构成完整的终结节点表
    classified_production_node_ids[terminal_index].push_back(id);
  }
  std::vector<std::vector<ParsingTableEntryId>> classified_ids =
      ParsingTableEntryClassify(
          classified_production_node_ids[terminal_index],
          classified_production_node_ids[nonterminal_index]);
  // 存储被删除的旧条目到相同条目的映射
  std::unordered_map<ParsingTableEntryId, ParsingTableEntryId>
      old_entry_id_to_new_entry_id;
  for (auto& entry_ids : classified_ids) {
    // 添加被删除的旧条目到相同条目的映射
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
  assert(lexical_config_parsing_table_.size() > 0);
  // 下一个要处理的条目
  ParsingTableEntryId index_next_process_entry(0);
  // 下一个插入位置
  ParsingTableEntryId index_next_insert(0);
  // 类似快排分类的算法
  // 寻找被合并的条目
  while (index_next_process_entry < lexical_config_parsing_table_.size()) {
    if (old_entry_id_to_new_entry_id.find(index_next_process_entry) ==
        old_entry_id_to_new_entry_id.end()) {
      // 该条目保留
      if (index_next_insert != index_next_process_entry) {
        // 需要移动
        lexical_config_parsing_table_[index_next_insert] =
            std::move(lexical_config_parsing_table_[index_next_process_entry]);
      }
      // 重映射保留条目的新位置
      moved_entry_to_new_entry_id[index_next_process_entry] = index_next_insert;
      ++index_next_insert;
    }
    ++index_next_process_entry;
  }
  for (auto& item : old_entry_id_to_new_entry_id) {
    // 重映射已删除条目到新条目
    assert(moved_entry_to_new_entry_id.find(item.second) !=
           moved_entry_to_new_entry_id.end());
    moved_entry_to_new_entry_id[item.first] =
        moved_entry_to_new_entry_id[item.second];
  }
  // 至此每一个条目都有了新条目的映射
  // 释放多余空间
  old_entry_id_to_new_entry_id.clear();
  lexical_config_parsing_table_.resize(index_next_insert + 1);
  // 将所有旧ID更新为新ID
  RemapParsingTableEntryId(moved_entry_to_new_entry_id);
}

void LexicalGenerator::AnalysisKeyWord(const std::string& str) {
  std::sregex_iterator iter(str.cbegin(), str.cend(), keyword_regex_);
  std::sregex_iterator iter_end;
  FILE* config_file = GetConfigConstructFilePointer();
  for (; iter != iter_end; ++iter) {
    std::string keyword = iter->str(1);
    fprintf(config_file, "// 关键字：%s\n", keyword.c_str());
    fprintf(config_file, "PrintKeyWordConstructData(\"%s\")", keyword.c_str());
  }
}

void LexicalGenerator::AnalysisProductionConfig(const std::string& file_name) {
  // 不支持换行功能
  std::ifstream file(file_name);
  if (!file.is_open()) {
    fprintf(stderr, "打开产生式配置文件：%s 失败\n", file_name.c_str());
    return;
  }
  std::string temp;
  // 当前功能区编号
  size_t part_num = 0;
  while (part_num < frontend::common::kFunctionPartSize) {
    std::getline(file, temp);
    if (!file) {
      break;
    }
    if (temp.empty()) {
      continue;
    }
    if (temp[0] == '@') [[unlikely]] {
      // 特殊标记，1个@代表注释，2个@代表功能区域分隔
      if (temp.size() >= 2 && temp[1] == '@') {
        // 该功能区（基础终结节点）结束
        ++part_num;
        break;
      } else {
        continue;
      }
    }
    switch (part_num) {
      case 0:
        // 关键字定义区
        AnalysisKeyWord(temp);
        break;
      case 1:
        // 基础终结节点定义区
        AnalysisTerminalProduction(temp);
        break;
      case 2:
        // 运算符定义区
        AnalysisOperatorProduction(temp);
        break;
      case 3:
        // 普通产生式定义区
        AnalysisNonTerminalProduction(temp);
      default:
        break;
    }
  }
}

void LexicalGenerator::AnalysisTerminalProduction(const std::string& str,
                                                  size_t priority) {
  std::smatch match_result;
  std::regex_match(str, match_result, terminal_node_regex_);
  if (match_result.size() < 3) [[unlikely]] {
    fprintf(stderr, "终结产生式：%s 不符合语法规范\n", str.c_str());
    return;
  }
  // 产生式名
  std::string node_symbol = match_result[1].str();
  // 产生式体
  std::string body_symbol = match_result[2].str();
  if (node_symbol.empty()) [[unlikely]] {
    fprintf(stderr, "终结产生式：%s 缺少产生式名\n", str.c_str());
    return;
  }
  if (body_symbol.empty()) [[unlikely]] {
    fprintf(stderr, "终结产生式：%s 缺少产生式体\n", str.c_str());
    return;
  }
  PrintTerminalNodeConstructData(std::move(node_symbol), std::move(body_symbol),
                                 priority);
}

#ifdef USE_AMBIGUOUS_GRAMMAR
void LexicalGenerator::AnalysisOperatorProduction(const std::string& str) {
  std::smatch match_result;
  std::regex_match(str, match_result, operator_node_regex_);
  if (match_result.size() == 0) {
    fprintf(stderr, "运算符声明不符合规范：%s\n", str.c_str());
    return;
  }
  OperatorData operator_data;
  // 运算符名
  operator_data.operator_symbol = match_result[1].str();
  if (operator_data.operator_symbol.empty()) {
    fprintf(stderr, "运算符名缺失\n");
    return;
  }
  // 运算符优先级
  operator_data.priority = match_result[2].str();
  if (operator_data.priority.empty()) {
    fprintf(stderr, "运算符：%s 优先级缺失\n", operator_data.priority.c_str());
    return;
  }
  // 运算符结合性
  operator_data.associatity_type = match_result[3].str();
  if (operator_data.associatity_type.empty()) {
    fprintf(stderr, "运算符：%s 运算符结合性缺失\n",
            operator_data.operator_symbol.c_str());
    return;
  }

#ifdef USE_INIT_FUNCTION
  // 运算符初始化函数
  operator_data.init_function = match_result[4].str();
#endif  // USE_INIT_FUNCTION

#ifdef USE_SHIFT_FUNCTION
  operator_data.shift_functions = GetFunctionsName(match_result[5].str());
#endif  // USE_SHIFT_FUNCTION

#ifdef USE_REDUCT_FUNCTION
  operator_data.reduct_function = match_result[6].str();
#endif  // USE_REDUCT_FUNCTION

#ifdef USE_USER_DEFINED_FILE
  operator_data.include_files = GetFilesName(match_result[7].str());
#endif  // USE_USER_DEFINED_FILE
  PrintOperatorNodeConstructData(std::move(operator_data));
}
#endif  // USE_AMBIGUOUS_GRAMMAR

void lexicalgenerator::LexicalGenerator::AnalysisNonTerminalProduction(
    const std::string& str) {
  std::smatch match_result;
  std::regex_match(str, match_result, nonterminal_node_regex_);
  if (match_result.size() == 0) {
    // 未捕获
    fprintf(stderr, "非终结节点：%s 声明不规范\n", str.c_str());
    return;
  }
  NonTerminalNodeData node_data;
  node_data.node_symbol = match_result[1].str();
  std::string body_symbol_str = match_result[2].str();
  node_data.body_symbols = GetBodySymbol(body_symbol_str);
  if (body_symbol_str.find('|') != std::string::npos) {
    node_data.use_same_process_function_class = true;
  } else {
    node_data.use_same_process_function_class = false;
  }
#ifdef USE_INIT_FUNCTION
  node_data.init_function = match_result[3].str();
#endif  // USE_INIT_FUNCTION
#ifdef USE_SHIFT_FUNCTION
  node_data.shift_functions = GetFunctionsName(match_result[4].str());
#endif  // USE_SHIFT_FUNCTION
#ifdef USE_REDUCT_FUNCTION
  node_data.reduct_function = match_result[5].str();
#endif  // USE_REDUCT_FUNCTION
#ifdef USE_USER_DEFINED_FILE
  node_data.include_files = GetFilesName(match_result[6].str());
#endif  // USE_USER_DEFINED_FILE
  PrintNonTerminalNodeConstructData(std::move(node_data));
}

void LexicalGenerator::ParsingTableConstruct() {
  // 为每个有效项创建语法分析表条目
  // 使用队列存储添加动作和目标节点的任务，全部处理完后再添加
  // 防止移入操作转移到的节点相应分析表条目还没被创建
  // 队列中存储指向需要添加转移条件的语法分析表条目的指针，转移到的核心ID和
  // 作为转移条件的产生式ID
  std::queue<std::tuple<ParsingTableEntry*, CoreId, ProductionNodeId>>
      node_action_waiting_set;
  std::unordered_map<CoreId, ParsingTableEntryId> core_id_to_entry_id;
  for (auto& core : cores_) {
    // 为每个项集分配语法分析表的一个条目
    ParsingTableEntryId entry_id(lexical_config_parsing_table_.size());
    ParsingTableEntry& entry = lexical_config_parsing_table_.emplace_back();
    core_id_to_entry_id[core.GetCoreId()] = entry_id;
    SetCoreIdToParsingEntryIdMapping(core.GetCoreId(), entry_id);
    for (auto& item : core.GetItems()) {
      auto [production_node_id, production_body_id, point_index] = item;
      // 下一个移入的产生式节点ID
      ProductionNodeId next_shift_node_id = GetProductionBodyNextShiftNodeId(
          production_node_id, production_body_id, point_index);
      if (next_shift_node_id.IsValid()) {
        // 可以移入一个节点，应执行移入操作
        assert(GetProductionNode(next_shift_node_id).Type() !=
               ProductionNodeType::kEndNode);
        CoreId next_core_id =
            GetCoreId(production_node_id, production_body_id, ++point_index);
        assert(next_core_id.IsValid());
        node_action_waiting_set.push(
            std::make_tuple(&entry, next_core_id, next_shift_node_id));
      } else {
        // 无可移入节点，对所有向前看节点执行规约
        for (ProductionNodeId forward_node_id : GetForwardNodeIds(
                 production_node_id, production_body_id, point_index)) {
          ProcessFunctionClassId class_id =
              GetProcessFunctionClass(production_node_id, production_body_id);
          entry.SetTerminalNodeActionAndTarget(
              forward_node_id, ActionType::kReduction,
              std::make_pair(production_node_id, class_id));
        }
      }
    }
  }
  // 处理队列中的所有任务
  while (!node_action_waiting_set.empty()) {
    auto [entry, next_core_id, next_shift_node_id] =
        node_action_waiting_set.front();
    node_action_waiting_set.pop();
    ParsingTableEntryId target_entry_id = GetParsingEntryIdCoreId(next_core_id);
    assert(target_entry_id.IsValid());
    switch (GetProductionNode(next_shift_node_id).Type()) {
      case ProductionNodeType::kNonTerminalNode:
        entry->SetNonTerminalNodeTransformId(next_shift_node_id,
                                             target_entry_id);
        break;
      case ProductionNodeType::kTerminalNode:
      case ProductionNodeType::kOperatorNode:
        entry->SetTerminalNodeActionAndTarget(
            next_shift_node_id, ActionType::kShift, target_entry_id);
        break;
      default:
        assert(false);
        break;
    }
  }
  ParsingTableMergeOptimize();
}

void LexicalGenerator::OpenConfigFile() {
  FILE* function_class_file_ptr = GetProcessFunctionClassFilePointer();
  if (function_class_file_ptr == nullptr) [[likely]] {
    errno_t result = fopen_s(&GetProcessFunctionClassFilePointer(),
                             "LexicalConfig/process_functions_classes.h", "w+");
    if (result != 0) [[unlikely]] {
      fprintf(stderr, "打开LexicalConfig/process_functions.h失败，错误码：%d\n",
              result);
    }
    fprintf(
        function_class_file_ptr,
        "#ifndef GENERATOR_LEXICALGENERATOR_LEXICALCONFIG_PROCESSFUNCTIONS_H_\n"
        "#define "
        "GENERATOR_LEXICALGENERATOR_LEXICALCONFIG_PROCESSFUNCTIONS_H_\n");
  }
  FILE* config_file_ptr = GetConfigConstructFilePointer();
  if (config_file_ptr == nullptr) [[likely]] {
    errno_t result = fopen_s(&GetConfigConstructFilePointer(),
                             "LexicalConfig/config_construct.cpp", "w+");
    if (result != 0) [[unlikely]] {
      fprintf(stderr,
              "打开LexicalConfig/config_construct.cpp失败，错误码：%d\n",
              result);
    }
    fprintf(config_file_ptr,
            "namespace frontend::generator::lexicalgenerator {\n"
            "void LexicalGenerator::ConfigConstruct() { \n");
  }
}

void LexicalGenerator::CloseConfigFile() {
  FILE* function_class_file_ptr = GetProcessFunctionClassFilePointer();
  if (function_class_file_ptr != nullptr) [[likely]] {
    fprintf(function_class_file_ptr,
            "#endif  // "
            "!GENERATOR_LEXICALGENERATOR_LEXICALCONFIG_PROCESSFUNCTIONS_H_");
    fclose(function_class_file_ptr);
  }
  FILE* config_file_ptr = GetConfigConstructFilePointer();
  if (config_file_ptr != nullptr) [[likely]] {
    // 检查是否有未定义产生式
    fprintf(config_file_ptr, "CheckUndefinedProductionRemained();\n");
    fprintf(config_file_ptr,
            "}\n"
            "}  // namespace frontend::generator::lexicalgenerator");
    fclose(config_file_ptr);
  }
}

std::vector<std::pair<std::string, bool>> LexicalGenerator::GetBodySymbol(
    const std::string& str) {
  std::sregex_iterator regex_iter(str.cbegin(), str.cend(), body_symbol_regex_);
  std::vector<std::pair<std::string, bool>> bodys;
  std::sregex_iterator regex_iter_end;
  for (; regex_iter != regex_iter_end; ++regex_iter) {
    bool is_terminal_symbol = *regex_iter->str(1).cbegin() == '"';
    bodys.emplace_back(std::make_pair(regex_iter->str(2), is_terminal_symbol));
  }
  return bodys;
}

std::vector<std::string> LexicalGenerator::GetFunctionsName(
    const std::string& str) {
  std::sregex_iterator regex_iter(str.cbegin(), str.cend(), function_regex_);
  std::vector<std::string> functions;
  std::sregex_iterator regex_iter_end;
  for (; regex_iter != regex_iter_end; ++regex_iter) {
    std::string function_name = regex_iter->str(1);
    if (function_name == "@") {
      // 使用占位符表示空函数
      function_name.clear();
    }
    functions.emplace_back(std::move(function_name));
  }
  return functions;
}

std::vector<std::string> LexicalGenerator::GetFilesName(
    const std::string& str) {
  std::sregex_iterator regex_iter(str.cbegin(), str.cend(), file_regex_);
  std::vector<std::string> include_files;
  std::sregex_iterator regex_iter_end;
  for (; regex_iter != regex_iter_end; ++regex_iter) {
    std::string include_file_name = regex_iter->str(1);
    if (include_file_name == "@") {
      // 使用占位符代表没有包含文件
      break;
    }
    include_files.emplace_back(std::move(include_file_name));
  }
  return include_files;
}

void LexicalGenerator::AddUnableContinueNonTerminalNode(
    const std::string& undefined_symbol, const std::string& node_symbol,
    const std::vector<std::pair<std::string, bool>>& subnode_symbols,
    ProcessFunctionClassId class_id) {
  // 使用insert因为可能有多个产生式依赖同一个未定义的产生式
  undefined_productions_.insert(
      std::make_pair(undefined_symbol,
                     std::make_tuple(node_symbol, subnode_symbols, class_id)));
}

void LexicalGenerator::CheckNonTerminalNodeCanContinue(
    const std::string& node_symbol) {
  if (!undefined_productions_.empty()) {
    auto [iter_begin, iter_end] =
        undefined_productions_.equal_range(node_symbol);
    if (iter_begin != undefined_productions_.end()) {
      for (; iter_begin != iter_end; ++iter_begin) {
        auto& [node_symbol, node_body_ptr, process_function_class_id] =
            iter_begin->second;
        AddNonTerminalNode(node_symbol, node_body_ptr,
                           process_function_class_id);
      }
      // 删除待添加的记录
      undefined_productions_.erase(iter_begin, iter_end);
    }
  }
}

void LexicalGenerator::CheckUndefinedProductionRemained() {
  if (!undefined_productions_.empty()) {
    // 仍存在未定义产生式
    for (auto& item : undefined_productions_) {
      auto& [node_symbol, node_bodys, class_id] = item.second;
      fprintf(stderr, "产生式：%s\n产生式体：", node_symbol.c_str());
      for (auto& body : node_bodys) {
        fprintf(stderr, "%s ", body.first.c_str());
      }
      fprintf(stderr, "中\n产生式：%s 未定义\n", item.first.c_str());
    }
  }
}

inline void LexicalGenerator::AddKeyWord(const std::string& key_word) {
  // 关键字优先级默认为1
  // 自动生成同名终结节点
  AddTerminalNode(
      key_word, key_word,
      frontend::generator::dfa_generator::DfaGenerator::TailNodePriority(1));
}

void LexicalGenerator::PrintKeyWordConstructData(const std::string& keyword) {
  fprintf(GetConfigConstructFilePointer(), "// 关键字：%s\n", keyword.c_str());
  fprintf(GetConfigConstructFilePointer(), "AddKeyWord(%s);\n",
          keyword.c_str());
}

void LexicalGenerator::PrintTerminalNodeConstructData(std::string&& node_symbol,
                                                      std::string&& body_symbol,
                                                      size_t priority) {
  fprintf(GetConfigConstructFilePointer(),
          "// 终结节点名：%s\n// 终结节点体：%s\n // 优先级：%llu",
          node_symbol.c_str(), body_symbol.c_str(),
          static_cast<unsigned long long>(priority));
  fprintf(GetConfigConstructFilePointer(),
          "AddTerminalNode(%s,%s,PriorityLevel(%s));\n", node_symbol.c_str(),
          body_symbol.c_str(), std::to_string(priority).c_str());
}

#ifdef USE_AMBIGUOUS_GRAMMAR
void LexicalGenerator::PrintOperatorNodeConstructData(OperatorData&& data) {
  // 分配一个编号
  int operator_num = GetNodeNum();
  std::string class_name = class_name + std::to_string(operator_num) +
                           data.associatity_type + data.priority;
  // 重新编码输出的结合性字符串
  if (data.associatity_type == "L") {
    data.associatity_type = "AssociatityType::kLeftAssociate";
  } else {
    data.associatity_type = "AssociatityType::kRightAssociate";
  }
  fprintf(GetConfigConstructFilePointer(),
          "// 运算符： %s\n// 结合性：%s\n// 优先级：%s\n",
          data.operator_symbol.c_str(), data.associatity_type.c_str(),
          data.priority.c_str());
  fprintf(
      GetConfigConstructFilePointer(),
      "AddOperatorNode<%s>(\"%s\",AssociatityType(%s),PriorityLevel(%s));\n",
      class_name.c_str(), data.operator_symbol.c_str(),
      data.associatity_type.c_str(), data.priority.c_str());
  FILE* function_file = GetProcessFunctionClassFilePointer();
  fprintf(function_file,
          "class %s : public ProcessFunctionInterface {\n public:\n",
          class_name.c_str());
  fprintf(function_file, "// 运算符： %s\n// 结合性：%s\n// 优先级：%s\n",
          data.operator_symbol.c_str(), data.associatity_type.c_str(),
          data.priority.c_str());
  PrintProcessFunction(function_file, data);
  fprintf(function_file, " private:\n");
#ifdef USE_USER_DEFINED_FILE
  for (size_t i = 0; i < data.include_files.size(); i++) {
    fprintf(function_file, "  #include\"%s\"\n", data.include_files[i].c_str());
  }
#endif  // USE_USER_DEFINED_FILE
  fprintf(function_file, "}\n");
}

#endif  // USE_AMBIGUOUS_GRAMMAR

void LexicalGenerator::PrintNonTerminalNodeConstructData(
    NonTerminalNodeData&& data) {
  int node_num = GetNodeNum();
  std::string class_name = data.node_symbol + '_' + std::to_string(node_num);
  FILE* function_file = GetProcessFunctionClassFilePointer();
  fprintf(function_file, "class %s :public ProcessFunctionInterface {\n",
          class_name.c_str());
  fprintf(function_file, "// 非终结节点名：%s\n", data.node_symbol.c_str());
  PrintProcessFunction(function_file, data);
  fprintf(function_file, " private:\n");
#ifdef USE_USER_DEFINED_FILE
  for (size_t i = 0; i < data.include_files.size(); i++) {
    fprintf(function_file, "  #include\"%s\"\n", data.include_files[i].c_str());
  }
#endif  // USE_USER_DEFINED_FILE
  fprintf(function_file, "}\n");

  FILE* config_file = GetConfigConstructFilePointer();
  if (data.use_same_process_function_class) {
    // 该非终结节点存储多个终结节点且共用处理函数
    // 创建一个独一无二的变量名
    std::string variety_name =
        class_name + std::to_string('_') + std::to_string(node_num);
    fprintf(config_file,
            "ProcessFunctionClassId %s = "
            "CreateProcessFunctionClassObject<%s>();\n",
            variety_name.c_str(), class_name.c_str());
    for (size_t i = 0; i < data.body_symbols.size(); i++) {
      if (!data.body_symbols[i].second) {
        fprintf(stderr,
                "非终结节点：%s 中，第%llu个产生式体不是终结节点产生式\n",
                data.node_symbol.c_str(), static_cast<unsigned long long>(i));
      }
      fprintf(config_file, "AddNonTerminalNode(\"%s\",{{\"%s\",true}},%s);\n",
              data.node_symbol.c_str(), data.body_symbols[i].first.c_str(),
              variety_name.c_str());
    }
  } else {
    // 正常处理非终结节点的一个产生式体
    if (data.body_symbols.empty()) [[unlikely]] {
      fprintf(stderr, "非终结节点：%s 没有产生式体\n",
              data.node_symbol.c_str());
      return;
    }
    // 输出使用初始化列表表示的每个产生式体
    std::string is_terminal_body =
        data.body_symbols.front().second ? "true" : "false";
    fprintf(config_file, "AddNonTerminalNode<%s>(\"%s\",{{\"%s\",%s}",
            class_name.c_str(), data.node_symbol.c_str(),
            data.body_symbols.front().first.c_str(), is_terminal_body.c_str());
    for (size_t i = 1; i < data.body_symbols.size(); i++) {
      std::string is_terminal_body =
          data.body_symbols[i].second ? "true" : "false";
      fprintf(config_file, ",{\"%s\",%s}", data.body_symbols[i].first.c_str(),
              is_terminal_body.c_str());
    }
    fprintf(config_file, "};\n");
  }
}

LexicalGenerator::ProductionNodeId
LexicalGenerator::GetProductionNodeIdFromBodySymbolId(SymbolId body_symbol_id) {
  auto iter = production_body_symbol_id_to_node_id_.find(body_symbol_id);
  if (iter != production_body_symbol_id_to_node_id_.end()) {
    return iter->second;
  } else {
    return ProductionNodeId::InvalidId();
  }
}

LexicalGenerator::ProductionNodeId
LexicalGenerator::GetProductionNodeIdFromNodeSymbol(
    const std::string& body_symbol) {
  SymbolId node_symbol_id = GetNodeSymbolId(body_symbol);
  if (node_symbol_id.IsValid()) {
    return GetProductionNodeIdFromNodeSymbolId(node_symbol_id);
  } else {
    return ProductionNodeId::InvalidId();
  }
}

LexicalGenerator::ProductionNodeId
LexicalGenerator::GetProductionNodeIdFromBodySymbol(
    const std::string& body_symbol) {
  SymbolId body_symbol_id = GetBodySymbolId(body_symbol);
  if (body_symbol_id.IsValid()) {
    return GetProductionNodeIdFromBodySymbolId(body_symbol_id);
  } else {
    return ProductionNodeId::InvalidId();
  }
}

inline LexicalGenerator::ProductionNodeId
LexicalGenerator::TerminalProductionNode::GetProductionNodeInBody(
    ProductionBodyId production_body_id, PointIndex point_index) {
  assert(production_body_id == 0);
  if (point_index == 0) {
    return Id();
  } else {
    return ProductionNodeId::InvalidId();
  }
}

inline void LexicalGenerator::TerminalProductionNode::AddForwardNodeId(
    ProductionBodyId production_body_id, PointIndex point_index,
    ProductionNodeId forward_node_id) {
  assert(production_body_id == 0 && point_index <= 1);
  if (point_index == 0) {
    AddFirstItemForwardNodeId(forward_node_id);
  } else {
    AddSecondItemForwardNodeId(forward_node_id);
  }
}

inline const std::unordered_set<LexicalGenerator::ProductionNodeId>&
LexicalGenerator::TerminalProductionNode::GetForwardNodeIds(
    ProductionBodyId production_body_id, PointIndex point_index) {
  assert(production_body_id == 0 && point_index <= 1);
  if (point_index == 0) {
    return GetFirstItemForwardNodeIds();
  } else {
    return GetSecondItemForwardNodeIds();
  }
}

inline void LexicalGenerator::TerminalProductionNode::SetCoreId(
    ProductionBodyId production_body_id, PointIndex point_index,
    CoreId core_id) {
  assert(production_body_id == 0 && point_index <= 1);
  if (point_index == 0) {
    SetFirstItemCoreId(core_id);
  } else {
    SetSecondItemCoreId(core_id);
  }
}

inline LexicalGenerator::CoreId
LexicalGenerator::TerminalProductionNode::GetCoreId(
    ProductionBodyId production_body_id, PointIndex point_index) {
  assert(production_body_id == 0 && point_index <= 1);
  if (point_index == 0) {
    return GetFirstItemCoreId();
  } else {
    return GetSecondItemCoreId();
  }
}

inline void
LexicalGenerator::NonTerminalProductionNode::ResizeProductionBodyNum(
    size_t new_size) {
  nonterminal_bodys_.resize(new_size);
  forward_nodes_.resize(new_size);
  core_ids_.resize(new_size);
  process_function_class_ids_.resize(new_size);
}

void LexicalGenerator::NonTerminalProductionNode::ResizeProductionBodyNodeNum(
    ProductionBodyId production_body_id, size_t new_size) {
  assert(production_body_id < nonterminal_bodys_.size());
  nonterminal_bodys_[production_body_id].resize(new_size);
  forward_nodes_[production_body_id].resize(new_size + 1);
  core_ids_[production_body_id].resize(new_size + 1);
}

inline LexicalGenerator::ProductionNodeId
LexicalGenerator::NonTerminalProductionNode::GetProductionNodeInBody(
    ProductionBodyId production_body_id, PointIndex point_index) {
  assert(production_body_id < nonterminal_bodys_.size());
  if (point_index < nonterminal_bodys_[production_body_id].size()) {
    return nonterminal_bodys_[production_body_id][point_index];
  } else {
    return ProductionNodeId::InvalidId();
  }
}

inline LexicalGenerator::ParsingTableEntry&
LexicalGenerator::ParsingTableEntry::operator=(
    ParsingTableEntry&& parsing_table_entry) {
  action_and_target_ = std::move(parsing_table_entry.action_and_target_);
  nonterminal_node_transform_table_ =
      std::move(parsing_table_entry.nonterminal_node_transform_table_);
  return *this;
}

LexicalGenerator::BaseProductionNode&
LexicalGenerator::BaseProductionNode::operator=(
    BaseProductionNode&& base_production_node) {
  base_type_ = std::move(base_production_node.base_type_);
  base_id_ = std::move(base_production_node.base_id_);
  base_symbol_id_ = std::move(base_production_node.base_symbol_id_);
  return *this;
}

#ifdef USE_AMBIGUOUS_GRAMMAR
LexicalGenerator::OperatorProductionNode&
LexicalGenerator::OperatorProductionNode::operator=(
    OperatorProductionNode&& operator_production_node) {
  TerminalProductionNode::operator=(std::move(operator_production_node));
  operator_associatity_type_ =
      std::move(operator_production_node.operator_associatity_type_);
  operator_priority_level_ =
      std::move(operator_production_node.operator_priority_level_);
  process_function_class_id_ =
      std::move(operator_production_node.process_function_class_id_);
  return *this;
}
#endif  // USE_AMBIGUOUS_GRAMMAR

LexicalGenerator::Core& LexicalGenerator::Core::operator=(Core&& core) {
  core_closure_available_ = std::move(core.core_closure_available_);
  core_id_ = std::move(core.core_id_);
  core_items_ = std::move(core.core_items_);
  return *this;
}

const std::regex LexicalGenerator::terminal_node_regex_(
    R"!(^\s*([a-zA-Z][a-zA-Z0-9_]*)\s*->\s*(\S+)\s*$)!");
const std::regex LexicalGenerator::operator_node_regex_(
    R"(^\s*(\S+)\s*@\s*([1-9][0-9]*)\s*@\s*([LR])\s*\{\s*([a-zA-Z_])"
    R"([a-zA-Z0-9_]*)?\s*,?\s*(?:\[\s*([^\]]+)\s*\])?\s*,?\s*([a-zA-Z_])"
    R"([a-zA-Z0-9_]*)?\s*\}\s*(?:\{\s*([^\}]+)\s*\})?\s*$)");
const std::regex LexicalGenerator::nonterminal_node_regex_(
    R"(^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*->\s*([^=]+)\s*(?:=>\s*\{\s*)"
    R"(((?:[a-zA-Z_][a-zA-Z0-9_]*)|@)?\s*,?\s*(?:\[\s*([^\]]+)\s*\])?)"
    R"(\s*,?\s*((?:[a-zA-Z_][a-zA-Z0-9_]*)|@)?\s*\})?\s*(?:\{\s*(.+)\s*\})?)"
    R"(\s*$)");
const std::regex LexicalGenerator::keyword_regex_(R"!("(\S*)")!");
const std::regex LexicalGenerator::body_symbol_regex_(
    R"((")?([a-zA-Z_][a-zA-Z0-9_]*)(\1))");
const std::regex LexicalGenerator::function_regex_(
    R"(((?:[a-zA-Z_][a-zA-Z0-9_]*)|@))");
const std::regex LexicalGenerator::file_regex_(
    R"(((?:[a-zA-Z_][a-zA-Z0-9_\.- ]*)|@))");

}  // namespace frontend::generator::lexicalgenerator