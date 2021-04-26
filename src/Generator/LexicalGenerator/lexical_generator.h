#include <cassert>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "Common/node_manager.h"
#include "Common/unordered_struct_manager.h"

#ifndef GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_
#define GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_

namespace frontend::generator::lexicalgenerator {
using frontend::common::NodeManager;
using frontend::common::UnorderedStructManager;

struct StringHasher {
  frontend::common::StringHashType DoHash(const std::string& str) {
    return frontend::common::HashString(str);
  }
};
// TODO 添加删除未使用产生式的功能
class LexicalGenerator {
  //为下面定义各种类型不得不使用前置声明
  class BaseNode;
  class ParsingTableEntry;
  class Core;

  //产生式的ID
  using NodeId = NodeManager<BaseNode>::NodeId;
  //符号的ID
  using SymbolId = UnorderedStructManager<std::string, StringHasher>::NodeId;
  //核心项ID
  using CoreId = size_t;
  //运算符优先级
  using PriorityLevel = size_t;
  //点的位置
  using PointIndex = size_t;
  //语法分析表条目ID
  using ParsingTableEntryId = size_t;
  //内核内单个项集
  using CoreItem = std::pair<NodeId, PointIndex>;
  //节点类型：终结符号，运算符，非终结符号
  enum class NodeType { kTerminalNode, kOperatorNode, kNonTerminalNode };
  //运算符结合类型：左结合，右结合
  enum class AssociatityType { kLeftAssociate, kRightAssociate };
  //分析动作类型：规约，移入，报错，接受
  enum class ActionType { kReduction, kShift, kError, kAccept };

  //产生式的体的ID，使用对象存储与NodeId在variant中得以区分
  class ProductionBodyId {
   public:
    ProductionBodyId() : production_body_id_(-1) {}
    ProductionBodyId(size_t production_body_id)
        : production_body_id_(production_body_id) {}
    operator size_t() { return production_body_id_; }

   private:
    size_t production_body_id_;
  };
  //解析产生式，仅需且必须输入一个完整的产生式
  NodeId AnalysisProduction(const std::string& str);
  //添加符号，返回符号的ID，返回-1代表符号已存在
  SymbolId AddSymbol(const std::string& str) {
    return manager_symbol_.AddObject(str).second;
  }
  //获取符号对应ID，不存在则返回-1
  SymbolId GetSymbolId(const std::string& str) {
    return manager_symbol_.GetObjectId(str);
  }
  //通过符号ID查询对应字符串，不存在则返回nullptr
  const std::string* GetSymbolString(SymbolId symbol_id) {
    return manager_symbol_.GetObjectPtr(symbol_id);
  }
  //设置符号ID到节点ID的映射
  void AddSymbolIdToNodeIdMapping(SymbolId symbol_id, NodeId node_id);
  //新建终结节点，返回节点ID，节点已存在且给定symbol_id不同于已有ID则返回-1
  NodeId AddTerminalNode(SymbolId symbol_id);
  //新建运算符节点，返回节点ID，节点已存在则返回-1
  NodeId AddOperatorNode(SymbolId symbol_id, AssociatityType associatity_type,
                         PriorityLevel priority_level);
  //新建非终结节点，返回节点ID，节点已存在则返回-1
  //第二个参数为产生式体，参数结构为std::initializer_list<std::vector<NodeId>>
  //或std::vector<std::vector<NodeId>>
  template <class T>
  NodeId AddNonTerminalNode(SymbolId symbol_id, T&& bodys);
  //通过符号ID查询对应所有节点的ID，不存在则返回空vector
  std::vector<NodeId> GetNodeIds(SymbolId symbol_id);
  //添加新的核心
  CoreId CreateNewCore() {
    CoreId core_id = cores_.size();
    cores_.emplace_back();
    return core_id;
  }
  // Items要求为std::vector<CoreItem>或std::initialize_list<CoreItem>
  // ForwardNodes要求为std::vector<NodeId>或std::initialize_list<NodeId>
  template <class Items, class ForwardNodes>
  CoreId CreateNewCore(Items&& items, ForwardNodes&& forward_nodes);
  //获取项对应内核的ID，如果不存在则插入到新项里并返回相应ID
  CoreId GetItemCoreIdOrInsert(const CoreItem& core_item);
  //添加内核项到对应内核的ID映射，如果已存在且对应core_id不等于入参则返回false
  bool AddItemToCoreIdMapping(const CoreItem& core_item, CoreId core_id);
  //向项集中添加项KPK
  CoreId AddItemToCore(const CoreItem& core_item, CoreId core_id);
  //获取核心的向前看符号集，核心不存在则返回nullptr
  const std::set<NodeId>* GetCoreFowardNodes(CoreId core_id);
  //获取核心的全部项，核心不存在则返回nullptr
  const std::set<CoreItem>* GetCoreItems(CoreId core_id);
  //向核心中添加向前看符号
  void AddCoreLookForwardNode(CoreId core_id, NodeId forward_node_id);

