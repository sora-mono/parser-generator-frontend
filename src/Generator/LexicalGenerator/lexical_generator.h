#include <any>
#include <array>
#include <cassert>
#include <functional>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/object_manager.h"
#include "Common/unordered_struct_manager.h"
#include "Generator/DfaGenerator/dfa_generator.h"

#ifndef GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_
#define GENERATOR_LEXICALGENERATOR_LEXICALGENERATOR_H_

namespace frontend::parser::lexicalmachine {
class LexicalMachine;
}

// TODO 添加删除未使用产生式的功能
namespace frontend::generator::lexicalgenerator {
using frontend::common::ObjectManager;
using frontend::common::UnorderedStructManager;

struct StringHasher {
  frontend::common::StringHashType DoHash(const std::string& str) {
    return frontend::common::HashString(str);
  }
};

// 接口类，所有用户定义函数均从该类派生
class ProcessFunctionInterface {
 public:
#ifdef USE_USER_DATA
  // 存储用户自定义数据
  using UserData = std::any;
#endif  // USE_USER_DATA

#ifdef USE_INIT_FUNCTION
  virtual void Init() = 0;
#endif  // USE_INIT_FUNCTION

#ifdef USE_SHIFT_FUNCTION
#ifdef USE_USER_DATA
  virtual UserData Shift(size_t index) = 0;
#endif  // USE_USER_DATA
#else
  virtual void Shift(size_t index) = 0;
#endif  // USE_SHIFT_FUNCTION

#ifdef USE_REDUCT_FUNCTION
#ifdef USE_USER_DATA
  // 参数标号越低入栈时间越晚
  virtual void Reduct(std::vector<UserData>&& user_data) = 0;
#else
  virtual void Reduct() = 0;
#endif  // USE_USER_DATA
#endif  // USE_REDUCT_FUNCTION
};

class LexicalGenerator {
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
  using CoreId = frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                                     WrapperLabel::kCoreId>;

#ifdef USE_AMBIGUOUS_GRAMMAR
  // 运算符优先级，数字越大优先级越高，仅对运算符节点有效
  // 与TailNodePriority意义不同，该优先级影响语法分析过程
  // 当遇到连续的运算符时决定移入还是归并
  using PriorityLevel =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kPriorityLevel>;
#endif  // USE_AMBIGUOUS_GRAMMAR

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
  // 包装用户自定义函数和数据的类的已分配对象ID
  using ProcessFunctionClassId = frontend::common::ProcessFunctionClassId;
  // 管理包装用户自定义函数和数据的类的已分配对象的容器
  using ProcessFunctionClassManagerType =
      ObjectManager<ProcessFunctionInterface>;
  // 运算符结合类型：左结合，右结合
  using AssociatityType = frontend::common::AssociatityType;
  // 分析动作类型：规约，移入，报错，接受
  enum class ActionType { kReduction, kShift, kError, kAccept };

#ifdef USE_AMBIGUOUS_GRAMMAR
  // 运算符数据
  struct OperatorData {
    std::string operator_symbol;
    std::string priority;
    std::string associatity_type;
#ifdef USE_INIT_FUNCTION
    std::string init_function;
#endif  // USE_INIT_FUNCTION
#ifdef USE_SHIFT_FUNCTION
    std::vector<std::string> shift_functions;
#endif  // USE_SHIFT_FUNCTION
#ifdef USE_REDUCT_FUNCTION
    std::string reduct_function;
#endif  // USE_REDUCT_FUNCTION
#ifdef USE_USER_DEFINED_FILE
    std::vector<std::string> include_files;
#endif  // USE_USER_DEFINED_FILE
  };
#endif  // USE_AMBIGUOUS_GRAMMAR

  struct NonTerminalNodeData {
    std::string node_symbol;
    // 保存的string两侧没有双引号
    // bool表示是否为终结节点体
    std::vector<std::pair<std::string, bool>> body_symbols;

#ifdef USE_INIT_FUNCTION
    std::string init_function;
#endif  // USE_INIT_FUNCTION
#ifdef USE_SHIFT_FUNCTION
    std::vector<std::string> shift_functions;
#endif  // USE_SHIFT_FUNCTION
#ifdef USE_REDUCT_FUNCTION
    std::string reduct_function;
#endif  // USE_REDUCT_FUNCTION
#ifdef USE_USER_DEFINED_FILE
    std::vector<std::string> include_files;
#endif  // USE_USER_DEFINED_FILE

