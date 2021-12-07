/// @file syntax_parser.h
/// @brief 语法分析机
#ifndef PARSER_SYNTAXPARSER_SYNTAXPARSER_H_
#define PARSER_SYNTAXPARSER_SYNTAXPARSER_H_
#include <queue>
#include <stack>

#include "Common/object_manager.h"
#include "Generator/SyntaxGenerator/process_function_interface.h"
#include "Generator/SyntaxGenerator/syntax_analysis_table.h"
#include "Generator/export_types.h"
#include "Parser/DfaParser/dfa_parser.h"

namespace frontend::parser::syntax_parser {

/// @class SyntaxParser syntax_parser.h
/// @brief 语法分析机
class SyntaxParser {
  using DfaParser = frontend::parser::dfa_parser::DfaParser;

 public:
  /// @brief DFA引擎返回的单词信息
  using WordInfo = DfaParser::WordInfo;
  /// @brief 语法分析表条目ID
  using SyntaxAnalysisTableEntryId =
      frontend::generator::syntax_generator::SyntaxAnalysisTableEntryId;
  /// @brief 语法分析表
  using SyntaxAnalysisTableType =
      frontend::generator::syntax_generator::SyntaxAnalysisTableType;
  /// @brief 产生式ID
  using ProductionNodeId =
      frontend::generator::syntax_generator::ProductionNodeId;
  /// @brief 产生式节点类型
  using ProductionNodeType =
      frontend::generator::syntax_generator::ProductionNodeType;
  /// @brief 运算符结合性
  using OperatorAssociatityType =
      frontend::generator::syntax_generator::OperatorAssociatityType;
  /// @brief 包装规约函数的类的实例化对象管理器
  using ProcessFunctionClassManagerType =
      frontend::generator::syntax_generator::ProcessFunctionClassManagerType;
  /// @brief 包装规约函数的类的实例化对象ID
  using ProcessFunctionClassId =
      frontend::generator::syntax_generator::ProcessFunctionClassId;
  /// @brief 包装规约函数的类的基类
  using ProcessFunctionInterface =
      frontend::generator::syntax_generator::ProcessFunctionInterface;
  /// @brief 动作和附属数据的基类
  using ActionAndAttachedDataInterface = frontend::generator::syntax_generator::
      SyntaxAnalysisTableEntry::ActionAndAttachedDataInterface;
  /// @brief 面对向前看符号时的动作
  using ActionType = frontend::generator::syntax_generator::ActionType;
  /// @brief 移入动作的数据
  using ShiftAttachedData = frontend::generator::syntax_generator::
      SyntaxAnalysisTableEntry::ShiftAttachedData;
  /// @brief 归约动作的数据
  using ReductAttachedData = frontend::generator::syntax_generator::
      SyntaxAnalysisTableEntry::ReductAttachedData;
  /// @brief 传递给用户的单个单词的数据
  using WordDataToUser = ProcessFunctionInterface::WordDataToUser;
  /// @brief 终结节点数据
  using TerminalWordData = ProcessFunctionInterface::TerminalWordData;
  /// @brief 非终结节点数据
  using NonTerminalWordData = ProcessFunctionInterface::NonTerminalWordData;
  /// @brief
  /// 运算符优先级，等于已移入的最高优先级运算符优先级，0保留为非运算符优先级
  using OperatorPriority =
      frontend::generator::syntax_generator::OperatorPriority;

  /// @class ParsingData syntax_parser.h
  /// @brief 解析时使用的数据
  struct ParsingData {
    /// @brief 当前语法分析表条目ID
    SyntaxAnalysisTableEntryId syntax_analysis_table_entry_id;
    /// @brief 在syntax_analysis_table_entry_id条目的基础上移入的产生式节点的ID
    /// @note 提供该项为了支持空规约功能
    ProductionNodeId shift_node_id = ProductionNodeId::InvalidId();
    /// @brief 移入的终结节点数据或非终结节点规约后用户返回的数据
    WordDataToUser word_data_to_user;
    /// @brief 非运算符优先级为0
    OperatorPriority operator_priority = OperatorPriority(0);
  };

