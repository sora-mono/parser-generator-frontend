/// @file c_parser_frontend.h
/// @brief C语言编译器前端控制器
/// @details
/// CParserFrontend管理所有供规约函数调用的接口，整合TypeSystem、
/// ActionScopeSystem和FlowControlSystem功能，提供上层接口
#ifndef CPARSERFRONTEND_C_PARSER_FRONTEND_H_
#define CPARSERFRONTEND_C_PARSER_FRONTEND_H_

#include <iostream>
#include <unordered_map>

#include "Parser/line_and_column.h"
#include "action_scope_system.h"
#include "flow_control.h"
#include "operator_node.h"
#include "type_system.h"

namespace c_parser_frontend {
/// @brief 获取当前行数
/// @note 从0开始计算
inline size_t GetLine() { return frontend::parser::GetLine(); }
/// @brief 获取当前列数
/// @note 从0开始计算
inline size_t GetColumn() { return frontend::parser::GetColumn(); }

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
  /// @brief 定义类型
  /// @param[in] type_name ：类型名
  /// @param[in] type_pointer ：类型链头结点指针
  /// @return 前半部分为指向添加位置的迭代器，后半部分为添加结果
  /// @note
  /// 函数的声明与定义均走该接口，仅存在声明时可以多次声明，定义可以覆盖声明
  /// 已存在定义则返回AddTypeResult::kAlreadyIn
  template <class TypeName>
  std::pair<TypeSystem::TypeNodeContainerIter, AddTypeResult> DefineType(
      TypeName&& type_name,
      const std::shared_ptr<const TypeInterface>& type_pointer) {
    return type_system_.DefineType(std::forward<TypeName>(type_name),
                                   type_pointer);
  }
  /// @brief 根据类型名获取类型链
  /// @param[in] type_name ：类型名
  /// @param[in] type_prefer ：类型选择倾向
  /// @return 前半部分为指向获取到的类型链头结点指针，后半部分为获取结果
  /// @ref c_parser_frontend::type_system::TypeSystem::GetType
  std::pair<std::shared_ptr<const TypeInterface>, GetTypeResult> GetType(
      const std::string& type_name, StructOrBasicType type_prefer) {
    return type_system_.GetType(type_name, type_prefer);
  }
  /// @brief 声明函数使用该接口
  /// @param[in] function_type ：待声明的函数类型
  /// @return 前半部分为指向添加位置的迭代器，后半部分为添加结果
  /// @note 声明函数不会将该函数设置为当前活跃函数
  std::pair<TypeSystem::TypeNodeContainerIter, AddTypeResult> AnnounceFunction(
      const std::shared_ptr<const FunctionType>& function_type);
  /// @brief 定义变量
  /// @param[in] operator_node ：变量节点
  /// @return 前半部分为指向插入位置的迭代器，后半部分为添加结果
  /// @note operator_node必须已经设置变量名
  std::pair<ActionScopeSystem::ActionScopeContainerType::const_iterator,
            DefineVarietyResult>
  DefineVariety(
      const std::shared_ptr<const VarietyOperatorNode>& operator_node) {
    return action_scope_system_.DefineVariety(operator_node);
  }
  /// @brief 根据名字获取变量或函数名
  /// @param[in] variety_name ：查询的名字
  /// @return 前半部分为指向变量或函数节点的指针，后半部分为节点是否存在
  std::pair<std::shared_ptr<const operator_node::OperatorNodeInterface>, bool>
  GetVarietyOrFunction(const std::string& variety_name) const {
    return action_scope_system_.GetVarietyOrFunction(variety_name);
  }
  /// @brief 增加一级作用域等级
  void AddActionScopeLevel() { action_scope_system_.AddActionScopeLevel(); }
  /// @brief 弹出一级作用域
  /// @details
  /// 自动弹出失效作用域内所有的变量和流程控制语句，并向上级语句添加弹出的
  /// （构建完成的）流程控制语句
  /// @note 结束函数构建时仅需调用该函数，无需调用其它函数
  void PopActionScope() { action_scope_system_.PopActionScope(); }
  size_t GetActionScopeLevel() const {
    return action_scope_system_.GetActionScopeLevel();
  }
  /// @brief 设置当前待构建函数
  /// @param[in] function_to_construct ：待构建函数的类型
  /// @return 返回是否设置成功
  /// @retval true ：设置成功
  /// @retval false ：存在正在构建的函数，不允许嵌套定义函数或重定义/重声明函数
  /// @details
  /// 在作用域内声明函数的全部参数和函数类型的初始化数据且自动提升1级作用域等级
  /// @attention 禁止使用PushFlowControlSentence设置待构建函数
  bool SetFunctionToConstruct(
      const std::shared_ptr<const FunctionType>& function_to_construct);
  /// @brief 将流程控制语句压入栈
  /// @param[in] flow_control_sentence ：待入栈的控制语句
  /// @return 返回是否添加成功
  /// @retval true ：添加成功
  /// @retval false ：无正在构建的函数，无法压入流程控制节点
  /// @attention 必须压入可以存储流程控制节点的流程控制语句
  bool PushFlowControlSentence(
      std::unique_ptr<FlowInterface>&& flow_control_sentence);
  /// @brief 获取最顶层的控制语句
  /// @return 返回流程控制语句的引用
  /// @note 用于完善do-while语句尾部和for语句头部
  /// @attention 必须存在正在构建的流程控制语句或函数
  FlowInterface& GetTopFlowControlSentence() {
    return action_scope_system_.GetTopFlowControlSentence();
  }
  /// @brief 获取当前活跃函数（正在构建的函数）
  /// @return 返回指向当前活跃函数的const指针
  /// @retval nullptr ：当前没有活跃函数
  const FunctionDefine* GetActiveFunctionPointer() const {
    return flow_control_system_.GetActiveFunctionPointer();
  }
  /// @brief 获取当前活跃函数（正在构建的函数）
  /// @return 返回当前活跃函数的const引用
  /// @note 必须存在活跃函数
  const FunctionDefine& GetActiveFunctionReference() const {
    return flow_control_system_.GetActiveFunctionReference();
  }
  /// @brief 向顶层流程控制语句内添加语句
  /// @param[in] sentence ：待添加的语句
  /// @return 返回是否添加成功
  /// @retval true ：添加成功，夺取sentence控制权
  /// @retval false
  /// ：无活跃的流程控制语句或给定语句无法添加到当前的流程控制节点中，不修改参数
  /// @note 添加到已有的语句后
  bool AddSentence(std::unique_ptr<FlowInterface>&& sentence) {
    return action_scope_system_.AddSentence(std::move(sentence));
  }
  /// @brief 向当前活跃的流程控制语句内添加多条语句
  /// @param[in] sentence_container ：存储待添加的语句的容器
  /// @return 返回是否添加成功
  /// @retval true
  /// ：添加成功，将sentence_container中所有语句移动到活跃流程控制语句的主容器中
  /// @retval false ：给定语句无法添加到当前的流程控制节点中，不修改参数
  /// @note 按begin->end的顺序添加，添加到已有的流程语句后
  bool AddSentences(
      std::list<std::unique_ptr<FlowInterface> >&& sentence_container) {
    return action_scope_system_.AddSentences(std::move(sentence_container));
  }

