/// @file syntax_analysis_table.h
/// @brief 语法分析表
/// @details
/// 语法分析表序列化到文件中，使用时只需反序列化
#ifndef GENERATOR_SYNTAXGENERATOR_SYNTAX_ANALYSIS_TABLE_H_
#define GENERATOR_SYNTAXGENERATOR_SYNTAX_ANALYSIS_TABLE_H_

#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <format>
#include <iostream>

#include "Generator/export_types.h"

namespace frontend::generator::syntax_generator {

/// @class SynatxAnalysisTableEntry syntax_analysis_table.h
/// @brief 语法分析表单个条目
class SyntaxAnalysisTableEntry {
 public:
  // 前向声明三种派生类，为了虚函数可以返回相应的类型
  class ShiftAttachedData;
  class ReductAttachedData;
  class ShiftReductAttachedData;

  /// @class ActionAndAttachedDataInterface syntax_analysis_table.h
  /// @brief 动作和附属数据基类
  /// @details
  /// 动作是根据向前看符号决定移入/规约/接受/报错等
  /// 附属数据是完成动作后维持状态机基础性质所需要的数据，例如移入单词后转移到的
  /// 下一个条目，规约后得到的非终结产生式ID等
  /// @attention 所有的动作和附属数据均应从该类派生
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

    /// @brief 获取移入操作的附属数据
    /// @return 返回移入操作的附属数据的const引用
    /// @attention 必须存在移入附属数据才可以调用该函数
    virtual const ShiftAttachedData& GetShiftAttachedData() const;
    /// @brief 获取规约操作的附属数据
    /// @return 返回规约操作的附属数据的const引用
    /// @attention 必须存在规约附属数据才可以调用该函数
    virtual const ReductAttachedData& GetReductAttachedData() const;
    /// @brief 获取移入和规约操作的附属数据
    /// @return 返回移入和规约操作的附属数据的const引用
    /// @note 由于二义性文法，有时在一个状态下既可以移入也可以规约
    /// @attention 必须存在移入和规约附属数据才可以调用该函数
    virtual const ShiftReductAttachedData& GetShiftReductAttachedData() const;
    /// @brief 判断两个对象是否相应部分相同
    /// @param[in] action_and_attached_data ：用来比较的另一部分数据
    /// @attention
    /// 与operator==()在ShiftReductAttachedData语义上不同
    /// operator==()比较两个对象是否完全相同，IsSameOrPart入参如果是
    /// ShiftAttachedData或ReductAttachedData则只比较相应部分
    virtual bool IsSameOrPart(const ActionAndAttachedDataInterface&
                                  action_and_attached_data) const = 0;

    /// @brief 获取动作类型
    /// @return 返回动作类型
    /// @retval ActionType::kShift ：移入
    /// @retval ActionType::kReduct ：规约
    /// @retval ActionType::kShiftReduct ：移入和规约
    /// @retval ActionType::kError ：报错
    /// @retval ActionType::kAccept ：接受
    ActionType GetActionType() const { return action_type_; }
    /// @brief 设置动作类型
    /// @param[in] action_type ：待设置的动作类型
    void SetActionType(ActionType action_type) { action_type_ = action_type; }

    /// @brief 序列化该类的函数
    /// @param[in,out] ar ：序列化使用的档案
    /// @param[in] version ：序列化文件版本
    /// @attention 该函数应由boost库调用而非手动调用
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& action_type_;
    }

   private:
    /// @brief 提供默认构造函数供序列化时构建对象
    ActionAndAttachedDataInterface() = default;

    /// @brief 允许序列化用的类访问
    friend class boost::serialization::access;
    /// @brief 允许语法分析表条目调用内部接口
    friend class SyntaxAnalysisTableEntry;