    // 该产生式节点是否只代表多个不同的终结产生式
    // 例：Example -> "int"|"char"|"double"
    // 这些终结产生式将共用处理函数
    bool use_same_process_function_class = false;
  };

 public:
  LexicalGenerator() {
    OpenConfigFile();
    ClearNodeNum();
  }
  ~LexicalGenerator() { CloseConfigFile(); }

  // 解析关键字
  void AnalysisKeyWord(const std::string& str);
  // 解析终结节点产生式文件
  void AnalysisProductionConfig(const std::string& file_name);
  // 解析终结节点产生式，仅需且必须输入一个完整的产生式
  // 普通终结节点默认优先级为0（最低）
  void AnalysisTerminalProduction(const std::string& str, size_t priority = 0);
  // 解析运算符产生式，仅需且必须输入一个完整的产生式
  void AnalysisOperatorProduction(const std::string& str);
  // 解析非终结节点产生式，仅需且必须输入一个完整的产生式
  void AnalysisNonTerminalProduction(const std::string& str);
  // 构建LALR所需的各种外围信息
  void ConfigConstruct();
  // 构建LALR(1)配置
  void ParsingTableConstruct();

 private:
  FILE*& GetConfigConstructFilePointer() { return config_construct_code_file_; }
  FILE*& GetProcessFunctionClassFilePointer() {
    return process_function_class_file_;
  }
  // 打开配置文件
  void OpenConfigFile();
  // 关闭配置文件
  void CloseConfigFile();
  // 获取一个节点编号
  int GetNodeNum() { return node_num_++; }
  // 复位节点编号
  void ClearNodeNum() { node_num_ = 0; }
  // 将字符串视作包含多个终结符号体，并按顺序提取出来
  // 每组中的bool代表是否为终结节点体，根据两侧是否有"来判断
  // 返回的string中没有双引号
  std::vector<std::pair<std::string, bool>> GetBodySymbol(
      const std::string& str);
  // 将字符串视作包含多个函数名，并按顺序提取出来
  std::vector<std::string> GetFunctionsName(const std::string& str);
  // 将字符串视作包含多个文件名，并按顺序提取出来
  // 遇到@时中止提取
  std::vector<std::string> GetFilesName(const std::string& str);
  // 向配置文件中写入生成关键字的操作
  void PrintKeyWordConstructData(const std::string& keyword);
  // 向配置文件中写入生成终结节点的操作
  void PrintTerminalNodeConstructData(std::string&& node_symbol,
                                      std::string&& body_symbol,
                                      size_t priority);
  // 子过程，向配置文件写入init_function, shift_function和reduct_function
  template <class T>
  void PrintProcessFunction(FILE* function_file, const T& data);
#ifdef USE_AMBIGUOUS_GRAMMAR
  // 向配置文件中写入生成运算符的操作
  void PrintOperatorNodeConstructData(OperatorData&& data);
#endif  // USE_AMBIGUOUS_GRAMMAR

