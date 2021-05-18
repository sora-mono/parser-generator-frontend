#include <any>
#include <array>
#include <cassert>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/object_manager.h"
#include "Common/unordered_struct_manager.h"

#ifdef _DEBUG
#define USE_PRODUCTION_NODE_SHIFT_FUNCTION
#define USE_PRODUCTION_NODE_INIT_FUNCTION
#define USE_PRODUCTION_NODE_REDUCTION_FUNCTION

#endif  // _DEBUG

#ifndef GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_
#define GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_

namespace frontend::parser::lexicalmachine {
class LexicalMachine;
}

namespace frontend::generator::lexicalgenerator {
using frontend::common::ObjectManager;
using frontend::common::UnorderedStructManager;

struct StringHasher {
  frontend::common::StringHashType DoHash(const std::string& str) {
    return frontend::common::HashString(str);
  }
};
// TODO 添加删除未使用产生式的功能
class LexicalGenerator {
  // 为下面定义各种类型不得不使用前置声明
  class BaseProductionNode;
  class ParsingTableEntry;
  class Core;

  enum class WrapperLabel {
    kCoreId,
    kPriorityLevel,
    kPointIndex,
    kParsingTableEntryId,
    kProductionBodyId
  };  // 核心项ID
  using CoreId = frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                                     WrapperLabel::kCoreId>;
  // 运算符优先级
  using PriorityLevel =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kPriorityLevel>;
  // 点的位置
  using PointIndex =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kPointIndex>;
  // 语法分析表条目ID
  using ParsingTableEntryId =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kParsingTableEntryId>;
  // 语法分析表类型
  using ParsingTableType = std::vector<ParsingTableEntry>;
  // 非终结节点内产生式编号
  using ProductionBodyId =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kProductionBodyId>;
  // 产生式节点ID
  using ProductionNodeId = ObjectManager<BaseProductionNode>::ObjectId;
  // 单个产生式体
  using ProductionBodyType = std::vector<ProductionNodeId>;
  // 产生式容器
  using BodyContainerType = std::vector<ProductionBodyType>;
  // 符号ID
  using SymbolId = UnorderedStructManager<std::string, StringHasher>::ObjectId;
  // 向前看符号容器（最内层容器，由ProductionBodyId和PointIndex共同决定）
  using InsideForwardNodesContainerType = std::unordered_set<ProductionNodeId>;
  // 向前看符号容器（中间层容器，由ProductionBodyId决定）
  using MiddleForwardNodesContainerType =
      std::vector<InsideForwardNodesContainerType>;
  // 向前看符号容器（最外层容器，与产生式一一对应）
  using OutSideForwardNodesContainerType =
      std::vector<MiddleForwardNodesContainerType>;
  // 内核内单个项集，参数从左到右依次为
  // 产生式节点ID，产生式体ID，产生式体中点的位置
  using CoreItem = std::tuple<ProductionNodeId, ProductionBodyId, PointIndex>;
  // 节点类型：终结符号，运算符，非终结符号，文件尾节点
  // 为了支持ClassfiyProductionNodes，允许自定义项的值
  // 如果自定义项的值则所有项的值都必须小于sizeof(ProductionNodeType)
  enum class ProductionNodeType {
    kTerminalNode,
    kOperatorNode,
    kNonTerminalNode,
    kEndNode
  };
  // 运算符结合类型：左结合，右结合
  enum class AssociatityType { kLeftAssociate, kRightAssociate };
  // 分析动作类型：规约，移入，报错，接受
  enum class ActionType { kReduction, kShift, kError, kAccept };

 public:
  // 解析产生式，仅需且必须输入一个完整的产生式
  ProductionNodeId AnalysisProduction(const std::string& str);
  // 构建LALR(1)配置
  void ParsingTableConstruct();