    /// @brief 获取移入操作的附属数据
    /// @return 返回移入操作的附属数据的引用
    /// @attention 必须存在移入附属数据才可以调用该函数
    virtual ShiftAttachedData& GetShiftAttachedData() {
      return const_cast<ShiftAttachedData&>(
          static_cast<const ActionAndAttachedDataInterface&>(*this)
              .GetShiftAttachedData());
    }
    /// @brief 获取规约操作的附属数据
    /// @return 返回规约操作的附属数据的引用
    /// @attention 必须存在规约附属数据才可以调用该函数
    virtual ReductAttachedData& GetReductAttachedData() {
      return const_cast<ReductAttachedData&>(
          static_cast<const ActionAndAttachedDataInterface&>(*this)
              .GetReductAttachedData());
    }
    /// @brief 获取移入和规约操作的附属数据
    /// @return 返回移入和规约操作的附属数据的引用
    /// @note 由于二义性文法，有时在一个状态下既可以移入也可以规约
    /// @attention 必须存在移入和规约附属数据才可以调用该函数
    virtual ShiftReductAttachedData& GetShiftReductAttachedData() {
      return const_cast<ShiftReductAttachedData&>(
          static_cast<const ActionAndAttachedDataInterface&>(*this)
              .GetShiftReductAttachedData());
    }

