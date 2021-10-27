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
#include <boost/serialization/variant.hpp>
#include <boost/serialization/vector.hpp>
#include <cassert>
#include <format>
#include <fstream>
#include <functional>
#include <regex>
#include <tuple>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/object_manager.h"
#include "Common/unordered_struct_manager.h"
#include "Generator/DfaGenerator/dfa_generator.h"
#include "process_function_interface.h"

namespace frontend::parser::syntax_machine {
class SyntaxMachine;
}

// TODO 添加删除未使用产生式的功能
namespace frontend::generator::syntax_generator {
using frontend::common::ObjectManager;
using frontend::common::UnorderedStructManager;

class SyntaxGenerator {
 private:
  class BaseProductionNode;
  class ParsingTableEntry;
  class Core;
  // ID包装器用来区别不同ID的枚举
  enum class WrapperLabel {
    kCoreId,
    kPriorityLevel,
    kPointIndex,
    kParsingTableEntryId,
    kProductionBodyId,
    kProcessFunctionClassId
  };
  // 产生式节点类型
  using ProductionNodeType = frontend::common::ProductionNodeType;
  // 核心项ID
  using CoreId = ObjectManager<Core>::ObjectId;
  // 语法分析表条目ID
  using ParsingTableEntryId =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kParsingTableEntryId>;

