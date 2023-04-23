/// @file flow_control.h
/// @brief 流程控制
/// @details
/// 本文件中定义流程控制节点，具体节点类型参考FlowType
#ifndef CPARSERFRONTEND_FLOW_CONTROL_H_
#define CPARSERFRONTEND_FLOW_CONTROL_H_

#include <format>
#include <list>
#include <memory>
#include <unordered_map>

#include "operator_node.h"
#include "type_system.h"

namespace c_parser_frontend::flow_control {
/// @brief 流程控制节点类型
enum class FlowType {
  kFunctionDefine,   ///< 函数定义
  kSimpleSentence,   ///< 基础非流程控制语句
  kLabel,            ///< 添加标签语句
  kJmp,              ///< 跳转语句
  kReturn,           ///< 返回语句
  kIfSentence,       ///< if语句
  kIfElseSentence,   ///< if-else语句
  kWhileSentence,    ///< while语句
  kDoWhileSentence,  ///< do-while语句
  kForSentence,      ///< for语句
  kSwitchSentence    ///< switch语句
};

// 引用操作节点和附属数据类型的定义
using c_parser_frontend::operator_node::AllocateOperatorNode;
using c_parser_frontend::operator_node::AssignOperatorNode;
using c_parser_frontend::operator_node::BasicTypeInitializeOperatorNode;
using c_parser_frontend::operator_node::DereferenceOperatorNode;
using c_parser_frontend::operator_node::FunctionCallOperatorNode;
using c_parser_frontend::operator_node::GeneralOperationType;
using c_parser_frontend::operator_node::InitializeOperatorNodeInterface;
using c_parser_frontend::operator_node::InitializeType;
using c_parser_frontend::operator_node::LogicalOperation;
using c_parser_frontend::operator_node::LogicalOperationOperatorNode;
using c_parser_frontend::operator_node::MathematicalOperatorNode;
using c_parser_frontend::operator_node::MemberAccessOperatorNode;
using c_parser_frontend::operator_node::OperatorNodeInterface;
using c_parser_frontend::operator_node::VarietyOperatorNode;
using c_parser_frontend::type_system::AssignableCheckResult;
using c_parser_frontend::type_system::BuiltInType;
using c_parser_frontend::type_system::CommonlyUsedTypeGenerator;
using c_parser_frontend::type_system::ConstTag;
using c_parser_frontend::type_system::FunctionType;
using c_parser_frontend::type_system::StructOrBasicType;
using c_parser_frontend::type_system::TypeInterface;

/// @class FlowInterface flow_control.h
/// @brief 流程控制节点基类
/// @details
/// 所有流程控制节点均从该类派生
class FlowInterface {
  /// @brief 流程节点ID的分发标签
  enum class IdWrapper { kFlowNodeId };

 public:
  /// @brief 流程节点ID
  using FlowId = frontend::common::ExplicitIdWrapper<size_t, IdWrapper,
                                                     IdWrapper::kFlowNodeId>;

  FlowInterface(FlowType flow_type) : flow_type_(flow_type){};
  virtual ~FlowInterface() {}

  /// @brief 向流程语句主体中中添加单条语句
  /// @param[in] sentence ：待添加的语句
  /// @return 返回添加是否成功
  /// @retval true ：添加成功
  /// @retval false ：sentence不能添加到流程控制语句中
  /// @details
  /// 1.对do-while/while/for调用该函数时添加到循环体中
  /// 2.对if语句调用时添加到if成立时执行的语句内
  /// 3.对if-else语句调用时添加到else成立时执行的语句内
  /// 4.对switch语句调用时添加到switch主体内
  /// @note 添加语句时添加到已有的语句后
  virtual bool AddMainSentence(std::unique_ptr<FlowInterface>&& sentence) = 0;
  /// @brief 向流程语句主体中中添加多条语句
  /// @param[in] sentences ：待添加的语句
  /// @return 返回添加是否成功
  /// @retval true ：添加成功
  /// @retval false ：sentences中含有不能添加到流程控制语句的语句
  /// @details
  /// 1.对do-while/while/for调用该函数时添加到循环体中
  /// 2.对if语句调用时添加到if成立时执行的语句内
  /// 3.对if-else语句调用时添加到else成立时执行的语句内
  /// 4.对switch语句调用时添加到switch主体内
  /// @note 添加语句时添加到已有的语句后
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) = 0;

  /// @brief 获取流程控制类型
  /// @return 返回流程控制类型
  FlowType GetFlowType() const { return flow_type_; }
  /// @brief 获取流程控制节点ID
  /// @return 返回流程控制节点ID
  /// @note 所有流程控制节点都有一个线程独立的ID
  FlowId GetFlowId() const { return flow_id_; }

