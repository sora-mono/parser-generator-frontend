/// @file syntax_generator.h
/// @brief 语法分析机配置生成器
/// @details
/// 1.语法分析机配置生成器使用LALR(1)语法
/// 2.支持非终结产生式空规约
/// 3.支持运算符优先级（二义性文法）
/// 4.支持双目和左侧单目语义共存
/// 5.支持产生式循环引用
/// 6.向前看符号不是运算符时采用移入优先原则
/// 例：
/// IF -> if() {} else
///       if() {}
/// 优先选择if-else路径
#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAX_GENERATOR_H_

#include <any>
#include <boost/archive/binary_oarchive.hpp>
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

/// TODO 添加删除未使用产生式的功能
namespace frontend::generator::syntax_generator {

/// @class SyntaxGenerator syntax_generator.h
/// @brief 语法分析机配置生成器
class SyntaxGenerator {
 private:
  /// @brief 词法分析中词的优先级
  using WordPriority =
      frontend::generator::dfa_generator::DfaGenerator::WordPriority;
  /// @brief 表示项的数据结构
  using ProductionItem = ProductionItemSet::ProductionItem;
  /// @brief 哈希ProductionItemSet的类
  using ProductionItemHasher = ProductionItemSet::ProductionItemHasher;
  /// @brief 项与向前看节点的容器
  using ProductionItemAndForwardNodesContainer =
      ProductionItemSet::ProductionItemAndForwardNodesContainer;
  /// @brief 存储向前看节点的容器
  using ForwardNodesContainer = ProductionItemSet::ForwardNodesContainer;

 public:
  SyntaxGenerator() = default;
  SyntaxGenerator(const SyntaxGenerator&) = delete;

  SyntaxGenerator& operator=(const SyntaxGenerator&) = delete;

  /// @brief 构建并保存编译器前端配置
  /// @note 自动构建语法分析和DFA配置并保存
  void ConstructSyntaxConfig();

 private:
  /// @brief 初始化
  void SyntaxGeneratorInit();
  /// @brief 添加构建配置所需的各种外围信息
  /// @note 该函数定义在config_construct.cpp中
  void ConfigConstruct();
  /// @brief 构建语法分析表
  void SyntaxAnalysisTableConstruct();