 private:
  // 添加符号，返回符号的ID和是否执行了插入操作，执行了插入操作则返回true
  std::pair<SymbolId, bool> AddSymbol(const std::string& str) {
    return manager_symbol_.AddObject(str);
  }
  // 获取符号对应ID
  SymbolId GetSymbolId(const std::string& str) {
    return manager_symbol_.GetObjectId(str);
  }
  // 通过符号ID查询对应字符串
  const std::string& GetSymbolString(SymbolId symbol_id) {
    return manager_symbol_.GetObject(symbol_id);
  }
  // 设置符号ID到节点ID的映射
  void SetSymbolIdToProductionNodeIdMapping(SymbolId symbol_id,
                                            ProductionNodeId node_id);
  // 新建终结节点，返回节点ID
  // 节点已存在且给定symbol_id不同于已有ID则返回ProductionNodeId::InvalidId()
  ProductionNodeId AddTerminalNode(SymbolId symbol_id);
  // 新建运算符节点，返回节点ID，节点已存在则返回ProductionNodeId::InvalidId()
  ProductionNodeId AddOperatorNode(SymbolId symbol_id,
                                   AssociatityType associatity_type,
                                   PriorityLevel priority_level);
  // 新建非终结节点，返回节点ID
  // 第二个参数为产生式体，参数结构为std::initializer_list<std::vector<Object>>
  // 或std::vector<std::vector<Object>>
  template <class IdType>
  ProductionNodeId AddNonTerminalNode(SymbolId symbol_id, IdType&& bodys);
  // 新建文件尾节点，返回节点ID
  ProductionNodeId AddEndNode(SymbolId symbol_id = SymbolId::InvalidId());
  // 获取节点
  BaseProductionNode& GetProductionNode(ProductionNodeId id) {
    return manager_nodes_[id];
  }
  // 给非终结节点添加产生式体
  template <class IdType>
  void AddNonTerminalNodeBody(ProductionNodeId node_id, IdType&& body) {
    static_cast<NonTerminalProductionNode*>(GetProductionNode(node_id))
        ->AddBody(std::forward<IdType>(body));
  }
  // 通过符号ID查询对应所有节点的ID，不存在则返回空vector
  std::vector<ProductionNodeId> GetSymbolToProductionNodeIds(
      SymbolId symbol_id);
  // 获取core_id对应产生式项集
  Core& GetCore(CoreId core_id) {
    assert(core_id < cores_.size());
    return cores_[core_id];
  }
  // 添加新的核心
  CoreId AddNewCore() {
    CoreId core_id = CoreId(cores_.size());
    cores_.emplace_back();
    cores_.back().SetCoreId(core_id);
    return core_id;
  }
  // Items要求为std::vector<CoreItem>或std::initialize_list<CoreItem>
  // ForwardNodes要求为std::vector<Object>或std::initialize_list<Object>
  template <class Items, class ForwardNodes>
  CoreId AddNewCore(Items&& items, ForwardNodes&& forward_nodes);
  // 添加项到对应项集的映射，会覆盖原有记录
  void SetItemCoreId(const CoreItem& core_item, CoreId core_id);
  // 向项集中添加项，不要使用core.AddItem()，会导致无法更新映射记录
  // 会设置相应BaseProductionNode中该项到CoreId的记录
  std::pair<std::set<CoreItem>::iterator, bool> AddItemToCore(
      const CoreItem& core_item, CoreId core_id);
  // 获取给定项对应项集的ID，使用的参数必须为有效参数
  // 未指定（不曾使用SetItemCoreId设置）则返回CoreId::InvalidId()
  CoreId GetCoreId(ProductionNodeId production_node_id,
                   ProductionBodyId production_body_id, PointIndex point_index);
  CoreId GetItemCoreId(const CoreItem& core_item);
  // 获取给定项对应项集的ID，如果不存在记录则插入到新项集里并返回相应ID
  // 返回的bool代表是否core_item不存在并成功创建并添加到新的项集
  // insert_core_id是不存在记录时应插入的Core的ID
  // 设为CoreId::InvalidId()代表新建Core
  std::pair<CoreId, bool> GetItemCoreIdOrInsert(
      const CoreItem& core_item, CoreId insert_core_id = CoreId::InvalidId());
  // 获取向前看符号集
  const std::unordered_set<ProductionNodeId>& GetForwardNodeIds(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index) {
    return GetProductionNode(production_node_id)
        .GetForwardNodeIds(production_body_id, point_index);
  }
  // 添加向前看符号
  void AddForwardNodeId(ProductionNodeId production_node_id,
                        ProductionBodyId production_body_id,
                        PointIndex point_index,
                        ProductionNodeId forward_node_id) {
    GetProductionNode(production_node_id)
        .AddForwardNodeId(production_body_id, point_index, forward_node_id);
  }
  // 添加一个容器内的所有向前看符号
  template <class Container>
  void AddForwardNodeContainer(ProductionNodeId production_node_id,
                               ProductionBodyId production_body_id,
                               PointIndex point_index,
                               Container&& forward_nodes_id_container);
  // 闭包操作中的first操作，如果first_node_id有效则返回仅含自己的集合
  // 无效则返回next_node_ids
  std::unordered_set<ProductionNodeId> First(
      ProductionNodeId first_node_id,
      const std::unordered_set<ProductionNodeId>& next_node_ids);
  // 获取核心的全部项
  const std::set<CoreItem>& GetCoreItems(CoreId core_id) {
    return GetCore(core_id).GetItems();
  }
  // 获取最终语法分析表条目
  ParsingTableEntry& GetParsingTableEntry(ParsingTableEntryId id) {
    assert(id < lexical_config_parsing_table_.size());
    return lexical_config_parsing_table_[id];
  }
  // 设置根产生式ID
  void SetRootProductionId(ProductionNodeId root_production) {
    root_production_ = root_production;
  }
  // 设置项集闭包有效（避免每次Goto都求闭包）
  void SetCoreClosureAvailable(CoreId core_id) {
    GetCore(core_id).SetClosureAvailable();
  }
  bool IsClosureAvailable(CoreId core_id) {
    return GetCore(core_id).IsClosureAvailable();
  }
  // 对给定核心项求闭包，结果存在原位置
  void CoreClosure(CoreId core_id);
  // 删除Goto缓存中给定core_id的所有缓存，函数执行后保证缓存不存在
  void RemoveGotoCacheEntry(CoreId core_id) {
    assert(core_id.IsValid());
    auto [iter_begin, iter_end] = core_id_goto_core_id_.equal_range(core_id);
    for (; iter_begin != iter_end; ++iter_begin) {
      core_id_goto_core_id_.erase(iter_begin);
    }
  }
  // 设置一条Goto缓存
  void SetGotoCacheEntry(CoreId core_id_src,
                         ProductionNodeId transform_production_node_id,
                         CoreId core_id_dst);
  // 获取一条Goto缓存，不存在则返回CoreId::InvalidId()
  CoreId GetGotoCacheEntry(CoreId core_id_src,
                           ProductionNodeId transform_production_node_id);
  // Goto缓存是否有效，有效返回true
  bool IsGotoCacheAvailable(CoreId core_id) {
    return GetCore(core_id).IsClosureAvailable();
  }
  // 获取该项下一个产生式节点ID，如果不存在则返回ProductionNodeId::InvalidId()
  ProductionNodeId GetProductionBodyNextNodeId(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index);
  // 获取该项的向前看节点ID，如果不存在则返回ProductionNodeId::InvalidId()
  ProductionNodeId GetProductionBodyNextNextNodeId(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index);
  // Goto的子过程，返回ItemGoto后的ID和是否插入
  // 如果不存在已有的记录则插入到insert_core_id中
  // 需要插入时insert_core_id为CoreId::InvalidId()则创建新的Core并插入
  // 返回Goto后的CoreId和是否执行了向core中插入操作
  // 如果在给定参数上无法执行返回CoreId::InvalidId()和false
  std::pair<CoreId, bool> ItemGoto(
      const CoreItem& item, ProductionNodeId transform_production_node_id,
      CoreId insert_core_id = CoreId::InvalidId());
  // 获取从给定核心项开始，移入node_id对应的节点后到达的核心项
  // 自动创建不存在的核心项，如果Goto的结果为空则返回CoreId::InvalidId()
  // 返回的第二个参数代表返回的Core是否是新建的，新建的为true
  std::pair<CoreId, bool> Goto(CoreId core_id_src,
                               ProductionNodeId transform_production_node_id);
  // 传播向前看符号
  void SpreadLookForwardSymbol(CoreId core_id);
  // 对所有产生式节点按照ProductionNodeType分类
  // 返回array内的vector内类型对应的下标为ProductionNodeType内类型的值
  std::array<std::vector<ProductionNodeId>, sizeof(ProductionNodeType)>
  ClassifyProductionNodes();
  // 设置项到语法分析表条目ID的映射
  void SetItemToParsingEntryIdMapping(const CoreItem& item,
                                      ParsingTableEntryId entry_id) {
    item_to_parsing_table_entry_id_[item] = entry_id;
  }
  // 获取已存储的项到语法分析表条目ID的映射
  // 不存在则返回ParsingTableEntryId::InvalidId()
  ParsingTableEntryId GetItemToParsingEntryId(const CoreItem& item);
  // ParsingTableMergeOptimize的子过程，分类具有相同终结节点项的语法分析表条目
  // 向equivalent_ids写入相同终结节点转移表的节点ID的组，不会执行实际合并操作
  // 不会写入只有一个项的组
  // entry_ids类型应为std::vector<ParsingTableEntryId>
  template <class ParsingTableEntryIdContainer>
  void ParsingTableTerminalNodeClassify(
      const std::vector<ProductionNodeId>& terminal_node_ids, size_t index,
      ParsingTableEntryIdContainer&& entry_ids,
      std::vector<std::vector<ParsingTableEntryId>>* equivalent_ids);
  // ParsingTableMergeOptimize的子过程，分类具有相同非终结节点项的语法分析表条目
  // 向equivalent_ids写入相同非终结节点转移表的节点ID的组，不会执行实际合并操作
  // 不会写入只有一个项的组
  // entry_ids类型应为std::vector<ParsingTableEntryId>
  template <class ParsingTableEntryIdContainer>
  void ParsingTableNonTerminalNodeClassify(
      const std::vector<ProductionNodeId>& nonterminal_node_ids, size_t index,
      ParsingTableEntryIdContainer&& entry_ids,
      std::vector<std::vector<ParsingTableEntryId>>* equivalent_ids);
  // ParsingTableMergeOptimize的子过程，分类具有相同项的语法分析表条目
  // 第一个参数为语法分析表内所有终结节点ID
  // 第二个参数为语法分析表内所有非终结节点ID
  // 返回可以合并的语法分析表条目组，所有组均有至少两个条目
  std::vector<std::vector<ParsingTableEntryId>> ParsingTableEntryClassify(
      const std::vector<ProductionNodeId>& terminal_node_ids,
      const std::vector<ProductionNodeId>& nonterminal_node_ids);
  // 根据给定的表项重映射语法分析表内ID
  void RemapParsingTableEntryId(
      const std::unordered_map<ParsingTableEntryId, ParsingTableEntryId>&
          moved_entry_to_new_entry_id);
  // 合并语法分析表内相同的项，同时缩减语法分析表大小
  // 会将语法分析表内的项修改为新的项
  void ParsingTableMergeOptimize();
  // TODO 将下面变量的引用改为直接使用lexical_config中的变量以优化性能