  //获取最终语法分析表条目
  ParsingTableEntry& GetParsingTableEntry(ParsingTableEntryId id) {
    assert(id < lexical_config_parsing_table_.size());
    return lexical_config_parsing_table_[id];
  }
  //设置根产生式ID
  void SetRootProductionId(NodeId root_production) {
    assert(manager_nodes_.GetNode(root_production) != nullptr);
    root_production_ = root_production;
  }
  //对给定核心项求闭包，结果存在原位置，自动将终结符号传播到相应项集
  //也会自动添加自发产生的终结符号
  void Closure(CoreId core_id);
  //获取从给定核心项开始，移入node_id对应的节点后到达的核心项
  //自动创建不存在的核心项，如果Goto的结果为空则返回-1
  CoreId Goto(CoreId core_id, NodeId node_id);
  //构建LALR(1)配置
  void LalrConstruct();
  //对给定节点与点的位置求闭包，并将向前看符号传播出去
  std::set<CoreItem> Closure(NodeId node_id, PointIndex point_index);

 private:
  //根产生式ID
  NodeId root_production_;
  //终结符号、非终结符号等的节点
  NodeManager<BaseNode> manager_nodes_;
  //管理符号
  UnorderedStructManager<std::string, StringHasher> manager_symbol_;
  //符号ID（manager_symbol_)返回的ID到对应节点的映射，支持一个符号对应多个节点
  std::unordered_multimap<SymbolId, NodeId> symbol_id_to_node_id_;

  //内核+非内核项集
  std::vector<Core> cores_;
  //项集到Cores_ ID的映射
  std::map<CoreItem, CoreId> item_to_core_id_;
  std::vector<ParsingTableEntry> lexical_config_parsing_table_;

  class BaseNode {
   public:
    BaseNode(NodeType type, SymbolId symbol_id)
        : base_type_(type), base_symbol_id_(symbol_id) {}
    BaseNode(const BaseNode&) = delete;
    BaseNode& operator=(const BaseNode&) = delete;

    void SetType(NodeType type) { base_type_ = type; }
    NodeType GetType() { return base_type_; }
    void SetId(NodeId id) { base_id_ = id; }
    NodeId GetId() { return base_id_; }
    void SetSymbolId(SymbolId id) { base_symbol_id_ = id; }
    SymbolId GetSymbolId() { return base_symbol_id_; }

   private:
    NodeType base_type_;
    NodeId base_id_;
    SymbolId base_symbol_id_;
  };

  class TerminalNode : public BaseNode {
   public:
    TerminalNode(NodeType type, SymbolId symbol_id)
        : BaseNode(type, symbol_id) {}
    TerminalNode(const TerminalNode&) = delete;
    TerminalNode&operator=(const TerminalNode&) = delete;
  };
  class OperatorNode : public TerminalNode {
    OperatorNode(NodeType type, SymbolId symbol_id,
                 AssociatityType associatity_type, PriorityLevel priority_level)
        : TerminalNode(type, symbol_id),
          operator_associatity_type_(associatity_type),
          operator_priority_level_(priority_level) {}
    OperatorNode(const OperatorNode&) = delete;
    OperatorNode& operator=(const OperatorNode&) = delete;

    void SetAssociatityType(AssociatityType type) {
      operator_associatity_type_ = type;
    }
    AssociatityType GetAssociatityType() { return operator_associatity_type_; }
    void SetPriorityLevel(PriorityLevel level) {
      operator_priority_level_ = level;
    }
    PriorityLevel GetPriorityLevel() { return operator_priority_level_; }

   private:
     template<class T>
    friend class NodeManager;
    AssociatityType operator_associatity_type_;
    PriorityLevel operator_priority_level_;
  };

  class NonTerminalNode : public BaseNode {
   public:
    using BodyId = size_t;
    NonTerminalNode(NodeType type, SymbolId symbol_id)
        : BaseNode(type, symbol_id) {}
    template <class T>
    NonTerminalNode(NodeType type, SymbolId symbol_id, T&& body)
        : BaseNode(type, symbol_id),
          nonterminal_bodys_(std::forward<T>(body)) {}
    NonTerminalNode(const NonTerminalNode&) = delete;
    NonTerminalNode& operator=(const NonTerminalNode&) = delete;

