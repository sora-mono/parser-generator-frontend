#include <array>
#include <cassert>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "Common/id_wrapper.h"
#include "Common/object_manager.h"
#include "Common/unordered_struct_manager.h"

#ifndef GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_
#define GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_

namespace frontend::generator::lexicalgenerator {
using frontend::common::ObjectManager;
using frontend::common::UnorderedStructManager;

struct StringHasher {
  frontend::common::StringHashType DoHash(const std::string& str) {
    return frontend::common::HashString(str);
  }
};
// TODO 添加删除未使用产生式的功能
// TODO 添加ProductionNodeType::kEndNode
class LexicalGenerator {
  //为下面定义各种类型不得不使用前置声明
  class BaseProductionNode;
  class ParsingTableEntry;
  class Core;

  enum class WrapperLabel {
    kCoreId,
    kPriorityLevel,
    kPointIndex,
    kParsingTableEntryId,
    kProductionBodyId
  };  //核心项ID
  using CoreId = frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                                     WrapperLabel::kCoreId>;
  //运算符优先级
  using PriorityLevel =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kPriorityLevel>;
  //点的位置
  using PointIndex =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kPointIndex>;
  //语法分析表条目ID
  using ParsingTableEntryId =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kParsingTableEntryId>;
  //非终结节点内产生式编号
  using ProductionBodyId =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kProductionBodyId>;
  //产生式节点ID
  using ProductionNodeId = ObjectManager<BaseProductionNode>::ObjectId;
  //单个产生式体
  using ProductionBodyType = std::vector<ProductionNodeId>;
  //产生式容器
  using BodyContainerType = std::vector<ProductionBodyType>;
  //符号ID
  using SymbolId = UnorderedStructManager<std::string, StringHasher>::ObjectId;
  //向前看符号容器（最内层容器，由ProductionBodyId和PointIndex共同决定）
  using InsideForwardNodesContainerType = std::unordered_set<ProductionNodeId>;
  //向前看符号容器（中间层容器，由ProductionBodyId决定）
  using MiddleForwardNodesContainerType =
      std::vector<InsideForwardNodesContainerType>;
  //向前看符号容器（最外层容器，与产生式一一对应）
  using OutSideForwardNodesContainerType =
      std::vector<MiddleForwardNodesContainerType>;
  //不同项对应项集ID（CoreId）
  using CoreIdContainerType = std::vector<std::vector<CoreId>>;

  //内核内单个项集，参数从左到右依次为
  // 产生式节点ID，产生式体ID，产生式体中点的位置
  using CoreItem = std::tuple<ProductionNodeId, ProductionBodyId, PointIndex>;
  //节点类型：终结符号，运算符，非终结符号
  enum class ProductionNodeType {
    kTerminalNode,
    kOperatorNode,
    kNonTerminalNode
  };
  //运算符结合类型：左结合，右结合
  enum class AssociatityType { kLeftAssociate, kRightAssociate };
  //分析动作类型：规约，移入，报错，接受
  enum class ActionType { kReduction, kShift, kError, kAccept };