  // 声明语法分析机为友类，便于其使用各种定义
  friend class frontend::parser::lexicalmachine::LexicalMachine;
  // 根产生式ID
  ProductionNodeId& root_production_ = lexical_config.root_production_node_id;
  // 管理终结符号、非终结符号等的节点
  ObjectManager<BaseProductionNode>& manager_nodes_ =
      lexical_config.manager_nodes;
  // 管理符号
  UnorderedStructManager<std::string, StringHasher> manager_symbol_;
  // 符号ID（manager_symbol_)返回的ID到对应节点的映射，支持一个符号对应多个节点
  std::unordered_multimap<SymbolId, ProductionNodeId> symbol_id_to_node_id_;

  // 项到语法分析表条目ID的映射
  std::map<CoreItem, ParsingTableEntryId> item_to_parsing_table_entry_id_;
  // 内核+非内核项集
  std::vector<Core> cores_;
  // Goto缓存，存储已有的CoreI在不同条件下Goto的节点
  std::multimap<CoreId, std::pair<ProductionNodeId, CoreId>>
      core_id_goto_core_id_;
  // 语法分析表
  ParsingTableType& lexical_config_parsing_table_ =
      lexical_config.parsing_table;
  // 写到配置文件中的数据
  struct LexicalConfig {
    ParsingTableType parsing_table;
    ObjectManager<BaseProductionNode> manager_nodes;
    ProductionNodeId root_production_node_id;
  } lexical_config;

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