  SyntaxParser() { LoadConfig(); }
  SyntaxParser(const SyntaxParser&) = delete;
  SyntaxParser& operator=(SyntaxParser&&) = delete;

  /// @brief 加载配置
  void LoadConfig();
  /// @brief 设置DFA返回的待移入单词数据
  /// @param[in] dfa_return_data ：DFA返回的待移入单词数据
  void SetDfaReturnData(WordInfo&& dfa_return_data) {
    dfa_return_data_ = std::move(dfa_return_data);
  }
  /// @brief 获取DFA返回的待移入单词的数据
  WordInfo& GetWaitingProcessWordInfo() { return dfa_return_data_; }
  /// @brief 获取下一个单词的数据并存在dfa_return_data_中
  void GetNextWord() { SetDfaReturnData(dfa_parser_.GetNextWord()); }
  /// @brief 获取根语法分析表条目D
  /// @return 返回根语法分析表条目ID
  SyntaxAnalysisTableEntryId GetRootParsingEntryId() const {
    return root_parsing_entry_id_;
  }
  /// @brief 获取包装规约函数的类的实例化对象
  /// @return 返回包装规约函数的类的实例化对象的const引用
  const ProcessFunctionInterface& GetProcessFunctionClass(
      ProcessFunctionClassId class_id) const {
    return manager_process_function_class_[class_id];
  }
  /// @brief 获取在给定向前看产生式节点条件下的动作和附属数据
  /// @param[in] src_entry_id ：起始语法分析表条目ID
  /// @param[in] node_id ：向前看产生式节点ID
  /// @return 返回指向动作和附属数据基类的const指针
  /// @retval nullptr 不存在该转移条件则返回空指针
  const ActionAndAttachedDataInterface* GetActionAndTarget(
      SyntaxAnalysisTableEntryId src_entry_id, ProductionNodeId node_id) const {
    assert(src_entry_id.IsValid());
    return syntax_analysis_table_[src_entry_id].AtTerminalNode(node_id);
  }
  /// @brief 获取移入非终结产生式节点后到达的产生式条目
  /// @param[in] src_entry_id ：起始语法分析表条目ID
  /// @param[in] node_id ：移入的非终结产生式节点
  /// @return 返回移入非终结产生式后转移到的语法分析表条目ID
  /// @return SyntaxAnalysisTableEntryId::InvalidId()
  /// 无法移入给定的非终结产生式节点ID
  SyntaxAnalysisTableEntryId GetNextEntryId(
      SyntaxAnalysisTableEntryId src_entry_id, ProductionNodeId node_id) const {
    assert(src_entry_id.IsValid());
    return syntax_analysis_table_[src_entry_id].AtNonTerminalNode(node_id);
  }
  /// @brief 获取当前活跃的解析数据（解析数据栈顶对象）
  /// @return 返回解析数据栈顶层对象的引用
  ParsingData& GetParsingDataNow() { return parsing_stack_.top(); }
  /// @brief 将数据压入解析数据栈
  /// @param[in] parsing_data ：解析数据
  /// @note 仅支持ParsingData类型的parsing_data作为参数
  template <class ParsingDataType>
  void PushParsingData(ParsingDataType&& parsing_data) {
    parsing_stack_.push(std::forward<ParsingDataType>(parsing_data));
  }
  /// @brief 弹出解析数据栈顶部数据
  void PopTopParsingData() { parsing_stack_.pop(); }
  /// @brief 查询解析数据栈数据数目
  /// @return 返回解析数据栈中数据数目
  size_t GetParsingStackSize() const { return parsing_stack_.size(); }
  /// @brief 分析代码文件并构建AST
  /// @param[in] filename ：代码文件名
  /// @return 解析是否成功
  /// @retval true 解析成功
  /// @retval false 无法打开文件/解析失败
  bool Parse(const std::string& filename);