  //解析产生式，仅需且必须输入一个完整的产生式
  ProductionNodeId AnalysisProduction(const std::string& str);
  //添加符号，返回符号的ID和是否执行了插入操作，执行了插入操作则返回true
  std::pair<SymbolId, bool> AddSymbol(const std::string& str) {
    return manager_symbol_.AddObject(str);
  }
  //获取符号对应ID
  SymbolId GetSymbolId(const std::string& str) {
    return manager_symbol_.GetObjectId(str);
  }
  //通过符号ID查询对应字符串
  const std::string& GetSymbolString(SymbolId symbol_id) {
    return manager_symbol_.GetObject(symbol_id);
  }
  //设置符号ID到节点ID的映射
  void SetSymbolIdToProductionNodeIdMapping(SymbolId symbol_id,
                                            ProductionNodeId node_id);
  //新建终结节点，返回节点ID
  //节点已存在且给定symbol_id不同于已有ID则返回ProductionNodeId::InvalidId()
  ProductionNodeId AddTerminalNode(SymbolId symbol_id);
  //新建运算符节点，返回节点ID，节点已存在则返回ProductionNodeId::InvalidId()
  ProductionNodeId AddOperatorNode(SymbolId symbol_id,
                                   AssociatityType associatity_type,
                                   PriorityLevel priority_level);
  //新建非终结节点，返回节点ID
  //第二个参数为产生式体，参数结构为std::initializer_list<std::vector<Object>>
  //或std::vector<std::vector<Object>>
  template <class IdType>
  ProductionNodeId AddNonTerminalNode(SymbolId symbol_id, IdType&& bodys);
  //获取节点
  BaseProductionNode& GetProductionNode(ProductionNodeId id) {
    return manager_nodes_[id];
  }
  //给非终结节点添加产生式体
  template <class IdType>
  void AddNonTerminalNodeBody(ProductionNodeId node_id, IdType&& body) {
    static_cast<NonTerminalProductionNode*>(GetProductionNode(node_id))
        ->AddBody(std::forward<IdType>(body));
  }
  //通过符号ID查询对应所有节点的ID，不存在则返回空vector
  std::vector<ProductionNodeId> GetSymbolToProductionNodeIds(
      SymbolId symbol_id);
  //获取core_id对应产生式项集
  Core& GetCore(CoreId core_id) {
    assert(core_id < cores_.size());
    return cores_[core_id];
  }
  //添加新的核心
  CoreId AddNewCore() {
    CoreId core_id = CoreId(cores_.size());
    cores_.emplace_back();
    return core_id;
  }
  // Items要求为std::vector<CoreItem>或std::initialize_list<CoreItem>
  // ForwardNodes要求为std::vector<Object>或std::initialize_list<Object>
  template <class Items, class ForwardNodes>
  CoreId AddNewCore(Items&& items, ForwardNodes&& forward_nodes);
  //添加项到对应项集的映射，会覆盖原有记录
  void SetItemCoreId(const CoreItem& core_item, CoreId core_id);
  //向项集中添加项，不要使用core.AddItem()，会导致无法更新映射记录
  //会设置相应BaseProductionNode中该项到CoreId的记录
  std::pair<std::set<CoreItem>::iterator, bool> AddItemToCore(
      const CoreItem& core_item, CoreId core_id) {
    assert(core_id != CoreId::InvalidId());
    auto iterator_and_state = GetCore(core_id).AddItem(core_item);
    if (iterator_and_state.second == true) {
      //以前不存在则添加映射记录
      SetItemCoreId(core_item, core_id);
    }
    return iterator_and_state;
  }
  //获取给定项对应项集的ID，使用的参数必须为有效参数
  //未指定（不曾使用SetItemCoreId设置）则返回CoreId::InvalidId()
  CoreId GetCoreId(ProductionNodeId production_node_id,
                   ProductionBodyId production_body_id,
                   PointIndex point_index) {
    return GetProductionNode(production_node_id)
        .GetCoreId(production_body_id, point_index);
  }
  CoreId GetItemCoreId(const CoreItem& core_item);
  //获取给定项对应项集的ID，如果不存在记录则插入到新项集里并返回相应ID
  //返回的bool代表是否core_item不存在并成功创建并添加到新的项集
  // insert_core_id是不存在记录时应插入的Core的ID
  //设为CoreId::InvalidId()代表新建Core
  std::pair<CoreId, bool> GetItemCoreIdOrInsert(
      const CoreItem& core_item, CoreId insert_core_id = CoreId::InvalidId());
  //获取向前看符号集
  const InsideForwardNodesContainerType& GetFowardNodes(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index) {
    return GetProductionNode(production_node_id)
        .GetForwardNodeIds(production_body_id, point_index);
  }
  //添加向前看符号
  void AddForwardNode(ProductionNodeId production_node_id,
                      ProductionBodyId production_body_id,
                      PointIndex point_index,
                      ProductionNodeId forward_node_id) {
    GetProductionNode(production_node_id)
        .AddForwardNode(production_body_id, point_index, forward_node_id);
  }
  //添加一个容器内的所有向前看符号
  template <class Container>
  void AddForwardNodeContainer(ProductionNodeId production_node_id,
                               ProductionBodyId production_body_id,
                               PointIndex point_index,
                               Container&& forward_nodes_container) {
    GetProductionNode(production_node_id)
        .AddForwardNodeContainer(
            production_body_id, point_index,
            std::forward<Container>(forward_nodes_container));
  }
  //闭包操作中的first操作，如果first_node_id有效则返回仅含自己的集合
  //无效则返回next_node_ids
  std::unordered_set<ProductionNodeId> First(
      ProductionNodeId first_node_id,
      const std::unordered_set<ProductionNodeId>& next_node_ids);
  //获取核心的全部项
  const std::set<CoreItem>& GetCoreItems(CoreId core_id) {
    return GetCore(core_id).GetItems();
  }
  //获取最终语法分析表条目
  ParsingTableEntry& GetParsingTableEntry(ParsingTableEntryId id) {
    assert(id < lexical_config_parsing_table_.size());
    return lexical_config_parsing_table_[id];
  }
  //设置根产生式ID
  void SetRootProductionId(ProductionNodeId root_production) {
    root_production_ = root_production;
  }
  //设置项集闭包有效（避免每次Goto都求闭包）
  void SetCoreClosureAvailable(CoreId core_id) {
    GetCore(core_id).SetClosureAvailable();
  }
  bool IsClosureAvailable(CoreId core_id) {
    return GetCore(core_id).IsClosureAvailable();
  }
  //对给定核心项求闭包，结果存在原位置
  void CoreClosure(CoreId core_id);
  //删除Goto缓存中给定core_id的所有缓存，函数执行后保证缓存不存在
  void RemoveGotoCacheEntry(CoreId core_id) {
    assert(core_id != CoreId::InvalidId());
    auto [iter_begin, iter_end] = core_id_goto_core_id_.equal_range(core_id);
    for (; iter_begin != iter_end; ++iter_begin) {
      core_id_goto_core_id_.erase(iter_begin);
    }
  }
  //设置一条Goto缓存
  void SetGotoCacheEntry(CoreId core_id_src,
                         ProductionNodeId transform_production_node_id,
                         CoreId core_id_dst) {
    assert(core_id_src != CoreId::InvalidId() &&
           core_id_dst != CoreId::InvalidId());
    auto [iter_begin, iter_end] =
        core_id_goto_core_id_.equal_range(core_id_src);
    for (; iter_begin != iter_end; ++iter_end) {
      //如果已有缓存项则修改
      if (iter_begin->second.first == transform_production_node_id) {
        iter_begin->second.second = core_id_dst;
        return;
      }
    }
    //不存在该条缓存则插入
    core_id_goto_core_id_.insert(std::make_pair(
        core_id_src,
        std::make_pair(transform_production_node_id, core_id_dst)));
  }
  //获取一条Goto缓存，不存在则返回CoreId::InvalidId()
  CoreId GetGotoCacheEntry(CoreId core_id_src,
                           ProductionNodeId transform_production_node_id) {
    assert(core_id_src != CoreId::InvalidId());
    CoreId core_id_dst = CoreId::InvalidId();
    auto [iter_begin, iter_end] =
        core_id_goto_core_id_.equal_range(core_id_src);
    for (; iter_begin != iter_end; ++iter_begin) {
      if (iter_begin->second.first == transform_production_node_id) {
        core_id_dst = iter_begin->second.second;
        break;
      }
    }
    return core_id_dst;
  }
  // Goto缓存是否有效，有效返回true
  bool IsGotoCacheAvailable(CoreId core_id) {
    return GetCore(core_id).IsClosureAvailable();
  }
  //获取该项下一个产生式节点ID，如果不存在则返回ProductionNodeId::InvalidId()
  ProductionNodeId GetProductionBodyNextNodeId(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index);
  //获取该项的向前看节点ID，如果不存在则返回ProductionNodeId::InvalidId()
  ProductionNodeId GetProductionBodyNextNextNodeId(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index);
  // Goto的子过程，返回ItemGoto后的ID和是否插入
  //如果不存在已有的记录则插入到insert_core_id中
  //需要插入时insert_core_id为CoreId::InvalidId()则创建新的Core并插入
  //返回Goto后的CoreId和是否执行了向core中插入操作
  //如果在给定参数上无法执行返回CoreId::InvalidId()和false
  std::pair<CoreId, bool> ItemGoto(
      const CoreItem& item, ProductionNodeId transform_production_node_id,
      CoreId insert_core_id = CoreId::InvalidId());
  //获取从给定核心项开始，移入node_id对应的节点后到达的核心项
  //自动创建不存在的核心项，如果Goto的结果为空则返回CoreId::InvalidId()
  //返回的第二个参数代表返回的Core是否是新建的，新建的为true
  std::pair<CoreId, bool> Goto(CoreId core_id_src,
                               ProductionNodeId transform_production_node_id);
  //传播向前看符号
  void SpreadLookForwardSymbol(CoreId core_id,
                               ProductionNodeId transform_production_node_id);
  //构建LALR(1)配置
  void LalrConstruct();
  //对给定节点与点的位置求闭包，并将向前看符号传播出去
  std::set<CoreItem> ItemClosure(CoreItem item);