  /// @brief 将顶层流程控制语句从if语句转换为if-else语句
  /// @note 转换后执行AddSentence系列函数时添加语句到假分支中
  /// @attention 顶层流程控制语句必须是if语句
  void ConvertIfSentenceToIfElseSentence() {
    action_scope_system_.ConvertIfSentenceToIfElseSentence();
  }
  /// @brief 添加switch普通case
  /// @param[in] case_value ：分支的值
  /// @return 返回是否添加成功
  /// @retval true ：添加成功
  /// @retval false ：待添加的值已存在或顶层流程控制语句不为switch
  bool AddSwitchSimpleCase(
      const std::shared_ptr<
          c_parser_frontend::flow_control::BasicTypeInitializeOperatorNode>&
          case_value) {
    return action_scope_system_.AddSwitchSimpleCase(case_value);
  }
  /// @brief 添加switch的default分支标签
  /// @return 返回是否添加成功
  /// @retval true ：添加成功
  /// @retval false ：已经添加过default标签或顶层流程控制语句不为switch
  bool AddSwitchDefaultCase() {
    return action_scope_system_.AddSwitchDefaultCase();
  }

 private:
  /// @brief 便于该类调用FinishFunctionConstruct
  friend ActionScopeSystem;

  /// @brief 做一些完成函数构建后的清理操作
  void FinishFunctionConstruct() {
    // 无论是否声明过函数，调用该函数后都会保证该函数类型存在
    type_system_.AnnounceFunctionType(
        flow_control_system_.GetActiveFunctionReference()
            .GetFunctionTypePointer());
    flow_control_system_.FinishFunctionConstruct();
  }

  /// @brief 类型系统
  TypeSystem type_system_;
  /// @brief 作用域系统
  ActionScopeSystem action_scope_system_;
  /// @brief 流程控制系统
  FlowControlSystem flow_control_system_;
};

/// @brief 全局CParserFrontend变量
/// @details
/// 用于子系统交互和用户访问
/// 线程间独立
extern thread_local CParserFrontend c_parser_frontend;

}  // namespace c_parser_frontend

#endif  /// !CPARSERFRONTEND_C_PARSER_FRONTEND_H_