 private:
  /// @brief 允许序列化类访问
  friend class boost::serialization::access;

  /// @brief boost-serialization加载语法分析配置的函数
  /// @param[in,out] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  template <class Archive>
  void load(Archive& ar, const unsigned int version) {
    // 转除const以允许序列化代码读取配置
    ar >> const_cast<SyntaxAnalysisTableEntryId&>(root_parsing_entry_id_);
    ar >> const_cast<SyntaxAnalysisTableType&>(syntax_analysis_table_);
    ar >> const_cast<ProcessFunctionClassManagerType&>(
              manager_process_function_class_);
  }
  /// 将序列化分为保存与加载，Parser仅加载配置，不保存
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  /// @brief 处理待移入单词是终结节点的情况
  /// @details
  /// 1.自动选择移入和归并
  /// 2.移入后执行GetNextWord()
  /// 3.归并后将得到的非终结节点移入
  void TerminalWordWaitingProcess();
  /// @brief 处理向前看符号待移入的情况
  /// @param[in] action_and_target ：移入动作和附属数据
  /// @note
  /// 1.处理后自动执行GetNextWord()
  /// 2.该函数为TerminalWordWaitingShift函数的子过程
  void ShiftTerminalWord(const ShiftAttachedData& action_and_target);
  /// @brief 处理产生式待规约的情况
  /// @param[in] action_and_target ：规约动作和附属数据
  /// @note
  /// 1.规约后自动移入得到的非终结节点并GetNextWord()
  /// 2.TerminalWordWaitingShift函数的子过程
  void Reduct(const ReductAttachedData& action_and_target);
  /// @brief 移入非终结节点
  /// @param[in] non_terminal_word_data ：规约后用户返回的数据
  /// @param[in] reducted_nonterminal_node_id ：规约后得到的非终结产生式ID
  void ShiftNonTerminalWord(NonTerminalWordData&& non_terminal_word_data,
                            ProductionNodeId reducted_nonterminal_node_id);
  /// @brief 设置上一次是规约操作
  /// @note
  /// 影响双语义运算符语义的选择，上一次为规约操作则使用左侧单目语义
  /// 否则使用双目语义
  void SetLastOperateIsReduct() { last_operate_is_reduct_ = true; }
  /// @brief 设置上一次操作不是规约操作
  /// @note
  /// 影响双语义运算符语义的选择，上一次为规约操作则使用左侧单目语义
  /// 否则使用双目语义
  void SetLastOperateIsNotReduct() { last_operate_is_reduct_ = false; }
  /// @brief 判断上一次操作是否为规约操作
  /// @return 返回上一次操作是否为规约操作
  /// @retval true 上一次操作是规约操作
  /// @retval false 上一次操作不是规约操作
  bool LastOperateIsReduct() const { return last_operate_is_reduct_; }

  /// @brief DFA分析机
  DfaParser dfa_parser_;
  /// @brief 根分析表条目ID，只有加载配置时可以修改
  const SyntaxAnalysisTableEntryId root_parsing_entry_id_;
  /// @brief 包装规约函数的类的实例化对象的管理器，只有加载配置时可以修改
  const ProcessFunctionClassManagerType manager_process_function_class_;
  /// @brief 语法分析表，只有加载配置时可以修改
  const SyntaxAnalysisTableType syntax_analysis_table_;

  /// @brief DFA返回的数据
  WordInfo dfa_return_data_;
  /// @brief 解析用数据栈，栈顶为当前解析数据
  std::stack<ParsingData> parsing_stack_;
  /// @brief 标记上次操作是否为规约操作
  /// @note
  /// 用来支持运算符优先级时同一个运算符可以细分为左侧单目运算符和双目运算符功能
  bool last_operate_is_reduct_ = true;
};

}  // namespace frontend::parser::syntax_parser
#endif  /// !PARSER_SYNTAXMACHINE_SYNTAXMACHINE_H_