 protected:
  /// @brief 设置流程控制类型
  void SetFlowType(FlowType flow_type) { flow_type_ = flow_type; }

 private:
  /// @brief 获取一个从未被分配过的流程控制节点ID
  /// @return 返回流程控制节点ID
  /// @note 流程控制节点线程独立
  static FlowId GetNewFlowId() {
    static thread_local FlowId flow_id(0);
    return flow_id++;
  }

  /// @brief 流程控制节点ID
  const FlowId flow_id_ = GetNewFlowId();
  /// @brief 流程控制节点类型
  FlowType flow_type_;
};

/// @class Label flow_control.h
/// @brief 标签
/// @details
/// 1.标签用于跳转语句，每个跳转语句都应绑定一个标签
/// 2.使用方法类似于汇编标签/goto语句跳转
/// 3.每个标签只有标签名这一成员
/// @note 相同标签名的标签视作一个标签
class Label : public FlowInterface {
 public:
  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    assert(false);
    /// 防止警告
    return false;
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    assert(false);
    /// 防止警告
    return false;
  }

  template <class LabelName>
  Label(LabelName&& label_name)
      : FlowInterface(FlowType::kLabel),
        label_name_(std::forward<LabelName>(label_name)) {}
  Label(const Label&) = delete;
  Label(Label&&) = delete;

  Label& operator=(const Label&) = default;
  Label& operator=(Label&&) = default;

  /// @brief 设置标签名
  /// @param[in] label_name ：标签名
  template <class LabelName>
  void SetLabelName(LabelName&& label_name) {
    label_name_ = std::forward<LabelName>(label_name);
  }
  /// @brief 获取标签名
  /// @return 返回标签名的const引用
  const std::string& GetLabelName() const { return label_name_; }

 private:
  /// @brief 标签名
  std::string label_name_;
};

/// @class Jmp flow_control.h
/// @brief 跳转
/// @details
/// 每个跳转语句都绑定跳转到的标签
class Jmp : public FlowInterface {
 public:
  Jmp(std::unique_ptr<Label>&& target_label)
      : FlowInterface(FlowType::kJmp), target_label_(std::move(target_label)) {}

  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    assert(false);
    /// 防止警告
    return false;
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    assert(false);
    /// 防止警告
    return false;
  }

  /// @brief 获取跳转语句绑定的标签
  /// @return 返回跳转语句绑定的标签的const引用
  const Label& GetTargetLabel() const { return *target_label_; }

 private:
  /// 跳转指令要跳到的标签
  std::unique_ptr<Label> target_label_;
};

/// @class FunctionDefine flow_control.h
/// @brief 函数定义
/// @attention 函数体不含执行语句时应插入一条return语句占位，否则视为函数声明
class FunctionDefine : public FlowInterface {
 public:
  FunctionDefine(const std::shared_ptr<const FunctionType>& function_type)
      : FlowInterface(FlowType::kFunctionDefine),
        function_type_(function_type) {}

  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    return AddSentences(std::move(sentences));
  }
  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    return AddSentence(std::move(sentence));
  }

  /// @brief 添加一条函数内执行的语句
  /// @param[in] sentence_to_add ：待添加的语句
  /// @return 返回添加是否成功
  /// @retval true 添加成功，夺取sentences_to_add控制权
  /// @retval false 不能添加，不修改入参
  /// @details
  /// 1.对语句的要求参考CheckSentenceInFunctionValid
  /// 2.添加到已添加的语句尾部
  bool AddSentence(std::unique_ptr<FlowInterface>&& sentence_to_add);
  /// @brief 添加一个容器内的全部语句
  /// @param[in] sentence_container ：存储待添加语句的容器
  /// @return 返回添加是否成功
  /// @retval true 成功添加，移动sentence_container中所有节点到函数执行语句容器
  /// @retval false 容器中存在非法语句，不能添加，不修改sentence_container
  /// @details
  /// 1.按照给定迭代器begin->end顺序添加
  /// 2.对语句的要求参考CheckSentenceInFunctionValid
  bool AddSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentence_container);
  /// @brief 获取函数体执行的语句
  /// @return 返回存储语句的容器的const引用
  const std::list<std::unique_ptr<FlowInterface>>& GetSentences() const {
    return sentences_in_function_;
  }
  /// @brief 检查是否为函数声明
  /// @return 返回是否为函数声明
  /// @details
  /// 函数声明中不存在任何流程控制语句
  /// @attention 空函数体内应包含一条return语句
  bool IsFunctionAnnounce() const { return GetSentences().empty(); }
  /// @brief 设置函数类型
  /// @param[in] function_type ：函数类型
  void SetFunctionType(
      const std::shared_ptr<const FunctionType>& function_type) {
    function_type_ = function_type;
  }
  /// @brief 获取函数类型
  /// @return 返回指向函数类型的const指针
  std::shared_ptr<const FunctionType> GetFunctionTypePointer() const {
    return function_type_;
  }
  /// @brief 获取函数类型
  /// @return 返回函数类型的const引用
  const FunctionType& GetFunctionTypeReference() const {
    return *function_type_;
  }

  /// @brief 检查给定语句是否可以作为函数内执行的语句
  /// @param[in] flow_interface ：待检查的语句
  /// @return 返回给定语句是否可以作为函数内执行的语句
  /// @retval true 允许在函数内执行
  /// @retval false 不允许在函数内执行
  /// @details
  /// 检查返回语句的返回值是否与函数定义的返回值匹配
  /// 不允许嵌套声明函数
  bool CheckSentenceInFunctionValid(const FlowInterface& flow_interface) const;

 private:
  /// @brief 函数的类型
  std::shared_ptr<const FunctionType> function_type_;
  /// @brief 函数体中执行的语句
  std::list<std::unique_ptr<FlowInterface>> sentences_in_function_;
};