  // 向配置文件中写入生成非终结节点的操作
  void PrintNonTerminalNodeConstructData(NonTerminalNodeData&& data);
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
  SymbolId GetNodeSymbolId(const std::string& node_symbol) {
    assert(node_symbol.size() != 0);
    return manager_node_symbol_.GetObjectId(node_symbol);
  }
  // 获取产生式体符号对应的ID，不存在则返回SymbolId::InvalidId()
  SymbolId GetBodySymbolId(const std::string& body_symbol) {
    assert(body_symbol.size() != 0);
    return manager_terminal_body_symbol_.GetObjectId(body_symbol);
  }
  // 通过产生式名ID查询对应字符串
  const std::string& GetNodeSymbolStringFromId(SymbolId node_symbol_id) {
    assert(node_symbol_id.IsValid());
    return manager_node_symbol_.GetObject(node_symbol_id);
  }
  // 通过产生式体符号ID查询对应的字符串
  const std::string& GetBodySymbolStringFromId(SymbolId body_symbol_id) {
    assert(body_symbol_id.IsValid());
    return manager_terminal_body_symbol_.GetObject(body_symbol_id);
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
  ProductionNodeId GetProductionNodeIdFromNodeSymbolId(
      SymbolId node_symbol_id) {
    auto iter = node_symbol_id_to_node_id_.find(node_symbol_id);
    assert(iter != node_symbol_id_to_node_id_.end());
    return iter->second;
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
  // 获取产生式体的产生式节点ID
  ProductionNodeId GetProductionNodeIdInBody(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index) {
    return GetProductionNode(production_node_id)
        .GetProductionNodeInBody(production_body_id, point_index);
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
    return GetProductionNode(production_node_id)
        .GetBodyProcessFunctionClassId(production_body_id);
  }
  // 添加因为产生式体未定义而导致不能继续添加的非终结节点
  // 第一个参数为未定义的产生式名
  // 其余三个参数同AddNonTerminalNode
  // 函数会复制一份副本，无需保持原来的参数的生命周期
  void AddUnableContinueNonTerminalNode(
      const std::string& undefined_symbol, const std::string& node_symbol,
      const std::vector<std::pair<std::string, bool>>& subnode_symbols,
      ProcessFunctionClassId class_id);
  // 检查给定的节点生成后是否可以重启因为部分产生式体未定义而搁置的
  // 非终结产生式添加过程
  void CheckNonTerminalNodeCanContinue(const std::string& node_symbol);
  // 检查是否还有未定义的产生式体
  // 用于完成所有产生式添加后检查
  // 在stderr输出错误信息（如果有）
  void CheckUndefinedProductionRemained();
  // 添加关键字，自动创建同名终结节点
  void AddKeyWord(const std::string& key_word);
  // 新建终结节点，返回节点ID
  // 节点已存在且给定symbol_id不同于已有ID则返回ProductionNodeId::InvalidId()
  // 第三个参数是节点优先级，0保留为普通词的优先级，1保留为关键字的优先级
  // 其余的优先级可以供定义操作符用或其它用途
  ProductionNodeId AddTerminalNode(
      const std::string& node_symbol, const std::string& body_symbol,
      frontend::generator::dfa_generator::DfaGenerator::TailNodePriority
          node_priority);
  // 子过程，仅用于创建节点
  // 自动更新节点名ID到节点ID的映射
  // 自动更新节点体ID到节点ID的映射
  // 自动为节点类设置节点ID
  ProductionNodeId SubAddTerminalNode(SymbolId node_symbol_id,
                                      SymbolId body_symbol_id);

#ifdef USE_AMBIGUOUS_GRAMMAR
  // 新建运算符节点，返回节点ID，节点已存在则返回ProductionNodeId::InvalidId()
  // 拆成模板函数和非模板函数为了降低代码生成量，阻止代码膨胀
  // 下面两个函数均可直接调用，class_id是包装用户定义函数和数据的类的对象ID
  // 默认添加的运算符词法分析优先级等于运算符优先级
  template <class ProcessFunctionClass>
  ProductionNodeId AddOperatorNode(const std::string& operator_symbol,
                                   AssociatityType associatity_type,
                                   PriorityLevel priority_level);
  ProductionNodeId AddOperatorNode(const std::string& operator_symbol,
                                   AssociatityType associatity_type,
                                   PriorityLevel priority_level,
                                   ProcessFunctionClassId class_id);
  // 子过程，仅用于创建节点
  // 运算符节点名同运算符名
  // 自动更新节点名ID到节点ID的映射表
  // 自动更新节点体ID到节点ID的映射
  // 自动为节点类设置节点ID
  ProductionNodeId SubAddOperatorNode(SymbolId node_symbol_id,
                                      AssociatityType associatity_type,
                                      PriorityLevel priority_level,
                                      ProcessFunctionClassId class_id);
#endif  // USE_AMBIGUOUS_GRAMMAR

  // 新建非终结节点，返回节点ID，节点已存在则不会创建新的节点
  // node_symbol为产生式名，subnode_symbols是产生式体
  // class_id是已添加的包装用户自定义函数和数据的类的对象ID
  // 拆成模板函数和非模板函数为了降低代码生成量，阻止代码膨胀
  // 下面两个函数均可直接调用，class_id是包装用户定义函数和数据的类的对象ID
  template <class ProcessFunctionClass>
  ProductionNodeId AddNonTerminalNode(
      const std::string& node_symbol,
      const std::vector<std::string>& subnode_symbols);
  ProductionNodeId AddNonTerminalNode(
      const std::string& node_symbol,
      const std::vector<std::pair<std::string, bool>>& subnode_symbols,
      ProcessFunctionClassId class_id);
  // 子过程，仅用于创建节点
  // 自动更新节点名ID到节点ID的映射表
  // 自动为节点类设置节点ID
  ProductionNodeId SubAddNonTerminalNode(SymbolId symbol_id);
  // 新建文件尾节点，返回节点ID
  ProductionNodeId AddEndNode(SymbolId symbol_id = SymbolId::InvalidId());

  // 获取节点
  BaseProductionNode& GetProductionNode(ProductionNodeId id) {
    return manager_nodes_[id];
  }
  BaseProductionNode& GetProductionNodeFromNodeSymbolId(SymbolId id) {
    ProductionNodeId production_node_id =
        GetProductionNodeIdFromNodeSymbolId(id);
    assert(production_node_id.IsValid());
    return GetProductionNode(production_node_id);
  }
  BaseProductionNode& GetProductionNodeBodySymbolId(SymbolId id) {
    ProductionNodeId production_node_id =
        GetProductionNodeIdFromBodySymbolId(id);
    assert(production_node_id.IsValid());
    return GetProductionNode(production_node_id);
  }
  // 给非终结节点添加产生式体
  template <class IdType>
  void AddNonTerminalNodeBody(ProductionNodeId node_id, IdType&& body) {
    static_cast<NonTerminalProductionNode*>(GetProductionNode(node_id))
        ->AddBody(std::forward<IdType>(body));
  }
  // 通过符号ID查询对应节点的ID，需要保证ID有效
  ProductionNodeId GetProductionNodeIdFromNodeSymbol(SymbolId symbol_id);
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
  // First的子过程，提取一个非终结节点中所有的first符号
  // 第二个参数用来存储已经处理过的节点，防止无限递归，初次调用应传入空集合
  // 如果输入ProductionNodeId::InvalidId()则返回空集合
  std::unordered_set<ProductionNodeId> GetNonTerminalNodeFirstNodes(
      ProductionNodeId production_node_id,
      std::unordered_set<ProductionNodeId>&& processed_nodes =
          std::unordered_set<ProductionNodeId>());
  // 闭包操作中的first操作，前三个参数标志β的位置
  // 采用三个参数因为终结节点节点可能归并为空节点，需要向下查找终结节点
  // 向前看节点可以规约为空节点则添加相应向前看符号
  // 然后继续向前查找直到结尾或不可空规约非终结节点或终结节点
  std::unordered_set<ProductionNodeId> First(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index,
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
  // 仅应由CoreClosure调用
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
  ProductionNodeId GetProductionBodyNextShiftNodeId(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index);
  // 获取该项的向前看节点ID，如果不存在则返回ProductionNodeId::InvalidId()
  ProductionNodeId GetProductionBodyNextNextNodeId(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index);
  // Goto的子过程，对单个项操作，返回ItemGoto后的ID和是否插入
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
  // 设置项集到语法分析表条目ID的映射
  void SetCoreIdToParsingEntryIdMapping(CoreId core_id,
                                        ParsingTableEntryId entry_id) {
    core_id_to_parsing_table_entry_id_[core_id] = entry_id;
  }
  // 获取已存储的项到语法分析表条目ID的映射
  // 不存在则返回ParsingTableEntryId::InvalidId()
  ParsingTableEntryId GetParsingEntryIdCoreId(CoreId core_id);
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

  // 声明语法分析机为友类，便于其使用各种定义
  friend class frontend::parser::lexicalmachine::LexicalMachine;

  // 终结节点产生式定义
  static const std::regex terminal_node_regex_;
  // 运算符定义
  static const std::regex operator_node_regex_;
  // 非终结节点产生式定义
  static const std::regex nonterminal_node_regex_;
  // 单个关键字定义
  static const std::regex keyword_regex_;
  // 单个终结符号定义
  static const std::regex body_symbol_regex_;
  // 单个函数名定义
  static const std::regex function_regex_;
  // 包含的单个用户文件定义
  static const std::regex file_regex_;

  // Generator生成的包装用户定义函数数据的类的源码文件
  FILE* process_function_class_file_ = nullptr;
  // 存储Generator生成的配置生成代码文件
  FILE* config_construct_code_file_ = nullptr;
  // 对节点编号，便于区分不同节点对应的类
  // 该值仅用于生成配置代码中生成包装用户定义函数数据的类
  int node_num_;
  // 存储引用的未定义产生式
  // key是未定义的产生式名
  // tuple内的std::string是非终结产生式名
  // const std::vector<std::pair<std::string, bool>>*存储已有的产生式体信息
  // ProcessFunctionClassId是给定的包装用户定义函数数据的类的对象ID
  std::unordered_multimap<
      std::string,
      std::tuple<std::string, std::vector<std::pair<std::string, bool>>,
                 ProcessFunctionClassId>>
      undefined_productions_;
  // 根产生式ID
  ProductionNodeId root_production_;
  // 管理终结符号、非终结符号等的节点
  ObjectManager<BaseProductionNode> manager_nodes_;
  // 管理产生式名的符号
  UnorderedStructManager<std::string, StringHasher> manager_node_symbol_;
  // 管理终结节点产生式体的符号
  UnorderedStructManager<std::string, StringHasher>
      manager_terminal_body_symbol_;
  // 产生式名ID到对应产生式节点的映射
  std::unordered_map<SymbolId, ProductionNodeId> node_symbol_id_to_node_id_;
  // 产生式体符号ID到对应节点ID的映射
  std::unordered_map<SymbolId, ProductionNodeId>
      production_body_symbol_id_to_node_id_;
  // 项集到语法分析表条目ID的映射
  std::map<CoreId, ParsingTableEntryId> core_id_to_parsing_table_entry_id_;
  // 内核+非内核项集
  std::vector<Core> cores_;
  // Goto缓存，存储已有的Core在不同条件下Goto的节点
  // 使用CoreId作为键值方便删除时查找该CoreId下的所有cache
  std::multimap<CoreId, std::pair<ProductionNodeId, CoreId>>
      core_id_goto_core_id_;
  // DFA配置生成器，配置写入文件
  frontend::generator::dfa_generator::DfaGenerator dfa_generator_;
  // 语法分析表，配置写入文件
  ParsingTableType lexical_config_parsing_table_;
  // 存储用户自定义函数和数据，配置写入文件
  ProcessFunctionClassManagerType manager_process_function_class_;

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
    SymbolId GetNodeSymbolId() const { return base_symbol_id_; }

    // 获取给定项对应的产生式ID，返回point_index后面的ID
    // point_index越界时返回ProducNodeId::InvalidId()
    // 为了支持向前看多个节点允许越界
    // 返回点右边的产生式ID，不存在则返回ProductionNodeId::InvalidId()
    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) = 0;
    //添加向前看节点
    virtual void AddForwardNodeId(ProductionBodyId production_body_id,
                                  PointIndex point_index,
                                  ProductionNodeId forward_node_id) = 0;
    //获取向前看节点集合
    virtual const std::unordered_set<ProductionNodeId>& GetForwardNodeIds(
        ProductionBodyId production_body_id, PointIndex point_index) = 0;
    //设置项对应的核心ID
    virtual void SetCoreId(ProductionBodyId production_body_id,
                           PointIndex point_index, CoreId core_id) = 0;
    //获取项对应的核心ID
    virtual CoreId GetCoreId(ProductionBodyId production_body_id,
                             PointIndex point_index) = 0;
    // 获取包装用户自定义函数数据的类的对象ID
    virtual ProcessFunctionClassId GetBodyProcessFunctionClassId(
        ProductionBodyId production_body_id) = 0;

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
          forward_nodes_(std::move(terminal_production_node.forward_nodes_)),
          core_ids_(std::move(terminal_production_node.core_ids_)),
          body_symbol_id_(std::move(terminal_production_node.body_symbol_id_)) {
    }
    TerminalProductionNode& operator=(
        TerminalProductionNode&& terminal_production_node) {
      BaseProductionNode::operator=(std::move(terminal_production_node));
      forward_nodes_ = std::move(terminal_production_node.forward_nodes_);
      core_ids_ = std::move(terminal_production_node.core_ids_);
      body_symbol_id_ = std::move(terminal_production_node.body_symbol_id_);
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
    // 获取/设置产生式体名
    SymbolId GetBodySymbolId() { return body_symbol_id_; }
    void SetBodySymbolId(SymbolId body_symbol_id) {
      body_symbol_id_ = body_symbol_id;
    }
    // 在越界时也有返回值为了支持获取下一个/下下个体内节点的操作
    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) override;
    // 添加向前看节点
    virtual void AddForwardNodeId(ProductionBodyId production_body_id,
                                  PointIndex point_index,
                                  ProductionNodeId forward_node_id);
    // 获取向前看节点集合
    virtual const std::unordered_set<ProductionNodeId>& GetForwardNodeIds(
        ProductionBodyId production_body_id, PointIndex point_index);
    // 设置项对应的核心ID
    virtual void SetCoreId(ProductionBodyId production_body_id,
                           PointIndex point_index, CoreId core_id);
    // 获取项对应的核心ID
    virtual CoreId GetCoreId(ProductionBodyId production_body_id,
                             PointIndex point_index);
    virtual ProcessFunctionClassId GetBodyProcessFunctionClassId(
      ProductionBodyId body_id) {
      assert(false);
      return ProcessFunctionClassId::InvalidId();
    }

   private:
    // 终结节点的两种项的向前看符号
    std::pair<std::unordered_set<ProductionNodeId>,
              std::unordered_set<ProductionNodeId>>
        forward_nodes_;
    // 两种项对应的核心ID
    std::pair<CoreId, CoreId> core_ids_;
    // 产生式体名
    SymbolId body_symbol_id_;
  };

#ifdef USE_AMBIGUOUS_GRAMMAR
  class OperatorProductionNode : public TerminalProductionNode {
   public:
    OperatorProductionNode(SymbolId node_symbol_id, SymbolId body_symbol_id,
                           AssociatityType associatity_type,
                           PriorityLevel priority_level,
                           ProcessFunctionClassId process_function_class_id)
        : TerminalProductionNode(node_symbol_id, body_symbol_id),
          operator_associatity_type_(associatity_type),
          operator_priority_level_(priority_level),
          process_function_class_id_(process_function_class_id) {}
    OperatorProductionNode(const OperatorProductionNode&) = delete;
    OperatorProductionNode& operator=(const OperatorProductionNode&) = delete;
    OperatorProductionNode(OperatorProductionNode&& operator_production_node)
        : TerminalProductionNode(std::move(operator_production_node)),
          operator_associatity_type_(
              std::move(operator_production_node.operator_associatity_type_)),
          operator_priority_level_(
              std::move(operator_production_node.operator_priority_level_)),
          process_function_class_id_(
              std::move(operator_production_node.process_function_class_id_)) {}
    OperatorProductionNode& operator=(
        OperatorProductionNode&& operator_production_node);

    struct NodeData : public TerminalProductionNode::NodeData {
      std::string symbol;
    };
    void SetProcessFunctionClassId(ProcessFunctionClassId class_id) {
      process_function_class_id_ = class_id;
    }
    ProcessFunctionClassId GetProcessFunctionClassId() {
      return process_function_class_id_;
    }
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

    virtual ProcessFunctionClassId GetBodyProcessFunctionClassId(
        ProductionBodyId production_body_id) {
      assert(production_body_id == 0);
      return GetProcessFunctionClassId();
    }

   private:
    // 运算符结合性
    AssociatityType operator_associatity_type_;
    // 运算符优先级
    PriorityLevel operator_priority_level_;
    // 用户定义处理函数类的ID
    ProcessFunctionClassId process_function_class_id_;
  };
#endif  // USE_AMBIGUOUS_GRAMMAR

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
    NonTerminalProductionNode(NonTerminalProductionNode&& node)
        : BaseProductionNode(std::move(node)),
          nonterminal_bodys_(std::move(node.nonterminal_bodys_)),
          forward_nodes_(std::move(node.forward_nodes_)),
          core_ids_(std::move(node.core_ids_)),
          process_function_class_ids_(
              std::move(node.process_function_class_ids_)),
          empty_body_(std::move(node.empty_body_)) {}
    NonTerminalProductionNode& operator=(NonTerminalProductionNode&& node) {
      BaseProductionNode::operator=(std::move(node));
      nonterminal_bodys_ = std::move(node.nonterminal_bodys_);
      forward_nodes_ = std::move(node.forward_nodes_);
      core_ids_ = std::move(node.core_ids_);
      process_function_class_ids_ = std::move(node.process_function_class_ids_);
      empty_body_ = std::move(node.empty_body_);
      return *this;
    }

    // 重新设置产生式体数目，会同时设置相关数据
    // 输入新的所有产生式体的数目，不要+1
    void ResizeProductionBodyNum(size_t new_size);
    // 重新设置产生式体里节点数目，会同时设置相关数据
    // 输入新的产生式体内所有节点的数目，不要+1
    void ResizeProductionBodyNodeNum(ProductionBodyId production_body_id,
                                     size_t new_size);
    // 添加一个产生式体，要求IdType为一个vector，按序存储产生式节点ID
    template <class IdType>
    ProductionBodyId AddBody(IdType&& body);
    // 设置给定产生式体ID对应的ProcessFunctionClass的ID
    void SetBodyProcessFunctionClassId(ProductionBodyId body_id,
                                       ProcessFunctionClassId class_id) {
      assert(body_id < process_function_class_ids_.size());
      process_function_class_ids_[body_id] = class_id;
    }
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
    // 设置该产生式不可以为空
    void SetProductionShouldNotEmpty() { empty_body_ = false; }
    void SetProductionCouldBeEmpty() { empty_body_ = true; }
    // 查询该产生式是否可以为空
    bool CouldBeEmpty() { return empty_body_; }

    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) override;
    // 添加向前看节点
    virtual void AddForwardNodeId(ProductionBodyId production_body_id,
                                  PointIndex point_index,
                                  ProductionNodeId forward_node_id) {
      assert(point_index < forward_nodes_[production_body_id].size() &&
             forward_node_id.IsValid());
      forward_nodes_[production_body_id][point_index].insert(forward_node_id);
    }
    // 获取向前看节点集合
    virtual const std::unordered_set<ProductionNodeId>& GetForwardNodeIds(
        ProductionBodyId production_body_id, PointIndex point_index) {
      assert(point_index < forward_nodes_[production_body_id].size());
      return forward_nodes_[production_body_id][point_index];
    }
    // 设置项对应的核心ID
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
    // 返回给定产生式体ID对应的ProcessFunctionClass的ID
    virtual ProcessFunctionClassId GetBodyProcessFunctionClassId(
        ProductionBodyId body_id) {
      assert(body_id < process_function_class_ids_.size());
      return process_function_class_ids_[body_id];
    }