 private:
  //根产生式ID
  ProductionNodeId root_production_;
  //管理终结符号、非终结符号等的节点
  ObjectManager<BaseProductionNode> manager_nodes_;
  //管理符号
  UnorderedStructManager<std::string, StringHasher> manager_symbol_;
  //符号ID（manager_symbol_)返回的ID到对应节点的映射，支持一个符号对应多个节点
  std::unordered_multimap<SymbolId, ProductionNodeId> symbol_id_to_node_id_;

  //内核+非内核项集
  std::vector<Core> cores_;
  // Goto缓存，存储已有的CoreI在不同条件下Goto的节点
  std::multimap<CoreId, std::pair<ProductionNodeId, CoreId>>
      core_id_goto_core_id_;
  //语法分析表
  std::vector<ParsingTableEntry> lexical_config_parsing_table_;

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
          base_symbol_id_(base_production_node.base_symbol_id_),
          base_id_(base_production_node.base_id_),
          base_forward_nodes_(
              std::move(base_production_node.base_forward_nodes_)),
          base_core_ids_(std::move(base_production_node.base_id_)) {}

    void SetType(ProductionNodeType type) { base_type_ = type; }
    ProductionNodeType Type() const { return base_type_; }
    void SetThisNodeId(ProductionNodeId id) { base_id_ = id; }
    ProductionNodeId Id() const { return base_id_; }
    void SetSymbolId(SymbolId id) { base_symbol_id_ = id; }
    SymbolId GetSymbolId() const { return base_symbol_id_; }

