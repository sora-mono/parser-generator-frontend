#include "syntax_generator.h"

#include <queue>

namespace frontend::generator::syntaxgenerator {
inline SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddTerminalNode(
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
      printf("syntax_generator_error:终结节点名已定义：%s\n",
             node_symbol.c_str());
      return ProductionNodeId::InvalidId();
    }
    // 需要添加一个新的终结节点
    production_node_id = SubAddTerminalNode(node_symbol_id, body_symbol_id);
    frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
        word_attached_data;
    word_attached_data.production_node_id = production_node_id;
    word_attached_data.node_type = ProductionNodeType::kTerminalNode;
    word_attached_data.process_function_class_id_ =
        ProcessFunctionClassId::InvalidId();
    // 向DFA生成器注册关键词
    dfa_generator_.AddRegexpression(body_symbol, std::move(word_attached_data),
                                    node_priority);
  } else {
    // 该终结节点的内容已存在，不应重复添加
    fprintf(stderr, "syntax_generator error:多次声明同一正则：%s\n",
            body_symbol.c_str());
    return ProductionNodeId::InvalidId();
  }
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

#ifdef USE_AMBIGUOUS_GRAMMAR
inline SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddOperatorNode(
    const std::string& operator_symbol, AssociatityType associatity_type,
    OperatorPriority priority_level) {
  // 运算符产生式名与运算符相同
  auto [operator_node_symbol_id, operator_node_symbol_inserted] =
      AddNodeSymbol(operator_symbol);
  if (!operator_node_symbol_inserted) [[unlikely]] {
    fprintf(stderr, "syntax_generator error:运算符：%s 已定义\n",
            operator_symbol.c_str());
    return ProductionNodeId::InvalidId();
  }
  auto [operator_body_symbol_id, operator_body_symbol_inserted] =
      AddBodySymbol(operator_symbol);
  if (!operator_body_symbol_inserted) {
    fprintf(
        stderr,
        "syntax_generator error:运算符：%s 已在DFA中添加，无法定义为运算符\n",
        operator_symbol.c_str());
    return ProductionNodeId::InvalidId();
  }
  ProductionNodeId operator_node_id = SubAddOperatorNode(
      operator_node_symbol_id, associatity_type, priority_level);
  assert(operator_node_id.IsValid());
  frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
      word_attached_data_;
  word_attached_data_.production_node_id = operator_node_id;
  word_attached_data_.node_type = ProductionNodeType::kOperatorNode;
  word_attached_data_.associate_type = associatity_type;
  word_attached_data_.operator_priority = priority_level;
  // 向DFA生成器注册关键词
  dfa_generator_.AddKeyword(
      operator_symbol, word_attached_data_,
      frontend::generator::dfa_generator::DfaGenerator::WordPriority(1));
  return operator_node_id;
}

inline SyntaxGenerator::ProductionNodeId SyntaxGenerator::SubAddOperatorNode(
    SymbolId node_symbol_id, AssociatityType associatity_type,
    OperatorPriority priority_level) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<OperatorProductionNode>(
          node_symbol_id, associatity_type, priority_level);
  manager_nodes_.GetObject(node_id).SetThisNodeId(node_id);
  SetNodeSymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  SetBodySymbolIdToProductionNodeIdMapping(node_symbol_id, node_id);
  return node_id;
}
#endif  // USE_AMBIGUOUS_GRAMMAR