    struct NodeData {
      ProductionNodeType node_type;
      std::string node_symbol;
    };

    void SetType(ProductionNodeType type) { base_type_ = type; }
    ProductionNodeType Type() const { return base_type_; }
    void SetThisNodeId(ProductionNodeId id) { base_id_ = id; }
    ProductionNodeId Id() const { return base_id_; }
    void SetSymbolId(SymbolId id) { base_symbol_id_ = id; }
    SymbolId GetSymbolId() const { return base_symbol_id_; }

#ifdef USE_PRODUCTION_NODE_INIT_FUNCTION
    const std::function<void()>& GetInitFunction() { return init_function_; }
    void SetInitFunction(const std::function<void()>& init_function) {
      init_function_ = init_function;
    }
#endif  // USE_PRODUCTION_NODE_INIT_FUNCTION

#ifdef USE_PRODUCTION_NODE_REDUCTION_FUNCTION
    const std::function<void()>& GetReductionFunction() {
      return reduction_function_;
    }
    void SetReductionFunction(const std::function<void()>& reduction_function) {
      reduction_function_ = reduction_function;
    }
#endif  // USE_PRODUCTION_NODE_REDUCTION_FUNCTION

#ifdef USE_PRODUCTION_NODE_USER_DATA
    std::any& GetUserData() { return user_data_; }
    template <class T>
    void SetUserData(T&& user_data) {
      user_data_ = std::forward<T>(user_data);
    }
#endif  // USE_PRODUCTION_NODE_USER_DATA

    // 获取给定项对应的产生式ID，返回point_index后面的ID
    // point_index越界时返回ProducNodeId::InvalidId()
    // 为了支持向前看多个节点允许越界
    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index);
    //添加向前看节点
    virtual void AddForwardNodeId(ProductionBodyId production_body_id,
                                  PointIndex point_index,
                                  ProductionNodeId forward_node_id);
    //获取向前看节点集合
    virtual const std::unordered_set<ProductionNodeId>& GetForwardNodeIds(
        ProductionBodyId production_body_id, PointIndex point_index);
    //设置项对应的核心ID
    virtual void SetCoreId(ProductionBodyId production_body_id,
                           PointIndex point_index, CoreId core_id);
    //获取项对应的核心ID
    virtual CoreId GetCoreId(ProductionBodyId production_body_id,
                             PointIndex point_index);

   private:
    // 节点类型
    ProductionNodeType base_type_;
    // 节点ID
    ProductionNodeId base_id_;
    // 节点符号ID
    SymbolId base_symbol_id_;

#ifdef USE_PRODUCTION_NODE_INIT_FUNCTION
    // 建立节点时调用的函数
    std::function<void()> init_function_;
#endif  // USE_PRODUCTION_NODE_INIT_FUNCTION

#ifdef USE_PRODUCTION_NODE_REDUCTION_FUNCTION
    // 归并节点时调用的函数
    std::function<void()> reduction_function_;
#endif  // USE_PRODUCTION_NODE_REDUCTION_FUNCTION

#ifdef USE_PRODUCTION_NODE_USER_DATA
    //存储用户定义数据，可用于构造AST等
    std::any user_data_;
#endif  // USE_PRODUCTION_NODE_USER_DATA
  };

  class TerminalProductionNode : public BaseProductionNode {
   public:
    TerminalProductionNode(SymbolId symbol_id)
        : BaseProductionNode(ProductionNodeType::kTerminalNode, symbol_id) {}
    TerminalProductionNode(const TerminalProductionNode&) = delete;
    TerminalProductionNode& operator=(const TerminalProductionNode&) = delete;
    TerminalProductionNode(TerminalProductionNode&& terminal_production_node)
        : BaseProductionNode(std::move(terminal_production_node)) {}
    TerminalProductionNode& operator=(
        TerminalProductionNode&& terminal_production_node) {
      BaseProductionNode::operator=(std::move(terminal_production_node));
      return *this;
    }
    using NodeData = BaseProductionNode::NodeData;

    // 添加向前看节点ID
    void AddFirstItemForwardNodeId(ProductionNodeId forward_node_id) {
      assert(forward_node_id.IsValid());
      forward_nodes_.first.insert(forward_node_id);
    }
    void AddSecondItemForwardNodeId(ProductionNodeId forward_node_id) {
      assert(forward_node_id.IsValid());
      forward_nodes_.second.insert(forward_node_id);
    }
    // 添加一个容器内所有向前看节点ID
    template <class Container>
    void AddFirstItemForwardNodeContainer(
        Container&& forward_nodes_id_container) {
      assert(!forward_nodes_id_container.empty());
      forward_nodes_.first.merge(
          std::forward<Container>(forward_nodes_id_container));
    }
    template <class Container>
    void AddSecondItemForwardNodeContainer(
        Container&& forward_nodes_id_container) {
      assert(!forward_nodes_id_container.empty());
      forward_nodes_.second.merge(
          std::forward<Container>(forward_nodes_id_container));
    }
    template <class Container>
    void AddForwardNodeContainer(ProductionBodyId production_body_id,
                                 PointIndex point_index,
                                 Container&& forward_nodes_id_container);
    const std::unordered_set<ProductionNodeId>& GetFirstItemForwardNodeIds() {
      return forward_nodes_.first;
    }
    const std::unordered_set<ProductionNodeId>& GetSecondItemForwardNodeIds() {
      return forward_nodes_.second;
    }
    // 设置给定项对应的项集ID
    void SetFirstItemCoreId(CoreId core_id) {
      assert(core_id.IsValid());
      core_ids_.first = core_id;
    }
    void SetSecondItemCoreId(CoreId core_id) {
      assert(core_id.IsValid());
      core_ids_.second = core_id;
    }
    // 获取给定项对应的项集ID
    CoreId GetFirstItemCoreId() { return core_ids_.first; }
    CoreId GetSecondItemCoreId() { return core_ids_.second; }

#ifdef USE_TERMINAL_NODE_SHIFT_FUNCTION
    void SetShiftFunction(
        const std::function<void(const NodeData&)>& shift_function) {
      shift_function_ = shift_function;
    }
    //该函数留给自定义移入函数用
    template <class T>
    void SetShiftFunction(const std::function<T>& shift_function) {
      shift_function_ = shift_function;
    }
    const std::function<void(const NodeData&)>& GetShiftFunction() {
      return reinterpret_cast<std::function<void(const NodeData&)>&>(
          shift_function_);
    }
    //该函数留给获取自定义移入函数用
    template <class T>
    const std::function<T>& GetShiftFunction() {
      return shift_function_;
    }
#endif  // USE_PRODUCTION_NODE_SHIFT_FUNCTION

    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) override;
    //添加向前看节点
    virtual void AddForwardNodeId(ProductionBodyId production_body_id,
                                  PointIndex point_index,
                                  ProductionNodeId forward_node_id);
    //获取向前看节点集合
    virtual const std::unordered_set<ProductionNodeId>& GetForwardNodeIds(
        ProductionBodyId production_body_id, PointIndex point_index);
    //设置项对应的核心ID
    virtual void SetCoreId(ProductionBodyId production_body_id,
                           PointIndex point_index, CoreId core_id);
    //获取项对应的核心ID
    virtual CoreId GetCoreId(ProductionBodyId production_body_id,
                             PointIndex point_index);

   private:
