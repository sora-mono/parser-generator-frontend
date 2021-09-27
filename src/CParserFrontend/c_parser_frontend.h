#ifndef CPARSERFRONTEND_C_PARSER_FRONTEND_H_
#define CPARSERFRONTEND_C_PARSER_FRONTEND_H_

#include <unordered_map>

#include "Generator/SyntaxGenerator/process_function_interface.h"
#include "Parser/DfaMachine/dfa_machine.h"
#include "action_scope_system.h"
#include "flow_control.h"
#include "operator_node.h"
#include "type_system.h"

// 引用解析到的行数和列数
namespace frontend::parser {
extern thread_local size_t line_;
extern thread_local size_t column_;
}  // namespace frontend::parser

namespace c_parser_frontend {
// 获取当前行数
size_t GetLine();
// 获取当前列数
size_t GetColumn();

class CParserFrontend {
  // 引用一些类型的定义
  using DefineVarietyResult =
      c_parser_frontend::action_scope_system::DefineVarietyResult;
  using ActionScopeSystem =
      c_parser_frontend::action_scope_system::ActionScopeSystem;
  using FunctionDefine = c_parser_frontend::flow_control::FunctionDefine;
  using VarietyOperatorNode =
      c_parser_frontend::operator_node::VarietyOperatorNode;
  using StructOrBasicType = c_parser_frontend::type_system::StructOrBasicType;
  using TypeInterface = c_parser_frontend::type_system::TypeInterface;
  using TypeSystem = c_parser_frontend::type_system::TypeSystem;
  using AddTypeResult = c_parser_frontend::type_system::AddTypeResult;
  using GetTypeResult = c_parser_frontend::type_system::GetTypeResult;
  using FlowControlSystem = c_parser_frontend::flow_control::FlowControlSystem;
  using FunctionType = c_parser_frontend::type_system::FunctionType;
  using FlowInterface = c_parser_frontend::flow_control::FlowInterface;

 public:
  enum class AddFunctionDefineResult {
    // 可以添加的情况
    kSuccess,  // 成功添加
    // 不可以添加的情况
    kReDefine,  // 重定义函数
    kOverLoad   // 重载函数
  };