    /// @brief 动作类型
    ActionType action_type_;
  };

  /// @class ShiftAttachedData syntax_analysis_table.h
  /// @brief 执行移入动作时的附属数据
  class ShiftAttachedData : public ActionAndAttachedDataInterface {
   public:
    ShiftAttachedData(SyntaxAnalysisTableEntryId next_entry_id)
        : ActionAndAttachedDataInterface(ActionType::kShift),
          next_entry_id_(next_entry_id) {}
    ShiftAttachedData(const ShiftAttachedData&) = default;

    ShiftAttachedData& operator=(const ShiftAttachedData&) = default;
    virtual bool operator==(const ActionAndAttachedDataInterface&
                                shift_attached_data) const override;

    /// @brief 获取移入操作的附属数据
    /// @return 返回移入操作的附属数据的const引用
    /// @attention 必须存在移入附属数据才可以调用该函数
    virtual const ShiftAttachedData& GetShiftAttachedData() const override {
      return *this;
    }
    /// @brief 判断两个对象是否相应部分相同
    /// @param[in] shift_attached_data ：用来比较的另一部分数据
    /// @attention
    /// 与operator==()在this为ShiftReductAttachedData类型时语义上不同
    /// operator==()比较两个对象是否完全相同，IsSameOrPart入参如果是
    /// ShiftAttachedData或ReductAttachedData则只比较相应部分
    virtual bool IsSameOrPart(const ActionAndAttachedDataInterface&
                                  shift_attached_data) const override {
      return operator==(shift_attached_data);
    }

    /// @brief 获取移入单词后转移到的语法分析表条目ID
    /// @return 返回移入单词后转移到的语法分析表条目ID
    SyntaxAnalysisTableEntryId GetNextSyntaxAnalysisTableEntryId() const {
      return next_entry_id_;
    }
    /// @brief 设置移入单词后转移到的语法分析表条目ID
    /// @param[in] next_entry_id ：移入单词后转移到的语法分析表条目ID
    void SetNextSyntaxAnalysisTableEntryId(
        SyntaxAnalysisTableEntryId next_entry_id) {
      next_entry_id_ = next_entry_id;
    }

   private:
    /// @brief 提供默认构造函数供序列化时构建对象
    ShiftAttachedData() = default;

    /// @brief 允许序列化类访问
    friend class boost::serialization::access;
    /// @brief 允许语法分析表条目访问内部接口
    friend class SyntaxAnalysisTableEntry;

    /// @brief 序列化该类的函数
    /// @param[in,out] ar ：序列化使用的档案
    /// @param[in] version ：序列化文件版本
    /// @attention 该函数应由boost库调用而非手动调用
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& boost::serialization::base_object<ActionAndAttachedDataInterface>(
          *this);
      ar& next_entry_id_;
    }

    /// @brief 获取移入操作的附属数据
    /// @return 返回移入操作的附属数据的引用
    /// @attention 必须存在移入附属数据才可以调用该函数
    virtual ShiftAttachedData& GetShiftAttachedData() override { return *this; }

    /// @brief 移入该单词后转移到的语法分析表条目ID
    SyntaxAnalysisTableEntryId next_entry_id_;
  };

  /// @class ReductAttachedData syntax_analysis_table.h
  /// @brief 执行规约动作时的附属数据
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

    /// @brief 获取规约操作的附属数据
    /// @return 返回规约操作的附属数据的const引用
    /// @attention 必须存在规约附属数据才可以调用该函数
    virtual const ReductAttachedData& GetReductAttachedData() const override {
      return *this;
    }
    /// @brief 判断两个对象是否相应部分相同
    /// @param[in] reduct_attached_data ：用来比较的另一部分数据
    /// @attention
    /// 与operator==()在this为ShiftReductAttachedData类型时语义上不同
    /// operator==()比较两个对象是否完全相同，IsSameOrPart入参如果是
    /// ShiftAttachedData或ReductAttachedData则只比较相应部分
    virtual bool IsSameOrPart(const ActionAndAttachedDataInterface&
                                  reduct_attached_data) const override {
      return operator==(reduct_attached_data);
    }

    /// @brief 获取规约得到的非终结产生式ID
    /// @return 返回规约后得到的非终结产生式ID
    ProductionNodeId GetReductedNonTerminalNodeId() const {
      return reducted_nonterminal_node_id_;
    }
    /// @brief 设置规约后得到的非终结产生式ID
    /// @param[in] reducted_nonterminal_node_id
    /// ：待设置的规约后得到的非终结产生式ID
    void SetReductedNonTerminalNodeId(
        ProductionNodeId reducted_nonterminal_node_id) {
      reducted_nonterminal_node_id_ = reducted_nonterminal_node_id;
    }
    /// @brief 获取包装规约函数的类的实例化对象ID
    /// @return 返回包装规约函数的类的实例化对象
    ProcessFunctionClassId GetProcessFunctionClassId() const {
      return process_function_class_id_;
    }
    /// @brief 设置包装规约函数的类的实例化对象ID
    /// @param[in] process_function_class_id
    /// ：待设置的包装规约函数的类的实例化对象ID
    void SetProcessFunctionClassId(
        ProcessFunctionClassId process_function_class_id) {
      process_function_class_id_ = process_function_class_id;
    }
    /// @brief 获取产生式体
    /// @return 返回产生式体的const引用
    /// @note 产生式体用于核对获取到了哪些单词，从而判断哪些产生式空规约
    const std::vector<ProductionNodeId>& GetProductionBody() const {
      return production_body_;
    }
    /// @brief 设置产生式体
    /// @param[in] production_body ：待设置的产生式体
    void SetProductionBody(std::vector<ProductionNodeId>&& production_body) {
      production_body_ = std::move(production_body);
    }

   private:
    /// @brief 提供默认构造函数供序列化时构建对象
    ReductAttachedData() = default;

    /// @brief 允许序列化类访问
    friend class boost::serialization::access;
    /// @brief 允许语法分析表条目访问内部接口
    friend class SyntaxAnalysisTableEntry;

    /// @brief 序列化该类的函数
    /// @param[in,out] ar ：序列化使用的档案
    /// @param[in] version ：序列化文件版本
    /// @attention 该函数应由boost库调用而非手动调用
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& boost::serialization::base_object<ActionAndAttachedDataInterface>(
          *this);
      ar& reducted_nonterminal_node_id_;
      ar& process_function_class_id_;
      ar& production_body_;
    }

    /// @brief 获取规约操作的附属数据
    /// @return 返回规约操作的附属数据的引用
    /// @attention 必须存在规约附属数据才可以调用该函数
    virtual ReductAttachedData& GetReductAttachedData() override {
      return *this;
    }

    /// @brief 规约后得到的非终结节点的ID
    ProductionNodeId reducted_nonterminal_node_id_;
    /// @brief 执行规约操作时使用的对象的ID
    ProcessFunctionClassId process_function_class_id_;
    /// @brief 规约所用产生式，用于核对该产生式包含哪些节点
    /// @note 不使用空规约功能则可改为产生式节点数目
    std::vector<ProductionNodeId> production_body_;
  };

  /// @class ShiftReductAttachedData syntax_analysis_table.h
  /// @brief 移入与规约动作的附属数据
  /// @note 使用二义性文法时某些状态下既可以移入也可以规约
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

    virtual bool operator==(
        const ActionAndAttachedDataInterface& attached_data) const override;

    /// @brief 获取移入操作的附属数据
    /// @return 返回移入操作的附属数据的const引用
    /// @attention 必须存在移入附属数据才可以调用该函数
    virtual const ShiftAttachedData& GetShiftAttachedData() const override {
      return shift_attached_data_;
    }
    /// @brief 获取规约操作的附属数据
    /// @return 返回规约操作的附属数据的const引用
    /// @attention 必须存在规约附属数据才可以调用该函数
    virtual const ReductAttachedData& GetReductAttachedData() const override {
      return reduct_attached_data_;
    }
    /// @brief 获取移入和规约操作的附属数据
    /// @return 返回移入和规约操作的附属数据的const引用
    /// @note 由于二义性文法，有时在一个状态下既可以移入也可以规约
    /// @attention 必须存在移入和规约附属数据才可以调用该函数
    virtual const ShiftReductAttachedData& GetShiftReductAttachedData()
        const override {
      return *this;
    }
    /// @brief 判断两个对象是否相应部分相同
    /// @param[in] attached_data ：用来比较的另一部分数据
    /// @attention
    /// 与operator==()在this为ShiftReductAttachedData类型时语义上不同
    /// operator==()比较两个对象是否完全相同，IsSameOrPart入参如果是
    /// ShiftAttachedData或ReductAttachedData则只比较相应部分
    virtual bool IsSameOrPart(
        const ActionAndAttachedDataInterface& attached_data) const override;

    /// @brief 设置移入附属数据
    /// @param[in] shift_attached_data ：待设置的移入数据
    void SetShiftAttachedData(ShiftAttachedData&& shift_attached_data) {
      shift_attached_data_ = std::move(shift_attached_data);
    }
    /// @brief 设置规约附属数据
    /// @param[in] reduct_attached_data ：待设置的规约数据
    void SetReductAttachedData(ReductAttachedData&& reduct_attached_data) {
      reduct_attached_data_ = std::move(reduct_attached_data);
    }

   private:
    /// @brief 提供默认构造函数供序列化时构建对象
    ShiftReductAttachedData() = default;

    /// @brief 允许序列化类访问
    friend class boost::serialization::access;
    /// @brief 允许语法分析表条目访问内部接口
    friend class SyntaxAnalysisTableEntry;

    /// @brief 序列化该类的函数
    /// @param[in,out] ar ：序列化使用的档案
    /// @param[in] version ：序列化文件版本
    /// @attention 该函数应由boost库调用而非手动调用
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& boost::serialization::base_object<ActionAndAttachedDataInterface>(
          *this);
      ar& shift_attached_data_;
      ar& reduct_attached_data_;
    }
    /// @brief 获取移入操作的附属数据
    /// @return 返回移入操作的附属数据的引用
    /// @attention 必须存在移入附属数据才可以调用该函数
    virtual ShiftAttachedData& GetShiftAttachedData() override {
      return shift_attached_data_;
    }
    /// @brief 获取规约操作的附属数据
    /// @return 返回规约操作的附属数据的引用
    /// @attention 必须存在规约附属数据才可以调用该函数
    virtual ReductAttachedData& GetReductAttachedData() override {
      return reduct_attached_data_;
    }

    /// @brief 获取移入和规约操作的附属数据
    /// @return 返回移入和规约操作的附属数据的引用
    /// @note 由于二义性文法，有时在一个状态下既可以移入也可以规约
    /// @attention 必须存在移入和规约附属数据才可以调用该函数
    virtual ShiftReductAttachedData& GetShiftReductAttachedData() override {
      return *this;
    }

    /// @brief 移入附属数据
    ShiftAttachedData shift_attached_data_;
    /// @brief 规约附属数据
    ReductAttachedData reduct_attached_data_;
  };

  /// @class AcceptAttachedData syntax_analysis_table.h
  /// @brief 表示Accept动作的节点
  class AcceptAttachedData : public ActionAndAttachedDataInterface {
   public:
    AcceptAttachedData()
        : ActionAndAttachedDataInterface(ActionType::kAccept) {}
    AcceptAttachedData(const AcceptAttachedData&) = delete;
    AcceptAttachedData(AcceptAttachedData&&) = default;

    AcceptAttachedData& operator=(const AcceptAttachedData&) = delete;
    AcceptAttachedData& operator=(AcceptAttachedData&&) = delete;

    /// @attention Accept语义下不支持该操作
    virtual bool operator==(
        const ActionAndAttachedDataInterface&) const override {
      assert(false);
      /// 防止警告
      return false;
    }
    /// @attention Accept语义下不支持该操作
    virtual bool IsSameOrPart(const ActionAndAttachedDataInterface&
                                  accept_attached_data) const override {
      assert(false);
      /// 防止警告
      return false;
    }

   private:
    /// @brief 允许序列化类访问
    friend class boost::serialization::access;

    /// @brief 序列化该类的函数
    /// @param[in,out] ar ：序列化使用的档案
    /// @param[in] version ：序列化文件版本
    /// @attention 该函数应由boost库调用而非手动调用
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& boost::serialization::base_object<ActionAndAttachedDataInterface>(
          *this);
    }
  };

  /// @brief 语法分析表条目中存储向前看符号对应的动作和附属数据的容器
  /// @note 键值为待移入节点ID，值为指向动作和附属数据的指针
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

  /// @brief 设置该条目在给定终结节点下的动作和目标节点
  /// @param[in] node_id ：待处理的终结节点ID
  /// @param[in] attached_data ：终结节点的附属数据
  /// @details
  /// 该函数设置在面对node_id代表的终结节点时需要做的操作和操作的附属数据
  /// 第二个参数仅支持ShiftAttachedData、ReductAttachedData和AcceptAttachedData
  /// @note 待添加的操作类型必须与node_id已有的动作类型不同，或动作类型和
  /// 附属数据完全相同
  template <class AttachedData>
  requires std::is_same_v<std::decay_t<AttachedData>,
                          SyntaxAnalysisTableEntry::ShiftAttachedData> ||
      std::is_same_v<std::decay_t<AttachedData>,
                     SyntaxAnalysisTableEntry::ReductAttachedData> ||
      std::is_same_v<std::decay_t<AttachedData>,
                     SyntaxAnalysisTableEntry::AcceptAttachedData>
  void SetTerminalNodeActionAndAttachedData(ProductionNodeId node_id,
                                            AttachedData&& attached_data);
  /// @brief 设置该条目移入非终结节点后转移到的语法分析表条目ID
  /// @param[in] node_id ：待处理的非终结节点ID
  /// @param[in] next_analysis_table_entry_id
  /// ：移入该非终结节点后转移到的语法分析表条目ID
  void SetNonTerminalNodeTransformId(
      ProductionNodeId node_id,
      SyntaxAnalysisTableEntryId next_analysis_table_entry_id) {
    nonterminal_node_transform_table_[node_id] = next_analysis_table_entry_id;
  }
  /// @brief 修改语法分析表中所有指定的语法分析表ID为新ID
  /// @param[in] old_id_to_new_id ：存储待修改的ID到新ID的映射
  /// @note old_id_to_new_id仅需存储需要更改的ID，不改变的ID无需存储
  void ResetEntryId(
      const std::unordered_map<SyntaxAnalysisTableEntryId,
                               SyntaxAnalysisTableEntryId>& old_id_to_new_id);
  /// @brief 获取在给定待处理终结节点ID下的动作与附属数据
  /// @param[in] node_id ：待处理的终结节点ID
  /// @return 返回指向动作与附属数据的const指针
  /// @retval nullptr ：该终结节点下不存在可以执行的动作（语法错误）
  const ActionAndAttachedDataInterface* AtTerminalNode(
      ProductionNodeId node_id) const {
    auto iter = action_and_attached_data_.find(node_id);
    return iter == action_and_attached_data_.end() ? nullptr
                                                   : iter->second.get();
  }
  /// @brief 获取移入给定非终结节点后转移到的语法分析表条目ID
  /// @param[in] node_id ：待移入的非终结节点ID
  /// @return 返回移入后转移到的语法分析表条目ID
  /// @retval SyntaxAnalysisTableEntryId::InvalidId()
  /// ：该非终结节点无法移入（语法错误）
  SyntaxAnalysisTableEntryId AtNonTerminalNode(ProductionNodeId node_id) const {
    auto iter = nonterminal_node_transform_table_.find(node_id);
    return iter == nonterminal_node_transform_table_.end()
               ? SyntaxAnalysisTableEntryId::InvalidId()
               : iter->second;
  }
  /// @brief 获取全部终结节点的动作和附属数据
  /// @return 返回存储全部终结节点的动作和附属数据的容器的const引用
  const ActionAndTargetContainer& GetAllActionAndAttachedData() const {
    return action_and_attached_data_;
  }
  /// @brief 获取全部非终结节点转移到的语法分析表条目ID
  /// @return
  /// 返回存储全部非终结节点移入后转移到的语法分析表条目ID的容器的const引用
  const std::unordered_map<ProductionNodeId, SyntaxAnalysisTableEntryId>&
  GetAllNonTerminalNodeTransformTarget() const {
    return nonterminal_node_transform_table_;
  }
  /// @brief 设置在文件尾向前看节点下执行Accept动作
  /// @param[in] eof_node_id ：文件尾向前看节点
  /// @note 文件尾节点永远不会被移入
  /// @attention 仅允许对内置根条目执行该操作
  void SetAcceptInEofForwardNode(ProductionNodeId eof_node_id) {
    SetTerminalNodeActionAndAttachedData(eof_node_id, AcceptAttachedData());
  }
  /// @brief 清除该条目中所有数据
  void Clear() {
    action_and_attached_data_.clear();
    nonterminal_node_transform_table_.clear();
  }

 private:
  /// @brief 允许序列化类访问
  friend class boost::serialization::access;

  /// @brief 序列化该类的函数
  /// @param[in,out] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& action_and_attached_data_;
    ar& nonterminal_node_transform_table_;
  }

  /// @brief 获取全部终结节点的动作和附属数据
  /// @return 返回存储全部终结节点的动作和附属数据的容器的引用
  ActionAndTargetContainer& GetAllActionAndAttachedData() {
    return const_cast<ActionAndTargetContainer&>(
        static_cast<const SyntaxAnalysisTableEntry&>(*this)
            .GetAllActionAndAttachedData());
  }
  /// @brief 获取全部非终结节点转移到的语法分析表条目ID
  /// @return 返回存储全部非终结节点移入后转移到的语法分析表条目ID的容器的引用
  std::unordered_map<ProductionNodeId, SyntaxAnalysisTableEntryId>&
  GetAllNonTerminalNodeTransformTarget() {
    return const_cast<
        std::unordered_map<ProductionNodeId, SyntaxAnalysisTableEntryId>&>(
        static_cast<const SyntaxAnalysisTableEntry&>(*this)
            .GetAllNonTerminalNodeTransformTarget());
  }

  /// @brief 向前看符号ID下的操作和目标节点
  ActionAndTargetContainer action_and_attached_data_;
  /// @brief 移入非终结节点后转移到的产生式体序号
  std::unordered_map<ProductionNodeId, SyntaxAnalysisTableEntryId>
      nonterminal_node_transform_table_;
};

