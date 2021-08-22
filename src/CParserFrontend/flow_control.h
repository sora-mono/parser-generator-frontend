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
using c_parser_frontend::operator_node::InitializeOperatorNode;
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
 public:
  FlowInterface(FlowType flow_type) : flow_type_(flow_type){};
  virtual ~FlowInterface();

  void SetFlowType(FlowType flow_type) { flow_type_ = flow_type; }
  FlowType GetFlowType() const { return flow_type_; }

 private:
  FlowType flow_type_;
};

// 标签
class Label : public FlowInterface {
 public:
  template <class LabelName>
  Label(LabelName&& label_name)
      : FlowInterface(FlowType::kLabel),
        label_name_(std::forward<LabelName>(label_name)) {}
  Label(const Label&) = default;
  Label& operator=(const Label&) = default;

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

  void SetTagetLabel(const Label& target_label) {
    target_label_ = std::make_unique<Label>(target_label);
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

  void SetReturnTarget(
      std::shared_ptr<const VarietyOperatorNode>&& return_target) {
    return_target_ = std::move(return_target);
  }

  std::shared_ptr<const VarietyOperatorNode> GetReturnTargetPointer() const {
    return return_target_;
  }
  const VarietyOperatorNode& GetReturnTargetReference() const {
    return *return_target_;
  }

 private:
  // 要返回的节点，将节点置为void代表无返回值
  std::shared_ptr<const VarietyOperatorNode> return_target_;
};

// 条件控制块基类（if,for,while等）
class ConditionBlockInterface : public FlowInterface {
 public:
  ConditionBlockInterface(FlowType flow_type) : FlowInterface(flow_type) {}

  void SetCondition(std::shared_ptr<const OperatorNodeInterface>&& condition) {
    condition_ = std::move(condition);
  }
  std::shared_ptr<const OperatorNodeInterface> GetConditionPointer() const {
    return condition_;
  }
  const OperatorNodeInterface& GetConditionReference() const {
    return *condition_;
  }
  void AddMainBlockSentence(std::unique_ptr<FlowInterface>&& sentence) {
    main_block_.emplace_back(std::move(sentence));
  }
  const auto& GetMainBlock() const { return main_block_; }

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
  // 控制块主块的内容
  std::list<std::unique_ptr<FlowInterface>> main_block_;
};

class FunctionDefine : public FlowInterface {
 public:
  FunctionDefine(std::shared_ptr<const FunctionType>&& function_type)
      : FlowInterface(FlowType::kFunctionDefine),
        function_type_(std::move(function_type)) {}

  // 添加一条函数内执行的语句（按出现顺序添加）
  // 成功添加返回true，不返还控制权
  // 如果不能添加则返回false与控制权
  std::pair<std::unique_ptr<FlowInterface>, bool> AddSentence(
      std::unique_ptr<FlowInterface>&& sentence_to_add);
  const auto& GetSentences() const { return sentences_in_function_; }
  void SetFunctionType(std::shared_ptr<const FunctionType>&& function_type) {
    function_type_ = std::move(function_type);
  }
  std::shared_ptr<const FunctionType> GetFunctionTypePointer() const {
    return function_type_;
  }
  const FunctionType& GetFunctionTypeReference() const {
    return *function_type_;
  }
  // 检查给定语句是否可以作为函数内出现的语句
  static bool CheckSentenceInFunctionValid(const FlowInterface& flow_interface);

 private:
  // 函数的类型
  std::shared_ptr<const FunctionType> function_type_;
  // 函数内执行的语句
  std::list<std::unique_ptr<FlowInterface>> sentences_in_function_;
};

class SimpleSentence : public FlowInterface {
 public:
  SimpleSentence() : FlowInterface(FlowType::kSimpleSentence) {}

  bool SetSentenceOperateNode(
      std::shared_ptr<const OperatorNodeInterface>&& sentence_operate_node);
  std::shared_ptr<const OperatorNodeInterface> GetSentenceOperateNodePointer()
      const {
    return sentence_operate_node_;
  }
  const OperatorNodeInterface& GetSentenceOperateNodeReference() const {
    return *sentence_operate_node_;
  }