  /// @brief 添加产生式名
  /// @param[in] node_symbol ：产生式名
  /// @return 前半部分为产生式名ID，后半部分为是否添加了新的产生式名
  /// @note 如果产生式名已经添加过则返回值后半部分返回false
  /// @attention 不允许输入空字符串
  std::pair<SymbolId, bool> AddNodeSymbol(const std::string& node_symbol) {
    assert(node_symbol.size() != 0);
    return manager_node_symbol_.EmplaceObject(node_symbol);
  }
  /// @brief 添加产生式体字符串
  /// @param[in] body_symbol ：产生式体字符串
  /// @return
  /// 前半部分为产生式体字符串的ID，后半部分为是否添加了新的产生式体字符串
  /// @note 如果产生式体字符串已经添加过则返回值后半部分返回false
  /// @attention 不允许输入空字符串
  std::pair<SymbolId, bool> AddBodySymbol(const std::string& body_symbol) {
    assert(body_symbol.size() != 0);
    return manager_terminal_body_symbol_.EmplaceObject(body_symbol);
  }
  /// @brief 根据产生式名获取对应ID
  /// @param[in] node_symbol ：产生式名
  /// @return 返回产生式名对应ID
  /// @retval SymbolId::InvalidId() ：给定的产生式名未添加过
  SymbolId GetNodeSymbolId(const std::string& node_symbol) const {
    assert(node_symbol.size() != 0);
    return manager_node_symbol_.GetObjectIdFromObject(node_symbol);
  }
  /// @brief 根据产生式体字符串获取对应ID
  /// @param[in] body_symbol ：产生式体字符串
  /// @return 返回产生式体字符串对应的ID
  /// @retval SymbolId::InvalidId() ：给定的产生式体字符串未添加过
  SymbolId GetBodySymbolId(const std::string& body_symbol) const {
    assert(body_symbol.size() != 0);
    return manager_terminal_body_symbol_.GetObjectIdFromObject(body_symbol);
  }
  /// @brief 根据产生式名ID获取对应产生式名
  /// @param[in] node_symbol_id ：产生式名ID
  /// @return 返回产生式名ID对应的产生式名的const引用
  /// @note 要求给定的node_symbol_id有效
  const std::string& GetNodeSymbolStringFromId(SymbolId node_symbol_id) const {
    assert(node_symbol_id.IsValid());
    return manager_node_symbol_.GetObject(node_symbol_id);
  }
  /// @brief 根据产生式体字符串ID获取产生式体字符串
  /// @param[in] body_symbol_id ：产生式体字符串ID
  /// @return 返回产生式体字符串的const引用
  /// @note 要求给定的body_symbol_id有效
  const std::string& GetBodySymbolStringFromId(SymbolId body_symbol_id) const {
    assert(body_symbol_id.IsValid());
    return manager_terminal_body_symbol_.GetObject(body_symbol_id);
  }
  /// @brief 通过产生式节点ID获取产生式名
  /// @param[in] production_node_id ：产生式节点ID
  /// @return 返回产生式名的const引用
  /// @note 给定的产生式节点ID对应的产生式节点必须已设置有效的产生式名ID
  const std::string& GetNodeSymbolStringFromProductionNodeId(
      ProductionNodeId production_node_id) const {
    return GetNodeSymbolStringFromId(
        GetProductionNode(production_node_id).GetNodeSymbolId());
  }
  /// @brief 查询项中下一个移入的产生式名
  /// @param[in] production_node_id ：产生式节点ID
  /// @param[in] production_body_id ：产生式体ID
  /// @param[in] next_word_to_shift_index ：下一个移入的节点在产生式体的下标
  /// @return 返回给定项下一个移入的产生式名的const引用
  /// @note 要求给定项必须存在且给定项存在可以移入的产生式
  const std::string& GetNextNodeToShiftSymbolString(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      NextWordToShiftIndex next_word_to_shift_index) const {
    return GetNodeSymbolStringFromProductionNodeId(GetProductionNodeIdInBody(
        production_node_id, production_body_id, next_word_to_shift_index));
  }
  /// @brief 通过项查询下一个移入的节点名
  /// @param[in] production_item ；项
  /// @return 返回给定项下一个移入的产生式名的const引用
  /// @note 要求给定项存在可以移入的产生式
  const std::string& GetNextNodeToShiftSymbolString(
      const ProductionItem& production_item) const {
    auto& [production_node_id, production_body_id, next_word_to_shift_index] =
        production_item;
    return GetNextNodeToShiftSymbolString(
        production_node_id, production_body_id, next_word_to_shift_index);
  }
  /// @brief 设置产生式名ID到产生式节点ID的映射
  /// @param[in] node_symbol_id ：产生式名ID
  /// @param[in] node_id ：产生式节点ID
  /// @note 如果已存在旧映射则会覆盖
  void SetNodeSymbolIdToProductionNodeIdMapping(SymbolId node_symbol_id,
                                                ProductionNodeId node_id) {
    assert(node_symbol_id.IsValid() && node_id.IsValid());
    node_symbol_id_to_node_id_[node_symbol_id] = node_id;
  }
  /// @brief 设置产生式体字符串ID到产生式节点ID的映射
  /// @param[in] body_symbol_id ：产生式体字符串ID
  /// @param[in] node_id ：产生式节点ID
  /// @note 如果已存在旧映射则会覆盖
  void SetBodySymbolIdToProductionNodeIdMapping(SymbolId body_symbol_id,
                                                ProductionNodeId node_id) {
    assert(body_symbol_id.IsValid() && node_id.IsValid());
    production_body_symbol_id_to_node_id_[body_symbol_id] = node_id;
  }
  /// @brief 根据产生式名ID获取对应产生式节点ID
  /// @param[in] node_symbol_id ：产生式名ID
  /// @return 返回产生式节点ID
  /// @retval ProductionNodeId::InvalidId() ：给定的产生式名ID不存在
  ProductionNodeId GetProductionNodeIdFromNodeSymbolId(SymbolId node_symbol_id);
  /// @brief 根据产生式体字符串ID获取产生式节点ID
  /// @param[in] body_symbol_id ：产生式字符串ID
  /// @return 返回产生式节点ID
  /// @retval ProductionNodeId::InvalidId() ：给定产生式体字符串ID不存在
  ProductionNodeId GetProductionNodeIdFromBodySymbolId(SymbolId body_symbol_id);
  /// @brief 根据产生式名获取产生式节点ID
  /// @param[in] node_symbol ：产生式名ID
  /// @return 返回产生式节点ID
  /// @retval ProductionNodeId::InvalidId() ：产生式名未添加过
  ProductionNodeId GetProductionNodeIdFromNodeSymbol(
      const std::string& node_symbol);
  /// @brief 将产生式体符号转换为产生式节点ID
  /// @param[in] body_symbol ：产生式体字符串ID
  /// @return 返回产生式节点ID
  /// @retval ProductionNodeId::InvalidId() ：产生式体字符串未添加过
  ProductionNodeId GetProductionNodeIdFromBodySymbol(
      const std::string& body_symbol);
  /// @brief 获取产生式体中指定位置的产生式节点ID
  /// @param[in] production_node_id ：产生式节点ID
  /// @param[in] production_body_id ：产生式体ID
  /// @param[in] next_word_to_shift_index ：要获取的产生式节点ID的位置
  /// @return 返回产生式节点ID
  /// @retval ProductionNodeId::InvalidId()
  /// ：next_word_to_shift_index大于等于指定的产生式体中含有的产生式数目
  /// @note production_node_id和production_body_id必须有效
  /// 终结节点和运算符节点中production_body_id必须使用0
  ProductionNodeId GetProductionNodeIdInBody(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id,
      NextWordToShiftIndex next_word_to_shift_index) const {
    return GetProductionNode(production_node_id)
        .GetProductionNodeInBody(production_body_id, next_word_to_shift_index);
  }
  /// @brief 设置用户定义的根产生式节点ID
  /// @param[in] root_production_node_id ：用户定义的根产生式节点ID
  void SetRootProductionNodeId(ProductionNodeId root_production_node_id) {
    root_production_node_id_ = root_production_node_id;
  }
  /// @brief 获取用户定义的根产生式ID
  /// @return 返回用户定义的根产生式ID
  /// @retval ProductionNodeId::InvalidId()
  /// ：未设置过根产生式ID或主动设置为这个值
  ProductionNodeId GetRootProductionNodeId() {
    return root_production_node_id_;
  }
  /// @brief 实例化包装处理函数的类对象
  /// @tparam ProcessFunctionClass ：包装处理函数的类对象的类名
  /// @return 返回包装处理函数的类的实例化对象的ID
  /// @note 不检查是否重复，但是使用时一个类仅允许实例化一次
  template <class ProcessFunctionClass>
  ProcessFunctionClassId CreateProcessFunctionClassObject() {
    return manager_process_function_class_
        .EmplaceObject<ProcessFunctionClass>();
  }
  /// @brief 获取规约某个产生式使用的包装处理函数的类的实例化对象ID
  /// @param[in] production_node_id ：产生式ID
  /// @param[in] production_body_id ：产生式体ID
  /// @return 返回包装处理函数的类的实例化对象ID
  /// @note 仅允许对非终结产生式节点执行该操作，要求production_node_id有效
  ProcessFunctionClassId GetProcessFunctionClass(
      ProductionNodeId production_node_id,
      ProductionBodyId production_body_id) {
    NonTerminalProductionNode& production_node =
        static_cast<NonTerminalProductionNode&>(
            GetProductionNode(production_node_id));
    assert(production_node.GetType() == ProductionNodeType::kNonTerminalNode);
    return production_node.GetBodyProcessFunctionClassId(production_body_id);
  }
  /// @brief 添加因为产生式体中某个产生式未定义而导致不能继续添加的记录
  /// @param[in] undefined_symbol ：未定义的产生式名
  /// @param[in] node_symbol ：待添加的产生式名
  /// @param[in] subnode_symbols ：产生式体
  /// @param[in] class_id ：包装处理函数的类的实例化对象ID
  /// @note
  /// 1.node_symbol、subnode_symbols、class_id同AddNonTerminalNode调用参数
  /// 2.函数会复制/移动构造一份副本，无需保持原来的参数的生命周期
  /// 3.该函数解决产生式循环引用功能无法避免使用未添加的产生式作为产生式体内容
  void AddUnableContinueNonTerminalNode(
      const std::string& undefined_symbol, std::string&& node_symbol,
      std::vector<std::string>&& subnode_symbols,
      ProcessFunctionClassId class_id);
  /// @brief
  /// 检查已添加给定的产生式节点后是否可以重启因为该产生式体未定义而搁置的
  /// 非终结产生式添加过程，如果可以则重启全部添加过程
  /// @param[in] node_symbol ：已添加的产生式名
  /// @note
  /// 1.该函数与AddUnableContinueNonTerminalNode配套使用
  /// 2.重启添加过程通过使用保存的参数重新调用AddNonTerminalNode实现
  void CheckNonTerminalNodeCanContinue(const std::string& node_symbol);
  /// @brief 检查是否存在未定义的产生式体
  /// @note
  /// 1.完成所有产生式添加后检查
  /// 2.如果有则输出错误信息后退出
  void CheckUndefinedProductionRemained();
  /// @brief 添加关键字
  /// @param[in] node_symbol ：产生式名
  /// @param[in] key_word ：关键字字符串
  /// @note
  /// 1.支持正则
  /// 2.关键字已存在则输出错误信息
  void AddKeyWord(std::string node_symbol, std::string key_word);
  /// @brief 添加普通终结产生式
  /// @param[in] node_symbol ：产生式名
  /// @param[in] regex_string ：产生式的正则表示
  void AddSimpleTerminalProduction(std::string node_symbol,
                                   std::string regex_string);
  /// @brief 添加终结产生式（关键字、运算符、普通终结产生式）
  /// @param[in] node_symbol ：终结产生式名
  /// @param[in] body_symbol ：产生式体字符串（正则形式）
  /// @param[in] node_priority ：产生式体的单词优先级
  /// @param[in] is_key_word ：该产生式是否是关键字
  /// @return 返回终结节点ID
  /// @retval ProductionNodeId::Invalid() ：待添加终结产生式名或体字符串已存在
  /// @note
  /// 1.节点已存在或给定symbol_id不同于已有ID则输出错误信息并返回
  /// ProductionNodeId::InvalidId()
  /// 2.单词优先级：0保留为普通词的优先级，1保留为运算符优先级，2保留为关键字
  /// 优先级，其余的优先级尚未指定
  /// @attention 词优先级与运算符优先级不同，请注意区分
  ProductionNodeId AddTerminalProduction(std::string&& node_symbol,
                                         std::string&& body_symbol,
                                         WordPriority node_priority,
                                         bool regex_allowed);
  /// @brief AddTerminalProduction的子过程，仅用于创建终结节点
  /// @param[in] node_symbol_id ：终结节点名ID
  /// @param[in] body_symbol_id ：终结节点体字符串ID
  /// @return 返回新建的终结产生式节点ID
  /// @note
  /// 1.自动为节点类设置节点ID
  /// 2.自动更新节点名ID到节点ID的映射
  /// 3.自动更新节点体ID到节点ID的映射
  ProductionNodeId SubAddTerminalNode(SymbolId node_symbol_id,
                                      SymbolId body_symbol_id);
  /// @brief 添加双目运算符
  /// @param[in] operator_symbol ：运算符名（支持正则）
  /// @param[in] binary_operator_associatity_type ：双目运算符结合性
  /// @param[in] binary_operator_priority ：双目运算符的运算符优先级
  /// @return 返回双目运算符节点ID
  /// @note
  /// 1.节点已存在则返回ProductionNodeId::InvalidId()
  /// 2.运算符词法分析优先级高于普通终结产生式低于关键字
  ProductionNodeId AddBinaryOperator(
      std::string node_symbol, std::string operator_symbol,
      OperatorAssociatityType binary_operator_associatity_type,
      OperatorPriority binary_operator_priority);
  /// @brief 添加单目运算符
  /// @param[in] operator_symbol ：运算符名（支持正则）
  /// @param[in] unary_operator_associatity_type ：单目运算符结合性
  /// @param[in] unary_operator_priority ：单目运算符的运算符优先级
  /// @return 返回单目运算符节点ID
  /// @note
  /// 1.节点已存在则返回ProductionNodeId::InvalidId()
  /// 2.运算符词法分析优先级高于普通终结产生式低于关键字
  /// @attention 仅支持左侧单目运算符，不支持右侧单目运算符
  ProductionNodeId AddLeftUnaryOperator(
      std::string node_symbol, std::string operator_symbol,
      OperatorAssociatityType unary_operator_associatity_type,
      OperatorPriority unary_operator_priority);
  /// @brief 添加具有双目运算符和左侧单目运算符语义的运算符
  /// @param[in] operator_symbol ：运算符名（支持正则）
  /// @param[in] binary_operator_associatity_type ：双目运算符结合性
  /// @param[in] binary_operator_priority ：双目运算符的运算符优先级
  /// @param[in] unary_operator_associatity_type ：单目运算符结合性
  /// @param[in] unary_operator_priority ：单目运算符的运算符优先级
  /// @return 返回运算符节点ID
  /// @note
  /// 1.节点已存在则返回ProductionNodeId::InvalidId()
  /// 2.运算符词法分析优先级高于普通终结产生式低于关键字
  /// 3.一个运算符节点含有两种语义
  ProductionNodeId AddBinaryLeftUnaryOperator(
      std::string node_symbol, std::string operator_symbol,
      OperatorAssociatityType binary_operator_associatity_type,
      OperatorPriority binary_operator_priority,
      OperatorAssociatityType unary_operator_associatity_type,
      OperatorPriority unary_operator_priority);
  /// @brief AddBinaryOperatorNode的子过程
  /// @param[in] node_symbol_id ：待添加的运算符符号ID
  /// @param[in] binary_associatity_type ：双目运算符结合性
  /// @param[in] binary_priority ：双目运算符优先级
  /// @details
  /// 该子过程仅用于创建节点
  /// 自动为节点类设置节点ID
  /// 自动更新节点名ID到节点ID的映射表
  /// 自动更新节点体ID到节点ID的映射
  /// @note
  /// 运算符优先级与单词优先级不同，请注意区分
  ProductionNodeId SubAddBinaryOperatorNode(
      SymbolId node_symbol_id, OperatorAssociatityType binary_associatity_type,
      OperatorPriority binary_priority);
  /// @brief AddUnaryOperatorNode的子过程
  /// @param[in] node_symbol_id ：待添加的运算符符号ID
  /// @param[in] unary_associatity_type ：单目运算符结合性
  /// @param[in] unary_priority ：单目运算符优先级
  /// @details
  /// 该子过程仅用于创建节点
  /// 自动为节点类设置节点ID
  /// 自动更新节点名ID到节点ID的映射表
  /// 自动更新节点体ID到节点ID的映射
  /// @note
  /// 运算符优先级与单词优先级不同，请注意区分
  ProductionNodeId SubAddUnaryOperatorNode(
      SymbolId node_symbol_id, OperatorAssociatityType unary_associatity_type,
      OperatorPriority unary_priority);
  /// @brief AddBinaryUnaryOperatorNode的子过程
  /// @param[in] node_symbol_id ：待添加的运算符符号ID
  /// @param[in] binary_associatity_type ：双目运算符结合性
  /// @param[in] binary_priority ：双目运算符优先级
  /// @param[in] unary_associatity_type ：单目运算符结合性
  /// @param[in] unary_priority ：单目运算符优先级
  /// @details
  /// 该子过程仅用于创建节点
  /// 自动为节点类设置节点ID
  /// 自动更新节点名ID到节点ID的映射表
  /// 自动更新节点体ID到节点ID的映射
  /// @note
  /// 运算符优先级与单词优先级不同，请注意区分
  ProductionNodeId SubAddBinaryUnaryOperatorNode(
      SymbolId node_symbol_id, OperatorAssociatityType binary_associatity_type,
      OperatorPriority binary_priority,
      OperatorAssociatityType unary_associatity_type,
      OperatorPriority unary_priority);
  /// @brief 添加非终结产生式
  /// @tparam ProcessFunctionClass ：包装规约函数的类
  /// @param[in] node_symbol ：非终结产生式名
  /// @param[in] subnode_symbols ：非终结产生式体
  /// @return 返回非终结产生式节点ID
  /// @details
  /// 拆成模板函数和非模板函数从而降低代码生成量，阻止代码膨胀，二者等价
  template <class ProcessFunctionClass>
  ProductionNodeId AddNonTerminalProduction(std::string node_symbol,
                                            std::string subnode_symbols);
  /// @brief 添加非终结产生式
  /// @param[in] node_symbol ：非终结产生式名
  /// @param[in] subnode_symbols ：非终结产生式体
  /// @param[in] class_id ：包装规约函数的类的实例化对象ID
  /// @return 返回非终结产生式节点ID
  ProductionNodeId AddNonTerminalProduction(
      std::string&& node_symbol, std::vector<std::string>&& subnode_symbols,
      ProcessFunctionClassId class_id);
  /// @brief AddNonTerminalProduction的子过程，仅用于创建节点
  /// @param[in] symbol_id ：非终结产生式名ID
  /// @return 返回创建的非终结产生式节点ID
  /// @note
  /// 自动更新节点名ID到节点ID的映射表
  /// 自动为节点类设置节点ID
  ProductionNodeId SubAddNonTerminalNode(SymbolId symbol_id);
  /// @brief 设置非终结产生式可以空规约
  /// @param[in] nonterminal_node_symbol ：非终结产生式名
  /// @note 指定的非终结产生式必须已添加过，否则输出错误信息后退出
  void SetNonTerminalNodeCouldEmptyReduct(
      const std::string& nonterminal_node_symbol);
  /// @brief 新建文件尾节点
  /// @return 返回尾节点ID
  ProductionNodeId AddEndNode();
  /// @brief 设置用户定义的产生式根节点
  /// @param[in] production_node_name ：产生式名
  /// @note production_node_name对应的产生式必须已添加，否则输出错误信息后退出
  void SetRootProduction(const std::string& production_node_name);