#ifdef USE_TERMINAL_NODE_SHIFT_FUNCTION
    // 移入产生式时调用的函数
    // 使用any便于操作符节点定义自己的移入函数
    std::any shift_function_;
#endif  // USE_PRODUCTION_NODE_SHIFT_FUNCTION

    // 终结节点的两种项的向前看符号
    std::pair<std::unordered_set<ProductionNodeId>,
              std::unordered_set<ProductionNodeId>>
        forward_nodes_;
    // 两种项对应的核心ID
    std::pair<CoreId, CoreId> core_ids_;
  };

  class OperatorProductionNode : public TerminalProductionNode {
   public:
    OperatorProductionNode(SymbolId symbol_id, AssociatityType associatity_type,
                           PriorityLevel priority_level)
        : TerminalProductionNode(symbol_id),
          operator_associatity_type_(associatity_type),
          operator_priority_level_(priority_level) {}
    OperatorProductionNode(const OperatorProductionNode&) = delete;
    OperatorProductionNode& operator=(const OperatorProductionNode&) = delete;
    OperatorProductionNode(OperatorProductionNode&& operator_production_node)
        : TerminalProductionNode(std::move(operator_production_node)),
          operator_associatity_type_(
              operator_production_node.operator_associatity_type_),
          operator_priority_level_(
              operator_production_node.operator_priority_level_) {}
    OperatorProductionNode& operator=(
        OperatorProductionNode&& operator_production_node);

    struct NodeData : public TerminalProductionNode::NodeData {
      std::string symbol;
    };

    void SetAssociatityType(AssociatityType type) {
      operator_associatity_type_ = type;
    }
    AssociatityType GetAssociatityType() const {
      return operator_associatity_type_;
    }
    void SetPriorityLevel(PriorityLevel level) {
      operator_priority_level_ = level;
    }
    PriorityLevel GetPriorityLevel() const { return operator_priority_level_; }

#ifdef USE_OPERATOR_NODE_SHIFT_FUNCTION
    void SetShiftFunction(
        const std::function<void(const NodeData&)>& shift_function) {
      TerminalProductionNode::SetShiftFunction(shift_function);
    }
    const std::function<void(const NodeData&)>& GetShiftFunction() {
      return TerminalProductionNode::GetShiftFunction();
    }
#endif  // USE_PRODUCTION_NODE_SHIFT_FUNCTION

   private:
    // 运算符结合性
    AssociatityType operator_associatity_type_;
    // 运算符优先级
    PriorityLevel operator_priority_level_;
  };

  class NonTerminalProductionNode : public BaseProductionNode {
   public:
    NonTerminalProductionNode(SymbolId symbol_id)
        : BaseProductionNode(ProductionNodeType::kNonTerminalNode, symbol_id) {}
    template <class IdType>
    NonTerminalProductionNode(SymbolId symbol_id, IdType&& body)
        : BaseProductionNode(ProductionNodeType::kNonTerminalNode, symbol_id),
          nonterminal_bodys_(std::forward<IdType>(body)) {}
    NonTerminalProductionNode(const NonTerminalProductionNode&) = delete;
    NonTerminalProductionNode& operator=(const NonTerminalProductionNode&) =
        delete;
    NonTerminalProductionNode(
        NonTerminalProductionNode&& nonterminal_production_node)
        : BaseProductionNode(std::move(nonterminal_production_node)),
          nonterminal_bodys_(nonterminal_production_node.nonterminal_bodys_) {}
    NonTerminalProductionNode& operator=(
        NonTerminalProductionNode&& nonterminal_production_node);

    //重新设置产生式体数目，会同时设置相关数据
    void ResizeProductionBodyNum(size_t new_size);
    //重新设置产生式体里节点数目，会同时设置相关数据
    void ResizeProductionBodyNodeNum(ProductionBodyId production_body_id,
                                     size_t new_size);
    //添加一个产生式体，要求IdType为一个vector，按序存储产生式节点ID
    template <class IdType>
    ProductionBodyId AddBody(IdType&& body);
    const ProductionBodyType& GetBody(ProductionBodyId body_id) const {
      return nonterminal_bodys_[body_id];
    }
    const BodyContainerType& GetAllBody() const { return nonterminal_bodys_; }
    // 添加一个容器内所有向前看节点ID
    template <class Container>
    void AddForwardNodeContainer(ProductionBodyId production_body_id,
                                 PointIndex point_index,
                                 Container&& forward_nodes_container) {
      assert(point_index < forward_nodes_[production_body_id].size());
      forward_nodes_[production_body_id][point_index].merge(
          std::forward<Container>(forward_nodes_container));
    }

#ifdef USE_PRODUCTION_NODE_SHIFT_FUNCTION
    //设置移入时调用的函数，应由语法分析机运行时设定
    void SetShiftFunction(
        const std::function<void(const NodeData&)>& shift_function,
        ProductionBodyId production_body_id, PointIndex point_index);
    const std::function<void(const NodeData&)>& GetShiftFunction(
        ProductionBodyId production_body_id, PointIndex point_index);
#endif  // USE_PRODUCTION_NODE_SHIFT_FUNCTION

    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) override;
    //添加向前看节点
    virtual void AddForwardNodeId(ProductionBodyId production_body_id,
                                  PointIndex point_index,
                                  ProductionNodeId forward_node_id) {
      assert(point_index < forward_nodes_[production_body_id].size() &&
             forward_node_id.IsValid());
      forward_nodes_[production_body_id][point_index].insert(forward_node_id);
    }
    //获取向前看节点集合
    virtual const std::unordered_set<ProductionNodeId>& GetForwardNodeIds(
        ProductionBodyId production_body_id, PointIndex point_index) {
      assert(point_index < forward_nodes_[production_body_id].size());
      return forward_nodes_[production_body_id][point_index];
    }
    //设置项对应的核心ID
    virtual void SetCoreId(ProductionBodyId production_body_id,
                           PointIndex point_index, CoreId core_id) {
      assert(production_body_id < core_ids_.size() &&
             point_index < core_ids_[production_body_id].size());
      core_ids_[production_body_id][point_index] = core_id;
    }
    // 获取项对应的核心ID
    virtual CoreId GetCoreId(ProductionBodyId production_body_id,
                             PointIndex point_index) {
      assert(production_body_id < core_ids_.size() &&
             point_index < core_ids_[production_body_id].size());
      return core_ids_[production_body_id][point_index];
    }

   private:
    // 外层vector存该非终结符号下各个产生式的体
    // 内层vector存该产生式的体里含有的各个符号对应的节点ID
    BodyContainerType nonterminal_bodys_;

