#ifndef CPARSERFRONTEND_FLOW_CONTROL_H_
#define CPARSERFRONTEND_FLOW_CONTROL_H_

#include <list>
#include <memory>
#include <unordered_map>

#include "operator_node.h"
#include "type_system.h"

namespace c_parser_frontend::flow_control {
enum class FlowType {
  kFunctionDefine,   // 函数定义
  kSimpleSentence,   // 基础非流程控制语句
  kLabel,            // 添加标签语句
  kJmp,              // 跳转语句
  kReturn,           // 返回语句
  kIfSentence,       // if语句
  kIfElseSentence,   // if-else语句
  kWhileSentence,    // while语句
  kDoWhileSentence,  // do-while语句
  kForSentence,      // for语句
  kSwitchSentence    // switch语句
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

class FlowInterface {
  enum class IdWrapper { kFlowNodeId };

 public:
  // 流程节点ID
  using FlowId = frontend::common::ExplicitIdWrapper<size_t, IdWrapper,
                                                     IdWrapper::kFlowNodeId>;

  FlowInterface(FlowType flow_type) : flow_type_(flow_type){};
  virtual ~FlowInterface();

  // 向主流程中添加语句
  virtual bool AddMainSentence(std::unique_ptr<FlowInterface>&& sentence) = 0;
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) = 0;

  FlowType GetFlowType() const { return flow_type_; }
  FlowId GetFlowId() const { return flow_id_; }

 protected:
  void SetFlowType(FlowType flow_type) { flow_type_ = flow_type; }

 private:
  // 获取一个从未被分配过的FlowId
  static FlowId GetNewFlowId() {
    static thread_local FlowId flow_id(0);
    return flow_id++;
  }

  // 流程控制节点编号
  const FlowId flow_id_ = GetNewFlowId();
  // 流程控制节点类型
  FlowType flow_type_;
};

// 标签
class Label : public FlowInterface {
 public:
  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    assert(false);
    // 防止警告
    return false;
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    assert(false);
    // 防止警告
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

  template <class LabelName>
  void SetLabelName(LabelName&& label_name) {
    label_name_ = std::forward<LabelName>(label_name);
  }
  const std::string& GetLabelName() const { return label_name_; }

 private:
  std::string label_name_;
};

// 跳转指令
class Jmp : public FlowInterface {
 public:
  Jmp() : FlowInterface(FlowType::kJmp) {}

  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    assert(false);
    // 防止警告
    return false;
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    assert(false);
    // 防止警告
    return false;
  }

  const Label& GetTargetLabel() const { return *target_label_; }

 private:
  // 跳转指令要跳到的标签
  std::unique_ptr<Label> target_label_;
};

//从函数中返回
class Return : public FlowInterface {
 public:
  Return() : FlowInterface(FlowType::kReturn) {}

  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    assert(false);
    // 防止警告
    return false;
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    assert(false);
    // 防止警告
    return false;
  }

  void SetReturnTarget(
      const std::shared_ptr<const OperatorNodeInterface>& return_target) {
    return_target_ = return_target;
  }

  std::shared_ptr<const OperatorNodeInterface> GetReturnValuePointer() const {
    return return_target_;
  }

 private:
  // 要返回的值，nullptr代表无返回值
  std::shared_ptr<const OperatorNodeInterface> return_target_;
};

// 条件控制块基类（if,for,while等）
// 应保持接口一致性，派生类不应修改接口语义且不可以任何方式重写或绕过接口
class ConditionBlockInterface : public FlowInterface {
 public:
  ConditionBlockInterface(FlowType flow_type) : FlowInterface(flow_type) {}

  bool SetCondition(
      const std::shared_ptr<const OperatorNodeInterface>& condition,
      std::list<std::unique_ptr<FlowInterface>>&& sentences_to_get_condition);
  const std::list<std::unique_ptr<FlowInterface>>& GetSentenceToGetCondition()
      const {
    return sentence_to_get_condition_;
  }
  std::shared_ptr<const OperatorNodeInterface> GetConditionPointer() const {
    return condition_;
  }
  const OperatorNodeInterface& GetConditionReference() const {
    return *condition_;
  }
  // 向主块内添加语句，参数检查不通过则不添加，不修改参数
  // 返回是否添加成功
  bool AddSentence(std::unique_ptr<FlowInterface>&& sentence);
  // 向主块内添加多个语句，任意语句参数检查不通过则不添加，不修改参数
  // 返回是否添加成功
  bool AddSentences(std::list<std::unique_ptr<FlowInterface>>&& sentences);
  const auto& GetSentences() const { return main_block_; }

  // 默认对循环条件节点进行类型检查的函数
  // 返回给定节点是否可以作为条件分支类语句的条件
  static bool DefaultConditionCheck(
      const OperatorNodeInterface& condition_node);
  // 默认检查主体内语句的函数
  // 返回给定流程控制节点是否可以作为条件分支语句主体的内容
  static bool DefaultMainBlockSentenceCheck(
      const FlowInterface& flow_interface);