  /// @brief 根据产生式节点ID获取产生式节点
  /// @param[in] production_node_id ：产生式节点ID
  /// @return 返回产生式节点的引用
  /// @note 给定的产生式节点ID必须有效
  BaseProductionNode& GetProductionNode(ProductionNodeId production_node_id);
  /// @brief 根据产生式节点ID获取产生式节点
  /// @param[in] production_node_id ：产生式节点ID
  /// @return 返回产生式节点的const引用
  /// @note 给定的产生式节点ID必须有效
  const BaseProductionNode& GetProductionNode(
      ProductionNodeId production_node_id) const;
  /// @brief 根据产生式名ID获取产生式节点
  /// @param[in] symbol_id ：产生式名ID
  /// @return 返回产生式节点的引用
  /// @note 给定的产生式名ID必须有效
  BaseProductionNode& GetProductionNodeFromNodeSymbolId(SymbolId symbol_id);
  /// @brief 根据产生式体字符串ID获取产生式节点
  /// @param[in] symbol_id ：产生式体字符串ID
  /// @return 返回产生式节点的引用
  /// @note 给定的产生式体字符串ID必须有效
  BaseProductionNode& GetProductionNodeFromBodySymbolId(SymbolId symbol_id);
  /// @brief 获取非终结节点中的一个产生式体
  /// @param[in] production_node_id ：非终结产生式节点ID
  /// @param[in] production_body_id ：产生式体ID
  /// @return 返回产生式体的const引用
  /// @note 必须使用有效的非终结产生式节点和配套的有效的产生式体ID
  const std::vector<ProductionNodeId>& GetProductionBody(
      ProductionNodeId production_node_id, ProductionBodyId production_body_id);
  /// @brief 向非终结节点中添加产生式体
  /// @param[in] node_id ：待添加产生式的非终结节点ID
  /// @param[in] body ：产生式体
  /// @return 返回产生式体ID
  /// @note 必须输入非终结产生式节点的ID
  template <class IdType>
  ProductionBodyId AddNonTerminalNodeBody(ProductionNodeId node_id,
                                          IdType&& body) {
    assert(GetProductionNode(node_id).GetType() ==
           ProductionNodeType::kNonTerminalNode);
    return static_cast<NonTerminalProductionNode&>(GetProductionNode(node_id))
               .AddBody,
           (std::forward<IdType>(body));
  }
  /// @brief 添加一条语法分析表条目
  /// @return 返回新添加的语法分析表条目ID
  SyntaxAnalysisTableEntryId AddSyntaxAnalysisTableEntry() {
    SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id(
        syntax_analysis_table_.size());
    syntax_analysis_table_.emplace_back();
    return syntax_analysis_table_entry_id;
  }
  /// @brief 根据项集ID获取项集
  /// @param[in] production_item_set_id ：项集ID
  /// @return 返回项集的const引用
  /// @note 给定项集ID必须有效
  const ProductionItemSet& GetProductionItemSet(
      ProductionItemSetId production_item_set_id) const {
    assert(production_item_set_id < production_item_sets_.Size());
    return production_item_sets_[production_item_set_id];
  }
  /// @brief 根据项集ID获取项集
  /// @param[in] production_item_set_id ：项集ID
  /// @return 返回项集的引用
  /// @note 给定项集ID必须有效
  ProductionItemSet& GetProductionItemSet(
      ProductionItemSetId production_item_set_id) {
    return const_cast<ProductionItemSet&>(
        static_cast<const SyntaxGenerator&>(*this).GetProductionItemSet(
            production_item_set_id));
  }
  /// @brief 设置语法分析表条目ID到项集ID的映射
  /// @param[in] syntax_analysis_table_entry_id ：语法分析表条目ID
  /// @param[in] production_item_set_id ：项集ID
  /// @note 如果存在旧映射则覆盖
  void SetSyntaxAnalysisTableEntryIdToProductionItemSetIdMapping(
      SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id,
      ProductionItemSetId production_item_set_id) {
    syntax_analysis_table_entry_id_to_production_item_set_id_
        [syntax_analysis_table_entry_id] = production_item_set_id;
  }
  /// @brief 获取语法分析表条目ID对应的项集ID
  /// @param[in] syntax_analysis_table_entry_id ：语法分析表条目ID
  /// @return 返回项集ID
  /// @note 语法分析表条目ID必须有效
  ProductionItemSetId GetProductionItemSetIdFromSyntaxAnalysisTableEntryId(
      SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id);
  /// @brief 添加新的项集
  /// @return 返回新添加的项集的ID
  /// @note 自动更新语法分析表条目到项集ID的映射
  ProductionItemSetId EmplaceProductionItemSet();
  /// @brief 向项集中添加项和项的向前看符号
  /// @param[in] production_item_set_id ：项集ID
  /// @param[in] production_item ：项
  /// @param[in] forward_node_ids ：项的向前看符号
  /// @return 前半部分为插入的位置，后半部分为是否插入新项
  /// @note
  /// 1.如果添加了新向前看符号则设置闭包无效
  /// 2.已经求过闭包的项集仅允许对项添加向前看符号，不允许添加新项
  /// 3.如果给定项已存在则返回值后半部分一定返回false
  template <class ForwardNodeIdContainer>
  std::pair<ProductionItemAndForwardNodesContainer::iterator, bool>
  AddItemAndForwardNodeIdsToProductionItem(
      ProductionItemSetId production_item_set_id,
      const ProductionItem& production_item,
      ForwardNodeIdContainer&& forward_node_ids);
  /// @brief 向项集中添加核心项和核心项的向前看符号
  /// @param[in] production_item_set_id ：项集ID
  /// @param[in] production_item ：核心项
  /// @param[in] forward_node_ids ：核心项的向前看符号
  /// @return 前半部分为插入的位置，后半部分为是否插入新项
  /// @note
  /// 1.forward_node_ids支持存储ID的容器也支持未包装的ID
  /// 2.如果添加了新向前看符号则设置闭包无效
  /// 3.如果给定项已存在则返回值后半部分一定返回false
  /// 4.添加了新项则自动更新该项所存在的项集的ID的记录
  /// @attention 不允许对已求过闭包的项集执行该操作
  template <class ForwardNodeIdContainer>
  std::pair<ProductionItemAndForwardNodesContainer::iterator, bool>
  AddMainItemAndForwardNodeIdsToProductionItem(
      ProductionItemSetId production_item_set_id,
      const ProductionItem& production_item,
      ForwardNodeIdContainer&& forward_node_ids);
  /// @brief 向给定核心项中添加向前看符号
  /// @param[in] production_item_set_id ：项所在的项集ID
  /// @param[in] production_item ：核心项
  /// @param[in] forward_node_ids ：核心项的向前看符号
  /// @return 返回是否添加新的向前看符号
  /// @retval true ：添加了新的向前看符号
  /// @retval false ：未添加新的向前看符号
  /// @note
  /// 1.forward_node_ids支持存储ID的容器也支持未包装的ID
  /// 2.要求项已经存在，否则应调用AddItemAndForwardNodeIds或同类函数
  /// 3.如果添加了新的向前看符号则设置闭包无效
  /// 4.production_item必须是核心项
  template <class ForwardNodeIdContainer>
  bool AddForwardNodes(ProductionItemSetId production_item_set_id,
                       const ProductionItem& production_item,
                       ForwardNodeIdContainer&& forward_node_ids) {
    return GetProductionItemSet(production_item_set_id)
        .AddForwardNodes(production_item, std::forward<ForwardNodeIdContainer>(
                                              forward_node_ids));
  }
  /// @brief 添加核心项所属的项集ID
  /// @param[in] production_item ：项
  /// @param[in] production_item_set_id ：项集ID
  /// @note production_item_set_id必须未曾添加到production_item所属项集中
  void AddProductionItemBelongToProductionItemSetId(
      const ProductionItem& production_item,
      ProductionItemSetId production_item_set_id);
  /// @brief 获取项集的全部核心项
  /// @param[in] production_item_set_id ：项集ID
  /// @return 返回存储项集的全部核心项的容器的const引用
  const std::list<ProductionItemAndForwardNodesContainer::const_iterator>
  GetProductionItemSetMainItems(
      ProductionItemSetId production_item_set_id) const {
    return GetProductionItemSet(production_item_set_id).GetMainItemIters();
  }
  /// @brief 获取单个核心项所属的全部项集
  /// @param[in] production_node_id ：非终结产生式节点ID
  /// @param[in] body_id ：产生式体ID
  /// @param[in] next_word_to_shift_index ：下一个移入的产生式下标
  /// @return 返回存储核心项所属的全部项集的容器的const引用
  /// @note
  /// 1.容器中储存的ID不重复
  /// 2.三个参数共同决定一个项
  /// @attention 仅记录给定项作为核心项存在的项集ID
  const std::list<ProductionItemSetId>&
  GetProductionItemSetIdFromProductionItem(
      ProductionNodeId production_node_id, ProductionBodyId body_id,
      NextWordToShiftIndex next_word_to_shift_index);
  /// @brief 获取项的向前看符号
  /// @param[in] production_item_set_id ：项集ID
  /// @param[in] production_item ：项
  /// @return 返回存储向前看符号的容器的const引用
  /// @note 给定项必须存在于给定项集
  const ForwardNodesContainer& GetForwardNodeIds(
      ProductionItemSetId production_item_set_id,
      const ProductionItem& production_item) const {
    return GetProductionItemSet(production_item_set_id)
        .GetItemsAndForwardNodeIds()
        .at(production_item);
  }