/// @class Return flow_control.h
/// @brief 从函数中返回
/// @note 不检查返回的值，检查步骤在添加到函数体时执行
class Return : public FlowInterface {
 public:
  Return(const std::shared_ptr<const OperatorNodeInterface>& return_target =
             nullptr)
      : FlowInterface(FlowType::kReturn), return_target_(return_target) {}

  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    assert(false);
    /// 防止警告
    return false;
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    assert(false);
    /// 防止警告
    return false;
  }

  /// @brief 设置返回值
  /// @param[in] return_target ：返回值，不存在则使用nullptr
  void SetReturnValue(const std::shared_ptr<const OperatorNodeInterface>&
                          return_target = nullptr) {
    return_target_ = return_target;
  }
  /// @brief 获取返回的值
  /// @return 返回指向返回的值的const指针
  std::shared_ptr<const OperatorNodeInterface> GetReturnValuePointer() const {
    return return_target_;
  }
  /// @brief 获取返回的值
  /// @return 返回指向返回的值的const指针
  const OperatorNodeInterface& GetReturnValueReference() const {
    return *return_target_;
  }

 private:
  /// @brief 返回值，nullptr代表无返回值
  std::shared_ptr<const OperatorNodeInterface> return_target_;
};

/// @class ConditionBlockInterface flow_control.h
/// @brief 条件控制块基类（if、for、while、switch等）
/// @details
/// 1.控制块主体例：
///   if(condition) { // 主体 } else { // 主体 }
///   for(int i = 0;i < x;i++){ // 主体 }
/// 2.控制块条件指判断控制块是否执行/跳转方向的语句，如1中if语句的condition，
///   for中的i < x;
/// @attention 派生类不应修改接口语义且不可以任何方式重写或绕过接口
class ConditionBlockInterface : public FlowInterface {
 public:
  ConditionBlockInterface(FlowType flow_type) : FlowInterface(flow_type) {}