#ifdef USE_PRODUCTION_NODE_SHIFT_FUNCTION
    // 移入节点时调用的函数
    // 外层大小等于产生式体数目
    // 内层大小等于该条产生式体内节点数目，不+1
    std::vector<std::vector<std::function<void(const NodeData&)>>>
        shift_function_;
#endif  // USE_PRODUCTION_NODE_SHIFT_FUNCTION

    // 向前看节点，分两层组成
    // 外层对应ProductionBodyId，等于产生式体数目
    // 内层对应PointIndex，大小比对应产生式体内节点数目多1
    std::vector<std::vector<std::unordered_set<ProductionNodeId>>>
        forward_nodes_;
    // 存储项对应的核心ID
    std::vector<std::vector<CoreId>> core_ids_;
  };

  // 文件尾节点
  class EndNode : public BaseProductionNode {
   public:
    EndNode(SymbolId symbol_id)
        : BaseProductionNode(ProductionNodeType::kEndNode, symbol_id) {}
    EndNode(const EndNode&) = delete;
    EndNode& operator=(const EndNode&) = delete;
    EndNode(EndNode&& end_node) : BaseProductionNode(std::move(end_node)) {}
    EndNode& operator=(EndNode&& end_node) {
      BaseProductionNode::operator=(std::move(end_node));
      return *this;
    }
  };
  // 项集与向前看符号
  class Core {
   public:
    Core() {}
    template <class Items, class ForwardNodes>
    Core(Items&& items, ForwardNodes&& forward_nodes)
        : core_closure_available_(false),
          core_id_(CoreId::InvalidId()),
          core_items_(std::forward<Items>(items)) {}
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;
    Core(Core&& core)
        : core_closure_available_(std::move(core.core_closure_available_)),
          core_id_(std::move(core.core_id_)),
          core_items_(std::move(core.core_items_)) {}
    Core& operator=(Core&& core);
    // 不应直接使用，会导致无法更新item到CoreId的映射
    // 返回给定Item插入后的iterator和bool
    // bool在不存在给定item且插入成功时为true
    std::pair<std::set<CoreItem>::iterator, bool> AddItem(
        const CoreItem& item) {
      SetClosureNotAvailable();
      return core_items_.insert(item);
    }
    // 判断给定item是否在该项集内，在则返回true
    bool IsItemIn(const CoreItem& item) const {
      return core_items_.find(item) != core_items_.end();
    }
    bool IsClosureAvailable() const { return core_closure_available_; }
    // 设置core_id
    void SetCoreId(CoreId core_id) { core_id_ = core_id; }
    CoreId GetCoreId() const { return core_id_; }
    // 设置该项集求的闭包有效，仅应由闭包函数调用
    void SetClosureAvailable() { core_closure_available_ = true; }
    // 设置该项集已求的闭包无效，应由每个修改了core_items_的函数调用
    void SetClosureNotAvailable() { core_closure_available_ = false; }
    const std::set<CoreItem>& GetItems() const { return core_items_; }
    size_t Size() { return core_items_.size(); }

   private:
    // 该项集求的闭包是否有效（求过闭包且之后没有做任何更改则为true）
    bool core_closure_available_ = false;
    // 项集ID
    CoreId core_id_;
    // 项集
    std::set<CoreItem> core_items_;
  };

  // 语法分析表条目
  class ParsingTableEntry {
   public:
    using TerminalNodeActionAndTargetContainerType = std::unordered_map<
        ProductionNodeId,
        std::pair<ActionType,
                  std::variant<ParsingTableEntryId,
                               std::pair<ProductionNodeId, ProductionBodyId>>>>;

    ParsingTableEntry() {}
    ParsingTableEntry(const ParsingTableEntry&) = delete;
    ParsingTableEntry& operator=(const ParsingTableEntry&) = delete;
    ParsingTableEntry(ParsingTableEntry&& parsing_table_entry)
        : action_and_target_(std::move(parsing_table_entry.action_and_target_)),
          nonterminal_node_transform_table_(std::move(
              parsing_table_entry.nonterminal_node_transform_table_)) {}
    ParsingTableEntry& operator=(ParsingTableEntry&& parsing_table_entry);

    // 设置该产生式在转移条件下的动作和目标节点
    void SetTerminalNodeActionAndTarget(
        ProductionNodeId node_id, ActionType action_type,
        std::variant<ParsingTableEntryId,
                     std::pair<ProductionNodeId, ProductionBodyId>>
            target_id) {
      action_and_target_[node_id] = std::make_pair(action_type, target_id);
    }
    // 设置该条目移入非终结节点后转移到的节点
    void SetNonTerminalNodeTransformId(ProductionNodeId node_id,
                                       ParsingTableEntryId id) {
      nonterminal_node_transform_table_[node_id] = id;
    }
    // 访问该条目下给定ID终结节点的行为与目标ID
    const TerminalNodeActionAndTargetContainerType::mapped_type& AtTerminalNode(
        ProductionNodeId node_id) {
      auto iter = action_and_target_.find(node_id);
      assert(iter != action_and_target_.end());
      return iter->second;
    }
    // 访问该条目下给定ID非终结节点移入时转移到的条目ID
    ParsingTableEntryId ShiftNonTerminalNode(ProductionNodeId node_id) {
      auto iter = nonterminal_node_transform_table_.find(node_id);
      assert(iter != nonterminal_node_transform_table_.end());
      return iter->second;
    }
    // 获取全部终结节点的操作
    const TerminalNodeActionAndTargetContainerType&
    GetAllTerminalNodeActionAndTarget() {
      return action_and_target_;
    }
    // 获取全部非终结节点转移到的表项
    const std::unordered_map<ProductionNodeId, ParsingTableEntryId>&
    GetAllNonTerminalNodeTransformTarget() {
      return nonterminal_node_transform_table_;
    }

   private:
    // 向前看符号ID下的操作和目标节点
    // 操作为移入时variant存移入后转移到的ID（ProductionBodyId）
    // 操作为规约时variant存使用的产生式ID和产生式体ID（ProductionNodeId）
    // 操作为接受和报错时variant未定义
    TerminalNodeActionAndTargetContainerType action_and_target_;
    // 移入非终结节点后转移到的产生式体序号
    std::unordered_map<ProductionNodeId, ParsingTableEntryId>
        nonterminal_node_transform_table_;
  };
};

