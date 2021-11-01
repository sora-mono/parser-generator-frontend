#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_H_

#include <any>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <cassert>
#include <format>
#include <fstream>
#include <regex>
#include <tuple>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/object_manager.h"
#include "Common/unordered_struct_manager.h"
#include "Generator/DfaGenerator/dfa_generator.h"
#include "Generator/export_types.h"
#include "process_function_interface.h"
#include "production_item_set.h"
#include "production_node.h"
#include "syntax_analysis_table.h"

// TODO 添加删除未使用产生式的功能
namespace frontend::generator::syntax_generator {

class SyntaxGenerator {
 private:
  // 词法分析中词的优先级
  using WordPriority =
      frontend::generator::dfa_generator::DfaGenerator::WordPriority;
  // 表示项的数据结构
  using ProductionItem = ProductionItemSet::ProductionItem;
  // hash表示项的数据结构的结构
  using ProductionItemHasher = ProductionItemSet::ProductionItemHasher;
  // 项与向前看节点的容器
  using ProductionItemAndForwardNodesContainer =
      ProductionItemSet::ProductionItemAndForwardNodesContainer;
  // 存储向前看节点的容器
  using ForwardNodesContainer = ProductionItemSet::ForwardNodesContainer;

 public:
  SyntaxGenerator() = default;
  SyntaxGenerator(const SyntaxGenerator&) = delete;

  SyntaxGenerator& operator=(const SyntaxGenerator&) = delete;

  // 构建并保存编译器前端配置
  void ConstructSyntaxConfig();

 private:
  // 初始化
  void SyntaxGeneratorInit();
  // 构建LALR（1））所需的各种外围信息
  void ConfigConstruct();
  // 构建LALR(1)配置
  void SyntaxAnalysisTableConstruct();