inline SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddNonTerminalNode(
    std::string&& node_symbol, std::vector<std::string>&& subnode_symbols,
    ProcessFunctionClassId class_id, bool could_empty_reduct) {
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
  NonTerminalProductionNode& production_node =
      static_cast<NonTerminalProductionNode&>(
          GetProductionNode(production_node_id));
  if (could_empty_reduct) {
    // 该产生式可以被空规约
    production_node.SetProductionCouldBeEmpty();
    return production_node_id;
  }
  for (auto& subnode_symbol : subnode_symbols) {
    // 将产生式体的所有产生式名转换为节点ID
    ProductionNodeId subproduction_node_id;
    if (subnode_symbol == "@") {
      // 此处不应出现空节点标记
      fprintf(stderr, "syntax_generator error: 产生式： %s 产生式体：",
              node_symbol.c_str());
      for (auto& symbol : subnode_symbols) {
        fprintf(stderr, "%s ", symbol.c_str());
      }
      fprintf(stderr, "语法错误，不应在产生式体中出现@\n");
      return ProductionNodeId::InvalidId();
    } else {
      subproduction_node_id = GetProductionNodeIdFromNodeSymbol(subnode_symbol);
    }
    // 产生式名
    if (!subproduction_node_id.IsValid()) {
      // 产生式节点未定义
      // 添加待处理记录
      AddUnableContinueNonTerminalNode(subnode_symbol, std::move(node_symbol),
                                       std::move(subnode_symbols), class_id,
                                       could_empty_reduct);
    }
    node_ids.push_back(subproduction_node_id);
  }

  ProductionBodyId body_id = production_node.AddBody(std::move(node_ids));
  production_node.SetBodyProcessFunctionClassId(body_id, class_id);
  return production_node_id;
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
  assert(production_node_id.IsValid());
  SetRootProductionNodeId(production_node_id);
}

template <class ForwardNodeIdContainer>
inline std::pair<
    std::map<SyntaxGenerator::CoreItem,
             std::unordered_set<SyntaxGenerator::ProductionNodeId>>::iterator,
    bool>
SyntaxGenerator::AddItemAndForwardNodeIdsToCore(
    CoreId core_id, const CoreItem& core_item,
    ForwardNodeIdContainer&& forward_node_ids) {
  assert(core_id.IsValid());
  return GetCore(core_id).AddItemAndForwardNodeIds(
      core_item, std::forward<ForwardNodeIdContainer>(forward_node_ids));
}

std::unordered_set<SyntaxGenerator::ProductionNodeId>
SyntaxGenerator::GetNonTerminalNodeFirstNodes(
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

std::unordered_set<SyntaxGenerator::ProductionNodeId> SyntaxGenerator::First(
    ProductionNodeId production_node_id, ProductionBodyId production_body_id,
    PointIndex point_index,
    const InsideForwardNodesContainerType& next_node_ids) {
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

void SyntaxGenerator::CoreClosure(CoreId core_id) {
  Core& core = GetCore(core_id);
  if (core.IsClosureAvailable()) {
    // 闭包有效，无需重求
    return;
  }
  std::queue<
      const std::pair<const CoreItem, std::unordered_set<ProductionNodeId>>*>
      items_waiting_process;
  for (const auto& item_and_forward_node_ids :
       GetCoreItemsAndForwardNodes(core_id)) {
    // 将项集中当前所有的项压入待处理队列
    items_waiting_process.push(&item_and_forward_node_ids);
  }
  while (!items_waiting_process.empty()) {
    auto item_now = items_waiting_process.front();
    items_waiting_process.pop();
    const auto [production_node_id, production_body_id, point_index] =
        item_now->first;
    ProductionNodeId next_production_node_id = GetProductionNodeIdInBody(
        production_node_id, production_body_id, point_index);
    if (!next_production_node_id.IsValid()) {
      // 无后继节点，设置规约条目
      ParsingTableEntry& parsing_table_entry =
          GetParsingTableEntry(core.GetParsingTableEntryId());
      // 组装规约使用的数据
      ParsingTableEntry::ReductAttachedData reduct_attached_data;
      reduct_attached_data.process_function_class_id_ =
          GetProcessFunctionClass(production_node_id, production_body_id);
      reduct_attached_data.production_body_ =
          GetProductionBody(production_node_id, production_body_id);
      reduct_attached_data.reducted_nonterminal_node_id_ = production_node_id;
      for (auto node_id : item_now->second) {
        // 对每个向前看符号设置规约操作
        parsing_table_entry.SetTerminalNodeActionAndAttachedData(
            node_id, ActionType::kReduct, reduct_attached_data);
      }
      continue;
    }
    NonTerminalProductionNode& production_node =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(next_production_node_id));
    if (production_node.Type() != ProductionNodeType::kNonTerminalNode) {
      // 不是非终结节点，无法展开
      continue;
    }
    // 展开非终结节点，并为其创建向前看符号集
    InsideForwardNodesContainerType forward_node_ids = First(
        production_node_id, production_body_id, PointIndex(point_index + 1));
    NonTerminalProductionNode& production_node_waiting_spread =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(next_production_node_id));
    assert(production_node_waiting_spread.Type() ==
           ProductionNodeType::kNonTerminalNode);
    for (auto body_id : production_node_waiting_spread.GetAllBodyIds()) {
      // 将该非终结节点中的每个产生式体加入到core中，点在最左侧
      auto [iter, inserted] = AddItemAndForwardNodeIdsToCore(
          core_id, CoreItem(next_production_node_id, body_id, PointIndex(0)),
          forward_node_ids);
      if (inserted) {
        // 如果插入新的项则添加到队列中等待处理
        items_waiting_process.push(&*iter);
      }
    }
    if (production_node.CouldBeEmpty()) {
      // 该非终结节点可以空规约
      // 向项集中添加空规约后的项，向前看符号复制原来的项的向前看符号集
      auto [iter, inserted] = AddItemAndForwardNodeIdsToCore(
          core_id,
          CoreItem(production_node_id, production_body_id,
                   PointIndex(point_index + 1)),
          GetForwardNodeIds(core_id, item_now->first));
      if (inserted) {
        // 如果插入新的项则添加到队列中等待处理
        items_waiting_process.push(&*iter);
      }
    }
  }
  SetCoreClosureAvailable(core_id);
}