  /// @brief 设定条件控制语句的条件与获取条件的操作
  /// @param[in] condition ：条件节点
  /// @param[in] sentences_to_get_condition ：获取条件的操作
  /// @return 返回是否设置成功
  /// @retval true ：设置成功
  /// @retval false ：condition不能作为条件控制语句的条件
  /// @details
  /// 1.无法设置则不会设置
  /// 2.成功设置后夺取condition的控制权
  /// 3.不能设置则不修改condition
  /// 4.对条件节点的要求参考DefaultConditionCheck
  bool SetCondition(
      const std::shared_ptr<const OperatorNodeInterface>& condition,
      std::list<std::unique_ptr<FlowInterface>>&& sentences_to_get_condition);
  /// @brief 获取得到条件节点的操作
  /// @return 返回存储操作的容器的const引用
  const std::list<std::unique_ptr<FlowInterface>>& GetSentenceToGetCondition()
      const {
    return sentence_to_get_condition_;
  }
  /// @brief 获取条件节点
  /// @return 返回指向条件节点的const指针
  std::shared_ptr<const OperatorNodeInterface> GetConditionPointer() const {
    return condition_;
  }
  /// @brief 获取条件节点
  /// @return 返回条件节点的const引用
  const OperatorNodeInterface& GetConditionReference() const {
    return *condition_;
  }
  /// @brief 向主体内添加一条语句
  /// @param[in] sentence ：待添加的语句
  /// @return 返回是否添加成功
  /// @retval true ：添加成功
  /// @retval false ：该语句不能添加到条件控制语句主体中
  /// @details
  /// 1.无法添加则不修改sentence
  /// 2.成功添加则夺取sentence控制权
  /// 3.所有语句添加到已有的语句后
  /// 4.对语句的要求参考DefaultMainBlockSentenceCheck
  bool AddSentence(std::unique_ptr<FlowInterface>&& sentence);
  /// @brief 向主体内添加多条语句
  /// @param[in] sentences ：存储待添加的语句的容器
  /// @return 返回是否添加成功
  /// @retval true ：添加成功
  /// @retval false ：该语句不能添加到条件控制语句主体中
  /// @details
  /// 1.无法添加则不修改sentence
  /// 2.成功添加则夺取sentence控制权
  /// 3.所有语句添加到已有的语句后
  /// 4.按begin->end的顺序添加容器内的语句
  /// 5.对语句的要求参考DefaultMainBlockSentenceCheck
  bool AddSentences(std::list<std::unique_ptr<FlowInterface>>&& sentences);
  /// @brief 获取所有已添加的语句
  /// @return 返回存储语句的容器的const引用
  const auto& GetSentences() const { return main_block_; }
  /// @brief 获取块起始处标签
  /// @return 返回块起始处标签
  std::unique_ptr<Label> GetSentenceStartLabel() const {
    return std::make_unique<Label>(
        std::format("condition_block_{:}_start", GetFlowId().GetRawValue()));
  }
  /// @brief 获取块结束处标签
  /// @return 返回块结束处标签
  std::unique_ptr<Label> GetSentenceEndLabel() const {
    return std::make_unique<Label>(
        std::format("condition_block_{:}_end", GetFlowId().GetRawValue()));
  }

  /// @brief 默认对循环条件节点进行类型检查的函数
  /// @param[in] condition_node ：待检查的条件节点
  /// @return 返回给定节点是否可以作为条件控制语句的条件
  /// @retval true 给定节点允许作为条件控制语句的条件
  /// @retval false 给定节点不能作为条件控制语句的条件
  static bool DefaultConditionCheck(
      const OperatorNodeInterface& condition_node);
  /// @brief 默认检查主体内语句的函数
  /// @param[in] flow_interface ：待检查的语句
  /// @return 返回给定流程控制节点是否可以作为条件控制语句主体的内容
  /// @retval true 给定节点允许作为条件控制语句主体的内容
  /// @retval false 给定节点不能作为条件控制语句主体的内容
  static bool DefaultMainBlockSentenceCheck(
      const FlowInterface& flow_interface);

 private:
  /// @brief 控制块的条件
  std::shared_ptr<const OperatorNodeInterface> condition_;
  /// @brief 获取控制块条件的语句
  std::list<std::unique_ptr<FlowInterface>> sentence_to_get_condition_;
  /// @brief 控制块主体的内容
  std::list<std::unique_ptr<FlowInterface>> main_block_;
};

/// @class SimpleSentence flow_control.h
/// @brief 基础语句
/// @details
/// 所有继承自OperatorNodeInterface均属于该类
class SimpleSentence : public FlowInterface {
 public:
  SimpleSentence() : FlowInterface(FlowType::kSimpleSentence) {}

  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    assert(false);
    /// 防止警告
    return false;
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    assert(false);
    /// 防止警告
    return false;
  }

  /// @brief 设置该语句保存的语句
  /// @param[in] sentence_operate_node ：待保存的操作
  /// @return 返回是否设置成功
  /// @retval true ：设置成功
  /// @retval false ：不支持待保存的操作，无法设置
  /// @note 对保存的操作的要求参考CheckOperatorNodeValid
  bool SetSentenceOperateNode(
      const std::shared_ptr<OperatorNodeInterface>& sentence_operate_node);
  /// @brief 获取保存的操作
  /// @return 返回指向保存的操作节点的const指针
  std::shared_ptr<OperatorNodeInterface> GetSentenceOperateNodePointer() const {
    return sentence_operate_node_;
  }
  /// @brief 获取保存的操作
  /// @return 返回保存的操作节点的const引用
  OperatorNodeInterface& GetSentenceOperateNodeReference() const {
    return *sentence_operate_node_;
  }

  /// @brief 检查给定节点是否可以保存
  /// @param[in] operator_node ：保存的操作
  /// @return 返回是否可以保存
  /// @retval true ：可以保存
  /// @retval false ：不可以保存
  static bool CheckOperatorNodeValid(
      const OperatorNodeInterface& operator_node);

 private:
  /// @brief 操作节点
  std::shared_ptr<OperatorNodeInterface> sentence_operate_node_;
};