  // 获取一个节点编号
  int GetNodeNum() { return node_num_++; }
  // 复位节点编号
  void NodeNumInit() { node_num_ = 0; }
  // 添加产生式名，返回符号的ID和是否执行了插入操作
  // 执行了插入操作则返回true
  std::pair<SymbolId, bool> AddNodeSymbol(const std::string& node_symbol) {
    assert(node_symbol.size() != 0);
    return manager_node_symbol_.AddObject(node_symbol);
  }
  // 添加产生式体符号，返回符号的ID和是否执行了插入操作
  // 执行了插入操作则返回true
  std::pair<SymbolId, bool> AddBodySymbol(const std::string& body_symbol) {
    assert(body_symbol.size() != 0);
    return manager_terminal_body_symbol_.AddObject(body_symbol);
  }
  // 获取产生式名对应ID，不存在则返回SymbolId::InvalidId()
  SymbolId GetNodeSymbolId(const std::string& node_symbol) const {
    assert(node_symbol.size() != 0);
    return manager_node_symbol_.GetObjectId(node_symbol);
  }
  // 获取产生式体符号对应的ID，不存在则返回SymbolId::InvalidId()
  SymbolId GetBodySymbolId(const std::string& body_symbol) const {
    assert(body_symbol.size() != 0);
    return manager_terminal_body_symbol_.GetObjectId(body_symbol);
  }
  // 通过产生式名ID查询对应产生式名
  const std::string& GetNodeSymbolStringFromId(SymbolId node_symbol_id) const {
    assert(node_symbol_id.IsValid());
    return manager_node_symbol_.GetObject(node_symbol_id);
  }
  // 通过产生式体符号ID查询产生式体原始数据
  const std::string& GetBodySymbolStringFromId(SymbolId body_symbol_id) const {
    assert(body_symbol_id.IsValid());
    return manager_terminal_body_symbol_.GetObject(body_symbol_id);
  }
  // 通过产生式ID查询对应产生式名
  const std::string& GetNodeSymbolStringFromProductionNodeId(
      ProductionNodeId production_node_id) const {
    return GetNodeSymbolStringFromId(
        GetProductionNode(production_node_id).GetNodeSymbolId());
  }
  // 通过项查询下一个移入的节点名
  const std::string& GetNextNodeToShiftSymbolString(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      NextWordToShiftIndex next_word_to_shift_index) const {
    return GetNodeSymbolStringFromProductionNodeId(GetProductionNodeIdInBody(
        production_node_id, production_body_id, next_word_to_shift_index));
  }
  // 通过项查询下一个移入的节点名
  const std::string& GetNextNodeToShiftSymbolString(
      const ProductionItem& production_item) const {
    auto& [production_node_id, production_body_id, next_word_to_shift_index] =
        production_item;
    return GetNextNodeToShiftSymbolString(
        production_node_id, production_body_id, next_word_to_shift_index);
  }
  // 设置产生式名ID到节点ID的映射
  void SetNodeSymbolIdToProductionNodeIdMapping(SymbolId node_symbol_id,
                                                ProductionNodeId node_id) {
    assert(node_symbol_id.IsValid() && node_id.IsValid());
    node_symbol_id_to_node_id_[node_symbol_id] = node_id;
  }
  // 设置产生式体符号ID到节点ID的映射
  void SetBodySymbolIdToProductionNodeIdMapping(SymbolId body_symbol_id,
                                                ProductionNodeId node_id) {
    assert(body_symbol_id.IsValid() && node_id.IsValid());
    production_body_symbol_id_to_node_id_[body_symbol_id] = node_id;
  }
  // 将产生式名ID转换为产生式节点ID
  // 如果不存在该产生式节点则返回ProductionNodeId::InvalidId()
  ProductionNodeId GetProductionNodeIdFromNodeSymbolId(
      SymbolId node_symbol_id) {
    auto iter = node_symbol_id_to_node_id_.find(node_symbol_id);
    if (iter == node_symbol_id_to_node_id_.end()) {
      return ProductionNodeId::InvalidId();
    } else {
      return iter->second;
    }
  }
  // 将产生式体符号ID转换为产生式节点ID
  // 不存在时则返回ProductionNodeId::InvalidId()
  ProductionNodeId GetProductionNodeIdFromBodySymbolId(SymbolId body_symbol_id);
  // 将产生式名转换为产生式节点ID
  ProductionNodeId GetProductionNodeIdFromNodeSymbol(
      const std::string& body_symbol);
  // 将产生式体符号转换为产生式节点ID
  ProductionNodeId GetProductionNodeIdFromBodySymbol(
      const std::string& body_symbol);
  // 获取产生式体的产生式节点ID，越界时返回ProductionNodeId::InvalidId()
  ProductionNodeId GetProductionNodeIdInBody(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      NextWordToShiftIndex next_word_to_shift_index) const {
    return GetProductionNode(production_node_id)
        .GetProductionNodeInBody(production_body_id, next_word_to_shift_index);
  }
  // 设置用户定义的根产生式ID
  void SetRootProductionNodeId(ProductionNodeId root_production_node_id) {
    root_production_node_id_ = root_production_node_id;
  }
  // 获取用户定义的根产生式ID
  ProductionNodeId GetRootProductionNodeId() {
    return root_production_node_id_;
  }
  // 添加包装处理函数的类对象
  template <class ProcessFunctionClass>
  ProcessFunctionClassId CreateProcessFunctionClassObject() {
    return manager_process_function_class_
        .EmplaceObject<ProcessFunctionClass>();
  }
  // 获取包装处理函数和数据的类的对象ID
  ProcessFunctionClassId GetProcessFunctionClass(
      ProductionNodeId production_node_id,
      ProductionBodyId production_body_id) {
    NonTerminalProductionNode& production_node =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(production_node_id));
    assert(production_node.Type() == ProductionNodeType::kNonTerminalNode);
    return production_node.GetBodyProcessFunctionClassId(production_body_id);
  }
  // 添加因为产生式体未定义而导致不能继续添加的非终结节点
  // 第一个参数为未定义的产生式名
  // 其余三个参数同AddNonTerminalNode
  // 函数会复制一份副本，无需保持原来的参数的生命周期
  void AddUnableContinueNonTerminalNode(
      const std::string& undefined_symbol, std::string&& node_symbol,
      std::vector<std::string>&& subnode_symbols,
      ProcessFunctionClassId class_id);
  // 检查给定的节点生成后是否可以重启因为部分产生式体未定义而搁置的
  // 非终结产生式添加过程
  void CheckNonTerminalNodeCanContinue(const std::string& node_symbol);
  // 检查是否还有未定义的产生式体
  // 用于完成所有产生式添加后检查
  // 如果有则输出错误信息并结束程序
  void CheckUndefinedProductionRemained();
  // 添加关键字，自动创建同名终结节点
  void AddKeyWord(std::string&& key_word);
  // 新建终结节点，返回节点ID
  // 该函数用于声明终结产生式的正则定义
  // 节点已存在且给定symbol_id不同于已有ID则返回ProductionNodeId::InvalidId()
  // 第三个参数是词优先级，0保留为普通词的优先级，1保留为运算符优先级
  // 2保留为关键字优先级，其余的优先级尚未指定
  // ！！！词优先级与运算符优先级不同，请注意区分！！！
  ProductionNodeId AddTerminalNode(std::string&& node_symbol,
                                   std::string&& body_symbol,
                                   WordPriority node_priority = WordPriority(0),
                                   bool is_key_word = false);
  // 子过程，仅用于创建节点
  // 自动更新节点名ID到节点ID的映射
  // 自动更新节点体ID到节点ID的映射
  // 自动为节点类设置节点ID
  ProductionNodeId SubAddTerminalNode(SymbolId node_symbol_id,
                                      SymbolId body_symbol_id);
  // 新建双目运算符节点，返回节点ID
  // 节点已存在则返回ProductionNodeId::InvalidId()
  // 默认添加的运算符词法分析优先级高于普通终结产生式低于关键字
  ProductionNodeId AddBinaryOperatorNode(
      std::string&& operator_symbol,
      OperatorAssociatityType binary_operator_associatity_type,
      OperatorPriority binary_operator_priority_level);
  // 新建左侧单目运算符节点，返回节点ID
  // 节点已存在则返回ProductionNodeId::InvalidId()
  // 默认添加的运算符词法分析优先级高于普通终结产生式低于关键字
  // 提供左侧单目运算符版本
  ProductionNodeId AddLeftUnaryOperatorNode(
      std::string&& operator_symbol,
      OperatorAssociatityType unary_operator_associatity_type,
      OperatorPriority unary_operator_priority_level);
  // 新建复用的左侧单目和双目运算符节点，返回节点ID
  // 节点已存在则返回ProductionNodeId::InvalidId()
  // 默认添加的运算符词法分析优先级高于普通终结产生式低于关键字
  // 提供左侧单目运算符版本
  ProductionNodeId AddBinaryUnaryOperatorNode(
      std::string&& operator_symbol,
      OperatorAssociatityType binary_operator_associatity_type,
      OperatorPriority binary_operator_priority_level,
      OperatorAssociatityType unary_operator_associatity_type,
      OperatorPriority unary_operator_priority_level);
  // 子过程，仅用于创建节点
  // 运算符节点名同运算符名
  // 自动更新节点名ID到节点ID的映射表
  // 自动更新节点体ID到节点ID的映射
  // 自动为节点类设置节点ID
  ProductionNodeId SubAddOperatorNode(SymbolId node_symbol_id,
                                      OperatorAssociatityType associatity_type,
                                      OperatorPriority priority_level);
  // 新建非终结节点，返回节点ID，节点已存在则不会创建新的节点
  // node_symbol为产生式名，subnode_symbols是产生式体
  // could_empty_reduct代表是否可以空规约
  // class_id是已添加的包装用户自定义函数和数据的类的对象ID
  // 拆成模板函数和非模板函数为了降低代码生成量，阻止代码膨胀
  // 下面两个函数均可直接调用，class_id是包装用户定义函数和数据的类的对象ID
  template <class ProcessFunctionClass>
  ProductionNodeId AddNonTerminalNode(
      std::string&& node_symbol, std::vector<std::string>&& subnode_symbols);
  ProductionNodeId AddNonTerminalNode(
      std::string&& node_symbol, std::vector<std::string>&& subnode_symbols,
      ProcessFunctionClassId class_id);
  // 子过程，仅用于创建节点
  // 自动更新节点名ID到节点ID的映射表
  // 自动为节点类设置节点ID
  ProductionNodeId SubAddNonTerminalNode(SymbolId symbol_id);
  // 设置非终结节点可以空规约
  // 必须保证给定名称节点存在且为非终结产生式名
  void SetNonTerminalNodeCouldEmptyReduct(
      const std::string& nonterminal_node_symbol);
  // 新建文件尾节点，返回节点ID
  ProductionNodeId AddEndNode();
  // 设置产生式根节点
  // 输入产生式名
  void SetRootProduction(const std::string& production_node_name);