   private:
    // 外层vector存该非终结符号下各个产生式的体
    // 内层vector存该产生式的体里含有的各个符号对应的节点ID
    BodyContainerType nonterminal_bodys_;
    // 向前看节点，分两层组成
    // 外层对应ProductionBodyId，等于产生式体数目
    // 内层对应PointIndex，大小比对应产生式体内节点数目多1
    std::vector<std::vector<std::unordered_set<ProductionNodeId>>>
        forward_nodes_;
    // 存储项对应的核心ID
    std::vector<std::vector<CoreId>> core_ids_;
    // 存储每个产生式体对应的包装用户自定义函数和数据的类的已分配对象ID
    std::vector<ProcessFunctionClassId> process_function_class_ids_;
    // 标志该产生式是否可能为空
    bool empty_body_ = false;
  };

  //// 文件尾节点
  //class EndNode : public BaseProductionNode {
  // public:
  //  EndNode(SymbolId symbol_id)
  //      : BaseProductionNode(ProductionNodeType::kEndNode, symbol_id) {}
  //  EndNode(const EndNode&) = delete;
  //  EndNode& operator=(const EndNode&) = delete;
  //  EndNode(EndNode&& end_node) : BaseProductionNode(std::move(end_node)) {}
  //  EndNode& operator=(EndNode&& end_node) {
  //    BaseProductionNode::operator=(std::move(end_node));
  //    return *this;
  //  }
  //};
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
    // 动作为规约时存储包装调用函数的类的对象的ID和规约后得到的非终结产生式ID
    // 产生式ID用于确定如何在规约后产生的非终结产生式条件下转移
    // 动作为移入时存储移入后转移到的条目ID
    using TerminalNodeActionAndTargetContainerType = std::unordered_map<
        ProductionNodeId,
        std::pair<ActionType, std::variant<std::pair<ProductionNodeId,
                                                     ProcessFunctionClassId>,
                                           ParsingTableEntryId>>>;

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
        std::variant<std::pair<ProductionNodeId, ProcessFunctionClassId>,
                     ParsingTableEntryId>
            target_id) {
      action_and_target_[node_id] =
                 std::make_pair(action_type, target_id);
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
  ProductionBodyId body_id(nonterminal_bodys_.size());
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

template <class T>
inline void LexicalGenerator::PrintProcessFunction(FILE* function_file,
                                                   const T& data) {
#ifdef USE_INIT_FUNCTION
  fprintf(function_file, "  virtual void Init() { return %s(); }\n",
          data.init_function.c_str());
#endif  // USE_INIT_FUNCTION
#ifdef USE_SHIFT_FUNCTION
#ifdef USE_USER_DATA
  fprintf(function_file, "  virtual UserData Shift(size_t index) { \n");
#else
  fprintf(function_file, "  virtual void Shift(size_t index) { \n");
#endif  // USE_USER_DATA
  fprintf(function_file, "    switch(index) {\n");
  for (int index = 0; index < data.shift_functions.size(); index++) {
    if (!data.shift_functions[index].empty()) {
      // 当前的移入函数不是空函数则打印case
      fprintf(function_file, "      case %d:\n", index);
      fprintf(function_file, "      return %s();\n",
              data.shift_functions[index].c_str());
    }
  }
  fprintf(function_file, "      default:\n      break;\n    }\n  }");
#endif  // USE_SHIFT_FUNCTION
#ifdef USE_REDUCT_FUNCTION
#ifdef USE_USER_DATA
  if (!data.reduct_function.empty()) {
    fprintf(function_file,
            " virtual void Reduct(std::vector<UserData>&& user_data) { return "
            "%s(std::move(user_data)); }\n",
            data.reduct_function.c_str());
  }
#else
  if (!data.reduct_function.empty()) {
    fprintf(function_file, " virtual void Reduct() { return %s(); }\n",
            data.reduct_function.c_str());
  }
#endif  // USE_USER_DATA
#endif  // USE_REDUCT_FUNCTION
}

#ifdef USE_AMBIGUOUS_GRAMMAR
template <class ProcessFunctionClass>
inline LexicalGenerator::ProductionNodeId LexicalGenerator::AddOperatorNode(
    const std::string& operator_symbol, AssociatityType associatity_type,
    PriorityLevel priority_level) {
  ProcessFunctionClassId class_id =
      CreateProcessFunctionClassObject<ProcessFunctionClass>();
  return AddOperatorNode(operator_symbol, associatity_type, priority_level,
                         class_id);
}
#endif  // USE_AMBIGUOUS_GRAMMAR

template <class ProcessFunctionClass>
inline LexicalGenerator::ProductionNodeId LexicalGenerator::AddNonTerminalNode(
    const std::string& node_symbol,
    const std::vector<std::string>& subnode_symbols) {
  ProcessFunctionClassId class_id =
      CreateProcessFunctionClassObject<ProcessFunctionClass>();
  return AddNonTerminalNode(node_symbol, subnode_symbols, class_id);
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