/// @class IfSentence flow_control.h
/// @brief if和if-else语句
class IfSentence : public ConditionBlockInterface {
 public:
  // 刚创建时只有true分支
  IfSentence() : ConditionBlockInterface(FlowType::kIfSentence) {}

  /// @details
  /// 根据if语句类型判断应该添加到哪个部分
  /// 如果是FlowType::kIfSentence则添加到true_branch
  /// 如果是FlowType::kIfElseSentence则添加到false_branch
  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override;
  /// @details
  /// 根据if语句类型判断应该添加到哪个部分
  /// 如果是FlowType::kIfSentence则添加到true_branch
  /// 如果是FlowType::kIfElseSentence则添加到false_branch
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override;

  /// @brief 将该节点转化为if-else语句
  /// @note 自动分配else块的存储结构
  void ConvertToIfElse();
  /// @brief 添加真分支内执行的一条语句
  /// @param[in] if_body_sentence ：待添加的语句
  /// @return 返回是否可以添加
  /// @retval true 添加成功，夺取if_body_sentence控制权
  /// @retval false 给定语句不能添加到真分支内，不会添加且不修改参数
  /// @note 添加到已存在的语句后
  /// 对语句的要求参考AddSentence的要求
  bool AddTrueBranchSentence(
      std::unique_ptr<FlowInterface>&& if_body_sentence) {
    // 在AddMainBlockSentence中检查
    return AddSentence(std::move(if_body_sentence));
  }
  /// @brief 添加真分支内执行的多条语句
  /// @param[in] if_body_sentences ：存储待添加的语句的容器
  /// @return 返回是否可以添加
  /// @retval true ：添加成功，将if_body_sentences内语句移动到真分支语句容器内
  /// @retval false ：容器内某条语句不能添加到真分支内，不会添加且不修改参数
  /// @note 从begin->end顺序添加，添加到已存在的语句后
  /// 对语句的要求参考AddSentence的要求
  bool AddTrueBranchSentences(
      std::list<std::unique_ptr<FlowInterface>>&& if_body_sentences) {
    return AddSentences(std::move(if_body_sentences));
  }
  /// @brief 添加假分支内执行的一条语句
  /// @param[in] else_body_sentence ：待添加的语句
  /// @return 返回是否可以添加
  /// @retval true 添加成功，夺取else_body_sentence控制权
  /// @retval false 给定语句不能添加到假分支内，不会添加且不修改参数
  /// @note 添加到已存在的语句后
  /// 对语句的要求参考CheckElseBodySentenceValid
  /// @attention 必须调用过ConvertToIfElse
  bool AddFalseBranchSentence(
      std::unique_ptr<FlowInterface>&& else_body_sentence);
  /// @brief 添加假分支内执行的多条语句
  /// @param[in] else_body_sentences ：存储待添加的语句的容器
  /// @return 返回是否可以添加
  /// @retval true ：添加成功，将else_body_sentences内语句移动到假分支语句容器内
  /// @retval false ：容器内某条语句不能添加到假分支内，不会添加且不修改参数
  /// @note 从begin->end顺序添加，添加到已存在的语句后
  /// 对语句的要求参考CheckElseBodySentenceValid
  /// @attention 必须调用过ConvertToIfElse
  bool AddFalseBranchSentences(
      std::list<std::unique_ptr<FlowInterface>>&& else_body_sentences);
  /// @brief 获取真分支执行的全部语句
  /// @return 返回指向存储真分支执行的语句的容器的const指针
  const auto& GetTrueBranchSentences() const { return GetSentences(); }
  /// @brief 获取假分支执行的全部语句
  /// @return 返回存储假分支执行的语句的容器的const引用
  const auto& GetFalseBranchSentences() const { return else_body_; }

  /// @brief 检查给定语句是否可以添加到假分支内
  /// @param[in] else_body_sentence ：待检查的语句
  /// @return 返回是否可以添加
  /// @retval true ：可以添加
  /// @retval false ：不可以添加
  static bool CheckElseBodySentenceValid(
      const FlowInterface& else_body_sentence) {
    // if语句与else语句的要求相同
    return ConditionBlockInterface::DefaultMainBlockSentenceCheck(
        else_body_sentence);
  }

 private:
  /// @brief 假分支内执行的内容，不存在else控制块则置为nullptr
  std::unique_ptr<std::list<std::unique_ptr<FlowInterface>>> else_body_ =
      nullptr;
};

/// @class LoopSentenceInterface flow_control.h
/// @brief 循环语句基类
class LoopSentenceInterface : public ConditionBlockInterface {
 protected:
  LoopSentenceInterface(FlowType flow_type)
      : ConditionBlockInterface(flow_type) {
    assert(flow_type == FlowType::kDoWhileSentence ||
           flow_type == FlowType::kWhileSentence ||
           flow_type == FlowType::kForSentence);
  }