  // 获取节点
  BaseProductionNode& GetProductionNode(ProductionNodeId production_node_id);
  const BaseProductionNode& GetProductionNode(
      ProductionNodeId production_node_id) const;
  BaseProductionNode& GetProductionNodeFromNodeSymbolId(SymbolId symbol_id);
  BaseProductionNode& GetProductionNodeBodyFromSymbolId(SymbolId symbol_id);
  // 获取非终结节点中的一个产生式体
  const std::vector<ProductionNodeId>& GetProductionBody(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id);
  // 给非终结节点添加产生式体
  template <class IdType>
  void AddNonTerminalNodeBody(ProductionNodeId node_id, IdType&& body) {
    static_cast<NonTerminalProductionNode&>(GetProductionNode(node_id)).AddBody,
        (std::forward<IdType>(body));
  }
  // 添加一条语法分析表条目
  SyntaxAnalysisTableEntryId AddSyntaxAnalysisTableEntry() {
    SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id(
        syntax_analysis_table_.size());
    syntax_analysis_table_.emplace_back();
    return syntax_analysis_table_entry_id;
  }
  // 获取production_item_set_id对应产生式项集
  const ProductionItemSet& GetProductionItemSet(
      ProductionItemSetId production_item_set_id) const {
    assert(production_item_set_id < production_item_sets_.Size());
    return production_item_sets_[production_item_set_id];
  }
  // 获取production_item_set_id对应产生式项集
  ProductionItemSet& GetProductionItemSet(
      ProductionItemSetId production_item_set_id) {
    return const_cast<ProductionItemSet&>(
        static_cast<const SyntaxGenerator&>(*this).GetProductionItemSet(
            production_item_set_id));
  }
  // 设置语法分析表条目ID到项集ID的映射
  void SetSyntaxAnalysisTableEntryIdToProductionItemSetIdMapping(
      SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id,
      ProductionItemSetId production_item_set_id) {
    syntax_analysis_table_entry_id_to_production_item_set_id_
        [syntax_analysis_table_entry_id] = production_item_set_id;
  }
  // 获取语法分析表条目ID对应的项集ID
  ProductionItemSetId GetProductionItemSetIdFromSyntaxAnalysisTableEntryId(
      SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id) {
    auto iter = syntax_analysis_table_entry_id_to_production_item_set_id_.find(
        syntax_analysis_table_entry_id);
    assert(iter !=
           syntax_analysis_table_entry_id_to_production_item_set_id_.end());
    return iter->second;
  }
  // 添加新的项集，需要更新语法分析表条目到项集ID的映射
  ProductionItemSetId EmplaceProductionItemSet() {
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
  // 向项集中添加项和相应的向前看符号，可以传入单个未包装ID
  // 返回插入位置和是否插入
  // 如果添加了新项或向前看符号则设置闭包无效
  // 如果该项已存在则仅添加向前看符号
  // 如果向项集中新添加了项则自动添加该项所添加到的新项集ID的记录
  template <class ForwardNodeIdContainer>
  std::pair<ProductionItemAndForwardNodesContainer::iterator, bool>
  AddItemAndForwardNodeIdsToCore(ProductionItemSetId production_item_set_id,
                                 const ProductionItem& production_item,
                                 ForwardNodeIdContainer&& forward_node_ids);
  // 在AddItemAndForwardNodeIdsToCore基础上设置添加的项为核心项
  template <class ForwardNodeIdContainer>
  std::pair<ProductionItemAndForwardNodesContainer::iterator, bool>
  AddMainItemAndForwardNodeIdsToCore(ProductionItemSetId production_item_set_id,
                                     const ProductionItem& production_item,
                                     ForwardNodeIdContainer&& forward_node_ids);
  // 向给定项中添加向前看符号，同时支持输入单个符号和符号容器
  // 返回是否添加
  // 要求项已经存在，否则请调用AddItemAndForwardNodeIds及类似函数
  // 如果添加了新的向前看符号则设置闭包无效
  template <class ForwardNodeIdContainer>
  bool AddForwardNodes(ProductionItemSetId production_item_set_id,
                       const ProductionItem& production_item,
                       ForwardNodeIdContainer&& forward_node_ids) {
    return GetProductionItemSet(production_item_set_id)
        .AddForwardNodes(production_item, std::forward<ForwardNodeIdContainer>(
                                              forward_node_ids));
  }
  // 记录核心项所属的项集ID
  void AddProductionItemBelongToProductionItemSetId(
      const ProductionItem& production_item,
      ProductionItemSetId production_item_set_id);
  // 获取项集的全部核心项
  const std::list<ProductionItemAndForwardNodesContainer::const_iterator>
  GetProductionItemSetMainItems(
      ProductionItemSetId production_item_set_id) const {
    return GetProductionItemSet(production_item_set_id).GetMainItems();
  }
  // 获取单个核心项所属的全部项集
  const std::list<ProductionItemSetId>&
  GetProductionItemSetIdFromProductionItem(
      ProductionNodeId production_node_id, ProductionBodyId body_id,
      NextWordToShiftIndex next_word_to_shift_index);
  // 获取向前看符号集
  const ForwardNodesContainer& GetForwardNodeIds(
      ProductionItemSetId production_item_set_id,
      const ProductionItem& production_item) const {
    return GetProductionItemSet(production_item_set_id)
        .GetItemsAndForwardNodeIds()
        .at(production_item);
  }

  // First的子过程，提取一个非终结节点中所有的first符号
  // 第二个参数指向存储结果的集合
  // 第三个参数用来存储已经处理过的节点，防止无限递归，初次调用应传入空集合
  // 如果输入ProductionNodeId::InvalidId()则返回空集合
  void GetNonTerminalNodeFirstNodeIds(
      ProductionNodeId production_node_id, ForwardNodesContainer* result,
      std::unordered_set<ProductionNodeId>&& processed_nodes =
          std::unordered_set<ProductionNodeId>());
  // 闭包操作中的first操作，前三个参数标志β的位置
  // 采用三个参数因为非终结节点可能规约为空节点，需要向下查找终结节点
  // 向前看节点可以规约为空节点则添加相应向前看符号
  // 然后继续向前查找直到结尾或不可空规约非终结节点或终结节点
  ForwardNodesContainer First(ProductionNodeId production_node_id,
                              ProductionBodyId production_body_id,
                              NextWordToShiftIndex next_word_to_shift_index,
                              const ForwardNodesContainer& next_node_ids);
  // 获取项集的全部项
  const ProductionItemAndForwardNodesContainer&
  GetProductionItemsAndForwardNodes(
      ProductionItemSetId production_item_set_id) {
    return GetProductionItemSet(production_item_set_id)
        .GetItemsAndForwardNodeIds();
  }
  // 获取语法分析表条目
  SyntaxAnalysisTableEntry& GetSyntaxAnalysisTableEntry(
      SyntaxAnalysisTableEntryId production_node_id) {
    assert(production_node_id < syntax_analysis_table_.size());
    return syntax_analysis_table_[production_node_id];
  }
  // 设置根语法分析表条目ID
  void SetRootSyntaxAnalysisTableEntryId(
      SyntaxAnalysisTableEntryId root_syntax_analysis_table_entry_id) {
    root_syntax_analysis_table_entry_id_ = root_syntax_analysis_table_entry_id;
  }
  // 设置项集闭包有效（避免每次Goto都求闭包）
  // 仅应由CoreClosure调用
  void SetCoreClosureAvailable(ProductionItemSetId production_item_set_id) {
    GetProductionItemSet(production_item_set_id).SetClosureAvailable();
  }
  bool IsClosureAvailable(ProductionItemSetId production_item_set_id) {
    return GetProductionItemSet(production_item_set_id).IsClosureAvailable();
  }
  // 对给定核心项求闭包，结果存在原位置
  // 自动添加所有当前位置可以空规约的项的后续项
  // 返回是否重求闭包
  // 重求闭包则清空语法分析表条目
  bool ProductionItemSetClosure(ProductionItemSetId production_item_set_id);
  // 获取给定的项构成的项集，如果不存在则返回ProductionItemSetId::InvalidId()
  // 输入指向转移前项的迭代器
  ProductionItemSetId GetProductionItemSetIdFromProductionItems(
      const std::list<std::unordered_map<
          ProductionItem, std::unordered_set<ProductionNodeId>,
          ProductionItemSet::ProductionItemHasher>::const_iterator>& items);
  // 传播向前看符号，同时在传播过程中构建语法分析表
  // 返回是否执行了传播过程
  // 如果未重求闭包则不会执行传播的步骤直接返回
  bool SpreadLookForwardSymbolAndConstructSyntaxAnalysisTableEntry(
      ProductionItemSetId production_item_set_id);
  // 对所有产生式节点按照ProductionNodeType分类
  // 返回array内的vector内类型对应的下标为ProductionNodeType内类型的值
  std::array<std::vector<ProductionNodeId>, 4> ClassifyProductionNodes() const;
  // SyntaxAnalysisTableMergeOptimize的子过程，分类具有相同终结节点项的语法分析表条目
  // 向equivalent_ids写入相同终结节点转移表的节点ID的组，不会执行实际合并操作
  // 不会写入只有一个项的组
  void SyntaxAnalysisTableTerminalNodeClassify(
      const std::vector<ProductionNodeId>& terminal_node_ids, size_t index,
      std::list<SyntaxAnalysisTableEntryId>&& syntax_analysis_table_entry_ids,
      std::vector<std::list<SyntaxAnalysisTableEntryId>>* equivalent_ids);
  // SyntaxAnalysisTableMergeOptimize的子过程，分类具有相同非终结节点项的语法分析表条目
  // 向equivalent_ids写入相同非终结节点转移表的节点ID的组，不会执行实际合并操作
  // 不会写入只有一个项的组
  void SyntaxAnalysisTableNonTerminalNodeClassify(
      const std::vector<ProductionNodeId>& nonterminal_node_ids, size_t index,
      std::list<SyntaxAnalysisTableEntryId>&& entry_ids,
      std::vector<std::list<SyntaxAnalysisTableEntryId>>* equivalent_ids);
  // SyntaxAnalysisTableMergeOptimize的子过程，分类具有相同项的语法分析表条目
  // 第一个参数为语法分析表内所有运算符节点ID
  // 第二个参数为语法分析表内所有终结节点ID
  // 第三个参数为语法分析表内所有非终结节点ID
  // 返回可以合并的语法分析表条目组，所有组均有至少两个条目
  std::vector<std::list<SyntaxAnalysisTableEntryId>>
  SyntaxAnalysisTableEntryClassify(
      std::vector<ProductionNodeId>&& operator_node_ids,
      std::vector<ProductionNodeId>&& terminal_node_ids,
      std::vector<ProductionNodeId>&& nonterminal_node_ids);
  // 根据给定的表项重映射语法分析表内ID
  void RemapSyntaxAnalysisTableEntryId(
      const std::unordered_map<SyntaxAnalysisTableEntryId,
                               SyntaxAnalysisTableEntryId>&
          moved_entry_id_to_new_entry_id);
  // 合并语法分析表内相同的项，同时缩减语法分析表大小
  // 会将语法分析表内的项修改为新的项
  void SyntaxAnalysisTableMergeOptimize();

  // boost-serialization用来保存语法分析表配置的函数
  template <class Archive>
  void save(Archive& ar, const unsigned int version) const;
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  // 将语法分析表配置写入文件
  void SaveConfig() const {
    dfa_generator_.SaveConfig();
    std::ofstream config_file(frontend::common::kSyntaxConfigFileName,
                              std::ios_base::binary | std::ios_base::out);
    // oarchive要在config_file析构前析构，否则文件不完整在反序列化时会抛异常
    {
      boost::archive::binary_oarchive oarchive(config_file);
      oarchive << *this;
    }
  }

  // 将给定产生式转化为字符串
  // 返回值例： IdOrEquivence -> IdOrEquivence [ Num ]
  std::string FormatSingleProductionBody(
      ProductionNodeId nonterminal_node_id,
      ProductionBodyId production_body_id) const;
  // 将给定非终结节点全部产生式转化为字符串，不同产生式间使用换行符（'\n'）分隔
  // 输出单个产生式格式同FormatSingleProductionBody
  std::string FormatProductionBodys(ProductionNodeId nonterminal_node_id);
  // 将给定项格式化
  // 返回值例：IdOrEquivence -> IdOrEquivence · [ Num ]
  std::string FormatProductionItem(const ProductionItem& production_item) const;
  // 将给定的向前看符号格式化
  // 向前看符号间通过空格分隔，符号两边不加双引号
  std::string FormatLookForwardSymbols(
      const ForwardNodesContainer& look_forward_node_ids) const;
  // 同时格式化给定项和向前看符号
  // 格式化给定项后接" 向前看符号："后接全部向前看符号
  // 给定项格式同FormatProductionItem，向前看符号格式同FormatLookForwardSymbols
  std::string FormatProductionItemAndLookForwardSymbols(
      const ProductionItem& production_item,
      const ForwardNodesContainer& look_forward_node_ids) const;
  // 将给定项集内全部项格式化
  // 产生式格式同FormatItem，产生式后跟" 向前看符号："后跟所有向前看符号
  // 向前看符号格式同FormatLookForwardSymbols
  // 项与项之间通过'\n'分隔
  std::string FormatProductionItems(
      ProductionItemSetId production_item_set_id) const;
  // 输出Info级诊断信息
  static void OutPutInfo(const std::string& info) {
    //std::cout << std::format("SyntaxGenerator Info: ") << info << std::endl;
  }
  // 输出Warning级诊断信息
  static void OutPutWarning(const std::string& warning) {
    std::cerr << std::format("SyntaxGenerator Warning: ") << warning
              << std::endl;
  }
  // 输出Error级诊断信息
  static void OutPutError(const std::string& error) {
    std::cerr << std::format("SyntaxGenerator Error: ") << error << std::endl;
  }

  // 允许序列化类访问
  friend class boost::serialization::access;

  // 对节点编号，便于区分不同节点对应的类
  // 该值仅用于生成配置代码中生成包装用户定义函数数据的类
  int node_num_;
  // 存储引用的未定义产生式
  // key是未定义的产生式名
  // tuple内的std::string是非终结产生式名
  // std::tuple<std::string, std::vector<std::string>,ProcessFunctionClassId>
  //     存储已有的产生式体信息
  // ProcessFunctionClassId是给定的包装用户定义函数数据的类的对象ID
  // bool是是否可以空规约标记
  std::unordered_multimap<
      std::string,
      std::tuple<std::string, std::vector<std::string>, ProcessFunctionClassId>>
      undefined_productions_;
  // 管理终结符号、非终结符号等的节点
  ObjectManager<BaseProductionNode> manager_nodes_;
  // 存储产生式名(终结/非终结/运算符）的符号
  UnorderedStructManager<std::string, std::hash<std::string>>
      manager_node_symbol_;
  // 存储终结节点产生式体的符号，用来防止多次添加同一正则
  UnorderedStructManager<std::string, std::hash<std::string>>
      manager_terminal_body_symbol_;
  // 产生式名ID到对应产生式节点的映射
  std::unordered_map<SymbolId, ProductionNodeId> node_symbol_id_to_node_id_;
  // 终结产生式体符号ID到对应节点ID的映射
  std::unordered_map<SymbolId, ProductionNodeId>
      production_body_symbol_id_to_node_id_;
  // 存储项集
  ObjectManager<ProductionItemSet> production_item_sets_;
  // 语法分析表ID到项集ID的映射
  std::unordered_map<SyntaxAnalysisTableEntryId, ProductionItemSetId>
      syntax_analysis_table_entry_id_to_production_item_set_id_;
  // 用户定义的根非终结产生式节点ID
  ProductionNodeId root_production_node_id_ = ProductionNodeId::InvalidId();
  // 初始语法分析表条目ID，配置写入文件
  SyntaxAnalysisTableEntryId root_syntax_analysis_table_entry_id_;
  // DFA配置生成器，配置写入文件
  frontend::generator::dfa_generator::DfaGenerator dfa_generator_;
  // 语法分析表，配置写入文件
  SyntaxAnalysisTableType syntax_analysis_table_;
  // 用户自定义函数和数据的类的对象，配置写入文件
  // 每个规约数据都必须关联唯一的包装Reduct函数的类的对象，不允许重用对象
  ProcessFunctionClassManagerType manager_process_function_class_;

  // 哈希两个ProductionNodeId用的类
  // 用于SyntaxAnalysisTableTerminalNodeClassify中分类同时支持规约与移入的数据
  struct PairOfSyntaxAnalysisTableEntryIdAndProcessFunctionClassIdHasher {
    size_t operator()(
        const std::pair<SyntaxAnalysisTableEntryId, ProcessFunctionClassId>&
            data_to_hash) const {
      return data_to_hash.first * data_to_hash.second;
    }
  };
};
template <class IdType>
inline ProductionBodyId NonTerminalProductionNode::AddBody(
    IdType&& body, ProcessFunctionClassId class_for_reduct_id_) {
  ProductionBodyId body_id(nonterminal_bodys_.size());
  // 将输入插入到产生式体向量中，无删除相同产生式功能
  nonterminal_bodys_.emplace_back(std::forward<IdType>(body),
                                  class_for_reduct_id_);
  return body_id;
}

template <class ProcessFunctionClass>
inline ProductionNodeId SyntaxGenerator::AddNonTerminalNode(
    std::string&& node_symbol, std::vector<std::string>&& subnode_symbols) {
  ProcessFunctionClassId class_id =
      CreateProcessFunctionClassObject<ProcessFunctionClass>();
  return AddNonTerminalNode(std::move(node_symbol), std::move(subnode_symbols),
                            class_id);
}

template <class ForwardNodeIdContainer>
inline std::pair<
    SyntaxGenerator::ProductionItemAndForwardNodesContainer::iterator, bool>
SyntaxGenerator::AddItemAndForwardNodeIdsToCore(
    ProductionItemSetId production_item_set_id,
    const ProductionItem& production_item,
    ForwardNodeIdContainer&& forward_node_ids) {
  assert(production_item_set_id.IsValid());
  auto result = GetProductionItemSet(production_item_set_id)
                    .AddItemAndForwardNodeIds(
                        production_item,
                        std::forward<ForwardNodeIdContainer>(forward_node_ids));
  if (result.second) {
    // 如果新插入了项则记录该项属于的新项集ID
    AddProductionItemBelongToProductionItemSetId(production_item,
                                                 production_item_set_id);
  }
  return result;
}

// 在AddItemAndForwardNodeIdsToCore基础上设置添加的项为核心项

template <class ForwardNodeIdContainer>
inline std::pair<
    SyntaxGenerator::ProductionItemAndForwardNodesContainer::iterator, bool>
SyntaxGenerator::AddMainItemAndForwardNodeIdsToCore(
    ProductionItemSetId production_item_set_id,
    const ProductionItem& production_item,
    ForwardNodeIdContainer&& forward_node_ids) {
  auto result = GetProductionItemSet(production_item_set_id)
                    .AddMainItemAndForwardNodeIds(
                        production_item,
                        std::forward<ForwardNodeIdContainer>(forward_node_ids));
  if (result.second) {
    // 向项集中插入了新的项则更新该项属于的项集
    AddProductionItemBelongToProductionItemSetId(production_item,
                                                 production_item_set_id);
  }
  return result;
}

template <class Archive>
inline void SyntaxGenerator::save(Archive& ar,
                                  const unsigned int version) const {
  ar << root_syntax_analysis_table_entry_id_;
  ar << syntax_analysis_table_;
  ar << manager_process_function_class_;
}
}  // namespace frontend::generator::syntax_generator

#endif  // !GENERATOR_SYNTAXGENERATOR_SYNTAXGENERATOR_H_