template <class AttachedData>
requires std::is_same_v<std::decay_t<AttachedData>,
                        SyntaxAnalysisTableEntry::ShiftAttachedData> ||
    std::is_same_v<std::decay_t<AttachedData>,
                   SyntaxAnalysisTableEntry::ReductAttachedData> ||
    std::is_same_v<std::decay_t<AttachedData>,
                   SyntaxAnalysisTableEntry::AcceptAttachedData>
void SyntaxAnalysisTableEntry::SetTerminalNodeActionAndAttachedData(
    ProductionNodeId node_id, AttachedData&& attached_data) {
  if constexpr (std::is_same_v<std::decay_t<AttachedData>,
                               SyntaxAnalysisTableEntry::AcceptAttachedData>) {
    // 传播向前看符号后，根语法分析表条目移入内置根节点后到达的条目在向前看符号
    // 是文件尾节点时执行规约操作，需要设置为接受操作
    assert(action_and_attached_data_.find(node_id)->second->GetActionType() ==
           ActionType::kReduct);
    action_and_attached_data_[node_id] = std::make_unique<AcceptAttachedData>(
        std::forward<AttachedData>(attached_data));
    return;
  } else {
    static_assert(
        !std::is_same_v<std::decay_t<AttachedData>, ShiftReductAttachedData>,
        "该类型仅允许通过在已有一种动作的基础上补全缺少的另一半后转换得到，不允"
        "许直接传入");
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
            if (!data_already_in.IsSameOrPart(attached_data)) [[unlikely]] {
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
            if (!data_already_in.IsSameOrPart(attached_data)) [[unlikely]] {
              // 设置相同条件不同产生式下规约
              std::cerr << std::format(
                  "一个项集在相同条件下只能规约一种产生式，请参考该项"
                  "集求闭包时的输出信息检查产生式");
              exit(-1);
            }
          }
          break;
        case ActionType::kShiftReduct: {
          if (!data_already_in.IsSameOrPart(attached_data)) [[unlikely]] {
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
}
};  // namespace frontend::generator::syntax_generator

#endif  // !GENERATOR_SYNTAXGENERATOR_SYNTAX_ANALYSIS_TABLE_H_