    //获取向前看节点ID集合
    const InsideForwardNodesContainerType& GetForwardNodeIds(
        ProductionBodyId production_body_id, PointIndex point_index) const {
      assert(point_index < base_forward_nodes_[production_body_id].size());
      return base_forward_nodes_[production_body_id][point_index];
    }
    //添加向前看节点ID
    void AddForwardNode(ProductionBodyId production_body_id,
                        PointIndex point_index,
                        ProductionNodeId forward_node_id) {
      assert(point_index < base_forward_nodes_[production_body_id].size() &&
             forward_node_id.IsValid());
      base_forward_nodes_[production_body_id][point_index].insert(
          forward_node_id);
    }
    //添加向前看节点ID容器
    template <class Container>
    void AddForwardNodeContainer(ProductionBodyId production_body_id,
                                 PointIndex point_index,
                                 Container&& forward_nodes_container) {
      assert(point_index < base_forward_nodes_[production_body_id].size());
      base_forward_nodes_[production_body_id][point_index].merge(
          std::forward<Container>(forward_nodes_container));
    }
    //设置给定项对应的项集ID
    void SetCoreId(ProductionBodyId production_body_id, PointIndex point_index,
                   CoreId core_id) {
      assert(production_body_id < base_core_ids_.size() &&
             point_index < base_core_ids_[production_body_id].size());
      base_core_ids_[production_body_id][point_index] = core_id;
    }
    //获取给定项对应的项集ID
    CoreId GetCoreId(ProductionBodyId production_body_id,
                     PointIndex point_index) {
      assert(production_body_id < base_core_ids_.size() &&
             point_index < base_core_ids_[production_body_id].size());
      return base_core_ids_[production_body_id][point_index];
    }
    //设置产生式体大小
    void ResizeProductionBody(size_t new_size) {
      base_forward_nodes_.resize(new_size);
      base_core_ids_.resize(new_size);
    }
    //设置产生式体内节点数（填production_body.size()，不要+1）
    void ResizeProductionBodyInsideNodes(ProductionBodyId production_node_id,
                                         size_t new_size) {
      assert(production_node_id < base_forward_nodes_.size());
      base_forward_nodes_[production_node_id].resize(new_size + 1);
      base_core_ids_[production_node_id].resize(new_size + 1,
                                                CoreId::InvalidId());
    }
    //获取给定项对应的产生式ID，返回point_index后面的ID
    // point_index越界时返回ProducNodeId::InvalidId()
    //为了支持向前看多个节点允许越界
    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) = 0;