  // 检查给定节点是否可以作为基础语句的节点
  static bool CheckOperatorNodeValid(
      const OperatorNodeInterface& operator_node);

 private:
  // 执行操作的节点
  std::shared_ptr<const OperatorNodeInterface> sentence_operate_node_;
};

// 两种if语句的基础类
class IfSentenceInterface : public ConditionBlockInterface {
 public:
  IfSentenceInterface() : ConditionBlockInterface(FlowType::kIfSentence) {}

  // 获取if语句的else部分，如果不存在则应返回空指针
  virtual const std::list<std::unique_ptr<FlowInterface>>* GetElseBody()
      const = 0;

  // 返回是否可以设置if判断条件，如果不能设置则返回false且不设置
  bool SetIfCondition(
      std::shared_ptr<const OperatorNodeInterface>&& if_condition);
  std::shared_ptr<const OperatorNodeInterface> GetIfConditionPointer() const {
    return GetConditionPointer();
  }
  const OperatorNodeInterface& GetIfConditionReference() const {
    return GetConditionReference();
  }
  // 添加if语句内执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
  // 添加成功则不返还流程节点控制权
  std::pair<std::unique_ptr<FlowInterface>, bool> AddIfBodySentence(
      std::unique_ptr<FlowInterface>&& if_body_sentence);
  const auto& GetIfBody() const { return GetMainBlock(); }

  // 检查给定节点是否可以作为if的条件
  static bool CheckIfConditionValid(
      const OperatorNodeInterface& condition_node) {
    return DefaultConditionCheck(condition_node);
  }
  // 检查给定语句是否可以作为if内部的语句
  static bool CheckIfBodySentenceValid(const FlowInterface& if_body_sentence) {
    return DefaultMainBlockSentenceCheck(if_body_sentence);
  }
};

// 只有if没有else的if语句
class IfSentence : public IfSentenceInterface {
 public:
  IfSentence() = default;

  virtual const std::list<std::unique_ptr<FlowInterface>>* GetElseBody()
      const override {
    return nullptr;
  }
};

// 有if也有else的语句
class IfElseSentence : public IfSentenceInterface {
 public:
  IfElseSentence() = default;

  virtual const std::list<std::unique_ptr<FlowInterface>>* GetElseBody()
      const override {
    return &else_body_;
  }

  // 添加else语句内执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
  // 添加成功则不返还流程节点控制权
  std::pair<std::unique_ptr<FlowInterface>, bool> AddElseBodySentence(
      std::unique_ptr<FlowInterface>&& else_body_sentence);

  static bool CheckElseBodySentenceValid(
      const FlowInterface& else_body_sentence) {
    // if语句与else语句的要求相同
    return IfSentenceInterface::CheckIfBodySentenceValid(else_body_sentence);
  }

 private:
  // else语句内执行的内容
  std::list<std::unique_ptr<FlowInterface>> else_body_;
};

class WhileSentence : public ConditionBlockInterface {
 public:
  WhileSentence() : ConditionBlockInterface(FlowType::kWhileSentence) {}
  WhileSentence(FlowType flow_type) : ConditionBlockInterface(flow_type) {
    // 提供给do-while循环继承用构造函数
    assert(flow_type == FlowType::kDoWhileSentence);
  }

  // 返回是否可以设置while判断条件，如果不能设置则返回false且不设置
  bool SetWhileCondition(
      std::shared_ptr<const OperatorNodeInterface>&& while_condition);
  std::shared_ptr<const OperatorNodeInterface> GetWhileConditionPointer()
      const {
    GetConditionPointer();
  }
  const OperatorNodeInterface& GetWhileConditionReference() const {
    return GetConditionReference();
  }
  const auto& GetWhileBody() const { return GetMainBlock(); }
  // 添加while语句内执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
  // 添加成功则不返还流程节点控制权
  std::pair<std::unique_ptr<FlowInterface>, bool> AddWhileBodySentence(
      std::unique_ptr<FlowInterface>&& while_body_sentence);

  static bool CheckWhileConditionValid(
      const OperatorNodeInterface& while_condition) {
    return ConditionBlockInterface::DefaultConditionCheck(while_condition);
  }