  // 运算符优先级，数字越大优先级越高，仅对运算符节点有效
  // 与TailNodePriority意义不同，该优先级影响语法分析过程
  // 当遇到连续的运算符时决定移入还是归并
  using OperatorPriority =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kPriorityLevel>;
  // 词法分析中词的优先级
  using WordPriority =
      frontend::generator::dfa_generator::DfaGenerator::WordPriority;
  // 点的位置
  using PointIndex =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kPointIndex>;
  // 语法分析表类型
  using ParsingTableType = std::vector<ParsingTableEntry>;
  // 非终结节点内产生式编号
  using ProductionBodyId =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kProductionBodyId>;
  // 产生式节点ID
  using ProductionNodeId = ObjectManager<BaseProductionNode>::ObjectId;
  // 符号ID
  using SymbolId =
      UnorderedStructManager<std::string, std::hash<std::string>>::ObjectId;
  // 向前看符号容器
  using ForwardNodesContainerType = std::unordered_set<ProductionNodeId>;
  // 包装用户自定义函数和数据的类的已分配对象ID
  using ProcessFunctionClassId =
      frontend::common::ObjectManager<frontend::generator::syntax_generator::
                                          ProcessFunctionInterface>::ObjectId;
  // 管理包装用户自定义函数和数据的类的已分配对象的容器
  using ProcessFunctionClassManagerType =
      ObjectManager<ProcessFunctionInterface>;
  // 运算符结合类型：左结合，右结合
  using OperatorAssociatityType = frontend::common::OperatorAssociatityType;
  // 分析动作类型：规约，移入，移入和规约，报错，接受
  enum class ActionType { kReduct, kShift, kShiftReduct, kError, kAccept };
  // 内核内单个项集，参数从左到右依次为
  // 产生式节点ID，产生式体ID，产生式体中点的位置
  using CoreItem = std::tuple<ProductionNodeId, ProductionBodyId, PointIndex>;
  // 用来哈希CoreItem的类
  // 在SyntaxGenerator声明结束后用该类特化std::hash<CoreItem>
  // 以允许CoreItem可以作为std::unordered_map键值
  struct CoreItemHasher {
    size_t operator()(const CoreItem& core_item) const {
      auto& [production_node_id, production_body_id, point_index] = core_item;
      return production_node_id * production_body_id * point_index;
    }
  };
  // 存储项和向前看节点的容器
  using CoreItemAndForwardNodesContainer =
      std::unordered_map<CoreItem, ForwardNodesContainerType, CoreItemHasher>;

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
  void ParsingTableConstruct();

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
      PointIndex point_index) const {
    return GetNodeSymbolStringFromProductionNodeId(GetProductionNodeIdInBody(
        production_node_id, production_body_id, point_index));
  }
  // 通过项查询下一个移入的节点名
  const std::string& GetNextNodeToShiftSymbolString(
      const CoreItem& core_item) const {
    auto& [production_node_id, production_body_id, point_index] = core_item;
    return GetNextNodeToShiftSymbolString(production_node_id,
                                          production_body_id, point_index);
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
      PointIndex point_index) const {
    return GetProductionNode(production_node_id)
        .GetProductionNodeInBody(production_body_id, point_index);
  }
  // 设置根产生式ID
  void SetRootProductionNodeId(ProductionNodeId root_production_node_id) {
    root_production_node_id_ = root_production_node_id;
  }
  // 获取根产生式ID
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
  void AddKeyWord(const std::string& key_word);
  // 新建终结节点，返回节点ID
  // 该函数用于声明终结产生式的正则定义
  // 节点已存在且给定symbol_id不同于已有ID则返回ProductionNodeId::InvalidId()
  // 第三个参数是词优先级，0保留为普通词的优先级，1保留为运算符优先级
  // 2保留为关键字优先级，其余的优先级尚未指定
  // ！！！词优先级与运算符优先级不同，请注意区分！！！
  ProductionNodeId AddTerminalNode(
      const std::string& node_symbol, const std::string& body_symbol,
      WordPriority node_priority = WordPriority(0));
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
      const std::string& operator_symbol,
      OperatorAssociatityType binary_operator_associatity_type,
      OperatorPriority binary_operator_priority_level);
  // 新建左侧单目运算符节点，返回节点ID
  // 节点已存在则返回ProductionNodeId::InvalidId()
  // 默认添加的运算符词法分析优先级高于普通终结产生式低于关键字
  // 提供左侧单目运算符版本
  ProductionNodeId AddLeftUnaryOperatorNode(
      const std::string& operator_symbol,
      OperatorAssociatityType unary_operator_associatity_type,
      OperatorPriority unary_operator_priority_level);
  // 新建复用的左侧单目和双目运算符节点，返回节点ID
  // 节点已存在则返回ProductionNodeId::InvalidId()
  // 默认添加的运算符词法分析优先级高于普通终结产生式低于关键字
  // 提供左侧单目运算符版本
  ProductionNodeId AddBinaryUnaryOperatorNode(
      const std::string& operator_symbol,
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
  ParsingTableEntryId AddParsingTableEntry() {
    ParsingTableEntryId parsing_table_entry_id(syntax_parsing_table_.size());
    syntax_parsing_table_.emplace_back();
    return parsing_table_entry_id;
  }
  // 获取core_id对应产生式项集
  const Core& GetCore(CoreId core_id) const {
    assert(core_id < cores_.Size());
    return cores_[core_id];
  }
  // 获取core_id对应产生式项集
  Core& GetCore(CoreId core_id) {
    return const_cast<Core&>(
        static_cast<const SyntaxGenerator&>(*this).GetCore(core_id));
  }
  // 设置语法分析表条目ID到核心ID的映射
  void SetParsingTableEntryIdToCoreIdMapping(
      ParsingTableEntryId parsing_table_entry_id, CoreId core_id) {
    parsing_table_entry_id_to_core_id_[parsing_table_entry_id] = core_id;
  }
  // 获取语法分析表条目ID对应的核心ID
  CoreId GetCoreIdFromParsingTableEntryId(
      ParsingTableEntryId parsing_table_entry_id) {
    auto iter = parsing_table_entry_id_to_core_id_.find(parsing_table_entry_id);
    assert(iter != parsing_table_entry_id_to_core_id_.end());
    return iter->second;
  }
  // 添加新的核心，需要更新语法分析表条目到核心ID的映射
  CoreId EmplaceCore() {
    ParsingTableEntryId parsing_table_entry_id = AddParsingTableEntry();
    CoreId core_id = cores_.EmplaceObject(parsing_table_entry_id);
    cores_[core_id].SetCoreId(core_id);
    SetParsingTableEntryIdToCoreIdMapping(parsing_table_entry_id, core_id);
    return core_id;
  }
  // 向项集中添加项和相应的向前看符号，可以传入单个未包装ID
  // 返回插入位置和是否插入
  // 如果添加了新项或向前看符号则设置闭包无效
  // 如果该项已存在则仅添加向前看符号
  // 如果向项集中新添加了项则自动添加该项所添加到的新核心ID的记录
  template <class ForwardNodeIdContainer>
  std::pair<CoreItemAndForwardNodesContainer::iterator, bool>
  AddItemAndForwardNodeIdsToCore(CoreId core_id, const CoreItem& core_item,
                                 ForwardNodeIdContainer&& forward_node_ids);
  // 在AddItemAndForwardNodeIdsToCore基础上设置添加的项为核心项
  template <class ForwardNodeIdContainer>
  std::pair<CoreItemAndForwardNodesContainer::iterator, bool>
  AddMainItemAndForwardNodeIdsToCore(CoreId core_id, const CoreItem& core_item,
                                     ForwardNodeIdContainer&& forward_node_ids);
  // 向给定项中添加向前看符号，同时支持输入单个符号和符号容器
  // 返回是否添加
  // 要求项已经存在，否则请调用AddItemAndForwardNodeIds及类似函数
  // 如果添加了新的向前看符号则设置闭包无效
  template <class ForwardNodeIdContainer>
  bool AddForwardNodes(CoreId core_id, const CoreItem& core_item,
                       ForwardNodeIdContainer&& forward_node_ids) {
    return GetCore(core_id).AddForwardNodes(
        core_item, std::forward<ForwardNodeIdContainer>(forward_node_ids));
  }
  // 记录核心项所属的核心ID
  void AddCoreItemBelongToCoreId(const CoreItem& core_item, CoreId core_id);
  // 获取项集的全部核心项
  const std::list<CoreItemAndForwardNodesContainer::const_iterator>
  GetCoreMainItems(CoreId core_id) const {
    return GetCore(core_id).GetMainItems();
  }
  // 获取单个核心项所属的全部核心
  const std::list<CoreId>& GetCoreIdFromCoreItem(
      ProductionNodeId production_node_id, ProductionBodyId body_id,
      PointIndex point_index);
  // 获取向前看符号集
  const ForwardNodesContainerType& GetForwardNodeIds(
      CoreId core_id, const CoreItem& core_item) const {
    return GetCore(core_id).GetItemsAndForwardNodeIds().at(core_item);
  }

  // First的子过程，提取一个非终结节点中所有的first符号
  // 第二个参数指向存储结果的集合
  // 第三个参数用来存储已经处理过的节点，防止无限递归，初次调用应传入空集合
  // 如果输入ProductionNodeId::InvalidId()则返回空集合
  void GetNonTerminalNodeFirstNodeIds(
      ProductionNodeId production_node_id, ForwardNodesContainerType* result,
      std::unordered_set<ProductionNodeId>&& processed_nodes =
          std::unordered_set<ProductionNodeId>());
  // 闭包操作中的first操作，前三个参数标志β的位置
  // 采用三个参数因为非终结节点可能规约为空节点，需要向下查找终结节点
  // 向前看节点可以规约为空节点则添加相应向前看符号
  // 然后继续向前查找直到结尾或不可空规约非终结节点或终结节点
  ForwardNodesContainerType First(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index, const ForwardNodesContainerType& next_node_ids);
  // 获取核心的全部项
  const CoreItemAndForwardNodesContainer& GetCoreItemsAndForwardNodes(
      CoreId core_id) {
    return GetCore(core_id).GetItemsAndForwardNodeIds();
  }
  // 获取语法分析表条目
  ParsingTableEntry& GetParsingTableEntry(
      ParsingTableEntryId production_node_id) {
    assert(production_node_id < syntax_parsing_table_.size());
    return syntax_parsing_table_[production_node_id];
  }
  // 设置根语法分析表条目ID
  void SetRootParsingTableEntryId(
      ParsingTableEntryId root_parsing_table_entry_id) {
    root_parsing_table_entry_id_ = root_parsing_table_entry_id;
  }
  // 设置项集闭包有效（避免每次Goto都求闭包）
  // 仅应由CoreClosure调用
  void SetCoreClosureAvailable(CoreId core_id) {
    GetCore(core_id).SetClosureAvailable();
  }
  bool IsClosureAvailable(CoreId core_id) {
    return GetCore(core_id).IsClosureAvailable();
  }
  // 对给定核心项求闭包，结果存在原位置
  // 自动添加所有当前位置可以空规约的项的后续项
  // 返回是否重求闭包
  // 重求闭包则清空语法分析表条目
  bool CoreClosure(CoreId core_id);
  // 获取给定的项构成的项集，如果不存在则返回CoreId::InvalidId()
  // 输入指向转移前项的迭代器
  CoreId GetCoreIdFromCoreItems(
      const std::list<std::unordered_map<
          SyntaxGenerator::CoreItem, std::unordered_set<ProductionNodeId>,
          CoreItemHasher>::const_iterator>& items);
  // 传播向前看符号，同时在传播过程中构建语法分析表
  // 返回是否执行了传播过程
  // 如果未重求闭包则不会执行传播的步骤直接返回
  bool SpreadLookForwardSymbolAndConstructParsingTableEntry(CoreId core_id);
  // 对所有产生式节点按照ProductionNodeType分类
  // 返回array内的vector内类型对应的下标为ProductionNodeType内类型的值
  std::array<std::vector<ProductionNodeId>, sizeof(ProductionNodeType)>
  ClassifyProductionNodes() const;
  // ParsingTableMergeOptimize的子过程，分类具有相同终结节点项的语法分析表条目
  // 向equivalent_ids写入相同终结节点转移表的节点ID的组，不会执行实际合并操作
  // 不会写入只有一个项的组
  void ParsingTableTerminalNodeClassify(
      const std::vector<ProductionNodeId>& terminal_node_ids, size_t index,
      std::list<ParsingTableEntryId>&& parsing_table_entry_ids,
      std::vector<std::list<ParsingTableEntryId>>* equivalent_ids);
  // ParsingTableMergeOptimize的子过程，分类具有相同非终结节点项的语法分析表条目
  // 向equivalent_ids写入相同非终结节点转移表的节点ID的组，不会执行实际合并操作
  // 不会写入只有一个项的组
  void ParsingTableNonTerminalNodeClassify(
      const std::vector<ProductionNodeId>& nonterminal_node_ids, size_t index,
      std::list<ParsingTableEntryId>&& entry_ids,
      std::vector<std::list<ParsingTableEntryId>>* equivalent_ids);
  // ParsingTableMergeOptimize的子过程，分类具有相同项的语法分析表条目
  // 第一个参数为语法分析表内所有终结节点ID
  // 第二个参数为语法分析表内所有非终结节点ID
  // 返回可以合并的语法分析表条目组，所有组均有至少两个条目
  std::vector<std::list<ParsingTableEntryId>> ParsingTableEntryClassify(
      std::vector<ProductionNodeId>&& operator_node_ids,
      std::vector<ProductionNodeId>&& terminal_node_ids,
      std::vector<ProductionNodeId>&& nonterminal_node_ids);
  // 根据给定的表项重映射语法分析表内ID
  void RemapParsingTableEntryId(
      const std::unordered_map<ParsingTableEntryId, ParsingTableEntryId>&
          moved_entry_id_to_new_entry_id);
  // 合并语法分析表内相同的项，同时缩减语法分析表大小
  // 会将语法分析表内的项修改为新的项
  void ParsingTableMergeOptimize();

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
  std::string FormatCoreItem(const CoreItem& core_item) const;
  // 将给定的向前看符号格式化
  // 向前看符号间通过空格分隔，符号两边不加双引号
  std::string FormatLookForwardSymbols(
      const ForwardNodesContainerType& look_forward_node_ids) const;
  // 同时格式化给定项和向前看符号
  // 格式化给定项后接" 向前看符号："后接全部向前看符号
  // 给定项格式同FormatCoreItem，向前看符号格式同FormatLookForwardSymbols
  std::string FormatCoreItemAndLookForwardSymbols(
      const CoreItem& core_item,
      const ForwardNodesContainerType& look_forward_node_ids) const;
  // 将给定核心内全部项格式化
  // 产生式格式同FormatItem，产生式后跟" 向前看符号："后跟所有向前看符号
  // 向前看符号格式同FormatLookForwardSymbols
  // 项与项之间通过'\n'分隔
  std::string FormatCoreItems(CoreId core_id) const;
  // 输出Info级诊断信息
  static void OutPutInfo(const std::string& info) {
    std::cout << std::format("SyntaxGenerator Info: ") << info << std::endl;
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

  // 声明语法分析机为友类，便于其使用各种定义
  friend class frontend::parser::syntax_machine::SyntaxMachine;
  // 暴露部分内部类型，从而在boost_serialization中注册
  friend struct ExportSyntaxGeneratorInsideTypeForSerialization;
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
  ObjectManager<Core> cores_;
  // 语法分析表ID到核心ID的映射
  std::unordered_map<ParsingTableEntryId, CoreId>
      parsing_table_entry_id_to_core_id_;
  // 根产生式条目ID
  ProductionNodeId root_production_node_id_ = ProductionNodeId::InvalidId();
  // 初始语法分析表条目ID，配置写入文件
  ParsingTableEntryId root_parsing_table_entry_id_;
  // DFA配置生成器，配置写入文件
  frontend::generator::dfa_generator::DfaGenerator dfa_generator_;
  // 语法分析表，配置写入文件
  ParsingTableType syntax_parsing_table_;
  // 用户自定义函数和数据的类的对象，配置写入文件
  // 每个规约数据都必须关联唯一的包装Reduct函数的类的对象，不允许重用对象
  ProcessFunctionClassManagerType manager_process_function_class_;

  // 哈希两个ProductionNodeId用的类
  // 用于ParsingTableTerminalNodeClassify中分类同时支持规约与移入的数据
  struct PairOfParsingTableEntryIdAndProcessFunctionClassIdHasher {
    size_t operator()(
        const std::pair<ParsingTableEntryId, ProcessFunctionClassId>&
            data_to_hash) const {
      return data_to_hash.first * data_to_hash.second;
    }
  };
  //所有产生式节点类都应继承自该类
  class BaseProductionNode {
   public:
    BaseProductionNode(ProductionNodeType type, SymbolId symbol_id)
        : base_type_(type),
          base_symbol_id_(symbol_id),
          base_id_(ProductionNodeId::InvalidId()) {}
    BaseProductionNode(const BaseProductionNode&) = delete;
    BaseProductionNode& operator=(const BaseProductionNode&) = delete;
    BaseProductionNode(BaseProductionNode&& base_production_node)
        : base_type_(base_production_node.base_type_),
          base_id_(base_production_node.base_id_),
          base_symbol_id_(base_production_node.base_symbol_id_) {}
    BaseProductionNode& operator=(BaseProductionNode&& base_production_node);
    virtual ~BaseProductionNode() {}

    struct NodeData {
      ProductionNodeType node_type;
      std::string node_symbol;
    };

    void SetType(ProductionNodeType type) { base_type_ = type; }
    ProductionNodeType Type() const { return base_type_; }
    void SetThisNodeId(ProductionNodeId production_node_id) {
      base_id_ = production_node_id;
    }
    ProductionNodeId Id() const { return base_id_; }
    void SetSymbolId(SymbolId production_node_id) {
      base_symbol_id_ = production_node_id;
    }
    SymbolId GetNodeSymbolId() const { return base_symbol_id_; }

    // 获取给定项对应的产生式ID，返回point_index后面的ID
    // point_index越界时返回ProducNodeId::InvalidId()
    // 为了支持向前看多个节点允许越界
    // 返回点右边的产生式ID，不存在则返回ProductionNodeId::InvalidId()
    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) const = 0;

   private:
    // 节点类型
    ProductionNodeType base_type_;
    // 节点ID
    ProductionNodeId base_id_;
    // 节点符号ID
    SymbolId base_symbol_id_;
  };

  class TerminalProductionNode : public BaseProductionNode {
   public:
    TerminalProductionNode(SymbolId node_symbol_id, SymbolId body_symbol_id)
        : BaseProductionNode(ProductionNodeType::kTerminalNode,
                             node_symbol_id) {
      SetBodySymbolId(body_symbol_id);
    }
    TerminalProductionNode(const TerminalProductionNode&) = delete;
    TerminalProductionNode& operator=(const TerminalProductionNode&) = delete;
    TerminalProductionNode(TerminalProductionNode&& terminal_production_node)
        : BaseProductionNode(std::move(terminal_production_node)),
          body_symbol_id_(std::move(terminal_production_node.body_symbol_id_)) {
    }
    TerminalProductionNode& operator=(
        TerminalProductionNode&& terminal_production_node) {
      BaseProductionNode::operator=(std::move(terminal_production_node));
      body_symbol_id_ = std::move(terminal_production_node.body_symbol_id_);
      return *this;
    }
    using NodeData = BaseProductionNode::NodeData;

    // 获取/设置产生式体名
    SymbolId GetBodySymbolId() { return body_symbol_id_; }
    void SetBodySymbolId(SymbolId body_symbol_id) {
      body_symbol_id_ = body_symbol_id;
    }
    // 在越界时也有返回值为了支持获取下一个/下下个体内节点的操作
    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id,
        PointIndex point_index) const override;

   private:
    // 产生式体名
    SymbolId body_symbol_id_;
  };

  class OperatorProductionNode : public BaseProductionNode {
   public:
    OperatorProductionNode(SymbolId node_symbol_id,
                           OperatorAssociatityType associatity_type,
                           OperatorPriority priority_level)
        : BaseProductionNode(ProductionNodeType::kOperatorNode, node_symbol_id),
          operator_associatity_type_(associatity_type),
          operator_priority_level_(priority_level) {}
    OperatorProductionNode(const OperatorProductionNode&) = delete;
    OperatorProductionNode& operator=(const OperatorProductionNode&) = delete;
    OperatorProductionNode(OperatorProductionNode&& operator_production_node)
        : BaseProductionNode(std::move(operator_production_node)),
          operator_associatity_type_(
              std::move(operator_production_node.operator_associatity_type_)),
          operator_priority_level_(
              std::move(operator_production_node.operator_priority_level_)) {}
    OperatorProductionNode& operator=(
        OperatorProductionNode&& operator_production_node);

    struct NodeData : public TerminalProductionNode::NodeData {
      std::string symbol_;
    };
    void SetAssociatityType(OperatorAssociatityType type) {
      operator_associatity_type_ = type;
    }
    OperatorAssociatityType GetAssociatityType() const {
      return operator_associatity_type_;
    }
    void SetPriorityLevel(OperatorPriority level) {
      operator_priority_level_ = level;
    }
    OperatorPriority GetPriorityLevel() const {
      return operator_priority_level_;
    }
    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) const {
      assert(false);
      // 防止警告
      return ProductionNodeId();
    }

   private:
    // 运算符结合性
    OperatorAssociatityType operator_associatity_type_;
    // 运算符优先级
    OperatorPriority operator_priority_level_;
  };

  class NonTerminalProductionNode : public BaseProductionNode {
   public:
    struct ProductionBodyType {
      template <class BodyContainer>
      ProductionBodyType(BodyContainer&& production_body_,
                         ProcessFunctionClassId class_for_reduct_id_)
          : production_body(std::forward<BodyContainer>(production_body_)),
            class_for_reduct_id(class_for_reduct_id_) {
        cores_items_in_.resize(production_body.size() + 1);
      }

      // 产生式体
      std::vector<ProductionNodeId> production_body;
      // 每个产生式体对应的所有项所存在的项集
      // 大小为production_body.size() + 1
      std::vector<std::list<CoreId>> cores_items_in_;
      // 规约产生式使用的类的ID
      ProcessFunctionClassId class_for_reduct_id;
    };

    NonTerminalProductionNode(SymbolId symbol_id)
        : BaseProductionNode(ProductionNodeType::kNonTerminalNode, symbol_id) {}
    template <class IdType>
    NonTerminalProductionNode(SymbolId symbol_id, IdType&& body)
        : BaseProductionNode(ProductionNodeType::kNonTerminalNode, symbol_id),
          nonterminal_bodys_(std::forward<IdType>(body)) {}
    NonTerminalProductionNode(const NonTerminalProductionNode&) = delete;
    NonTerminalProductionNode& operator=(const NonTerminalProductionNode&) =
        delete;
    NonTerminalProductionNode(NonTerminalProductionNode&& node)
        : BaseProductionNode(std::move(node)),
          nonterminal_bodys_(std::move(node.nonterminal_bodys_)),
          could_empty_reduct_(std::move(node.could_empty_reduct_)) {}
    NonTerminalProductionNode& operator=(NonTerminalProductionNode&& node) {
      BaseProductionNode::operator=(std::move(node));
      nonterminal_bodys_ = std::move(node.nonterminal_bodys_);
      could_empty_reduct_ = std::move(node.could_empty_reduct_);
      return *this;
    }

    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id,
        PointIndex point_index) const override;

    // 添加一个产生式体，要求IdType为一个vector，按序存储产生式节点ID
    template <class IdType>
    ProductionBodyId AddBody(IdType&& body,
                             ProcessFunctionClassId class_for_reduct_id_);
    // 获取产生式的一个体
    const ProductionBodyType& GetProductionBody(
        ProductionBodyId production_body_id) {
      assert(production_body_id < nonterminal_bodys_.size());
      return nonterminal_bodys_[production_body_id];
    }
    // 设置给定产生式体ID对应的ProcessFunctionClass的ID
    void SetBodyProcessFunctionClassId(
        ProductionBodyId body_id, ProcessFunctionClassId class_for_reduct_id) {
      assert(body_id < nonterminal_bodys_.size());
      nonterminal_bodys_[body_id].class_for_reduct_id = class_for_reduct_id;
    }
    const ProductionBodyType& GetBody(ProductionBodyId body_id) const {
      return nonterminal_bodys_[body_id];
    }
    const std::vector<ProductionBodyType>& GetAllBody() const {
      return nonterminal_bodys_;
    }
    // 获取全部产生式体ID
    std::vector<ProductionBodyId> GetAllBodyIds() const;
    // 设置该产生式不可以空规约
    void SetProductionShouldNotEmptyReduct() { could_empty_reduct_ = false; }
    void SetProductionCouldBeEmptyRedut() { could_empty_reduct_ = true; }
    // 查询该产生式是否可以空规约
    bool CouldBeEmptyReduct() const { return could_empty_reduct_; }
    // 添加项所属的核心ID
    // 要求不与已有的核心ID重复
    void AddCoreItemBelongToCoreId(ProductionBodyId body_id,
                                   PointIndex point_index, CoreId core_id);
    // 获取核心中项所属的全部核心ID
    const std::list<CoreId>& GetCoreIdFromCoreItem(ProductionBodyId body_id,
                                                   PointIndex point_index) {
      return nonterminal_bodys_[body_id].cores_items_in_[point_index];
    }
    // 返回给定产生式体ID对应的ProcessFunctionClass的ID
    ProcessFunctionClassId GetBodyProcessFunctionClassId(
        ProductionBodyId body_id) const {
      assert(body_id < nonterminal_bodys_.size());
      return nonterminal_bodys_[body_id].class_for_reduct_id;
    }

   private:
    // 存储产生式体
    std::vector<ProductionBodyType> nonterminal_bodys_;
    // 标志该产生式是否可能为空
    bool could_empty_reduct_ = false;
  };

  // 文件尾节点
  class EndNode : public BaseProductionNode {
   public:
    EndNode()
        : BaseProductionNode(ProductionNodeType::kEndNode,
                             SymbolId::InvalidId()) {}
    EndNode(const EndNode&) = delete;
    EndNode& operator=(const EndNode&) = delete;
    EndNode(EndNode&& end_node) : BaseProductionNode(std::move(end_node)) {}
    EndNode& operator=(EndNode&& end_node) {
      BaseProductionNode::operator=(std::move(end_node));
      return *this;
    }
    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id,
        PointIndex point_index) const override {
      assert(false);
      return ProductionNodeId::InvalidId();
    }
    // 获取包装用户自定义函数数据的类的对象ID
    virtual ProcessFunctionClassId GetBodyProcessFunctionClassId(
        ProductionBodyId production_body_id) const {
      assert(false);
      return ProcessFunctionClassId::InvalidId();
    }
  };

  // 项集与向前看符号
  class Core {
   public:
    Core() {}
    Core(ParsingTableEntryId parsing_table_entry_id)
        : parsing_table_entry_id_(parsing_table_entry_id) {}
    template <class ItemAndForwardNodes>
    Core(ItemAndForwardNodes&& item_and_forward_node_ids,
         ParsingTableEntryId parsing_table_entry_id)
        : core_closure_available_(false),
          parsing_table_entry_id_(parsing_table_entry_id),
          item_and_forward_node_ids_(
              std::forward<ItemAndForwardNodes>(item_and_forward_node_ids)) {}
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;
    Core(Core&& core)
        : core_closure_available_(std::move(core.core_closure_available_)),
          core_id_(std::move(core.core_id_)),
          parsing_table_entry_id_(std::move(core.parsing_table_entry_id_)),
          item_and_forward_node_ids_(
              std::move(core.item_and_forward_node_ids_)) {}
    Core& operator=(Core&& core);

    // 返回给定Item插入后的iterator和是否成功插入bool标记
    // 如果Item已存在则仅添加向前看符号
    // bool在不存在给定item且插入成功时为true
    // 可以使用单个未包装ID
    // 如果添加了新项或向前看符号则设置闭包无效
    template <class ForwardNodeIdContainer>
    std::pair<CoreItemAndForwardNodesContainer::iterator, bool>
    AddItemAndForwardNodeIds(const CoreItem& item,
                             ForwardNodeIdContainer&& forward_node_ids);
    // 在AddItemAndForwardNodeIds基础上设置添加的项为核心项
    template <class ForwardNodeIdContainer>
    std::pair<CoreItemAndForwardNodesContainer::iterator, bool>
    AddMainItemAndForwardNodeIds(const CoreItem& item,
                                 ForwardNodeIdContainer&& forward_node_ids) {
      auto result = AddItemAndForwardNodeIds(
          item, std::forward<ForwardNodeIdContainer>(forward_node_ids));
      SetMainItem(result.first);
      return result;
    }

    // 判断给定item是否在该项集内，在则返回true
    bool IsItemIn(const CoreItem& item) const {
      return item_and_forward_node_ids_.find(item) !=
             item_and_forward_node_ids_.end();
    }
    // 判断该项集求的闭包是否有效
    bool IsClosureAvailable() const { return core_closure_available_; }
    // 设置core_id
    void SetCoreId(CoreId core_id) { core_id_ = core_id; }
    // 获取core_id
    CoreId GetCoreId() const { return core_id_; }

    // 设置该项集求的闭包有效，仅应由闭包函数调用
    void SetClosureAvailable() { core_closure_available_ = true; }
    // 获取全部核心项
    const std::list<CoreItemAndForwardNodesContainer::const_iterator>&
    GetMainItems() const {
      return main_items_;
    }
    // 设置一项为核心项
    // 要求该核心项未添加过
    // 设置闭包无效
    void SetMainItem(
        CoreItemAndForwardNodesContainer ::const_iterator& item_iter);
    // 获取全部项和对应的向前看节点
    const CoreItemAndForwardNodesContainer& GetItemsAndForwardNodeIds() const {
      return item_and_forward_node_ids_;
    }
    // 获取项对应的语法分析表条目ID
    ParsingTableEntryId GetParsingTableEntryId() const {
      return parsing_table_entry_id_;
    }

    // 向给定项中添加向前看符号，同时支持输入单个符号和符号容器
    // 返回是否添加
    // 要求项已经存在，否则请调用AddItemAndForwardNodeIds
    // 如果添加了新的向前看符号则设置闭包无效
    template <class ForwardNodeIdContainer>
    bool AddForwardNodes(const CoreItem& item,
                         ForwardNodeIdContainer&& forward_node_id_container);
    size_t Size() const { return item_and_forward_node_ids_.size(); }

   private:
    // 向给定项中添加向前看符号
    // 返回是否添加
    // 如果添加了新的向前看符号则设置闭包无效
    template <class ForwardNodeIdContainer>
    bool AddForwardNodes(
        const std::unordered_map<
            CoreItem, std::unordered_set<ProductionNodeId>>::iterator& iter,
        ForwardNodeIdContainer&& forward_node_id_container);
    // 设置闭包无效
    // 应由每个修改了项/项的向前看符号的函数调用
    void SetClosureNotAvailable() { core_closure_available_ = false; }

    // 存储指向核心项的迭代器
    std::list<CoreItemAndForwardNodesContainer::const_iterator> main_items_;
    // 该项集求的闭包是否有效（求过闭包则为true）
    bool core_closure_available_ = false;
    // 项集ID
    CoreId core_id_ = CoreId::InvalidId();
    // 项对应的语法分析表条目ID
    ParsingTableEntryId parsing_table_entry_id_ =
        ParsingTableEntryId::InvalidId();
    // 项和对应的向前看符号
    CoreItemAndForwardNodesContainer item_and_forward_node_ids_;
  };

  // 语法分析表条目
  class ParsingTableEntry {
   public:
    // 前向声明三种派生类，为了虚函数可以返回相应的类型
    class ShiftAttachedData;
    class ReductAttachedData;
    class ShiftReductAttachedData;

    class ActionAndAttachedDataInterface {
     public:
      ActionAndAttachedDataInterface(ActionType action_type)
          : action_type_(action_type) {}
      ActionAndAttachedDataInterface(const ActionAndAttachedDataInterface&) =
          default;
      virtual ~ActionAndAttachedDataInterface() {}

      ActionAndAttachedDataInterface& operator=(
          const ActionAndAttachedDataInterface&) = default;

      virtual bool operator==(const ActionAndAttachedDataInterface&
                                  attached_data_interface) const = 0 {
        return action_type_ == attached_data_interface.action_type_;
      }

      virtual const ShiftAttachedData& GetShiftAttachedData() const;
      virtual const ReductAttachedData& GetReductAttachedData() const;
      virtual const ShiftReductAttachedData& GetShiftReductAttachedData() const;

      ActionType GetActionType() const { return action_type_; }
      void SetActionType(ActionType action_type) { action_type_ = action_type; }

      template <class Archive>
      void serialize(Archive& ar, const unsigned int version) {
        ar& action_type_;
      }

     private:
      // 提供默认构造函数供序列化时构建对象
      ActionAndAttachedDataInterface() = default;

      // 允许序列化用的类访问
      friend class boost::serialization::access;
      // 允许语法分析表条目调用内部接口
      friend class ParsingTableEntry;
      // 暴露部分内部类型，从而在boost_serialization中注册
      friend struct ExportSyntaxGeneratorInsideTypeForSerialization;

      virtual ShiftAttachedData& GetShiftAttachedData() {
        return const_cast<ShiftAttachedData&>(
            static_cast<const ActionAndAttachedDataInterface&>(*this)
                .GetShiftAttachedData());
      }
      virtual ReductAttachedData& GetReductAttachedData() {
        return const_cast<ReductAttachedData&>(
            static_cast<const ActionAndAttachedDataInterface&>(*this)
                .GetReductAttachedData());
      }
      virtual ShiftReductAttachedData& GetShiftReductAttachedData() {
        return const_cast<ShiftReductAttachedData&>(
            static_cast<const ActionAndAttachedDataInterface&>(*this)
                .GetShiftReductAttachedData());
      }

      ActionType action_type_;
    };
    // 执行移入动作时的附属数据
    class ShiftAttachedData : public ActionAndAttachedDataInterface {
     public:
      ShiftAttachedData(ParsingTableEntryId next_entry_id)
          : ActionAndAttachedDataInterface(ActionType::kShift),
            next_entry_id_(next_entry_id) {}
      ShiftAttachedData(const ShiftAttachedData&) = default;

      ShiftAttachedData& operator=(const ShiftAttachedData&) = default;
      virtual bool operator==(const ActionAndAttachedDataInterface&
                                  shift_attached_data) const override;

      virtual const ShiftAttachedData& GetShiftAttachedData() const override {
        return *this;
      }

      ParsingTableEntryId GetNextParsingTableEntryId() const {
        return next_entry_id_;
      }
      void SetNextParsingTableEntryId(ParsingTableEntryId next_entry_id) {
        next_entry_id_ = next_entry_id;
      }

     private:
      // 提供默认构造函数供序列化时构建对象
      ShiftAttachedData() = default;

      // 允许序列化类访问
      friend class boost::serialization::access;
      // 允许语法分析表条目访问内部接口
      friend class ParsingTableEntry;

      template <class Archive>
      void serialize(Archive& ar, const unsigned int version) {
        ar& boost::serialization::base_object<ActionAndAttachedDataInterface>(
            *this);
        ar& next_entry_id_;
      }
      virtual ShiftAttachedData& GetShiftAttachedData() override {
        return const_cast<ShiftAttachedData&>(
            static_cast<const ShiftAttachedData&>(*this)
                .GetShiftAttachedData());
      }

      // 移入该单词后转移到的语法分析表条目ID
      ParsingTableEntryId next_entry_id_;
    };
    // 执行规约动作时的附属数据
    class ReductAttachedData : public ActionAndAttachedDataInterface {
     public:
      template <class ProductionBody>
      ReductAttachedData(ProductionNodeId reducted_nonterminal_node_id,
                         ProcessFunctionClassId process_function_class_id,
                         ProductionBody&& production_body)
          : ActionAndAttachedDataInterface(ActionType::kReduct),
            reducted_nonterminal_node_id_(reducted_nonterminal_node_id),
            process_function_class_id_(process_function_class_id),
            production_body_(std::forward<ProductionBody>(production_body)) {}
      ReductAttachedData(const ReductAttachedData&) = default;
      ReductAttachedData(ReductAttachedData&&) = default;

      ReductAttachedData& operator=(const ReductAttachedData&) = default;
      ReductAttachedData& operator=(ReductAttachedData&&) = default;
      virtual bool operator==(const ActionAndAttachedDataInterface&
                                  reduct_attached_data) const override;

      virtual const ReductAttachedData& GetReductAttachedData() const override {
        return *this;
      }

      ProductionNodeId GetReductedNonTerminalNodeId() const {
        return reducted_nonterminal_node_id_;
      }
      void SetReductedNonTerminalNodeId(
          ProductionNodeId reducted_nonterminal_node_id) {
        reducted_nonterminal_node_id_ = reducted_nonterminal_node_id;
      }
      ProcessFunctionClassId GetProcessFunctionClassId() const {
        return process_function_class_id_;
      }
      void SetProcessFunctionClassId(
          ProcessFunctionClassId process_function_class_id) {
        process_function_class_id_ = process_function_class_id;
      }
      const std::vector<ProductionNodeId>& GetProductionBody() const {
        return production_body_;
      }
      void SetProductionBody(std::vector<ProductionNodeId>&& production_body) {
        production_body_ = std::move(production_body);
      }

     private:
      // 提供默认构造函数供序列化时构建对象
      ReductAttachedData() = default;

      // 允许序列化类访问
      friend class boost::serialization::access;
      // 允许语法分析表条目访问内部接口
      friend class ParsingTableEntry;

      template <class Archive>
      void serialize(Archive& ar, const unsigned int version) {
        ar& boost::serialization::base_object<ActionAndAttachedDataInterface>(
            *this);
        ar& reducted_nonterminal_node_id_;
        ar& process_function_class_id_;
        ar& production_body_;
      }

      virtual ReductAttachedData& GetReductAttachedData() override {
        return const_cast<ReductAttachedData&>(
            static_cast<const ReductAttachedData&>(*this)
                .GetReductAttachedData());
      }

      // 规约后得到的非终结节点的ID
      ProductionNodeId reducted_nonterminal_node_id_;
      // 执行规约操作时使用的对象的ID
      ProcessFunctionClassId process_function_class_id_;
      // 规约所用产生式，用于核对该产生式包含哪些节点
      // 不使用空规约功能则可改为产生式节点数目
      std::vector<ProductionNodeId> production_body_;
    };

    // 使用二义性文法时对一个单词既可以移入也可以规约
    class ShiftReductAttachedData : public ActionAndAttachedDataInterface {
     public:
      template <class ShiftData, class ReductData>
      ShiftReductAttachedData(ShiftData&& shift_attached_data,
                              ReductData&& reduct_attached_data)
          : ActionAndAttachedDataInterface(ActionType::kShiftReduct),
            shift_attached_data_(std::forward<ShiftData>(shift_attached_data)),
            reduct_attached_data_(
                std::forward<ReductData>(reduct_attached_data)) {}
      ShiftReductAttachedData(const ShiftReductAttachedData&) = delete;

      // 在与ShiftAttachedData和ReductAttachedData比较时仅比较相应部分
      virtual bool operator==(
          const ActionAndAttachedDataInterface& attached_data) const override;

      virtual const ShiftAttachedData& GetShiftAttachedData() const override {
        return shift_attached_data_;
      }
      virtual const ReductAttachedData& GetReductAttachedData() const override {
        return reduct_attached_data_;
      }
      virtual const ShiftReductAttachedData& GetShiftReductAttachedData()
          const override {
        return *this;
      }

      void SetShiftAttachedData(ShiftAttachedData&& shift_attached_data) {
        shift_attached_data_ = std::move(shift_attached_data);
      }
      void SetReductAttachedData(ReductAttachedData&& reduct_attached_data) {
        reduct_attached_data_ = std::move(reduct_attached_data);
      }

     private:
      // 提供默认构造函数供序列化时构建对象
      ShiftReductAttachedData() = default;

      // 允许序列化类访问
      friend class boost::serialization::access;
      // 允许语法分析表条目访问内部接口
      friend class ParsingTableEntry;

      template <class Archive>
      void serialize(Archive& ar, const unsigned int version) {
        ar& boost::serialization::base_object<ActionAndAttachedDataInterface>(
            *this);
        ar& shift_attached_data_;
        ar& reduct_attached_data_;
      }
      virtual ShiftAttachedData& GetShiftAttachedData() override {
        return const_cast<ShiftAttachedData&>(
            static_cast<const ShiftReductAttachedData&>(*this)
                .GetShiftAttachedData());
      }
      virtual ReductAttachedData& GetReductAttachedData() override {
        return const_cast<ReductAttachedData&>(
            static_cast<const ShiftReductAttachedData&>(*this)
                .GetReductAttachedData());
      }
      virtual ShiftReductAttachedData& GetShiftReductAttachedData() override {
        return const_cast<ShiftReductAttachedData&>(
            static_cast<const ShiftReductAttachedData&>(*this)
                .GetShiftReductAttachedData());
      }

      ShiftAttachedData shift_attached_data_;
      ReductAttachedData reduct_attached_data_;
    };

    // 键值为待移入节点ID，值为指向相应数据的指针
    using ActionAndTargetContainer =
        std::unordered_map<ProductionNodeId,
                           std::unique_ptr<ActionAndAttachedDataInterface>>;

    ParsingTableEntry() {}
    ParsingTableEntry(const ParsingTableEntry&) = delete;
    ParsingTableEntry& operator=(const ParsingTableEntry&) = delete;
    ParsingTableEntry(ParsingTableEntry&& parsing_table_entry)
        : action_and_attached_data_(
              std::move(parsing_table_entry.action_and_attached_data_)),
          nonterminal_node_transform_table_(std::move(
              parsing_table_entry.nonterminal_node_transform_table_)) {}
    ParsingTableEntry& operator=(ParsingTableEntry&& parsing_table_entry);

    // 设置该产生式在转移条件下的动作和目标节点
    // 由于空规约机制，末尾部分节点空规约时在相同的向前看符号下有不同产生式规约
    // 例如产生式：Example1 -> Example2 · Example3 和 Example3 -> Example4 ·
    // 均在相同向前看符号下可规约（Example3可以空规约）
    // 但是规约后得到的产生式不同，所以需要获取语法分析表条目对应的核心
    // 如果一个产生式可以规约后移入另一个产生式，则设置给定条件下规约前者
    // 否则报错，所以需要提供SyntaxGenerator
    template <class AttachedData>
    requires std::is_same_v<
        std::decay_t<AttachedData>,
        SyntaxGenerator::ParsingTableEntry::ShiftAttachedData> ||
        std::is_same_v<std::decay_t<AttachedData>,
                       SyntaxGenerator::ParsingTableEntry::ReductAttachedData>
    void SetTerminalNodeActionAndAttachedData(ProductionNodeId node_id,
                                              AttachedData&& attached_data);
    // 设置该条目移入非终结节点后转移到的节点
    void SetNonTerminalNodeTransformId(ProductionNodeId node_id,
                                       ParsingTableEntryId production_node_id) {
      nonterminal_node_transform_table_[node_id] = production_node_id;
    }
    // 修改该条目中所有条目ID为新ID
    // 当前设计下仅修改移入时转移到的下一个条目ID（移入终结节点/非终结节点）
    void ResetEntryId(
        const std::unordered_map<ParsingTableEntryId, ParsingTableEntryId>&
            old_entry_id_to_new_entry_id);
    // 访问该条目下给定ID终结节点的行为与目标ID
    // 如果不存在该转移条件则返回空指针
    const ActionAndAttachedDataInterface* AtTerminalNode(
        ProductionNodeId node_id) const {
      auto iter = action_and_attached_data_.find(node_id);
      return iter == action_and_attached_data_.end() ? nullptr
                                                     : iter->second.get();
    }
    // 访问该条目下给定ID非终结节点移入时转移到的条目ID
    // 不存在该转移条件则返回ParsingTableEntryId::InvalidId()
    ParsingTableEntryId AtNonTerminalNode(ProductionNodeId node_id) const {
      auto iter = nonterminal_node_transform_table_.find(node_id);
      return iter == nonterminal_node_transform_table_.end()
                 ? ParsingTableEntryId::InvalidId()
                 : iter->second;
    }
    // 获取全部终结节点的操作
    const ActionAndTargetContainer& GetAllActionAndAttachedData() const {
      return action_and_attached_data_;
    }
    // 获取全部非终结节点转移到的表项
    const std::unordered_map<ProductionNodeId, ParsingTableEntryId>&
    GetAllNonTerminalNodeTransformTarget() const {
      return nonterminal_node_transform_table_;
    }
    // 清除该条目中所有数据
    void Clear() {
      action_and_attached_data_.clear();
      nonterminal_node_transform_table_.clear();
    }

   private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& action_and_attached_data_;
      ar& nonterminal_node_transform_table_;
    }

    // 只有自身可以修改原始数据结构，外来请求要走接口
    ActionAndTargetContainer& GetAllActionAndAttachedData() {
      return const_cast<ActionAndTargetContainer&>(
          static_cast<const ParsingTableEntry&>(*this)
              .GetAllActionAndAttachedData());
    }
    // 获取全部非终结节点转移到的表项
    std::unordered_map<ProductionNodeId, ParsingTableEntryId>&
    GetAllNonTerminalNodeTransformTarget() {
      return const_cast<
          std::unordered_map<ProductionNodeId, ParsingTableEntryId>&>(
          static_cast<const ParsingTableEntry&>(*this)
              .GetAllNonTerminalNodeTransformTarget());
    }

    // 向前看符号ID下的操作和目标节点
    ActionAndTargetContainer action_and_attached_data_;
    // 移入非终结节点后转移到的产生式体序号
    std::unordered_map<ProductionNodeId, ParsingTableEntryId>
        nonterminal_node_transform_table_;
  };
};
template <class IdType>
inline SyntaxGenerator::ProductionBodyId
SyntaxGenerator::NonTerminalProductionNode::AddBody(
    IdType&& body, ProcessFunctionClassId class_for_reduct_id_) {
  ProductionBodyId body_id(nonterminal_bodys_.size());
  // 将输入插入到产生式体向量中，无删除相同产生式功能
  nonterminal_bodys_.emplace_back(std::forward<IdType>(body),
                                  class_for_reduct_id_);
  return body_id;
}