   private:
    //节点类型
    ProductionNodeType base_type_;
    //节点ID
    ProductionNodeId base_id_;
    //节点符号ID
    SymbolId base_symbol_id_;
    //向前看节点，分三层组成，最外层仅有一个
    //中间层对应ProductionBodyId，等于产生式体数目
    //最内层对应PointIndex，大小比对应产生式体内节点数目多1
    OutSideForwardNodesContainerType base_forward_nodes_;
    //存储项对应的项集ID（CoreId）
    CoreIdContainerType base_core_ids_;
  };

  class TerminalProductionNode : public BaseProductionNode {
   public:
    TerminalProductionNode(ProductionNodeType type, SymbolId symbol_id)
        : BaseProductionNode(type, symbol_id) {
      ResizeProductionBodyInsideNodes(ProductionBodyId(0), 1);
    }
    TerminalProductionNode(const TerminalProductionNode&) = delete;
    TerminalProductionNode& operator=(const TerminalProductionNode&) = delete;
    TerminalProductionNode(TerminalProductionNode&& terminal_production_node)
        : BaseProductionNode(std::move(terminal_production_node)) {}

    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) override;
  };

  class OperatorProductionNode : public TerminalProductionNode {
   public:
    OperatorProductionNode(ProductionNodeType type, SymbolId symbol_id,
                           AssociatityType associatity_type,
                           PriorityLevel priority_level)
        : TerminalProductionNode(type, symbol_id),
          operator_associatity_type_(associatity_type),
          operator_priority_level_(priority_level) {}
    OperatorProductionNode(const OperatorProductionNode&) = delete;
    OperatorProductionNode& operator=(const OperatorProductionNode&) = delete;
    OperatorProductionNode(OperatorProductionNode&& operator_production_node)
        : TerminalProductionNode(std::move(operator_production_node)) {}

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

   private:
    //运算符结合性
    AssociatityType operator_associatity_type_;
    //运算符优先级
    PriorityLevel operator_priority_level_;
  };

  class NonTerminalProductionNode : public BaseProductionNode {
   public:
    NonTerminalProductionNode(ProductionNodeType type, SymbolId symbol_id)
        : BaseProductionNode(type, symbol_id) {}
    template <class IdType>
    NonTerminalProductionNode(ProductionNodeType type, SymbolId symbol_id,
                              IdType&& body)
        : BaseProductionNode(type, symbol_id),
          nonterminal_bodys_(std::forward<IdType>(body)) {}
    NonTerminalProductionNode(const NonTerminalProductionNode&) = delete;
    NonTerminalProductionNode& operator=(const NonTerminalProductionNode&) =
        delete;
    NonTerminalProductionNode(
        NonTerminalProductionNode&& nonterminal_production_node)
        : BaseProductionNode(std::move(nonterminal_production_node)) {}

    template <class IdType>
    ProductionBodyId AddBody(IdType&& body);
    const ProductionBodyType& GetBody(ProductionBodyId body_id) const {
      return nonterminal_bodys_[body_id];
    }
    const BodyContainerType& GetAllBody() const { return nonterminal_bodys_; }

    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) override;

   private:
    //外层vector存该非终结符号下各个产生式的体
    //内层vector存该产生式的体里含有的各个符号对应的节点ID
    BodyContainerType nonterminal_bodys_;
  };

  //项集与向前看符号
  class Core {
   public:
    Core() {}
    template <class Items, class ForwardNodes>
    Core(Items&& items, ForwardNodes&& forward_nodes)
        : items_(std::forward<Items>(items)), closure_available_(false) {}
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;
    Core(Core&& core)
        : items_(std::move(core.items_)),
          closure_available_(core.closure_available_) {}

    //不应直接使用，会导致无法更新item到CoreId的映射
    //返回给定Item插入后的iterator和bool
    // bool在不存在给定item且插入成功时为true
    std::pair<std::set<CoreItem>::iterator, bool> AddItem(
        const CoreItem& item) {
      SetClosureNotAvailable();
      return items_.insert(item);
    }
    //判断给定item是否在该项集内，在则返回true
    bool IsItemIn(const CoreItem& item) const {
      return items_.find(item) != items_.end();
    }
    bool IsClosureAvailable() const { return closure_available_; }
    //设置core_id
    void SetCoreId(CoreId core_id) { core_id_ = core_id; }
    CoreId GetCoreId() const { return core_id_; }
    //设置该项集求的闭包有效，仅应由闭包函数调用
    void SetClosureAvailable() { closure_available_ = true; }
    //设置该项集已求的闭包无效，应由每个修改了items_的函数调用
    void SetClosureNotAvailable() { closure_available_ = false; }
    const std::set<CoreItem>& GetItems() const { return items_; }

   private:
    //该项集求的闭包是否有效（求过闭包且之后没有做任何更改则为true）
    bool closure_available_ = false;
    //项集ID
    CoreId core_id_;
    //项集
    std::set<CoreItem> items_;
  };

  //语法分析表条目
  class ParsingTableEntry {
   public:
    ParsingTableEntry() {}
    ParsingTableEntry(const ParsingTableEntry&) = delete;
    ParsingTableEntry& operator=(const ParsingTableEntry&) = delete;

    //设置终结节点存储大小
    void ResizeTerminalStore(size_t new_size) {
      action_and_target_production_body.resize(new_size);
    }
    //设置非终结节点存储大小
    void ResizeNonTerminalStore(size_t new_size) {
      nonterminal_node_transform_table.resize(new_size);
    }
    //设置该产生式在转移条件下的动作和目标节点
    void SetTerminalNodeActionAndTarget(
        size_t index, ActionType action_type,
        std::variant<ParsingTableEntryId, ProductionNodeId> target_id) {
      action_and_target_production_body[index] =
          std::make_pair(action_type, target_id);
    }
    void SetNonTerminalNodeTransformId(size_t index, ParsingTableEntryId id) {
      nonterminal_node_transform_table[index] = id;
    }
    //访问该条目下给定ID终结节点的行为与目标ID
    std::pair<ActionType, std::variant<ParsingTableEntryId, ProductionNodeId>>
    AtTerminalNode(size_t index) {
      return action_and_target_production_body[index];
    }
    //访问该条目下给定ID非终结节点移入时转移到的条目ID
    ParsingTableEntryId AtNonTerminalNode(size_t index) {
      return nonterminal_node_transform_table[index];
    }

   private:
    //向前看符号ID下的操作和目标节点
    //操作为移入时variant存移入后转移到的ID（ProductionBodyId）
    //操作为规约时variant存使用的产生式体的ID（ProductionNodeId）
    //操作为接受和报错时variant未定义
    std::vector<std::pair<ActionType,
                          std::variant<ParsingTableEntryId, ProductionNodeId>>>
        action_and_target_production_body;
    //移入非终结节点后转移到的产生式体序号
    std::vector<ParsingTableEntryId> nonterminal_node_transform_table;
  };
};

template <class IdType>
inline LexicalGenerator::ProductionBodyId
LexicalGenerator::NonTerminalProductionNode::AddBody(IdType&& body) {
  ProductionBodyId body_id = nonterminal_bodys_.size();
  //将输入插入到产生式体向量中，无删除相同产生式功能
  nonterminal_bodys_.emplace_back(std::forward<IdType>(body));
  //为产生式体的所有不同位置的点对应的向前看符号留出空间
  ResizeProductionBody(nonterminal_bodys_.size());
  //为不同点的位置留出空间
  ResizeProductionBodyInsideNodes(body.size());
  return body_id;
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

}  // namespace frontend::generator::lexicalgenerator
#endif  // !GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_