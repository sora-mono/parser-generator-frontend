#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAX_ANALYSIS_TABLE_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAX_ANALYSIS_TABLE_H_

#include <boost/serialization/unique_ptr.hpp>
#include <format>
#include <iostream>

#include "Generator/export_types.h"

namespace frontend::generator::syntax_generator {

// 语法分析表条目
class SyntaxAnalysisTableEntry {
 public:
  // 前向声明三种派生类，为了虚函数可以返回相应的类型
  class ShiftAttachedData;
  class ReductAttachedData;
  class ShiftReductAttachedData;

  class ActionAndAttachedDataInterface {
   public:
    ActionAndAttachedDataInterface(ActionType action_type)
        : action_type_(action_type) {}
    ActionAndAttachedDataInterface(const ActionAndAttachedDataInterface&) =
        default;
    virtual ~ActionAndAttachedDataInterface() {}

    ActionAndAttachedDataInterface& operator=(
        const ActionAndAttachedDataInterface&) = default;

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
    // 提供默认构造函数供序列化时构建对象
    ActionAndAttachedDataInterface() = default;

    // 允许序列化用的类访问
    friend class boost::serialization::access;
    // 允许语法分析表条目调用内部接口
    friend class SyntaxAnalysisTableEntry;
    // 暴露部分内部类型，从而在boost_serialization中注册
    friend struct ExportSyntaxGeneratorInsideTypeForSerialization;

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
    ShiftAttachedData(SyntaxAnalysisTableEntryId next_entry_id)
        : ActionAndAttachedDataInterface(ActionType::kShift),
          next_entry_id_(next_entry_id) {}
    ShiftAttachedData(const ShiftAttachedData&) = default;

    ShiftAttachedData& operator=(const ShiftAttachedData&) = default;
    virtual bool operator==(const ActionAndAttachedDataInterface&
                                shift_attached_data) const override;

    virtual const ShiftAttachedData& GetShiftAttachedData() const override {
      return *this;
    }

    SyntaxAnalysisTableEntryId GetNextSyntaxAnalysisTableEntryId() const {
      return next_entry_id_;
    }
    void SetNextSyntaxAnalysisTableEntryId(
        SyntaxAnalysisTableEntryId next_entry_id) {
      next_entry_id_ = next_entry_id;
    }

   private:
    // 提供默认构造函数供序列化时构建对象
    ShiftAttachedData() = default;

    // 允许序列化类访问
    friend class boost::serialization::access;
    // 允许语法分析表条目访问内部接口
    friend class SyntaxAnalysisTableEntry;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& boost::serialization::base_object<ActionAndAttachedDataInterface>(
          *this);
      ar& next_entry_id_;
    }
    virtual ShiftAttachedData& GetShiftAttachedData() override {
      return const_cast<ShiftAttachedData&>(
          static_cast<const ShiftAttachedData&>(*this).GetShiftAttachedData());
    }

    // 移入该单词后转移到的语法分析表条目ID
    SyntaxAnalysisTableEntryId next_entry_id_;
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
    ReductAttachedData(const ReductAttachedData&) = default;
    ReductAttachedData(ReductAttachedData&&) = default;

    ReductAttachedData& operator=(const ReductAttachedData&) = default;
    ReductAttachedData& operator=(ReductAttachedData&&) = default;
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
    // 提供默认构造函数供序列化时构建对象
    ReductAttachedData() = default;

    // 允许序列化类访问
    friend class boost::serialization::access;
    // 允许语法分析表条目访问内部接口
    friend class SyntaxAnalysisTableEntry;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& boost::serialization::base_object<ActionAndAttachedDataInterface>(
          *this);
      ar& reducted_nonterminal_node_id_;
      ar& process_function_class_id_;
      ar& production_body_;
    }