 private:
  // 控制块的条件
  std::shared_ptr<const OperatorNodeInterface> condition_;
  // 获取控制块的语句
  std::list<std::unique_ptr<FlowInterface>> sentence_to_get_condition_;
  // 控制块主块的内容
  std::list<std::unique_ptr<FlowInterface>> main_block_;
};

class FunctionDefine : public FlowInterface {
 public:
  FunctionDefine(const std::shared_ptr<FunctionType>& function_type)
      : FlowInterface(FlowType::kFunctionDefine),
        function_type_(function_type) {}

  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    return GetFunctionTypeReference().AddSentences(std::move(sentences));
  }
  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    return GetFunctionTypeReference().AddSentence(std::move(sentence));
  }

  void SetFunctionObjectToCall(
      const std::shared_ptr<FunctionType>& function_type) {
    function_type_ = function_type;
  }
  std::shared_ptr<FunctionType> GetFunctionTypePointer() const {
    return function_type_;
  }
  FunctionType& GetFunctionTypeReference() const { return *function_type_; }

 private:
  // 函数的类型
  std::shared_ptr<FunctionType> function_type_;
};

class SimpleSentence : public FlowInterface {
 public:
  SimpleSentence() : FlowInterface(FlowType::kSimpleSentence) {}

  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override {
    assert(false);
    // 防止警告
    return false;
  }
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override {
    assert(false);
    // 防止警告
    return false;
  }

  bool SetSentenceOperateNode(
      const std::shared_ptr<OperatorNodeInterface>& sentence_operate_node);
  std::shared_ptr<OperatorNodeInterface> GetSentenceOperateNodePointer() const {
    return sentence_operate_node_;
  }
  OperatorNodeInterface& GetSentenceOperateNodeReference() const {
    return *sentence_operate_node_;
  }

  // 检查给定节点是否可以作为基础语句的节点
  static bool CheckOperatorNodeValid(
      const OperatorNodeInterface& operator_node);

 private:
  // 执行操作的节点
  std::shared_ptr<OperatorNodeInterface> sentence_operate_node_;
};

// 两种if语句
class IfSentence : public ConditionBlockInterface {
 public:
  // 刚创建时只有true分支
  IfSentence() : ConditionBlockInterface(FlowType::kIfSentence) {}

  // 根据if语句类型判断应该添加到哪个部分
  // 如果是FlowType::kIfSentence则添加到true_branch
  // 如果是FlowType::kIfElseSentence则添加到false_branch
  virtual bool AddMainSentence(
      std::unique_ptr<FlowInterface>&& sentence) override;
  virtual bool AddMainSentences(
      std::list<std::unique_ptr<FlowInterface>>&& sentences) override;

  // 将该节点转化为if-else语句
  // 并分配else块的存储结构
  void ConvertToIfElse();
  // 添加if语句内执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且不修改参数
  // 添加成功则不返还流程节点控制权
  bool AddTrueBranchSentence(
      std::unique_ptr<FlowInterface>&& if_body_sentence) {
    // 在AddMainBlockSentence中检查
    return AddSentence(std::move(if_body_sentence));
  }
  // 添加多条if语句内执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且不修改参数
  // 添加成功则不返还流程节点控制权
  bool AddTrueBranchSentences(
      std::list<std::unique_ptr<FlowInterface>>&& if_body_sentences) {
    return AddSentences(std::move(if_body_sentences));
  }
  // 添加else语句内执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
  // 添加成功则不返还流程节点控制权
  bool AddFalseBranchSentence(
      std::unique_ptr<FlowInterface>&& else_body_sentence);
  // 添加多条else语句内执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
  // 添加成功则不返还流程节点控制权
  bool AddFalseBranchSentences(
      std::list<std::unique_ptr<FlowInterface>>&& else_body_sentences);
  const auto& GetTrueBranchSentences() const { return GetSentences(); }
  const auto& GetFalseBranchSentences() const { return else_body_; }

  static bool CheckElseBodySentenceValid(
      const FlowInterface& else_body_sentence) {
    // if语句与else语句的要求相同
    return ConditionBlockInterface::DefaultMainBlockSentenceCheck(
        else_body_sentence);
  }

 private:
  // else语句内执行的内容
  std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>> else_body_ =
      nullptr;
};

// 循环语句基类
class LoopSentenceInterface : public ConditionBlockInterface {
 protected:
  LoopSentenceInterface(FlowType flow_type)
      : ConditionBlockInterface(flow_type) {
    assert(flow_type == FlowType::kDoWhileSentence ||
           flow_type == FlowType::kWhileSentence ||
           flow_type == FlowType::kForSentence);
  }
};