template <class ProcessFunctionClass>
inline SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddNonTerminalNode(
    std::string&& node_symbol, std::vector<std::string>&& subnode_symbols) {
  ProcessFunctionClassId class_id =
      CreateProcessFunctionClassObject<ProcessFunctionClass>();
  return AddNonTerminalNode(std::move(node_symbol), std::move(subnode_symbols),
                            class_id);
}

template <class ForwardNodeIdContainer>
inline std::pair<
    typename std::unordered_map<SyntaxGenerator::CoreItem,
                                SyntaxGenerator::ForwardNodesContainerType,
                                SyntaxGenerator::CoreItemHasher>::iterator,
    bool>
SyntaxGenerator::AddItemAndForwardNodeIdsToCore(
    CoreId core_id, const CoreItem& core_item,
    ForwardNodeIdContainer&& forward_node_ids) {
  assert(core_id.IsValid());
  auto result = GetCore(core_id).AddItemAndForwardNodeIds(
      core_item, std::forward<ForwardNodeIdContainer>(forward_node_ids));
  if (result.second) {
    // 如果新插入了项则记录该项属于的新核心ID
    AddCoreItemBelongToCoreId(core_item, core_id);
  }
  return result;
}

// 在AddItemAndForwardNodeIdsToCore基础上设置添加的项为核心项