    template <class T>
    BodyId AddBody(T&& body);
    std::vector<NodeId> GetBody(BodyId body_id) {
      return nonterminal_bodys_[body_id];
    }
    void SetItemCoreId(BodyId body_id, PointIndex point_index, CoreId core_id) {
      nonterminal_item_to_coreid_[body_id][point_index] = core_id;
    }
    CoreId GetItemCoreId(BodyId body_id, PointIndex point_index,
                         CoreId core_id) {
      return nonterminal_item_to_coreid_[body_id][point_index];
    }

   private:
    //外层vector存该非终结符号下各个产生式的体
    //内层vector存该产生式的体里含有的各个符号对应的节点ID
    std::vector<std::vector<NodeId>> nonterminal_bodys_;
    //存放不同产生式，不同point位置对应的CoreId
    std::vector<std::vector<CoreId>> nonterminal_item_to_coreid_;
  };
  //项集与向前看符号
  class Core {
   public:
    Core() {}
    template <class Items, class ForwardNodes>
    Core(Items&& items, ForwardNodes&& forward_nodes)
        : items_(std::forward<Items>(items)),
          forward_nodes_(std::forward<ForwardNodes>(forward_nodes)) {}
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;
    Core(Core&& core)
        : items_(std::move(core.items_)),
          forward_nodes_(std::move(core.forward_nodes_)) {}

    void AddItem(const CoreItem& item) { items_.insert(item); }
    const std::set<CoreItem>& GetItems() { return items_; }
    void AddForwardNode(NodeId node_id) { forward_nodes_.insert(node_id); }
    const std::set<NodeId>& GetForwardNodes() { return forward_nodes_; }

   private:
    //项集
    std::set<CoreItem> items_;
    //向前看符号
    std::set<NodeId> forward_nodes_;
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
        std::variant<ProductionBodyId, NodeId> target_id) {
      action_and_target_production_body[index] =
          std::make_pair(action_type, target_id);
    }
    void SetNonTerminalNodeTransformId(size_t index, ProductionBodyId id) {
      nonterminal_node_transform_table[index] = id;
    }
    //访问该条目下给定ID终结节点的行为与目标ID
    std::pair<ActionType, std::variant<ProductionBodyId, NodeId>>
    AtTerminalNode(size_t index) {
      return action_and_target_production_body[index];
    }
    //访问该条目下给定ID非终结节点移入时转移到的条目ID
    ProductionBodyId AtNonTerminalNode(size_t index) {
      return nonterminal_node_transform_table[index];
    }

   private:
    //向前看符号ID下的操作和目标节点
    //操作为移入时variant存移入后转移到的ID（ProductionBodyId）
    //操作为规约时variant存使用的产生式体的ID（NodeId）
    //操作为接受和报错时variant未定义
    std::vector<std::pair<ActionType, std::variant<ProductionBodyId, NodeId>>>
        action_and_target_production_body;
    //移入非终结节点后转移到的产生式体序号
    std::vector<ProductionBodyId> nonterminal_node_transform_table;
  };
};

template <class T>
inline LexicalGenerator::NonTerminalNode::BodyId
LexicalGenerator::NonTerminalNode::AddBody(T&& body) {
  size_t body_id = nonterminal_bodys_.size();
  //将输入插入到产生式体向量中，无删除相同产生式功能
  nonterminal_bodys_.emplace_back(std::forward<T>(body));
  size_t point_indexs = body.size() + 1;
  //留出存储产生式不同点位置对应核心ID的空间
  nonterminal_item_to_coreid_.emplace_back();
  nonterminal_item_to_coreid_.back().resize(point_indexs);
  return body_id;
}

template <class T>
inline LexicalGenerator::NodeId LexicalGenerator::AddNonTerminalNode(SymbolId symbol_id,
                                                   T&& bodys) {
  NodeId node_id = manager_nodes_.EmplaceNode<NonTerminalNode>(
      NodeType::kNonTerminalNode, symbol_id, std::forward<T>(bodys));
  manager_nodes_.GetNode(node_id)->SetId(node_id);
  return NodeId(node_id);
}

template <class Items, class ForwardNodes>
inline LexicalGenerator::CoreId LexicalGenerator::CreateNewCore(Items&& items,
                                              ForwardNodes&& forward_nodes) {
  CoreId core_id= cores_.size();
  cores_.emplace_back(std::forward<Items>(items),
                      std::forward<ForwardNodes>(forward_nodes));
  return core_id;
}

}  // namespace frontend::generator::lexicalgenerator
#endif  // !GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_