class WhileSentenceInterface : public LoopSentenceInterface {
 protected:
  WhileSentenceInterface(FlowType while_type)
      : LoopSentenceInterface(while_type) {
    assert(while_type == FlowType::kWhileSentence ||
           while_type == FlowType::kDoWhileSentence);
  }
};

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
};

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
};

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

  // 添加for语句执行前的内容
  // 返回是否可以添加，如果不可以添加则不会添加且不修改参数
  // 添加成功则不返还流程节点控制权
  bool AddForInitSentence(std::unique_ptr<FlowInterface>&& init_body_sentence);
  // 添加多条for语句执行前的内容
  // 返回是否可以添加，如果不可以添加则不会添加且不修改参数
  // 添加成功则不返还流程节点控制权
  bool AddForInitSentences(
      std::list<std::unique_ptr<FlowInterface>>&& init_body_sentences);
  // 添加for语句主体执行一次后执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且不修改参数
  // 添加成功则不返还流程节点控制权
  bool AddForRenewSentence(
      std::unique_ptr<FlowInterface>&& after_body_sentence);
  // 添加多条for语句主体执行一次后执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且不修改参数
  // 添加成功则不返还流程节点控制权
  bool AddForRenewSentences(
      std::list<std::unique_ptr<FlowInterface>>&& after_body_sentences);
  const auto& GetForBody() const { return GetSentences(); }
  const auto& GetSentencesToGetForCondition() const {
    return GetSentenceToGetCondition();
  }
  const auto& GetForInitSentences() const { return init_block_; }
  const auto& GetForAfterBodySentences() const { return after_body_sentences_; }

  static bool CheckForConditionValid(
      const OperatorNodeInterface& for_condition) {
    return ConditionBlockInterface::DefaultConditionCheck(for_condition);
  }
  static bool CheckForBodySentenceValid(
      const FlowInterface& for_body_sentence) {
    return ConditionBlockInterface::DefaultMainBlockSentenceCheck(
        for_body_sentence);
  }

 private:
  // 初始化语句
  std::list<std::unique_ptr<FlowInterface>> init_block_;
  // 一次循环结束后执行的语句
  std::list<std::unique_ptr<FlowInterface>> after_body_sentences_;
};

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

  // 添加普通的case，返回是否添加成功
  // 添加失败不修改参数，添加成功则夺取case_label控制权
  bool AddSimpleCase(
      const std::shared_ptr<const BasicTypeInitializeOperatorNode>& case_value);
  bool AddDefaultCase();
  const auto& GetSimpleCases() const { return simple_cases_; }
  Label& GetDefaultCase() const { return *default_case_; }

  static bool CheckSwitchBodySentenceValid(
      const FlowInterface& switch_body_sentence) {
    return ConditionBlockInterface::DefaultMainBlockSentenceCheck(
        switch_body_sentence);
  }
  // 检查给定的case是否可以添加到给定的节点中
  static bool CheckSwitchCaseAbleToAdd(
      const SwitchSentence& switch_node,
      const BasicTypeInitializeOperatorNode& case_value);
  // 将case的值转化为标签
  // 返回值前半部分为跳转语句中使用的标签，后半部分为switch主体内使用的标签
  static std::pair<std::unique_ptr<Label>, std::unique_ptr<Label>>
  ConvertCaseValueToLabel(const SwitchSentence& switch_sentence,
                          const BasicTypeInitializeOperatorNode& case_value);
  // 根据switch语句创建默认标签
  // 返回值前半部分为跳转语句中使用的标签，后半部分为switch主体内使用的标签
  static std::pair<std::unique_ptr<Label>, std::unique_ptr<Label>>
  GetDefaultLabel(const SwitchSentence& switch_sentence);

 private:
  // switch的分支(case)，使用case的值作为键值
  std::unordered_map<
      std::string,
      std::pair<std::shared_ptr<const BasicTypeInitializeOperatorNode>,
                std::unique_ptr<Label>>>
      simple_cases_;
  // default分支
  std::unique_ptr<Label> default_case_ = nullptr;
};

class FlowControlSystem {
 public:
  void SetFunctionToConstruct(
      const std::shared_ptr<FunctionType>& active_function) {
    active_function_ = active_function;
  }
  std::shared_ptr<FunctionType> GetActiveFunctionPointer() const {
    return active_function_;
  }
  FunctionType& GetActiveFunctionReference() const { return *active_function_; }
  // 完成函数构建后调用
  void FinishFunctionConstruct() {
    functions_.emplace_back(std::move(active_function_));
    active_function_ = nullptr;
  }

 private:
  // 当前活动的函数（正在被构建的函数）
  std::shared_ptr<FunctionType> active_function_ = nullptr;
  // 所有构建完成的函数
  std::list<std::shared_ptr<FunctionType>> functions_;
};
}  // namespace c_parser_frontend::flow_control

#endif