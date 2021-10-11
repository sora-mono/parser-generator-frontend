#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_H_

#include <any>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/array.hpp>
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

namespace frontend::parser::syntaxmachine {
class SyntaxMachine;
}

// TODO 添加删除未使用产生式的功能
namespace frontend::generator::syntaxgenerator {
using frontend::common::ObjectManager;
using frontend::common::UnorderedStructManager;

class SyntaxGenerator {
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
  using ProcessFunctionClassId = frontend::common::ObjectManager<
      frontend::generator::syntaxgenerator::ProcessFunctionInterface>::ObjectId;
  // 管理包装用户自定义函数和数据的类的已分配对象的容器
  using ProcessFunctionClassManagerType =
      ObjectManager<ProcessFunctionInterface>;
  // 运算符结合类型：左结合，右结合
  using OperatorAssociatityType = frontend::common::OperatorAssociatityType;
  // 分析动作类型：规约，移入，移入和规约，报错，接受
  enum class ActionType { kReduct, kShift, kShiftReduct, kError, kAccept };

  // 运算符数据
  struct OperatorData {
    std::string operator_symbol;
    std::string binary_operator_priority;
    std::string binary_operator_associatity_type;
    std::string reduct_function;
#ifdef USE_USER_DEFINED_FILE
    std::vector<std::string> include_files;
#endif  // USE_USER_DEFINED_FILE
  };

  struct NonTerminalNodeData {
    std::string node_symbol;
    // 保存的string两侧没有双引号
    // bool表示是否为终结节点体
    std::vector<std::pair<std::string, bool>> body_symbols;
    std::string reduct_function;
#ifdef USE_USER_DEFINED_FILE
    std::vector<std::string> include_files;
#endif  // USE_USER_DEFINED_FILE

    // 该产生式节点是否只代表多个不同的终结产生式
    // 例：Example -> "int"|"char"|"double"
    // 这些终结产生式将共用处理函数
    bool use_same_process_function_class = false;
  };

 public:
  SyntaxGenerator() {
    SyntaxGeneratorInit();
    ConfigConstruct();
    CheckUndefinedProductionRemained();
    dfa_generator_.DfaReconstrcut();
    ParsingTableConstruct();
    // 保存配置
    SaveConfig();
  }
  SyntaxGenerator(const SyntaxGenerator&) = delete;
  ~SyntaxGenerator() { CloseConfigFile(); }

  SyntaxGenerator& operator=(const SyntaxGenerator&) = delete;

  // 解析关键字
  void AnalysisKeyWord(const std::string& str);
  // 解析终结节点产生式文件
  void AnalysisProductionConfig(const std::string& file_name);
  // 解析终结节点产生式，仅需且必须输入一个完整的产生式
  // 普通终结节点默认优先级为0（最低）
  void AnalysisTerminalProduction(const std::string& str,
                                  size_t binary_operator_priority = 0);
  // 解析运算符产生式，仅需且必须输入一个完整的产生式
  void AnalysisOperatorProduction(const std::string& str);
  // 解析非终结节点产生式，仅需且必须输入一个完整的产生式
  void AnalysisNonTerminalProduction(const std::string& str);
  // 构建LALR所需的各种外围信息
  void ConfigConstruct();
  // 构建LALR(1)配置
  void ParsingTableConstruct();
  // 初始化
  void SyntaxGeneratorInit();