void SyntaxGenerator::SpreadLookForwardSymbolAndConstructParsingTableEntry(
    CoreId core_id) {
  CoreClosure(core_id);
  // 执行闭包操作后可以空规约达到的每一项都在core内
  // 所以处理时无需考虑某项空规约可能达到的项，因为这些项都存在于core内

  // 新创建的core的ID
  std::vector<CoreId> new_core_ids;
  for (auto& item : GetCoreItemsAndForwardNodes(core_id)) {
    auto [production_node_id, production_body_id, point_index] = item.first;
    ProductionNodeId next_production_node_id = GetProductionNodeIdInBody(
        production_node_id, production_body_id, point_index);
    if (!next_production_node_id.IsValid()) {
      // 没有下一个节点
      continue;
    }
    ParsingTableEntry& parsing_table_entry =
        GetParsingTableEntry(GetCore(core_id).GetParsingTableEntryId());
    BaseProductionNode& next_production_node =
        GetProductionNode(next_production_node_id);
    switch (next_production_node.Type()) {
      case ProductionNodeType::kTerminalNode:
      case ProductionNodeType::kOperatorNode: {
        const ParsingTableEntry::ActionAndAttachedData* action_and_target =
            parsing_table_entry.AtTerminalNode(next_production_node_id);
        // 移入节点后到达的核心ID
        CoreId next_core_id;
        if (action_and_target == nullptr) {
          // 不存在该转移条件下到达的分析表条目，需要新建core
          next_core_id = EmplaceCore();
          // 记录新添加的核心ID
          new_core_ids.push_back(next_core_id);
        } else {
          // 通过转移到的语法分析表条目ID反查对应的核心ID
          next_core_id = GetCoreIdFromParsingTableEntryId(
              action_and_target->GetShiftAttachedData().next_entry_id_);
        }
        Core& new_core = GetCore(next_core_id);
        // 将移入节点后的项添加到核心中
        // 向前看符号集与移入节点前的向前看符号集相同
        new_core.AddItemAndForwardNodeIds(
            CoreItem(production_node_id, production_body_id,
                     PointIndex(point_index + 1)),
            GetForwardNodeIds(core_id, item.first));
        // 设置语法分析表在该终结节点下的转移条件
        parsing_table_entry.SetTerminalNodeActionAndAttachedData(
            next_production_node_id, ActionType::kShift,
            ParsingTableEntry::ShiftAttachedData(
                new_core.GetParsingTableEntryId()));
      } break;
      case ProductionNodeType::kNonTerminalNode: {
        ParsingTableEntryId next_parsing_table_entry_id =
            parsing_table_entry.AtNonTerminalNode(next_production_node_id);
        CoreId next_core_id;
        if (!next_parsing_table_entry_id.IsValid()) {
          // 不存在该转移条件下到达的语法分析表条目，需要新建core
          next_core_id = EmplaceCore();
          new_core_ids.push_back(next_core_id);
        } else {
          next_core_id =
              GetCoreIdFromParsingTableEntryId(next_parsing_table_entry_id);
        }
        Core& new_core = GetCore(next_core_id);
        // 添加项和向前看符号集，向前看符号集与移入节点前的向前看符号集相同
        new_core.AddItemAndForwardNodeIds(
            CoreItem(production_node_id, production_body_id,
                     PointIndex(point_index + 1)),
            GetForwardNodeIds(core_id, item.first));
        // 更新语法分析表在该非终结节点下的转移条件
        parsing_table_entry.SetNonTerminalNodeTransformId(
            next_production_node_id, new_core.GetParsingTableEntryId());
      } break;
      default:
        break;
    }
  }
  for (auto core_id : new_core_ids) {
    // 对新生成的每个项集都传播向前看符号
    SpreadLookForwardSymbolAndConstructParsingTableEntry(core_id);
  }
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
  entry_ids.reserve(syntax_config_parsing_table_.size());
  for (size_t i = 0; i < syntax_config_parsing_table_.size(); i++) {
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
  for (auto& entry : syntax_config_parsing_table_) {
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
  assert(syntax_config_parsing_table_.size() > 0);
  // 下一个要处理的条目
  ParsingTableEntryId next_process_entry_index(0);
  // 下一个插入位置
  ParsingTableEntryId next_insert_position_index(0);
  // 类似快排分类的算法
  // 寻找没有被合并的条目并紧凑排列在vector低下标一侧
  while (next_process_entry_index < syntax_config_parsing_table_.size()) {
    if (old_entry_id_to_new_entry_id.find(next_process_entry_index) ==
        old_entry_id_to_new_entry_id.end()) {
      // 该条目保留
      if (next_insert_position_index != next_process_entry_index) {
        // 需要移动到新位置保持vector紧凑
        syntax_config_parsing_table_[next_insert_position_index] =
            std::move(syntax_config_parsing_table_[next_process_entry_index]);
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
  syntax_config_parsing_table_.resize(next_insert_position_index);
  // 将所有旧ID更新为新ID
  RemapParsingTableEntryId(moved_entry_to_new_entry_id);
}

void SyntaxGenerator::AnalysisKeyWord(const std::string& str) {
  std::sregex_iterator iter(str.cbegin(), str.cend(), keyword_regex_);
  std::sregex_iterator iter_end;
  FILE* config_file = GetConfigConstructFilePointer();
  for (; iter != iter_end; ++iter) {
    std::string keyword = iter->str(1);
    fprintf(config_file, "// 关键字：%s\n", keyword.c_str());
    fprintf(config_file, "PrintKeyWordConstructData(\"%s\");\n",
            keyword.c_str());
  }
}

void SyntaxGenerator::AnalysisProductionConfig(const std::string& file_name) {
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
        AnalysisKeyWord(std::move(temp));
        break;
      case 1:
        // 根产生式定义区
        AnalysisNonTerminalProduction(std::move(temp));
        ++part_num;
        break;
      case 2:
        // 运算符定义区
        AnalysisOperatorProduction(std::move(temp));
        break;
      case 3:
        // 基础终结节点定义区
        AnalysisTerminalProduction(std::move(temp));
        break;
      case 4:
        // 普通产生式定义区
        AnalysisNonTerminalProduction(std::move(temp));
        break;
      default:
        assert(false);
        break;
    }
  }
}

void SyntaxGenerator::AnalysisTerminalProduction(const std::string& str,
                                                 size_t operator_priority) {
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
                                 operator_priority);
}

#ifdef USE_AMBIGUOUS_GRAMMAR
void SyntaxGenerator::AnalysisOperatorProduction(const std::string& str) {
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
  operator_data.operator_priority = match_result[2].str();
  if (operator_data.operator_priority.empty()) {
    fprintf(stderr, "运算符：%s 优先级缺失\n",
            operator_data.operator_priority.c_str());
    return;
  }
  // 运算符结合性
  operator_data.associatity_type = match_result[3].str();
  if (operator_data.associatity_type.empty()) {
    fprintf(stderr, "运算符：%s 运算符结合性缺失\n",
            operator_data.operator_symbol.c_str());
    return;
  }

  operator_data.reduct_function = match_result[4].str();

#ifdef USE_USER_DEFINED_FILE
  operator_data.include_files = GetFilesName(match_result[5].str());
#endif  // USE_USER_DEFINED_FILE
  PrintOperatorNodeConstructData(std::move(operator_data));
}
#endif  // USE_AMBIGUOUS_GRAMMAR

void syntaxgenerator::SyntaxGenerator::AnalysisNonTerminalProduction(
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
  node_data.reduct_function = match_result[3].str();
#ifdef USE_USER_DEFINED_FILE
  node_data.include_files = GetFilesName(match_result[4].str());
#endif  // USE_USER_DEFINED_FILE
  PrintNonTerminalNodeConstructData(std::move(node_data));
}

void SyntaxGenerator::ParsingTableConstruct() {
  // 创建输入到文件尾时返回的节点
  CoreId root_core_id = EmplaceCore();
  ProductionNodeId end_production_node_id = AddEndNode();
  // 设置遇到代码文件尾部时返回的数据
  frontend::generator::dfa_generator::DfaGenerator::WordAttachedData
      end_of_file_saved_data;
  end_of_file_saved_data.production_node_id = end_production_node_id;
  dfa_generator_.SetEndOfFileSavedData(std::move(end_of_file_saved_data));
  // 生成初始的项集，并将根产生式填入
  Core& root_core = GetCore(root_core_id);
  root_core.AddItemAndForwardNodeIds(
      CoreItem(GetRootProductionNodeId(), ProductionBodyId(0), PointIndex(0)),
      std::initializer_list<ProductionNodeId>{end_production_node_id});
  SetRootParsingTableEntryId(root_core.GetParsingTableEntryId());
  // 传播向前看符号同时构造语法分析表
  SpreadLookForwardSymbolAndConstructParsingTableEntry(root_core_id);
  // 合并相同项，压缩语法分析表
  ParsingTableMergeOptimize();
  // 保存配置
  SaveConfig();
}

void SyntaxGenerator::SyntaxGeneratorInit() {
  CloseConfigFile();
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
  syntax_config_parsing_table_.clear();
  manager_process_function_class_.ObjectManagerInit();
  OpenConfigFile();
}

void SyntaxGenerator::OpenConfigFile() {
  if (GetProcessFunctionClassFilePointer() == nullptr) [[likely]] {
    errno_t result = fopen_s(&GetProcessFunctionClassFilePointer(),
                             "process_functions_classes.h", "w+");
    if (result != 0) [[unlikely]] {
      fprintf(stderr, "打开SyntaxConfig/process_functions.h失败，错误码：%d\n",
              result);
      return;
    }
    fprintf(
        GetProcessFunctionClassFilePointer(),
        "#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESSFUNCTIONS_H_\n"
        "#define "
        "GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESSFUNCTIONS_H_\n");
  }
  if (GetUserDefinedFunctionAndDataRegisterFilePointer() == nullptr)
      [[likely]] {
    errno_t result =
        fopen_s(&GetUserDefinedFunctionAndDataRegisterFilePointer(),
                "user_defined_function_and_data_register.h", "w+");
    if (result != 0) [[unlikely]] {
      fprintf(stderr, "打开SyntaxConfig/process_functions.h失败，错误码：%d\n",
              result);
      return;
    }
    fprintf(GetUserDefinedFunctionAndDataRegisterFilePointer(),
            "#ifndef "
            "GENERATOR_SYNTAXGENERATOR_USER_DEFINED_FUNCTION_AND_DATA_"
            "REGISTER_H_\n"
            "#define "
            "GENERATOR_SYNTAXGENERATOR_USER_DEFINED_FUNCTION_AND_DATA_"
            "REGISTER_H_\n"
            "#include \"process_functions_classes.h\"");
  }
  if (GetConfigConstructFilePointer() == nullptr) [[likely]] {
    errno_t result =
        fopen_s(&GetConfigConstructFilePointer(), "config_construct.cpp", "w+");
    if (result != 0) [[unlikely]] {
      fprintf(stderr, "打开SyntaxConfig/config_construct.cpp失败，错误码：%d\n",
              result);
      return;
    }
    fprintf(GetConfigConstructFilePointer(),
            "#include \"syntax_generator.h\"\n"
            "namespace frontend::generator::syntaxgenerator {\n"
            "void SyntaxGenerator::ConfigConstruct() { \n");
  }
}

void SyntaxGenerator::CloseConfigFile() {
  FILE* function_class_file_ptr = GetProcessFunctionClassFilePointer();
  if (function_class_file_ptr != nullptr) [[likely]] {
    fprintf(function_class_file_ptr,
            "#endif  // "
            "!GENERATOR_SYNTAXGENERATOR_SYNTAXCONFIG_PROCESSFUNCTIONS_H_\n");
    fclose(function_class_file_ptr);
  }
  if (GetUserDefinedFunctionAndDataRegisterFilePointer() != nullptr)
      [[likely]] {
    fclose(GetUserDefinedFunctionAndDataRegisterFilePointer());
    fprintf(GetUserDefinedFunctionAndDataRegisterFilePointer(),
            "#endif  // "
            "!GENERATOR_SYNTAXGENERATOR_USER_DEFINED_FUNCTION_AND_DATA_"
            "REGISTER_H_\n");
  }
  FILE* config_file_ptr = GetConfigConstructFilePointer();
  if (config_file_ptr != nullptr) [[likely]] {
    // 检查是否有未定义产生式
    fprintf(config_file_ptr, "CheckUndefinedProductionRemained();\n");
    fprintf(config_file_ptr,
            "}\n"
            "}  // namespace frontend::generator::syntaxgenerator\n");
    fclose(config_file_ptr);
  }
}

std::vector<std::pair<std::string, bool>> SyntaxGenerator::GetBodySymbol(
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

std::vector<std::string> SyntaxGenerator::GetFunctionsName(
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

std::vector<std::string> SyntaxGenerator::GetFilesName(const std::string& str) {
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

void SyntaxGenerator::AddUnableContinueNonTerminalNode(
    const std::string& undefined_symbol, std::string&& node_symbol,
    std::vector<std::string>&& subnode_symbols, ProcessFunctionClassId class_id,
    bool could_empty_reduct) {
  // 使用insert因为可能有多个产生式依赖同一个未定义的产生式
  undefined_productions_.emplace(
      undefined_symbol,
      std::make_tuple(std::move(node_symbol), std::move(subnode_symbols),
                      class_id, could_empty_reduct));
}

void SyntaxGenerator::CheckNonTerminalNodeCanContinue(
    const std::string& node_symbol) {
  if (!undefined_productions_.empty()) {
    auto [iter_begin, iter_end] =
        undefined_productions_.equal_range(node_symbol);
    if (iter_begin != undefined_productions_.end()) {
      while (iter_begin != iter_end) {
        auto& [node_symbol, node_body_ptr, process_function_class_id_,
               could_empty_reduct] = iter_begin->second;
        ProductionNodeId node_id =
            AddNonTerminalNode(std::move(node_symbol), std::move(node_body_ptr),
                               process_function_class_id_, could_empty_reduct);
        if (node_id.IsValid()) {
          // 成功添加非终结节点，删除该条记录
          auto temp_iter = iter_begin;
          ++iter_begin;
          undefined_productions_.erase(temp_iter);
        } else {
          ++iter_begin;
        }
      }
    }
  }
}

void SyntaxGenerator::CheckUndefinedProductionRemained() {
  if (!undefined_productions_.empty()) {
    // 仍存在未定义产生式
    for (auto& item : undefined_productions_) {
      auto& [node_symbol, node_bodys, class_id, could_empty_reduct] =
          item.second;
      if (could_empty_reduct) {
        fprintf(stderr, "(可以空规约）");
      }
      fprintf(stderr, "产生式：%s\n产生式体：", node_symbol.c_str());
      for (auto& body : node_bodys) {
        fprintf(stderr, "%s ", body.c_str());
      }

      fprintf(stderr, "中\n产生式：%s 未定义\n", item.first.c_str());
    }
  }
}

inline void SyntaxGenerator::AddKeyWord(const std::string& key_word) {
  // 关键字优先级默认为1
  // 自动生成同名终结节点
  AddTerminalNode(
      key_word, key_word,
      frontend::generator::dfa_generator::DfaGenerator::WordPriority(2));
}

void SyntaxGenerator::PrintKeyWordConstructData(const std::string& keyword) {
  fprintf(GetConfigConstructFilePointer(), "// 关键字：%s\n", keyword.c_str());
  fprintf(GetConfigConstructFilePointer(), "AddKeyWord(%s);\n",
          keyword.c_str());
}

void SyntaxGenerator::PrintTerminalNodeConstructData(std::string&& node_symbol,
                                                     std::string&& body_symbol,
                                                     size_t operator_priority) {
  fprintf(GetConfigConstructFilePointer(),
          "// 终结节点名：%s\n// 终结节点体：%s\n // 优先级：%llu",
          node_symbol.c_str(), body_symbol.c_str(),
          static_cast<unsigned long long>(operator_priority));
  fprintf(GetConfigConstructFilePointer(),
          "AddTerminalNode(\"%s\", \"%s\", PriorityLevel(%s));\n",
          node_symbol.c_str(), body_symbol.c_str(),
          std::to_string(operator_priority).c_str());
}

#ifdef USE_AMBIGUOUS_GRAMMAR
void SyntaxGenerator::PrintOperatorNodeConstructData(OperatorData&& data) {
  // 分配一个编号
  int operator_num = GetNodeNum();
  std::string class_name = class_name + std::to_string(operator_num) +
                           data.associatity_type + data.operator_priority;
  // 重新编码输出的结合性字符串
  if (data.associatity_type == "L") {
    data.associatity_type = "AssociatityType::kLeftAssociate";
  } else {
    assert(data.associatity_type == "R");
    data.associatity_type = "AssociatityType::kRightAssociate";
  }
  fprintf(GetConfigConstructFilePointer(),
          "// 运算符： %s\n// 结合性：%s\n// 优先级：%s\n",
          data.operator_symbol.c_str(), data.associatity_type.c_str(),
          data.operator_priority.c_str());
  fprintf(
      GetConfigConstructFilePointer(),
      "AddOperatorNode<%s>(\"%s\",AssociatityType(%s),PriorityLevel(%s));\n",
      class_name.c_str(), data.operator_symbol.c_str(),
      data.associatity_type.c_str(), data.operator_priority.c_str());
  FILE* function_file = GetProcessFunctionClassFilePointer();
  // TODO 去掉运算符的处理函数
  fprintf(function_file,
          "class %s : public ProcessFunctionInterface {\n public:\n",
          class_name.c_str());
  fprintf(function_file, "// 运算符： %s\n// 结合性：%s\n// 优先级：%s\n",
          data.operator_symbol.c_str(), data.associatity_type.c_str(),
          data.operator_priority.c_str());
#ifdef USE_USER_DEFINED_FILE
  for (size_t i = 0; i < data.include_files.size(); i++) {
    fprintf(function_file, "  #include\"%s\"\n", data.include_files[i].c_str());
  }
#endif  // USE_USER_DEFINED_FILE
  PrintProcessFunction(function_file, data);
  fprintf(function_file, "}\n");
  fprintf(GetUserDefinedFunctionAndDataRegisterFilePointer(),
          "BOOST_CLASS_EXPORT_GUID(frontend::generator::syntaxgenerator::%s,"
          "\"frontend::generator::syntaxgenerator::%s\")",
          class_name.c_str(), class_name.c_str());
}

#endif  // USE_AMBIGUOUS_GRAMMAR

void SyntaxGenerator::PrintNonTerminalNodeConstructData(
    NonTerminalNodeData&& data) {
  int node_num = GetNodeNum();
  std::string class_name = data.node_symbol + '_' + std::to_string(node_num);
  // 为了序列化注册新生成的类
  fprintf(GetUserDefinedFunctionAndDataRegisterFilePointer(),
          "BOOST_CLASS_EXPORT_GUID(frontend::generator::syntaxgenerator::%s,"
          "\"frontend::generator::syntaxgenerator::%s\")",
          class_name.c_str(), class_name.c_str());
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
            "CreateProcessFunctionClassObject<frontend::generator::"
            "syntaxgenerator::%s>();\n",
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
    ProductionBodyId production_body_id, PointIndex point_index) {
  assert(production_body_id == 0);
  if (point_index == 0) {
    return Id();
  } else {
    return ProductionNodeId::InvalidId();
  }
}

inline void SyntaxGenerator::NonTerminalProductionNode::ResizeProductionBodyNum(
    size_t new_size) {
  nonterminal_bodys_.resize(new_size);
  process_function_class_ids_.resize(new_size);
}

void SyntaxGenerator::NonTerminalProductionNode::ResizeProductionBodyNodeNum(
    ProductionBodyId production_body_id, size_t new_size) {
  assert(production_body_id < nonterminal_bodys_.size());
  nonterminal_bodys_[production_body_id].resize(new_size);
}

std::vector<SyntaxGenerator::ProductionBodyId>
SyntaxGenerator::NonTerminalProductionNode::GetAllBodyIds() const {
  std::vector<ProductionBodyId> production_body_ids;
  for (size_t i = 0; i < nonterminal_bodys_.size(); i++) {
    production_body_ids.push_back(ProductionBodyId(i));
  }
  return production_body_ids;
}

inline SyntaxGenerator::ProductionNodeId
SyntaxGenerator::NonTerminalProductionNode::GetProductionNodeInBody(
    ProductionBodyId production_body_id, PointIndex point_index) {
  assert(production_body_id < nonterminal_bodys_.size());
  if (point_index < nonterminal_bodys_[production_body_id].size()) {
    return nonterminal_bodys_[production_body_id][point_index];
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
    ParsingTableEntryId& old_entry_id =
        action_and_attached_data.second.GetShiftAttachedData().next_entry_id_;
    // 更新为新的条目ID
    old_entry_id = old_entry_id_to_new_entry_id.find(old_entry_id)->second;
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

#ifdef USE_AMBIGUOUS_GRAMMAR
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
#endif  // USE_AMBIGUOUS_GRAMMAR

SyntaxGenerator::Core& SyntaxGenerator::Core::operator=(Core&& core) {
  core_closure_available_ = std::move(core.core_closure_available_);
  core_id_ = std::move(core.core_id_);
  parsing_table_entry_id_ = std::move(core.parsing_table_entry_id_);
  item_and_forward_node_ids_ = std::move(core.item_and_forward_node_ids_);
  return *this;
}

void SyntaxGenerator::ConfigConstruct() {
  // 下面的宏将包含的文件中用户定义的产生式转化为产生式构建配置
  // 如AddTerminalNode、AddNonTerminalNode等
#define GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_
}

const std::regex SyntaxGenerator::terminal_node_regex_(
    R"!(^\s*([a-zA-Z][a-zA-Z0-9_]*)\s*->\s*(\S+)\s*$)!");
const std::regex SyntaxGenerator::operator_node_regex_(
    R"(^\s*(\S+)\s*@\s*([1-9][0-9]*)\s*@\s*([LR])\s*\{\s*([a-zA-Z_])"
    R"([a-zA-Z0-9_]*)\s*\}\s*(?:\{\s*([^\}]+)\s*\})?\s*$)");
const std::regex SyntaxGenerator::nonterminal_node_regex_(
    R"(^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*->\s*([^=]+)\s*(?:=>\s*\{\s*)"
    R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*\})\s*(?:\{\s*([^\}]+)\s*\})?\s*$)");
const std::regex SyntaxGenerator::keyword_regex_(R"!("(\S*)")!");
const std::regex SyntaxGenerator::body_symbol_regex_(
    R"((")?([a-zA-Z_][a-zA-Z0-9_]*)(\1))");
const std::regex SyntaxGenerator::function_regex_(
    R"(((?:[a-zA-Z_][a-zA-Z0-9_]*)|@))");
const std::regex SyntaxGenerator::file_regex_(
    R"(((?:[a-zA-Z_][a-zA-Z0-9_.\- ]*)|@))");

}  // namespace frontend::generator::syntaxgenerator