template <class ForwardNodeIdContainer>
inline std::pair<
    typename std::unordered_map<SyntaxGenerator::CoreItem,
                                SyntaxGenerator::ForwardNodesContainerType,
                                SyntaxGenerator::CoreItemHasher>::iterator,
    bool>
SyntaxGenerator::AddMainItemAndForwardNodeIdsToCore(
    CoreId core_id, const CoreItem& core_item,
    ForwardNodeIdContainer&& forward_node_ids) {
  auto result = GetCore(core_id).AddMainItemAndForwardNodeIds(
      core_item, std::forward<ForwardNodeIdContainer>(forward_node_ids));
  if (result.second) {
    // 向核心中插入了新的项则更新该项属于的核心
    AddCoreItemBelongToCoreId(core_item, core_id);
  }
  return result;
}

template <class Archive>
inline void SyntaxGenerator::save(Archive& ar,
                                  const unsigned int version) const {
  ar << root_parsing_table_entry_id_;
  ar << syntax_parsing_table_;
  ar << manager_process_function_class_;
}

template <class AttachedData>
requires std::is_same_v<
    std::decay_t<AttachedData>,
    SyntaxGenerator::ParsingTableEntry::ShiftAttachedData> ||
    std::is_same_v<std::decay_t<AttachedData>,
                   SyntaxGenerator::ParsingTableEntry::ReductAttachedData>