  static bool CheckWhileBodySentenceValid(
      const FlowInterface& while_body_sentence) {
    return ConditionBlockInterface::DefaultMainBlockSentenceCheck(
        while_body_sentence);
  }
};

class DoWhileSentence : public WhileSentence {
 public:
  // do-while与while循环AST相同，仅在翻译时不同
  DoWhileSentence() : WhileSentence(FlowType::kDoWhileSentence) {}
};
class ForSentence : public ConditionBlockInterface {
 public:
  ForSentence() : ConditionBlockInterface(FlowType::kForSentence) {}

  // 返回是否可以设置if判断条件，如果不能设置则返回false且不设置
  bool SetForCondition(
      std::shared_ptr<const OperatorNodeInterface>&& for_condition);
  // 添加for语句内执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
  // 添加成功则不返还流程节点控制权
  std::pair<std::unique_ptr<FlowInterface>, bool> AddForBodySentence(
      std::unique_ptr<FlowInterface>&& for_body_sentence);
  // 添加for语句执行前的内容
  // 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
  // 添加成功则不返还流程节点控制权
  std::pair<std::unique_ptr<FlowInterface>, bool> AddForInitSentence(
      std::unique_ptr<FlowInterface>&& init_body_sentence);
  // 添加for语句主体执行一次后执行的内容
  // 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
  // 添加成功则不返还流程节点控制权
  std::pair<std::unique_ptr<FlowInterface>, bool> AddForAfterBodySentence(
      std::unique_ptr<FlowInterface>&& after_body_sentence);
  const auto& GetForBody() const { return GetMainBlock(); }
  const auto& GetForInitSentences() const { return init_block_; }
  const auto& GetForAfterBodySentences() const { return after_body_; }

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
  std::list<std::unique_ptr<FlowInterface>> after_body_;
};

class SwitchSentence : public ConditionBlockInterface {
 public:
  SwitchSentence() : ConditionBlockInterface(FlowType::kSwitchSentence) {}

  // 添加普通的case，返回是否添加成功，如果添加失败则返回case_label的控制权
  std::pair<std::unique_ptr<Label>, bool> AddSimpleCase(
      std::shared_ptr<BasicTypeInitializeOperatorNode>&& case_value,
      std::unique_ptr<Label>&& case_label);
  // 添加switch语句主体部分
  // 返回是否可以添加，如果不可以添加则不会添加且返回流程节点的控制权
  // 添加成功则不返还流程节点控制权
  std::pair<std::unique_ptr<FlowInterface>, bool> AddSwitchBodySentence(
      std::unique_ptr<FlowInterface>&& switch_body_sentence);
  // 设置switch语句分支条件
  // 返回是否可以设置，如果不可以设置则不会修改
  bool SetSwitchCondition(
      std::shared_ptr<const OperatorNodeInterface>&& switch_condition);
  void SetDefaultCase(std::unique_ptr<Label>&& default_case) {
    default_case_ = std::move(default_case);
  }
  const auto& GetSimpleCases() const { return simple_cases_; }
  Label& GetDefaultCase() const { return *default_case_; }

  static bool CheckSwitchConditionValid(
      const OperatorNodeInterface& switch_condition) {
    return ConditionBlockInterface::DefaultConditionCheck(switch_condition);
  }
  static bool CheckSwitchBodySentenceValid(
      const FlowInterface& switch_body_sentence) {
    return ConditionBlockInterface::DefaultMainBlockSentenceCheck(
        switch_body_sentence);
  }
  // 检查给定的case是否可以添加到给定的节点中
  static bool CheckSwitchCaseAbleToAdd(
      const SwitchSentence& switch_node,
      const BasicTypeInitializeOperatorNode& case_value);

 private:
  // switch的分支(case)，使用case的值作为键值
  std::unordered_map<std::string,
                     std::pair<std::shared_ptr<BasicTypeInitializeOperatorNode>,
                               std::unique_ptr<Label>>>
      simple_cases_;
  // default分支
  std::unique_ptr<Label> default_case_;
};

}  // namespace c_parser_frontend::flow_control

#endif