template <class IdType>
inline LexicalGenerator::ProductionBodyId
LexicalGenerator::NonTerminalProductionNode::AddBody(IdType&& body) {
  ProductionBodyId body_id = nonterminal_bodys_.size();
  // 将输入插入到产生式体向量中，无删除相同产生式功能
  nonterminal_bodys_.emplace_back(std::forward<IdType>(body));
  // 为产生式体的所有不同位置的点对应的向前看符号留出空间
  ResizeProductionBodyNum(nonterminal_bodys_.size());
  // 为不同点的位置留出空间
  ResizeProductionBodyNodeNum(body_id, body.size());
  return body_id;
}

template <class Container>
inline void LexicalGenerator::TerminalProductionNode::AddForwardNodeContainer(
    ProductionBodyId production_body_id, PointIndex point_index,
    Container&& forward_nodes_id_container) {
  assert(production_body_id == 0 && point_index <= 1);
  if (point_index == 0) {
    AddFirstItemForwardNodeContainer(
        std::forward<Container>(forward_nodes_id_container));
  } else {
    AddSecondItemForwardNodeContainer(
        std::forward<Container>(forward_nodes_id_container));
  }
}

template <class IdType>
inline LexicalGenerator::ProductionNodeId LexicalGenerator::AddNonTerminalNode(
    SymbolId symbol_id, IdType&& bodys) {
  ProductionNodeId node_id =
      manager_nodes_.EmplaceObject<NonTerminalProductionNode>(
          ProductionNodeType::kNonTerminalNode, symbol_id,
          std::forward<IdType>(bodys));
  manager_nodes_.GetObject(node_id).SetThisNodeId(node_id);
  return node_id;
}