 public:
  /// @brief 获取循环块循环条件判断开始处标签（即将开始判断循环条件）
  /// @return 返回循环块循环条件判断开始处标签
  virtual std::unique_ptr<Label> GetLoopConditionStartLabel() const = 0;
  /// @brief 获取循环块循环条件判断结束处标签（刚刚完成循环条件判断）
  /// @return 返回循环块循环条件判断结束处标签
  virtual std::unique_ptr<Label> GetLoopConditionEndLabel() const = 0;

  /// @brief 获取循环块主块开始处标签（即将开始执行主体内容）
  /// @return 返回循环块主块开始处标签
  std::unique_ptr<Label> GetLoopMainBlockStartLabel() const {
    return std::make_unique<Label>(
        std::format("loop_main_block_{:}_start", GetFlowId().GetRawValue()));
  }
  /// @brief 获取循环主块结尾处标签（刚刚结束主体执行）
  /// @return 返回循环主块结尾处标签
  std::unique_ptr<Label> GetLoopMainBlockEndLabel() const {
    return std::make_unique<Label>(
        std::format("loop_main_block_{:}_end", GetFlowId().GetRawValue()));
  }
};

/// @class WhileSentenceInterface flow_control.h
/// @brief while循环和do-while循环的基类
class WhileSentenceInterface : public LoopSentenceInterface {
 protected:
  WhileSentenceInterface(FlowType while_type)
      : LoopSentenceInterface(while_type) {
    assert(while_type == FlowType::kWhileSentence ||
           while_type == FlowType::kDoWhileSentence);
  }
};

/// @class WhileSentence flow_control.h
/// @brief while循环
class WhileSentence : public WhileSentenceInterface {
 public:
  WhileSentence() : WhileSentenceInterface(FlowType::kWhileSentence) {}

  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    return AddSentence(std::move(sentence));
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    return AddSentences(std::move(sentences));
  }
  virtual std::unique_ptr<Label> GetLoopConditionStartLabel() const override {
    return GetSentenceStartLabel();
  }
  virtual std::unique_ptr<Label> GetLoopConditionEndLabel() const override {
    return GetLoopMainBlockStartLabel();
  }
};

/// @class DoWhileSentence flow_control.h
/// @brief do-while循环
class DoWhileSentence : public WhileSentenceInterface {
 public:
  DoWhileSentence() : WhileSentenceInterface(FlowType::kDoWhileSentence) {}

  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    return AddSentence(std::move(sentence));
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    return AddSentences(std::move(sentences));
  }
  virtual std::unique_ptr<Label> GetLoopConditionStartLabel() const override {
    return GetLoopMainBlockEndLabel();
  }
  virtual std::unique_ptr<Label> GetLoopConditionEndLabel() const override {
    return GetLoopMainBlockEndLabel();
  }
};