  /// @brief First的子过程，提取一个非终结节点全部第一个可移入的产生式节点ID
  /// @param[in] production_node_id ：非终结产生式节点ID
  /// @param[in,out] result ：存储提取到的节点ID
  /// @param[in,out] processed_nodes ：已提取的非终结产生式节点
  /// @details
  /// 提取可以规约为给定非终结产生式的所有产生式组合中每个组合的第一个产生式ID
  /// 提取到的所有节点ID存到result中
  /// @note
  /// 1.processed_nodes存储已经提取过的产生式节点ID，这些节点不会被再次提取
  /// 2.该函数只会将结果添加到result中，不会清空result已有的内容
  /// @attention
  /// production_node_id必须有效，result不接受nullptr
  void GetNonTerminalNodeFirstNodeIds(
      ProductionNodeId production_node_id, ForwardNodesContainer* result,
      std::unordered_set<ProductionNodeId>&& processed_nodes =
          std::unordered_set<ProductionNodeId>());
  /// @brief 闭包操作中的first操作，用来提取非终结产生式的向前看节点
  /// @param[in] production_node_id ：非终结产生式节点ID
  /// @param[in] production_body_id ：非终结产生式体ID
  /// @param[in] next_word_to_shift_index
  /// ：指向提取向前看符号的起始产生式位置（非终结产生式的下一个位置）
  /// @param[in] next_node_ids
  /// ：提取到达非终结产生式超尾时将该容器的内容添加到返回结果中
  /// （待展开的非终结产生式所在的非终结产生式的向前看符号）
  /// @return 返回存储获取到的向前看符号ID的容器
  /// @details
  /// 1.production_node_id、production_body_id和next_word_to_shift_index标志
  /// β的位置（提取向前看符号的位置）
  /// 2.自动递归处理可以空规约的非终结产生式
  /// 3.因为非终结节点可能空规约，需要向下查找产生式，
  /// 所以前三个参数不能合并为待展开的产生式ID
  ForwardNodesContainer First(ProductionNodeId production_node_id,
                              ProductionBodyId production_body_id,
                              NextWordToShiftIndex next_word_to_shift_index,
                              const ForwardNodesContainer& next_node_ids);
  /// @brief 获取项集的全部项和项的向前看符号
  /// @param[in] production_item_set_id ：项集ID
  /// @return 返回存储项和对应向前看符号的容器的const引用
  /// @note 要求production_item_set_id有效
  const ProductionItemAndForwardNodesContainer&
  GetProductionItemsAndForwardNodes(
      ProductionItemSetId production_item_set_id) {
    return GetProductionItemSet(production_item_set_id)
        .GetItemsAndForwardNodeIds();
  }
  /// @brief 获取语法分析表条目
  /// @param[in] syntax_analysis_table_entry_id ：语法分析表条目ID
  /// @return 返回语法分析表条目的引用
  /// @note 要求syntax_analysis_table_entry_id有效
  SyntaxAnalysisTableEntry& GetSyntaxAnalysisTableEntry(
      SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id) {
    assert(syntax_analysis_table_entry_id < syntax_analysis_table_.size());
    return syntax_analysis_table_[syntax_analysis_table_entry_id];
  }
  /// @brief 设置根语法分析表条目ID
  /// @param[in] root_syntax_analysis_table_entry_id ：根语法分析表条目ID
  void SetRootSyntaxAnalysisTableEntryId(
      SyntaxAnalysisTableEntryId root_syntax_analysis_table_entry_id) {
    root_syntax_analysis_table_entry_id_ = root_syntax_analysis_table_entry_id;
  }
  /// @brief 设置项集闭包有效
  /// @param[in] production_item_set_id ：项集ID
  /// @note
  /// 闭包有效的项集在调用ProductionItemSetClosure时直接返回，不会重求闭包
  /// @attention 仅应由ProductionItemSetClosure调用
  void SetProductionItemSetClosureAvailable(
      ProductionItemSetId production_item_set_id) {
    GetProductionItemSet(production_item_set_id).SetClosureAvailable();
  }
  /// @brief 查询一个项集是否闭包有效
  /// @param[in] production_item_set_id ：项集ID
  /// @return 返回给定项集是否闭包有效
  /// @retval true ：闭包有效
  /// @retval false ：闭包无效
  bool IsClosureAvailable(ProductionItemSetId production_item_set_id) {
    return GetProductionItemSet(production_item_set_id).IsClosureAvailable();
  }
  /// @brief 对项集求闭包，同时构建语法分析表reduct操作部分
  /// @param[in] production_item_set_id ：项集ID
  /// @return 返回是否重求闭包
  /// @retval true ：重求闭包
  /// @retval false ：闭包有效，无需重求
  /// @note
  /// 1.自动添加所有当前位置可以空规约的项的后续项
  /// 2.重求闭包前会清空语法分析表条目和非核心项
  /// 3.求闭包过程中自动填写语法分析表中可规约的项
  bool ProductionItemSetClosure(ProductionItemSetId production_item_set_id);
  /// @brief 获取给定项移入相同产生式后构成的项集
  /// @param[in] items ：指向转移前的项的迭代器
  /// @return 返回获取到的项集ID
  /// @retval ProductionItemSetId::InvalidId()
  /// ：给定项移入相同产生式后未构成已有项集
  /// @details
  /// 查找项集，给定项移入相同产生式后该项集有且仅有这些项是核心项
  ProductionItemSetId GetProductionItemSetIdFromProductionItems(
      const std::list<std::unordered_map<
          ProductionItem, std::unordered_set<ProductionNodeId>,
          ProductionItemSet::ProductionItemHasher>::const_iterator>& items);
  /// @brief 传播向前看符号，同时在传播过程中构建语法分析表shift操作的部分
  /// @param[in] production_item_set_id ：待传播向前看符号的项集ID
  /// @return 返回是否执行了传播过程
  /// @retval true ：执行了传播向前看符号的过程
  /// @retval false ：无需执行传播向前看符号的过程
  /// @details
  /// 闭包有效说明该项集和对应的语法分析表条目未修改，无需重新传播向前看符号
  bool SpreadLookForwardSymbolAndConstructSyntaxAnalysisTableEntry(
      ProductionItemSetId production_item_set_id);
  /// @brief 对所有产生式节点按照ProductionNodeType分类
  /// @return 返回存储不同类型节点的容器
  /// @note
  /// ProductionNodeType中类型的值作为下标访问array以得到该类型的所有产生式节点
  std::array<std::vector<ProductionNodeId>, 4> ClassifyProductionNodes() const;
  /// @brief 分类在终结产生式作为向前看符号时下动作相同的语法分析表条目
  /// @param[in] terminal_node_ids ：用来分类的终结产生式节点
  /// @param[in] index
  /// ：本轮用来分类的终结产生式节点ID在terminal_node_ids中的位置
  /// @param[in] syntax_analysis_table_entry_ids ：待分类的语法分析表条目
  /// @param[in,out] equivalent_ids ：等价的语法分析表条目
  /// @details
  /// 1.首次调用时index一般使用0
  /// 2.该函数只向equivalent_ids按组写入等价的语法分析表条目ID，不合并这些条目
  /// 3.不会写入只有一个项的组
  /// 4.该函数是SyntaxAnalysisTableEntryClassify的子过程
  void SyntaxAnalysisTableTerminalNodeClassify(
      const std::vector<ProductionNodeId>& terminal_node_ids, size_t index,
      std::list<SyntaxAnalysisTableEntryId>&& syntax_analysis_table_entry_ids,
      std::vector<std::list<SyntaxAnalysisTableEntryId>>* equivalent_ids);
  /// @brief 分类非终结产生式作为向前看符号时下动作相同的语法分析表条目
  /// @param[in] nonterminal_node_ids ：用来分类的非终结产生式节点
  /// @param[in] index
  /// ：本轮用来分类的非终结产生式节点ID在nonterminal_node_ids中的位置
  /// @param[in] syntax_analysis_table_entry_ids ：待分类的语法分析表条目
  /// @param[in,out] equivalent_ids ：等价的语法分析表条目
  /// @details
  /// 1.首次调用时index一般使用0
  /// 2.该函数只向equivalent_ids按组写入等价的语法分析表条目ID，不合并这些条目
  /// 3.不会写入只有一个项的组
  /// 4.该函数是SyntaxAnalysisTableEntryClassify的子过程
  void SyntaxAnalysisTableNonTerminalNodeClassify(
      const std::vector<ProductionNodeId>& nonterminal_node_ids, size_t index,
      std::list<SyntaxAnalysisTableEntryId>&& syntax_analysis_table_entry_ids,
      std::vector<std::list<SyntaxAnalysisTableEntryId>>* equivalent_ids);
  /// @brief 分类等价的语法分析表条目
  /// @param[in] operator_node_ids ：所有运算符节点ID
  /// @param[in] terminal_node_ids ：所有终结节点ID
  /// @param[in] nonterminal_node_ids ：所有非终结节点ID
  /// @return 返回可以合并的语法分析表条目组，所有组均有至少两个条目
  /// @details SyntaxAnalysisTableMergeOptimize的子过程
  std::vector<std::list<SyntaxAnalysisTableEntryId>>
  SyntaxAnalysisTableEntryClassify(
      std::vector<ProductionNodeId>&& operator_node_ids,
      std::vector<ProductionNodeId>&& terminal_node_ids,
      std::vector<ProductionNodeId>&& nonterminal_node_ids);
  /// @brief 重映射语法分析表内使用的ID
  /// @param[in] old_id_to_new_id ：旧ID到新ID的映射
  /// @note old_id_to_new_id仅存储需要修改的ID，不改变的ID无需存储
  void RemapSyntaxAnalysisTableEntryId(
      const std::unordered_map<SyntaxAnalysisTableEntryId,
                               SyntaxAnalysisTableEntryId>& old_id_to_new_id);
  /// @brief 将项集输出为图片
  /// @param[in] root_production_set_id ：根项集ID
  /// @param[in] image_output_path ：图片输出路径（不含文件名， 以'/'结尾）
  void FormatProductionItemSetToImageGraphivz(
      ProductionItemSetId root_production_item_set_id,
      const std::string& image_output_path = "./");
  /// @brief 将项集输出为markdown描述的图表
  /// @param[in] root_production_set_id ：根项集ID
  /// @param[in] image_output_path ：文件输出路径（不含文件名， 以'/'结尾）
  void FormatProductionItemSetToMarkdown(
      ProductionItemSetId root_production_item_set_id,
      const std::string& image_output_path = "./");
  /// @brief 合并语法分析表内等价条目以缩减语法分析表大小
  void SyntaxAnalysisTableMergeOptimize();