void SyntaxGenerator::ParsingTableEntry::SetTerminalNodeActionAndAttachedData(
    ProductionNodeId node_id, AttachedData&& attached_data) {
  static_assert(
      !std::is_same_v<std::decay_t<AttachedData>, ShiftReductAttachedData>,
      "该类型仅允许通过在已有一种动作的基础上补全缺少的另一半后转换得到，不允许"
      "直接传入");
  // 使用二义性文法，语法分析表某些情况下需要对同一个节点支持移入和规约操作并存
  auto iter = action_and_attached_data_.find(node_id);
  if (iter == action_and_attached_data_.end()) {
    // 新插入转移节点
    action_and_attached_data_.emplace(
        node_id, std::make_unique<std::decay_t<AttachedData>>(
                     std::forward<AttachedData>(attached_data)));
  } else {
    // 待插入的转移条件已存在
    // 获取已经存储的数据
    ActionAndAttachedDataInterface& data_already_in = *iter->second;
    // 要么修改已有的规约后得到的节点，要么补全移入/规约的另一部分（规约/移入）
    // 不应修改已有的移入后转移到的语法分析表条目
    switch (data_already_in.GetActionType()) {
      case ActionType::kShift:
        if constexpr (std::is_same_v<std::decay_t<AttachedData>,
                                     ReductAttachedData>) {
          // 提供的数据是规约数据，转换为ShiftReductAttachedData
          iter->second = std::make_unique<ShiftReductAttachedData>(
              std::move(iter->second->GetShiftAttachedData()),
              std::forward<AttachedData>(attached_data));
        } else {
          // 提供的是移入数据，要求必须与已有的数据相同
          static_assert(
              std::is_same_v<std::decay_t<AttachedData>, ShiftAttachedData>);
          // 检查提供的移入数据是否与已有的移入数据相同
          // 不能写反，否则当data_already_in为ShiftReductAttachedData时
          // 无法获得与其它附属数据比较的正确语义
          // 可能是MSVC的BUG
          // 使用隐式调用会导致调用attached_data的operator==，所以显式调用
          if (!data_already_in.operator==(attached_data)) [[unlikely]] {
            // 设置相同条件不同产生式下规约
            OutPutError(
                std::format("一个项集在相同条件下只能规约一种产生式，请参考该项"
                            "集求闭包时的输出信息检查产生式"));
            exit(-1);
          }
        }
        break;
      case ActionType::kReduct:
        if constexpr (std::is_same_v<std::decay_t<AttachedData>,
                                     ShiftAttachedData>) {
          // 提供的数据为移入部分数据，转换为ShiftReductAttachedData
          iter->second = std::make_unique<ShiftReductAttachedData>(
              std::forward<AttachedData>(attached_data),
              std::move(iter->second->GetReductAttachedData()));
        } else {
          // 提供的是移入数据，需要检查是否与已有的数据相同
          static_assert(
              std::is_same_v<std::decay_t<AttachedData>, ReductAttachedData>);
          // 不能写反，否则当data_already_in为ShiftReductAttachedData时
          // 无法获得与其它附属数据比较的正确语义
          // 可能是MSVC的BUG
          // 使用隐式调用会导致调用attached_data的operator==，所以显式调用
          if (!data_already_in.operator==(attached_data)) {
            // 设置相同条件不同产生式下规约
            OutPutError(
                std::format("一个项集在相同条件下只能规约一种产生式，请参考该项"
                            "集求闭包时的输出信息检查产生式"));
            exit(-1);
          }
        }
        break;
      case ActionType::kShiftReduct: {
        // 可能是MSVC的BUG
        // 使用隐式调用会导致调用attached_data的operator==，所以显式调用
        if (!data_already_in.operator==(attached_data)) {
          // 设置相同条件不同产生式下规约
          OutPutError(
              std::format("一个项集在相同条件下只能规约一种产生式，请参考该项"
                          "集求闭包时的输出信息检查产生式"));
          exit(-1);
        }
        break;
      }
      default:
        assert(false);
        break;
    }
  }
}