/// @class ForSentence flow_control.h
/// @brief for循环
class ForSentence : public LoopSentenceInterface {
 public:
  ForSentence() : LoopSentenceInterface(FlowType::kForSentence) {}

  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    return AddSentence(std::move(sentence));
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    return AddSentences(std::move(sentences));
  }
  virtual std::unique_ptr<Label> GetLoopConditionStartLabel() const override {
    return GetSentenceStartLabel();
  }
  virtual std::unique_ptr<Label> GetLoopConditionEndLabel() const override {
    return GetLoopMainBlockStartLabel();
  }

  /// @brief 添加一条for语句执行前的初始化语句
  /// @param[in] init_body_sentence ：for循环初始化语句
  /// @return 返回是否可以添加
  /// @retval true ：添加成功，夺取init_body_sentence控制权
  /// @retval false ：不可以添加，不会添加且不修改参数
  /// @note 对语句的要求参考AddSentence
  /// 添加到已有的初始化语句后
  bool AddForInitSentence(std::unique_ptr<FlowInterface>&& init_body_sentence);
  /// @brief 添加多条for语句执行前的初始化语句
  /// @param[in] init_body_sentences ：存储for循环初始化语句的容器
  /// @return 返回是否可以添加
  /// @retval true ：添加成功，将init_body_sentences中所有语句移动到初始化容器中
  /// @retval false ：不可以添加，不会添加且不修改参数
  /// @note 对语句的要求参考AddSentence
  /// 添加顺序：begin->end，添加到已有的初始化语句后
  bool AddForInitSentences(
      std::list<std::unique_ptr<FlowInterface>>&& init_body_sentences);
  /// @brief 添加一条for语句执行后更新循环条件的语句
  /// @param[in] after_body_sentence ：for执行后更新循环条件的语句
  /// @return 返回是否可以添加
  /// @retval true ：添加成功，夺取after_body_sentences的控制权
  /// @retval false ：不可以添加，不会添加且不修改参数
  /// @note 对语句的要求参考CheckForBodySentenceValid
  /// 添加到已有的初始化语句后
  bool AddForRenewSentence(
      std::unique_ptr<FlowInterface>&& after_body_sentence);
  /// @brief 添加多条for语句执行后更新循环条件的语句
  /// @param[in] after_body_sentences ：存储for执行后更新循环条件的容器
  /// @return 返回是否可以添加
  /// @retval true ：添加成功，将after_body_sentences中所有语句移动到初始化容器
  /// @retval false ：不可以添加，不会添加且不修改参数
  /// @note 对语句的要求参考CheckForBodySentenceValid
  /// 添加顺序：begin->end，添加到已有的初始化语句后
  bool AddForRenewSentences(
      std::list<std::unique_ptr<FlowInterface>>&& after_body_sentences);
  /// @brief 获取for主体内所有语句
  /// @return 返回存储语句的容器的const引用
  const auto& GetForBody() const { return GetSentences(); }
  /// @brief 获取for初始化语句
  /// @return 返回存储初始化语句的容器的const引用
  const auto& GetForInitSentences() const { return init_block_; }
  /// @brief 获取更新循环条件的语句
  /// @return 返回存储更新循环条件的语句的容器的const引用
  const auto& GetForAfterBodySentences() const { return renew_sentences_; }

  /// @brief 检查给定节点是否可以作为for条件
  /// @param[in] for_condition ：待检查的条件节点
  /// @return 返回是否可以添加
  /// @retval true ：可以添加
  /// @retval false ：不可以添加
  static bool CheckForConditionValid(
      const OperatorNodeInterface& for_condition) {
    return ConditionBlockInterface::DefaultConditionCheck(for_condition);
  }
  /// @brief 检查给定节点是否可以作为for主体内执行的语句
  /// @param[in] for_body_sentence ：待检查的条件节点
  /// @return 返回是否可以添加
  /// @retval true ：可以添加
  /// @retval false ：不可以添加
  static bool CheckForBodySentenceValid(
      const FlowInterface& for_body_sentence) {
    return ConditionBlockInterface::DefaultMainBlockSentenceCheck(
        for_body_sentence);
  }

 private:
  /// @brief 初始化语句
  std::list<std::unique_ptr<FlowInterface>> init_block_;
  /// @brief 循环结束后更新循环条件的语句
  std::list<std::unique_ptr<FlowInterface>> renew_sentences_;
};

/// @class SwitchSentence flow_control.h
/// @brief switch分支语句
class SwitchSentence : public ConditionBlockInterface {
 public:
  SwitchSentence() : ConditionBlockInterface(FlowType::kSwitchSentence) {}

  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    return AddSentence(std::move(sentence));
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    return AddSentences(std::move(sentences));
  }

  /// @brief 添加普通的case
  /// @param[in] case_value ：分支条件（编译期常量）
  /// @return 返回是否添加成功
  /// @retval true 添加成功，夺取case_value控制权
  /// @retval false 添加失败，不添加且不修改参数
  /// @note 对case的要求参考CheckSwitchCaseAbleToAdd
  bool AddSimpleCase(
      const std::shared_ptr<const BasicTypeInitializeOperatorNode>& case_value);
  /// @brief 添加default标签
  /// @return 返回是否添加成功
  /// @retval true 添加成功
  /// @retval false 添加失败，该函数已经被调用过
  bool AddDefaultCase();
  /// @brief 获取全部普通case标签
  /// @return 返回存储普通case标签的容器的const引用
  const auto& GetSimpleCases() const { return simple_cases_; }
  /// @brief 获取default分支标签
  /// @return 返回指向default分支标签的const指针
  /// @retval nullptr ：未调用AddDefaultCase创建并添加default标签
  const Label* GetDefaultCase() const { return default_case_.get(); }

  /// @brief 检查给定节点是否可以作为switch主体内执行的语句
  /// @param[in] switch_body_sentence ：待检查的条件节点
  /// @return 返回是否可以添加
  /// @retval true ：可以添加
  /// @retval false ：不可以添加
  static bool CheckSwitchBodySentenceValid(
      const FlowInterface& switch_body_sentence) {
    return ConditionBlockInterface::DefaultMainBlockSentenceCheck(
        switch_body_sentence);
  }
  /// @brief 检查给定case是否可以作为switch内的标签
  /// @param[in] switch_node ：添加到的switch节点
  /// @param[in] case_value ：标签的值
  /// @return 返回能否作为作为switch内的标签使用
  /// @retval true ：可以添加
  /// @retval false ：标签不是立即数或与已有的case值重复
  static bool CheckSwitchCaseAbleToAdd(
      const SwitchSentence& switch_node,
      const BasicTypeInitializeOperatorNode& case_value);

 private:
  /// @brief 根据case的值创建对应的标签
  /// @param[in] case_value ：case的值
  /// @return 前半部分为跳转语句中使用的标签，后半部分为switch主体内使用的标签
  /// @note 只创建不设置
  std::pair<std::unique_ptr<Label>, std::unique_ptr<Label>>
  ConvertCaseValueToLabel(
      const BasicTypeInitializeOperatorNode& case_value) const;
  /// @brief 创建default标签
  /// @brief 前半部分为跳转语句中使用的标签，后半部分为switch主体内使用的标签
  /// @note 只创建不设置
  std::pair<std::unique_ptr<Label>, std::unique_ptr<Label>> CreateDefaultLabel()
      const;

  /// @brief switch的分支(case)
  /// @details
  /// 键为case的值
  /// 值前半部分为编译期常量节点，后半部分为该case的标签
  std::unordered_map<
      std::string,
      std::pair<std::shared_ptr<const BasicTypeInitializeOperatorNode>,
                std::unique_ptr<Label>>>
      simple_cases_;
  /// @brief default分支标签
  std::unique_ptr<Label> default_case_ = nullptr;
};