template <class Items, class ForwardNodes>
inline LexicalGenerator::CoreId LexicalGenerator::AddNewCore(
    Items&& items, ForwardNodes&& forward_nodes) {
  CoreId core_id = cores_.size();
  cores_.emplace_back(std::forward<Items>(items),
                      std::forward<ForwardNodes>(forward_nodes));
  return core_id;
}

template <class ParsingTableEntryIdContainer>
inline void LexicalGenerator::ParsingTableTerminalNodeClassify(
    const std::vector<ProductionNodeId>& terminal_node_ids, size_t index,
    ParsingTableEntryIdContainer&& entry_ids,
    std::vector<std::vector<ParsingTableEntryId>>* equivalent_ids) {
  if (index >= terminal_node_ids.size()) {
    // 分类完成，执行合并操作
    assert(entry_ids.size() > 1);
    equivalent_ids->emplace_back(
        std::forward<ParsingTableEntryIdContainer>(entry_ids));
  } else {
    assert(GetProductionNode(terminal_node_ids[index]).Type() ==
               ProductionNodeType::kTerminalNode ||
           GetProductionNode(terminal_node_ids[index]).Type() ==
               ProductionNodeType::kOperatorNode);
    // 分类表，根据转移条件下的转移结果分类
    std::map<const ParsingTableEntry::TerminalNodeActionAndTargetContainerType::
                 mapped_type,
             std::vector<ParsingTableEntryId>>
        classify_table;
    ProductionNodeId transform_id = terminal_node_ids[index];
    for (auto id : entry_ids) {
      // 利用map进行分类
      classify_table[GetParsingTableEntry(id).AtTerminalNode(transform_id)]
          .push_back(id);
    }
    size_t next_index = index + 1;
    for (auto vec : classify_table) {
      if (vec.second.size() > 1) {
        // 该类含有多个条目且有剩余未比较的转移条件，需要继续分类
        ParsingTableTerminalNodeClassify(terminal_node_ids, next_index,
                                         std::move(vec.second), equivalent_ids);
      }
    }
  }
}

template <class ParsingTableEntryIdContainer>
inline void LexicalGenerator::ParsingTableNonTerminalNodeClassify(
    const std::vector<ProductionNodeId>& nonterminal_node_ids, size_t index,
    ParsingTableEntryIdContainer&& entry_ids,
    std::vector<std::vector<ParsingTableEntryId>>* equivalent_ids) {
  if (index >= nonterminal_node_ids.size()) {
    // 分类完成，执行合并操作
    assert(entry_ids.size() > 1);
    equivalent_ids->emplace_back(
        std::forward<ParsingTableEntryIdContainer>(entry_ids));
  } else {
    assert(GetProductionNode(nonterminal_node_ids[index]).Type() ==
           ProductionNodeType::kNonTerminalNode);
    // 分类表，根据转移条件下的转移结果分类
    std::map<const ParsingTableEntryId, std::vector<ParsingTableEntryId>>
        classify_table;
    ProductionNodeId transform_id = nonterminal_node_ids[index];
    for (auto id : entry_ids) {
      // 利用map进行分类
      classify_table[GetParsingTableEntry(id).ShiftNonTerminalNode(
                         transform_id)]
          .push_back(id);
    }
    size_t next_index = index + 1;
    for (auto vec : classify_table) {
      if (vec.second.size() > 1) {
        // 该类含有多个条目且有剩余未比较的转移条件，需要继续分类
        ParsingTableTerminalNodeClassify(nonterminal_node_ids, next_index,
                                         std::move(vec.second), equivalent_ids);
      }
    }
  }
}

template <class Container>
inline void LexicalGenerator::AddForwardNodeContainer(
    ProductionNodeId production_node_id, ProductionBodyId production_body_id,
    PointIndex point_index, Container&& forward_nodes_container) {
  BaseProductionNode& production_node = GetProductionNode(production_node_id);
  switch (production_node.Type()) {
    case ProductionNodeType::kTerminalNode:
    case ProductionNodeType::kOperatorNode:
      static_cast<TerminalProductionNode&>(production_node)
          .AddForwardNodeContainer(
              production_body_id, point_index,
              std::forward<Container>(forward_nodes_container));
      break;
    case ProductionNodeType::kNonTerminalNode:
      static_cast<NonTerminalProductionNode&>(production_node)
          .AddForwardNodeContainer(
              production_body_id, point_index,
              std::forward<Container>(forward_nodes_container));
      break;
    default:
      assert(false);
      break;
  }
}

}  // namespace frontend::generator::lexicalgenerator
#endif  // !GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_