    virtual ReductAttachedData& GetReductAttachedData() override {
      return const_cast<ReductAttachedData&>(
          static_cast<const ReductAttachedData&>(*this)
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
    ShiftReductAttachedData(const ShiftReductAttachedData&) = delete;

    // 在与ShiftAttachedData和ReductAttachedData比较时仅比较相应部分
    virtual bool operator==(
        const ActionAndAttachedDataInterface& attached_data) const override;

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
    // 提供默认构造函数供序列化时构建对象
    ShiftReductAttachedData() = default;

    // 允许序列化类访问
    friend class boost::serialization::access;
    // 允许语法分析表条目访问内部接口
    friend class SyntaxAnalysisTableEntry;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& boost::serialization::base_object<ActionAndAttachedDataInterface>(
          *this);
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

  // 键值为待移入节点ID，值为指向相应数据的指针
  using ActionAndTargetContainer =
      std::unordered_map<ProductionNodeId,
                         std::unique_ptr<ActionAndAttachedDataInterface>>;

  SyntaxAnalysisTableEntry() {}
  SyntaxAnalysisTableEntry(const SyntaxAnalysisTableEntry&) = delete;
  SyntaxAnalysisTableEntry& operator=(const SyntaxAnalysisTableEntry&) = delete;
  SyntaxAnalysisTableEntry(
      SyntaxAnalysisTableEntry&& syntax_analysis_table_entry)
      : action_and_attached_data_(
            std::move(syntax_analysis_table_entry.action_and_attached_data_)),
        nonterminal_node_transform_table_(std::move(
            syntax_analysis_table_entry.nonterminal_node_transform_table_)) {}
  SyntaxAnalysisTableEntry& operator=(
      SyntaxAnalysisTableEntry&& syntax_analysis_table_entry);

  // 设置该产生式在转移条件下的动作和目标节点
  template <class AttachedData>
  requires std::is_same_v<std::decay_t<AttachedData>,
                          SyntaxAnalysisTableEntry::ShiftAttachedData> ||
      std::is_same_v<std::decay_t<AttachedData>,
                     SyntaxAnalysisTableEntry::ReductAttachedData>
  void SetTerminalNodeActionAndAttachedData(ProductionNodeId node_id,
                                            AttachedData&& attached_data);
  // 设置该条目移入非终结节点后转移到的节点
  void SetNonTerminalNodeTransformId(
      ProductionNodeId node_id, SyntaxAnalysisTableEntryId production_node_id) {
    nonterminal_node_transform_table_[node_id] = production_node_id;
  }
  // 修改该条目中所有条目ID为新ID
  // 当前设计下仅修改移入时转移到的下一个条目ID（移入终结节点/非终结节点）
  void ResetEntryId(const std::unordered_map<SyntaxAnalysisTableEntryId,
                                             SyntaxAnalysisTableEntryId>&
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
  // 不存在该转移条件则返回SyntaxAnalysisTableEntryId::InvalidId()
  SyntaxAnalysisTableEntryId AtNonTerminalNode(ProductionNodeId node_id) const {
    auto iter = nonterminal_node_transform_table_.find(node_id);
    return iter == nonterminal_node_transform_table_.end()
               ? SyntaxAnalysisTableEntryId::InvalidId()
               : iter->second;
  }
  // 获取全部终结节点的操作
  const ActionAndTargetContainer& GetAllActionAndAttachedData() const {
    return action_and_attached_data_;
  }
  // 获取全部非终结节点转移到的表项
  const std::unordered_map<ProductionNodeId, SyntaxAnalysisTableEntryId>&
  GetAllNonTerminalNodeTransformTarget() const {
    return nonterminal_node_transform_table_;
  }
  // 清除该条目中所有数据
  void Clear() {
    action_and_attached_data_.clear();
    nonterminal_node_transform_table_.clear();
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
        static_cast<const SyntaxAnalysisTableEntry&>(*this)
            .GetAllActionAndAttachedData());
  }
  // 获取全部非终结节点转移到的表项
  std::unordered_map<ProductionNodeId, SyntaxAnalysisTableEntryId>&
  GetAllNonTerminalNodeTransformTarget() {
    return const_cast<
        std::unordered_map<ProductionNodeId, SyntaxAnalysisTableEntryId>&>(
        static_cast<const SyntaxAnalysisTableEntry&>(*this)
            .GetAllNonTerminalNodeTransformTarget());
  }

  // 向前看符号ID下的操作和目标节点
  ActionAndTargetContainer action_and_attached_data_;
  // 移入非终结节点后转移到的产生式体序号
  std::unordered_map<ProductionNodeId, SyntaxAnalysisTableEntryId>
      nonterminal_node_transform_table_;
};

template <class AttachedData>
requires std::is_same_v<std::decay_t<AttachedData>,
                        SyntaxAnalysisTableEntry::ShiftAttachedData> ||
    std::is_same_v<std::decay_t<AttachedData>,
                   SyntaxAnalysisTableEntry::ReductAttachedData>
void SyntaxAnalysisTableEntry::SetTerminalNodeActionAndAttachedData(
    ProductionNodeId node_id, AttachedData&& attached_data) {
  static_assert(
      !std::is_same_v<std::decay_t<AttachedData>, ShiftReductAttachedData>,
      "该类型仅允许通过在已有一种动作的基础上补全缺少的另一半后转换得到，不允许"
      "直接传入");
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
    // 要么修改已有的规约后得到的节点，要么补全移入/规约的另一部分（规约/移入）
    // 不应修改已有的移入后转移到的语法分析表条目
    switch (data_already_in.GetActionType()) {
      case ActionType::kShift:
        if constexpr (std::is_same_v<std::decay_t<AttachedData>,
                                     ReductAttachedData>) {
          // 提供的数据是规约数据，转换为ShiftReductAttachedData
          iter->second = std::make_unique<ShiftReductAttachedData>(
              std::move(iter->second->GetShiftAttachedData()),
              std::forward<AttachedData>(attached_data));
        } else {
          // 提供的是移入数据，要求必须与已有的数据相同
          static_assert(
              std::is_same_v<std::decay_t<AttachedData>, ShiftAttachedData>);
          // 检查提供的移入数据是否与已有的移入数据相同
          // 不能写反，否则当data_already_in为ShiftReductAttachedData时
          // 无法获得与其它附属数据比较的正确语义
          // 可能是MSVC的BUG
          // 使用隐式调用会导致调用attached_data的operator==，所以显式调用
          if (!data_already_in.operator==(attached_data)) [[unlikely]] {
            // 设置相同条件不同产生式下规约
            std::cerr << std::format(
                "一个项集在相同条件下只能规约一种产生式，请参考该项"
                "集求闭包时的输出信息检查产生式");
            exit(-1);
          }
        }
        break;
      case ActionType::kReduct:
        if constexpr (std::is_same_v<std::decay_t<AttachedData>,
                                     ShiftAttachedData>) {
          // 提供的数据为移入部分数据，转换为ShiftReductAttachedData
          iter->second = std::make_unique<ShiftReductAttachedData>(
              std::forward<AttachedData>(attached_data),
              std::move(iter->second->GetReductAttachedData()));
        } else {
          // 提供的是移入数据，需要检查是否与已有的数据相同
          static_assert(
              std::is_same_v<std::decay_t<AttachedData>, ReductAttachedData>);
          // 不能写反，否则当data_already_in为ShiftReductAttachedData时
          // 无法获得与其它附属数据比较的正确语义
          // 可能是MSVC的BUG
          // 使用隐式调用会导致调用attached_data的operator==，所以显式调用
          if (!data_already_in.operator==(attached_data)) {
            // 设置相同条件不同产生式下规约
            std::cerr << std::format(
                "一个项集在相同条件下只能规约一种产生式，请参考该项"
                "集求闭包时的输出信息检查产生式");
            exit(-1);
          }
        }
        break;
      case ActionType::kShiftReduct: {
        // 可能是MSVC的BUG
        // 使用隐式调用会导致调用attached_data的operator==，所以显式调用
        if (!data_already_in.operator==(attached_data)) {
          // 设置相同条件不同产生式下规约
          std::cerr << std::format(
              "一个项集在相同条件下只能规约一种产生式，请参考该项"
              "集求闭包时的输出信息检查产生式");
          exit(-1);
        }
        break;
      }
      default:
        assert(false);
        break;
    }
  }
}
};  // namespace frontend::generator::syntax_generator

#endif  // !GENERATOR_SYNTAXGENERATOR_SYNTAX_ANALYSIS_TABLE_H_