/// @class FlowControlSystem flow_control.h
/// @brief 流程控制系统
/// @details
/// 1.该系统当前仅用于存储函数
/// 2.已经构建完成的函数和正在构建的函数都在这里注册
/// 3.活跃函数指正在构建的函数
/// @attention 构建函数时会提取当前构建函数的flow_control_node并添加到
/// ActionScopeSystem中，FunctionDefine流程节点的控制权
/// 仍属于FlowControlSystem，详细信息参考ActionScopeSystem
class FlowControlSystem {
 public:
  /// @brief 声明/设置活跃函数时返回的结果
  enum class FunctionCheckResult {
    kSuccess,  ///< 成功声明/设置当前活跃函数
    ///< 失败的情况
    kOverrideFunction,  ///< 试图重载函数
    kDoubleAnnounce,    ///< 声明已经声明/定义的函数
    kFunctionConstructed  ///< 试图设置已经构建完成的函数为待构建函数
  };
  /// @brief 设置当前构建的函数
  /// @param[in] active_function ：当前构建的函数的类型
  /// @return 返回构建函数的情况，意义见定义
  /// @note 不能设置则不会设置
  FunctionCheckResult SetFunctionToConstruct(
      const std::shared_ptr<const FunctionType>& active_function);
  /// @brief 获取指向当前活跃函数节点的指针
  /// @return 返回指向当前活跃函数节点的指针
  /// @retval nullptr ：当前无活跃函数
  /// @warning 禁止delete返回的指针
  FunctionDefine* GetActiveFunctionPointer() const {
    return active_function_ == functions_.end() ? nullptr
                                                : &active_function_->second;
  }
  /// @brief 获取当前活跃函数节点的引用
  /// @return 返回当前活跃函数节点的引用
  /// @note 必须存在活跃函数
  FunctionDefine& GetActiveFunctionReference() const {
    return active_function_->second;
  }
  /// @brief 完成函数构建
  /// @note 完成函数构建后调用该函数进行后续处理
  void FinishFunctionConstruct() { active_function_ = functions_.end(); }
  /// @brief 查询函数数据
  /// @param[in] function_name ：函数名
  /// @return 返回指向函数节点的const指针
  /// @retval nullptr ：不存在function_name指定的函数
  const FunctionDefine* GetFunction(const std::string& function_name) {
    auto iter = functions_.find(function_name);
    if (iter == functions_.end()) [[unlikely]] {
      return nullptr;
    } else {
      return &iter->second;
    }
  }

  /// @brief 声明函数
  /// @param[in] function_type ：待声明函数的类型
  /// @note 不会设置当前活动的函数
  FunctionCheckResult AnnounceFunction(
      const std::shared_ptr<const FunctionType>& function_type);

 private:
  /// @brief 所有函数，键为函数名，值为函数对象（FunctionDefine）
  std::unordered_map<std::string, FunctionDefine> functions_;
  /// @brief 当前活跃的函数（正在被构建的函数）
  std::unordered_map<std::string, FunctionDefine>::iterator active_function_ =
      functions_.end();
};
}  // namespace c_parser_frontend::flow_control

#endif