template <class ForwardNodeIdContainer>
inline std::pair<std::unordered_map<SyntaxGenerator::CoreItem,
                                    SyntaxGenerator::ForwardNodesContainerType,
                                    SyntaxGenerator::CoreItemHasher>::iterator,
                 bool>
SyntaxGenerator::Core::AddItemAndForwardNodeIds(
    const CoreItem& item, ForwardNodeIdContainer&& forward_node_ids) {
  // 已经求过闭包的核心不能添加新项
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
inline bool SyntaxGenerator::Core::AddForwardNodes(
    const CoreItem& item, ForwardNodeIdContainer&& forward_node_id_container) {
  auto iter = item_and_forward_node_ids_.find(item);
  assert(iter != item_and_forward_node_ids_.end());
  return AddForwardNodes(
      iter, std::forward<ForwardNodeIdContainer>(forward_node_id_container));
}

template <class ForwardNodeIdContainer>
inline bool SyntaxGenerator::Core::AddForwardNodes(
    const std::unordered_map<
        CoreItem, std::unordered_set<ProductionNodeId>>::iterator& iter,

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

// 暴露部分内部类供boost序列化注册类型(BOOST_CLASS_EXPORT_GUID)用
struct ExportSyntaxGeneratorInsideTypeForSerialization {
  using ProductionNodeId = SyntaxGenerator::ProductionNodeId;
  using ProcessFunctionClassId = SyntaxGenerator::ProcessFunctionClassId;
  using ProcessFunctionClassManagerType =
      SyntaxGenerator::ProcessFunctionClassManagerType;
  using ParsingTableType = SyntaxGenerator::ParsingTableType;
  using ParsingTableEntry = SyntaxGenerator::ParsingTableEntry;
  using ParsingTableEntryId = SyntaxGenerator::ParsingTableEntryId;
  using ParsingTableEntryActionAndReductDataInterface =
      SyntaxGenerator::ParsingTableEntry::ActionAndAttachedDataInterface;
  using ParsingTableEntryShiftAttachedData =
      SyntaxGenerator::ParsingTableEntry::ShiftAttachedData;
  using ParsingTableEntryReductAttachedData =
      SyntaxGenerator::ParsingTableEntry::ReductAttachedData;
  using ParsingTableEntryShiftReductAttachedData =
      SyntaxGenerator::ParsingTableEntry::ShiftReductAttachedData;
};
}  // namespace frontend::generator::syntax_generator

#endif  // !GENERATOR_SYNTAXGENERATOR_SYNTAXGENERATOR_H_