 private:
  FILE*& GetConfigConstructFilePointer() { return config_construct_code_file_; }
  FILE*& GetProcessFunctionClassFilePointer() {
    return process_function_class_file_;
  }
  FILE*& GetUserDefinedFunctionAndDataRegisterFilePointer() {
    return user_defined_function_and_data_register_file_;
  }
  // 打开配置文件
  void OpenConfigFile();
  // 关闭配置文件
  void CloseConfigFile();
  // 获取一个节点编号
  int GetNodeNum() { return node_num_++; }
  // 复位节点编号
  void NodeNumInit() { node_num_ = 0; }
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
                                      size_t binary_operator_priority);
  // 子过程，向配置文件写入Reduct函数
  // data中函数名为空则返回false
  template <class T>
  bool PrintProcessFunction(FILE* function_file, const T& data);
  // 向配置文件中写入生成运算符的操作
  // data中的优先级必须为单个字符L或R
  void PrintOperatorNodeConstructData(OperatorData&& data);

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
      PointIndex point_index) {
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
  // 在stderr输出错误信息（如果有）
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
  ProductionNodeId AddUnaryOperatorNode(
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
  BaseProductionNode& GetProductionNode(ProductionNodeId production_node_id) {
    return manager_nodes_[production_node_id];
  }
  BaseProductionNode& GetProductionNodeFromNodeSymbolId(SymbolId symbol_id) {
    ProductionNodeId production_node_id =
        GetProductionNodeIdFromNodeSymbolId(symbol_id);
    assert(symbol_id.IsValid());
    return GetProductionNode(production_node_id);
  }
  BaseProductionNode& GetProductionNodeBodyFromSymbolId(SymbolId symbol_id) {
    ProductionNodeId production_node_id =
        GetProductionNodeIdFromBodySymbolId(symbol_id);
    assert(symbol_id.IsValid());
    return GetProductionNode(production_node_id);
  }
  // 获取非终结节点中的一个产生式体
  const std::vector<ProductionNodeId>& GetProductionBody(
      ProductionNodeId production_node_id,
      ProductionBodyId production_body_id) {
    NonTerminalProductionNode& nonterminal_node =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(production_node_id));
    // 只有非终结节点才有多个产生式体，对其它节点调用该函数无意义
    assert(nonterminal_node.Type() == ProductionNodeType::kNonTerminalNode);
    return nonterminal_node.GetBody(production_body_id).production_body;
  }
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
  Core& GetCore(CoreId core_id) {
    assert(core_id < cores_.Size());
    return cores_[core_id];
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
  // 如果该项已存在则仅添加向前看符号
  template <class ForwardNodeIdContainer>
  std::pair<std::map<CoreItem, std::unordered_set<ProductionNodeId>>::iterator,
            bool>
  AddItemAndForwardNodeIdsToCore(CoreId core_id, const CoreItem& core_item,
                                 ForwardNodeIdContainer&& forward_node_ids);
  // 获取向前看符号集
  const std::unordered_set<ProductionNodeId>& GetForwardNodeIds(
      CoreId core_id, const CoreItem& core_item) {
    return GetCore(core_id).GetItemsAndForwardNodeIds().at(core_item);
  }
  // 添加向前看符号，可以只传入单个未被包装的ID
  template <class ForwardNodeIdContainer>
  void AddForwardNode(CoreId core_id, const CoreItem& core_item,
                      ForwardNodeIdContainer&& forward_node_ids) {
    GetCore(core_id).AddForwardNode(
        core_item, std::forward<ForwardNodeIdContainer>(forward_node_ids));
  }
  // First的子过程，提取一个非终结节点中所有的first符号
  // 第二个参数用来存储已经处理过的节点，防止无限递归，初次调用应传入空集合
  // 如果输入ProductionNodeId::InvalidId()则返回空集合
  std::unordered_set<ProductionNodeId> GetNonTerminalNodeFirstNodes(
      ProductionNodeId production_node_id,
      std::unordered_set<ProductionNodeId>&& processed_nodes =
          std::unordered_set<ProductionNodeId>());
  // 闭包操作中的first操作，前三个参数标志β的位置
  // 采用三个参数因为非终结节点可能规约为空节点，需要向下查找终结节点
  // 向前看节点可以规约为空节点则添加相应向前看符号
  // 然后继续向前查找直到结尾或不可空规约非终结节点或终结节点
  std::unordered_set<ProductionNodeId> First(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      PointIndex point_index,
      const InsideForwardNodesContainerType& next_node_ids =
          InsideForwardNodesContainerType());
  // 获取核心的全部项
  const std::map<CoreItem, std::unordered_set<ProductionNodeId>>&
  GetCoreItemsAndForwardNodes(CoreId core_id) {
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
  void CoreClosure(CoreId core_id);
  // 传播向前看符号，同时在传播过程中构建语法分析表
  void SpreadLookForwardSymbolAndConstructParsingTableEntry(CoreId core_id);
  // 对所有产生式节点按照ProductionNodeType分类
  // 返回array内的vector内类型对应的下标为ProductionNodeType内类型的值
  std::array<std::vector<ProductionNodeId>, sizeof(ProductionNodeType)>
  ClassifyProductionNodes();
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
    std::ofstream config_file(frontend::common::kSyntaxConfigFileName);
    boost::archive::binary_oarchive oarchive(config_file);
    dfa_generator_.SaveConfig();
    oarchive << *this;
  }

  // 声明语法分析机为友类，便于其使用各种定义
  friend class frontend::parser::syntaxmachine::SyntaxMachine;
  // 允许序列化类访问
  friend class boost::serialization::access;

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
  // boost序列化派生类时需要注册派生类具体类型
  // 该文件里储存用户定义的函数数据对象类型的声明
  FILE* user_defined_function_and_data_register_file_ = nullptr;
  // Generator生成的外围数据加载代码文件(ConfigConstruct函数的具体实现)
  FILE* config_construct_code_file_ = nullptr;
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
  // 内核+非内核项集
  ObjectManager<Core> cores_;
  // 语法分析表ID到核心ID的映射
  std::unordered_map<ParsingTableEntryId, CoreId>
      parsing_table_entry_id_to_core_id_;
  // 根产生式条目ID
  ProductionNodeId root_production_node_id_;
  // 初始语法分析表条目ID，配置写入文件
  ParsingTableEntryId root_parsing_table_entry_id_;
  // DFA配置生成器，配置写入文件
  frontend::generator::dfa_generator::DfaGenerator dfa_generator_;
  // 语法分析表，配置写入文件
  ParsingTableType syntax_parsing_table_;
  // 用户自定义函数和数据的类的对象，配置写入文件
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
        ProductionBodyId production_body_id, PointIndex point_index) = 0;

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
        ProductionBodyId production_body_id, PointIndex point_index) override;

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
        ProductionBodyId production_body_id, PointIndex point_index) {
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
      // 产生式体
      std::vector<ProductionNodeId> production_body;
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
    bool CouldBeEmptyReduct() { return could_empty_reduct_; }

    virtual ProductionNodeId GetProductionNodeInBody(
        ProductionBodyId production_body_id, PointIndex point_index) override;
    // 返回给定产生式体ID对应的ProcessFunctionClass的ID
    ProcessFunctionClassId GetBodyProcessFunctionClassId(
        ProductionBodyId body_id) {
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
        ProductionBodyId production_body_id, PointIndex point_index) {
      assert(false);
      return ProductionNodeId::InvalidId();
    }
    // 获取包装用户自定义函数数据的类的对象ID
    virtual ProcessFunctionClassId GetBodyProcessFunctionClassId(
        ProductionBodyId production_body_id) {
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
    template <class ForwardNodeIdContainer>
    std::pair<
        std::map<CoreItem, std::unordered_set<ProductionNodeId>>::iterator,
        bool>
    AddItemAndForwardNodeIds(const CoreItem& item,
                             ForwardNodeIdContainer&& forward_node_ids);
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
    // 设置该项集已求的闭包无效，应由每个修改了core_items_的函数调用
    void SetClosureNotAvailable() { core_closure_available_ = false; }
    // 获取全部项和对应的向前看节点
    const std::map<CoreItem, std::unordered_set<ProductionNodeId>>&
    GetItemsAndForwardNodeIds() const {
      return item_and_forward_node_ids_;
    }
    // 设置项对应的语法分析表条目ID
    void SetParsingTableEntryId(ParsingTableEntryId parsing_table_entry_id) {
      parsing_table_entry_id_ = parsing_table_entry_id;
    }
    // 获取项对应的语法分析表条目ID
    ParsingTableEntryId GetParsingTableEntryId() const {
      return parsing_table_entry_id_;
    }
    // 向给定项中添加向前看符号，对单个节点特化
    void AddForwardNode(
        const std::map<CoreItem,
                       std::unordered_set<ProductionNodeId>>::iterator& iter,
        ProductionNodeId forward_node_id) {
      iter->second.insert(forward_node_id);
    }
    // 向给定项中添加向前看符号，对容器特化
    template <class ForwardNodeIdContainer>
    void AddForwardNode(
        const std::map<CoreItem,
                       std::unordered_set<ProductionNodeId>>::iterator& iter,
        const ForwardNodeIdContainer& forward_node_id_container) {
      iter->second.insert(forward_node_id_container.begin(),
                          forward_node_id_container.end());
    }
    size_t Size() const { return item_and_forward_node_ids_.size(); }

   private:
    // 该项集求的闭包是否有效（求过闭包且之后没有做任何更改则为true）
    bool core_closure_available_ = false;
    // 项集ID
    CoreId core_id_ = CoreId::InvalidId();
    // 项对应的语法分析表条目ID
    ParsingTableEntryId parsing_table_entry_id_ =
        ParsingTableEntryId::InvalidId();
    // 项和对应的向前看符号
    std::map<CoreItem, InsideForwardNodesContainerType>
        item_and_forward_node_ids_;
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
      virtual ~ActionAndAttachedDataInterface() {}

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
      // 允许序列化用的类访问
      friend class boost::serialization::access;
      // 允许语法分析表条目调用内部接口
      friend class ParsingTableEntry;

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
      // 允许序列化类访问
      friend class boost::serialization::access;
      // 允许语法分析表条目访问内部接口
      friend class ParsingTableEntry;

      template <class Archive>
      void serialize(Archive& ar, const unsigned int version) {
        ActionAndAttachedDataInterface::serialize(ar, version);
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
      // 允许序列化类访问
      friend class boost::serialization::access;
      // 允许语法分析表条目访问内部接口
      friend class ParsingTableEntry;

      template <class Archive>
      void serialize(Archive& ar, const unsigned int version) {
        ActionAndAttachedDataInterface::serialize(ar, version);
        ar& reducted_nonterminal_node_id_;
        ar& process_function_class_id_;
        ar& production_body_;
      }

      virtual ReductAttachedData& GetReductAttachedData() override {
        return const_cast<ReductAttachedData&>(
            static_cast<const ReductAttachedData>(*this)
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

      virtual bool operator==(const ActionAndAttachedDataInterface&
                                  shift_reduct_attached_data) const override;

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
      // 允许序列化类访问
      friend class boost::serialization::access;
      // 允许语法分析表条目访问内部接口
      friend class ParsingTableEntry;

      template <class Archive>
      void serialize(Archive& ar, const unsigned int version) {
        ActionAndAttachedDataInterface::serialize(ar, version);
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

    // 动作为规约时存储包装调用函数的类的对象的ID和规约后得到的非终结产生式ID
    // 产生式ID用于确定如何在规约后产生的非终结产生式条件下转移
    // 动作为移入时存储移入后转移到的条目ID
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
    template <class AttachedData>
    requires std::is_same_v<
        std::decay_t<AttachedData>,
        SyntaxGenerator::ParsingTableEntry::ShiftAttachedData> ||
        std::is_same_v<
            std::decay_t<AttachedData>,
            SyntaxGenerator::ParsingTableEntry::ReductAttachedData> ||
        std::is_same_v<
            std::decay_t<AttachedData>,
            SyntaxGenerator::ParsingTableEntry::ShiftReductAttachedData>
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
    // 操作为移入时variant存移入后转移到的ID（ProductionBodyId）
    // 操作为规约时variant存使用的产生式ID和产生式体ID（ProductionNodeId）
    // 操作为接受和报错时variant未定义
    ActionAndTargetContainer action_and_attached_data_;
    // 移入非终结节点后转移到的产生式体序号
    std::unordered_map<ProductionNodeId, ParsingTableEntryId>
        nonterminal_node_transform_table_;
  };
  // 用于ParsingTableTerminalNodeClassify中判断两个转移结果是否相同
  struct ActionAndAttachedDataPointerEqualTo {
    bool operator()(
        const ParsingTableEntry::ActionAndAttachedDataInterface* const& lhs,
        const ParsingTableEntry::ActionAndAttachedDataInterface* const& rhs)
        const;
  };
};
template <class IdType>
inline SyntaxGenerator::ProductionBodyId
SyntaxGenerator::NonTerminalProductionNode::AddBody(
    IdType&& body, ProcessFunctionClassId class_for_reduct_id_) {
  ProductionBodyId body_id(nonterminal_bodys_.size());
  // 将输入插入到产生式体向量中，无删除相同产生式功能
  nonterminal_bodys_.emplace_back(
      ProductionBodyType{.production_body = std::forward<IdType>(body),
                         .class_for_reduct_id = class_for_reduct_id_});
  return body_id;
}

template <class T>
inline bool SyntaxGenerator::PrintProcessFunction(FILE* function_file,
                                                  const T& data) {
  assert(function_file != nullptr);
  if (data.reduct_function.empty()) {
    fprintf(stderr, "Warning: 产生式未提供规约函数\n");
    return false;
  }
  fprintf(function_file,
          " virtual ProcessFunctionInterface::UserData "
          "Reduct(std::vector<WordDataToUser>&& user_returned_data) { return "
          "%s(user_returned_data); }\n",
          data.reduct_function.c_str());
  return true;
}

template <class ProcessFunctionClass>
inline SyntaxGenerator::ProductionNodeId SyntaxGenerator::AddNonTerminalNode(
    std::string&& node_symbol, std::vector<std::string>&& subnode_symbols) {
  ProcessFunctionClassId class_id =
      CreateProcessFunctionClassObject<ProcessFunctionClass>();
  return AddNonTerminalNode(std::move(node_symbol), std::move(subnode_symbols),
                            class_id);
}

// 向项集中添加项和相应的向前看符号，可以传入单个未包装ID
// 如果该项已存在则仅添加向前看符号

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

template <class ParsingTableEntryIdContainer>
void SyntaxGenerator::ParsingTableTerminalNodeClassify(
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
    // 使用ActionAndAttachedDataPointerEqualTo来判断两个转移结果是否相同
    std::unordered_map<
        const ParsingTableEntry::ActionAndAttachedDataInterface*,
        std::vector<ParsingTableEntryId>,
        std::hash<const ParsingTableEntry::ActionAndAttachedDataInterface*>,
        ActionAndAttachedDataPointerEqualTo>
        classify_table;
    ProductionNodeId transform_id = terminal_node_ids[index];
    for (auto production_node_id : entry_ids) {
      // 利用unordered_map进行分类
      classify_table[GetParsingTableEntry(production_node_id)
                         .AtTerminalNode(transform_id)]
          .push_back(production_node_id);
    }
    size_t next_index = index + 1;
    for (auto& vec : classify_table) {
      if (vec.second.size() > 1) {
        // 该类含有多个条目且有剩余未比较的转移条件，需要继续分类
        ParsingTableTerminalNodeClassify(terminal_node_ids, next_index,
                                         std::move(vec.second), equivalent_ids);
      }
    }
  }
}

template <class ParsingTableEntryIdContainer>
inline void SyntaxGenerator::ParsingTableNonTerminalNodeClassify(
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
    std::map<ParsingTableEntryId, std::vector<ParsingTableEntryId>>
        classify_table;
    ProductionNodeId transform_id = nonterminal_node_ids[index];
    for (auto production_node_id : entry_ids) {
      // 利用map进行分类
      classify_table[GetParsingTableEntry(production_node_id)
                         .AtNonTerminalNode(transform_id)]
          .push_back(production_node_id);
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
                   SyntaxGenerator::ParsingTableEntry::ReductAttachedData> ||
    std::is_same_v<std::decay_t<AttachedData>,
                   SyntaxGenerator::ParsingTableEntry::ShiftReductAttachedData>
void SyntaxGenerator::ParsingTableEntry::SetTerminalNodeActionAndAttachedData(
    ProductionNodeId node_id, AttachedData&& attached_data) {
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
    // 语法分析表创建过程中不应重写已存在的转移条件
    // 要么插入新转移条件，要么补全移入/规约的另一部分（规约/移入）
    if (attached_data.GetActionType() != data_already_in.GetActionType()) {
      assert(data_already_in.GetActionType() != ActionType::kShiftReduct);
      if (attached_data.GetActionType() == ActionType::kShift) {
        // 提供的数据为移入部分数据
        if constexpr (std::is_same_v<std::decay_t<AttachedData>,
                                     ShiftAttachedData>) {
          iter->second = std::make_unique<ShiftReductAttachedData>(
              std::forward<AttachedData>(attached_data),
              std::move(iter->second->GetReductAttachedData()));
        } else {
          assert(false);
        }
      } else {
        // 提供的数据为规约部分数据
        assert(attached_data.GetActionType() == ActionType::kReduct);
        if constexpr (std::is_same_v<std::decay_t<AttachedData>,
                                     ReductAttachedData>) {
          iter->second = std::make_unique<ShiftReductAttachedData>(
              std::move(iter->second->GetShiftAttachedData()),
              std::forward<AttachedData>(attached_data));
        } else {
          assert(false);
        }
      }
    } else {
      assert(static_cast<const AttachedData&>(data_already_in) ==
             attached_data);
    }
  }
}

template <class ForwardNodeIdContainer>
inline std::pair<
    std::map<SyntaxGenerator::CoreItem,
             std::unordered_set<SyntaxGenerator::ProductionNodeId>>::iterator,
    bool>
SyntaxGenerator::Core::AddItemAndForwardNodeIds(
    const CoreItem& item, ForwardNodeIdContainer&& forward_node_ids) {
  SetClosureNotAvailable();
  auto iter = item_and_forward_node_ids_.find(item);
  if (iter == item_and_forward_node_ids_.end()) {
    return item_and_forward_node_ids_.insert(std::make_pair(
        item, std::unordered_set<ProductionNodeId>(
                  std::forward<ForwardNodeIdContainer>(forward_node_ids))));
  } else {
    AddForwardNode(iter,
                   std::forward<ForwardNodeIdContainer>(forward_node_ids));
    return std::make_pair(iter, false);
  }
}

}  // namespace frontend::generator::syntaxgenerator
#endif  // !GENERATOR_SYNTAXGENERATOR_SYNTAXGENERATOR_H_