  /// @brief 将语法分析表配置写入文件
  /// @param[in] config_file_output_path
  /// ：配置文件输出路径（不含文件名，以'/'结尾）
  /// @details 在指定路径处输出语法分析表和词法分析表，
  /// 语法分析表配置文件名为frontend::common::kSyntaxConfigFileName，
  /// 词法分析表配置文件名为frontend::common::kDfaConfigFileName
  void SaveConfig(const std::string& config_file_output_path = "./") const;

  /// @brief 格式化产生式
  /// @param[in] nonterminal_node_id ：非终结产生式ID
  /// @param[in] production_body_id ：非终结产生式体ID
  /// @return 返回格式化后的字符串
  /// @details 格式化nonterminal_node_id和production_body_id指定的产生式
  /// @note 返回值例： IdOrEquivence -> IdOrEquivence [ Num ]
  std::string FormatSingleProductionBody(
      ProductionNodeId nonterminal_node_id,
      ProductionBodyId production_body_id) const;
  /// @brief 格式化项
  /// @param[in] production_item ：待格式化的项
  /// @return 返回格式化后的字符串
  /// @details
  /// 返回值例：IdOrEquivence -> IdOrEquivence · [ Num ]
  /// ·右侧为下一个移入的产生式
  std::string FormatProductionItem(const ProductionItem& production_item) const;
  /// @brief 格式化向前看符号集
  /// @param[in] look_forward_node_ids ：待格式化的向前看符号集
  /// @return 返回格式化后的字符串
  /// @details
  /// 1.向前看符号间通过空格分隔
  /// 2.符号两边不加双引号
  /// 3.最后一个符号后无空格
  /// @note 返回值例：const Id * ( [ )
  std::string FormatLookForwardSymbols(
      const ForwardNodesContainer& look_forward_node_ids) const;
  /// @brief 格式化项和项的向前看符号
  /// @param[in] production_item ：项
  /// @param[in] look_forward_node_ids ：项的向前看符号
  /// @return 返回格式化后的字符串
  /// @details
  /// 1.格式化给定项后接" 向前看符号："后接全部向前看符号
  /// 2.给定项格式同FormatProductionItem
  /// 3.向前看符号格式同FormatLookForwardSymbols
  /// 4.返回值例：StructType -> · StructureDefine 向前看符号：const Id * ( [ )
  std::string FormatProductionItemAndLookForwardSymbols(
      const ProductionItem& production_item,
      const ForwardNodesContainer& look_forward_node_ids) const;
  /// @brief 格式化项集中全部项及项对应的向前看符号
  /// @param[in] production_item_set_id ：项集ID
  /// @return 返回格式化后的字符串
  /// @details
  /// 1.产生式格式同FormatProductionItem
  /// 2.项与项之间通过line_feed分隔
  /// 3.最后一项后无line_feed
  /// 4.返回值例（line_feed为"\n"）：
  /// StructureDefineHead -> union · 向前看符号：{
  /// StructureAnnounce -> union · Id 向前看符号：const , Id * ( { [ )
  std::string FormatProductionItems(ProductionItemSetId production_item_set_id,
                                    std::string line_feed = "\n") const;
  /// @brief 对graphviz中label的特殊字符转义
  static std::string EscapeLabelSpecialCharacter(const std::string& source);
  /// @brief 输出Info级诊断信息
  /// @param[in] info ：Info级诊断信息
  /// @note 输出后自动换行
  static void OutPutInfo(const std::string& info) {
     //std::cout << std::format("SyntaxGenerator Info: ") << info << std::endl;
  }
  /// @brief 输出Warning级诊断信息
  /// @param[in] warning ：Warning级诊断信息
  /// @note 输出后自动换行
  static void OutPutWarning(const std::string& warning) {
    std::cerr << std::format("SyntaxGenerator Warning: ") << warning
              << std::endl;
  }
  /// @brief 输出Error级诊断信息
  /// @param[in] error ：Error级诊断信息
  /// @note 输出后自动换行
  static void OutPutError(const std::string& error) {
    std::cerr << std::format("SyntaxGenerator Error: ") << error << std::endl;
  }