  // 添加类型
  // 返回指向添加位置的迭代器和添加结果
  // 函数的声明与定义均走该接口，仅存在声明时可以多次声明，定义可以覆盖声明
  // 已存在定义则返回AddTypeResult::kAlreadyIn
  template <class TypeName>
  std::pair<TypeSystem::TypeNodeContainerIter, AddTypeResult> DefineType(
      TypeName&& type_name,
      const std::shared_ptr<const TypeInterface>& type_pointer) {
    return type_system_.DefineType(std::forward<TypeName>(type_name),
                                   type_pointer);
  }
  // 声明类型，保证迭代器有效，管理节点已创建
  template <class TypeName>
  TypeSystem::TypeNodeContainerIter AnnounceTypeName(TypeName&& type_name) {
    return type_system_.AnnounceTypeName(std::forward<TypeName>(type_name));
  }
  std::pair<std::shared_ptr<const TypeInterface>, GetTypeResult> GetType(
      const std::string& type_name, StructOrBasicType type_prefer) {
    return type_system_.GetType(type_name, type_prefer);
  }
  // 声明函数类型使用该接口
  std::pair<TypeSystem::TypeNodeContainerIter, AddTypeResult>
  AnnounceFunctionType(
      const std::shared_ptr<const FunctionType>& function_type) {
    return type_system_.AnnounceFunctionType(function_type);
  }
  // 定义函数类型使用该接口
  std::pair<TypeSystem::TypeNodeContainerIter, AddTypeResult>
  DefineFunctionType(const std::shared_ptr<const FunctionType>& function_type) {
    return type_system_.DefineFunctionType(function_type);
  }
  // 添加变量
  // 返回指向插入位置的迭代器与添加结果，添加结果意义见定义
  template <class VarietyName>
  std::pair<ActionScopeSystem::ActionScopeContainerType::const_iterator,
            DefineVarietyResult>
  DefineVariety(
      VarietyName&& variety_name,
      const std::shared_ptr<const VarietyOperatorNode>& operator_node) {
    return action_scope_system_.DefineVariety(
        std::forward<VarietyName>(variety_name), operator_node);
  }
  // 声明变量，返回指向插入位置的迭代器
  // 保证插入节点有效且管理节点存在
  template <class VarietyName>
  ActionScopeSystem::ActionScopeContainerType::const_iterator
  AnnounceVarietyName(VarietyName&& variety_name) {
    return action_scope_system_.AnnounceVarietyName(
        std::forward<VarietyName>(variety_name));
  }
  std::pair<std::shared_ptr<const VarietyOperatorNode>, bool> GetVariety(
      const std::string& variety_name) {
    return action_scope_system_.GetVariety(variety_name);
  }
  // 增加一级作用域等级
  void AddActionScopeLevel() { action_scope_system_.AddActionScopeLevel(); }
  // 弹出一级作用域，自动处理该作用域内所有的变量和流程控制语句
  // 结束函数构建时仅需调用该函数，无需调用其它函数
  void PopActionScope() { action_scope_system_.PopActionScope(); }
  size_t GetActionScopeLevel() const {
    return action_scope_system_.GetActionScopeLevel();
  }
  // 移除类型系统空节点， 节点不空则不移除
  // 返回是否移除了节点
  bool RemoveTypeSystemEmptyNode(const std::string& empty_node_to_remove_name) {
    return type_system_.RemoveEmptyNode(empty_node_to_remove_name);
  }
  // 移除变量系统空节点， 节点不空则不移除
  // 返回是否移除了节点
  bool RemoveVarietySystemEmptyNode(
      const std::string& empty_node_to_remove_name) {
    return action_scope_system_.RemoveEmptyNode(empty_node_to_remove_name);
  }
  // 设置当前待构建函数
  // 在作用域内声明函数的全部参数和函数类型的对象且自动提升作用域等级
  void SetFunctionToConstruct(
      const std::shared_ptr<FunctionType>& function_to_construct) {
    flow_control_system_.SetFunctionToConstruct(function_to_construct);
    action_scope_system_.SetFunctionToConstruct(function_to_construct);
  }
  // 将流程控制语句压入栈
  // 返回是否添加成功
  // 如果无法添加语句则输出错误信息
  void PushFlowControlSentence(
      std::unique_ptr<FlowInterface>&& flow_control_sentence);
  // 获取最顶层的控制语句
  // 用于完善do-while语句尾部和for语句头部
  FlowInterface& GetTopFlowControlSentence() {
    return action_scope_system_.GetTopFlowControlSentence();
  }
  // 向当前活跃的函数内执行的语句尾部附加语句
  // 返回是否添加成功，添加失败则不修改参数
  // 如果当前无流程控制语句则返回false
  bool AddSentence(std::unique_ptr<FlowInterface>&& sentence) {
    return action_scope_system_.AddSentence(std::move(sentence));
  }
  // 添加待执行语句的集合
  // 返回是否添加成功，添加失败则不修改参数
  // 如果当前无流程控制语句则返回false
  bool AddSentences(
      std::list<std::unique_ptr<FlowInterface> >&& sentence_container) {
    return action_scope_system_.AddSentences(std::move(sentence_container));
  }
  bool ConvertIfSentenceToIfElseSentence() {
    return action_scope_system_.ConvertIfSentenceToIfElseSentence();
  }
  // 添加switch普通分支选项
  // 返回是否添加成功
  // 如果当前流程控制语句不为switch则返回false
  bool AddSwitchSimpleCase(
      const std::shared_ptr<
          c_parser_frontend::flow_control::BasicTypeInitializeOperatorNode>&
          case_value) {
    return action_scope_system_.AddSwitchSimpleCase(case_value);
  }
  // 添加switch默认分支标签
  // 返回是否添加成功
  // 如果当前流程控制语句不为switch则返回false
  bool AddSwitchDefaultCase() {
    return action_scope_system_.AddSwitchDefaultCase();
  }

 private:
  // 便于该类调用FinishFunctionConstruct
  friend ActionScopeSystem;

  // 做一些完成函数构建后的清理操作
  void FinishFunctionConstruct(
      const std::shared_ptr<c_parser_frontend::type_system::FunctionType>&
          function_type) {
    flow_control_system_.FinishFunctionConstruct();
    // 向类型系统中插入函数类型
    DefineFunctionType(function_type);
  }
  FunctionType& GetActiveFunctionReference() const {
    return flow_control_system_.GetActiveFunctionReference();
  }
  std::shared_ptr<FunctionType> GetActiveFunctionPointer() const {
    return flow_control_system_.GetActiveFunctionPointer();
  }
  // 类型系统
  TypeSystem type_system_;
  // 作用域系统
  ActionScopeSystem action_scope_system_;
  // 流程控制系统
  FlowControlSystem flow_control_system_;
};

extern thread_local CParserFrontend parser_frontend;

}  // namespace c_parser_frontend

#endif  // !CPARSERFRONTEND_C_PARSER_FRONTEND_H_