  /// @brief 允许序列化类访问
  friend class boost::serialization::access;
  /// @brief boost-serialization用来保存语法分析表配置的函数
  /// @param[in,out] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  template <class Archive>
  void save(Archive& ar, const unsigned int version) const;
  /// 将序列化分为保存与加载，Generator仅保存配置，不加载
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  /// @class SyntaxAnalysisTableEntryIdAndProcessFunctionClassIdHasher
  /// syntax_generator.h
  /// @brief 哈希语法分析表条目ID与规约函数ID的类
  /// @note
  /// 用于SyntaxAnalysisTableTerminalNodeClassify中分类同时支持规约与移入的数据
  struct SyntaxAnalysisTableEntryIdAndProcessFunctionClassIdHasher {
    size_t operator()(
        const std::pair<SyntaxAnalysisTableEntryId, ProcessFunctionClassId>&
            data_to_hash) const {
      return data_to_hash.first * data_to_hash.second;
    }
  };

  /// @brief 存储由于未定义产生式而推迟添加的产生式数据
  /// @details
  /// 键是未定义的产生式名
  /// tuple内的std::string是非终结产生式名
  /// std::tuple<std::string, std::vector<std::string>,ProcessFunctionClassId>
  /// 为待添加的产生式体信息
  /// ProcessFunctionClassId是包装规约函数的类的实例化对象ID
  std::unordered_multimap<
      std::string,
      std::tuple<std::string, std::vector<std::string>, ProcessFunctionClassId>>
      undefined_productions_;
  /// @brief 存储产生式节点
  ObjectManager<BaseProductionNode> manager_nodes_;
  /// @brief 存储产生式名（终结/非终结/运算符）
  UnorderedStructManager<std::string, std::hash<std::string>>
      manager_node_symbol_;
  /// @brief 存储终结节点产生式体的符号，用来防止多次添加同一正则
  UnorderedStructManager<std::string, std::hash<std::string>>
      manager_terminal_body_symbol_;
  /// @brief 产生式名ID到对应产生式节点的映射
  std::unordered_map<SymbolId, ProductionNodeId> node_symbol_id_to_node_id_;
  /// @brief 终结产生式体符号ID到对应节点ID的映射
  std::unordered_map<SymbolId, ProductionNodeId>
      production_body_symbol_id_to_node_id_;
  /// @brief 存储项集
  ObjectManager<ProductionItemSet> production_item_sets_;
  /// @brief 存储语法分析表条目ID到项集ID的映射
  std::unordered_map<SyntaxAnalysisTableEntryId, ProductionItemSetId>
      syntax_analysis_table_entry_id_to_production_item_set_id_;
  /// @brief 用户定义的根非终结产生式节点ID
  ProductionNodeId root_production_node_id_ = ProductionNodeId::InvalidId();
  /// @brief 初始语法分析表条目ID，配置写入文件
  SyntaxAnalysisTableEntryId root_syntax_analysis_table_entry_id_;
  /// @brief 语法分析表，配置写入文件
  SyntaxAnalysisTableType syntax_analysis_table_;
  /// @brief 包装规约函数的类的实例化对象，配置写入文件
  /// @details
  /// 每个规约数据都必须关联唯一的包装规约函数的实例化对象，不允许重用对象
  ProcessFunctionClassManagerType manager_process_function_class_;
  /// @brief DFA配置生成器，配置写入文件
  frontend::generator::dfa_generator::DfaGenerator dfa_generator_;
};

template <class IdType>
inline ProductionBodyId NonTerminalProductionNode::AddBody(
    IdType&& body, ProcessFunctionClassId class_for_reduct_id) {
  ProductionBodyId body_id(nonterminal_bodys_.size());
  // 将输入插入到产生式体向量中，无删除相同产生式功能
  nonterminal_bodys_.emplace_back(std::forward<IdType>(body),
                                  class_for_reduct_id);
  return body_id;
}

template <class ProcessFunctionClass>
inline ProductionNodeId SyntaxGenerator::AddNonTerminalProduction(
    std::string node_symbol, std::string subnode_symbols) {
  // 受#__VA_ARGS__规则所限，只能将__VA_ARGS__代表的所有内容转为一个字符串，需要手动分割
  std::vector<std::string> splited_subnode_symbols(1);
  for (char c : subnode_symbols) {
    if (c == ',') {
      // 当前字符串结束，准备处理下一个字符串
      splited_subnode_symbols.emplace_back();
    } else if (!isblank(c)) {
        // [a-zA-Z_][a-zA-Z0-9_]*
      splited_subnode_symbols.back().push_back(c);
    }
  }

  ProcessFunctionClassId class_id =
      CreateProcessFunctionClassObject<ProcessFunctionClass>();
  return AddNonTerminalProduction(std::move(node_symbol),
                                  std::move(splited_subnode_symbols), class_id);
}

template <class ForwardNodeIdContainer>
inline std::pair<
    SyntaxGenerator::ProductionItemAndForwardNodesContainer::iterator, bool>
SyntaxGenerator::AddItemAndForwardNodeIdsToProductionItem(
    ProductionItemSetId production_item_set_id,
    const ProductionItem& production_item,
    ForwardNodeIdContainer&& forward_node_ids) {
  assert(production_item_set_id.IsValid());
  auto result = GetProductionItemSet(production_item_set_id)
                    .AddItemAndForwardNodeIds(
                        production_item,
                        std::forward<ForwardNodeIdContainer>(forward_node_ids));
  return result;
}

template <class ForwardNodeIdContainer>
inline std::pair<
    SyntaxGenerator::ProductionItemAndForwardNodesContainer::iterator, bool>
SyntaxGenerator::AddMainItemAndForwardNodeIdsToProductionItem(
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

#endif  /// !GENERATOR_SYNTAXGENERATOR_SYNTAXGENERATOR_H_