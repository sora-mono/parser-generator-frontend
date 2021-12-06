/// @file production_node.h
/// @brief 描述产生式的类
/// @details
/// 该文件中定义多个描述产生式的类
/// TerminalProductionNode ：终结产生式
/// OperatorProductionNode ：运算符
/// NonTerminalProductionNode ：非终结产生式
/// EndNode ：文件尾
/// 这些类用来表示不同产生式
#ifndef GENERATOR_SYNTAXGENERATOR_PRODUCTION_NODE_H_
#define GENERATOR_SYNTAXGENERATOR_PRODUCTION_NODE_H_
#include "Generator/export_types.h"

namespace frontend::generator::syntax_generator {
/// @class BaseProductionNode production_node.h
/// @brief 该类是所有表示具体产生式类别的派生类的基类
/// @details
/// 该类中包含：节点类型、节点ID、产生式名ID
/// @attention 所有表示具体产生式的派生类都应继承自该类
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

  /// @brief 设置产生式类型
  /// @param[in] type ：产生式类型
  void SetType(ProductionNodeType type) { base_type_ = type; }
  /// @brief 获取产生式类型
  /// @return 返回产生式类型
  ProductionNodeType GetType() const { return base_type_; }
  /// @brief 设置该节点ID
  /// @param[in] production_node_id ：待设置的节点ID
  void SetNodeId(ProductionNodeId production_node_id) {
    base_id_ = production_node_id;
  }
  /// @brief 获取该节点ID
  /// @return 返回该节点ID
  ProductionNodeId GetNodeId() const { return base_id_; }
  /// @brief 设置该节点表示的产生式名ID
  /// @param[in] production_node_id ：待设置的产生式名ID
  void SetSymbolId(SymbolId production_node_id) {
    base_symbol_id_ = production_node_id;
  }
  /// @brief 获取该节点表示的产生式名ID
  /// @return 返回该节点表示的产生式名ID
  SymbolId GetNodeSymbolId() const { return base_symbol_id_; }

  /// @brief 根据给定产生式体和下一个移入的产生式位置获取下一个移入的产生式ID
  /// @param[in] production_body_id ：子产生式体标号
  /// @param[in] next_word_to_shift_index ：产生式体中产生式ID的位置
  /// @return 返回production_body_id和next_word_to_shift_index指定的产生式ID
  /// @details
  /// point_index越界（大于等于production_body_id指定的子产生式中产生式数目）时
  /// 返回ProducNodeId::InvalidId()
  /// production_body_id不得超出该产生式拥有的产生式体个数
  /// 终结产生式和运算符产生式只存在一个子产生式，子产生式中只含有一个产生式，
  /// 所以应使用
  /// ProductionBodyId(0),NextWordToShiftIndex(0)才可以获取有效产生式ID
  /// @note
  /// 使用举例：
  ///   产生式例：
  ///   IdOrEquivence -> ConstTag Id
  ///   IdOrEquivence -> IdOrEquivence "[" Num "]"
  ///   IdOrEquivence -> IdOrEquivence "[" "]"
  ///   IdOrEquivence -> ConstTag "*" IdOrEquivence
  /// 获取IdOrEquivence产生式内第二个子产生式中第三个产生式（"Num"）的ID
  /// @code{.cpp}
  ///   NonTerminalProductionNode example_node;
  ///   example_node.GetProductionNodeInBody(ProductionBodyId(1),
  ///                                        NextWordToShiftIndex(2));
  /// @endcode
  /// @attention 所有派生类均应重写该函数，不得改变原始语义
  virtual ProductionNodeId GetProductionNodeInBody(
      ProductionBodyId production_body_id,
      NextWordToShiftIndex next_word_to_shift_index) const = 0;

 private:
  /// @brief 产生式类型
  ProductionNodeType base_type_;
  /// @brief 节点ID
  ProductionNodeId base_id_;
  /// @brief 产生式名ID
  SymbolId base_symbol_id_;
};

/// @class TerminalProductionNode production_node.h
/// @brief 表示终结产生式
/// @note
/// 终结产生式：
/// Id    ->    [a-zA-Z_][a-zA-Z0-9_]*
/// ^产生式名   ^产生式体名
class TerminalProductionNode : public BaseProductionNode {
 public:
  TerminalProductionNode(SymbolId node_symbol_id, SymbolId body_symbol_id)
      : BaseProductionNode(ProductionNodeType::kTerminalNode, node_symbol_id) {
    SetBodySymbolId(body_symbol_id);
  }
  TerminalProductionNode(const TerminalProductionNode&) = delete;
  TerminalProductionNode& operator=(const TerminalProductionNode&) = delete;
  TerminalProductionNode(TerminalProductionNode&& terminal_production_node)
      : BaseProductionNode(std::move(terminal_production_node)),
        body_symbol_id_(std::move(terminal_production_node.body_symbol_id_)) {}
  TerminalProductionNode& operator=(
      TerminalProductionNode&& terminal_production_node) {
    BaseProductionNode::operator=(std::move(terminal_production_node));
    body_symbol_id_ = std::move(terminal_production_node.body_symbol_id_);
    return *this;
  }

  /// @brief 获取产生式体名ID
  /// @return 返回产生式体名ID
  SymbolId GetBodySymbolId() const { return body_symbol_id_; }
  /// @brief 设置产生式体名ID
  /// @param[in] body_symbol_id ：待设置的产生式体名ID
  void SetBodySymbolId(SymbolId body_symbol_id) {
    body_symbol_id_ = body_symbol_id;
  }
  /// @brief 获取产生式体内的产生式
  /// @param[in] production_body_id ：待获取的产生式体ID
  /// @param[in] next_word_to_shift_index ：待获取的子产生式位置
  /// @attention 终结节点调用该函数时production_body_id必须为0
  /// next_word_to_shift_index不是0时则返回NextWordToShiftIndex::InvalidId()
  virtual ProductionNodeId GetProductionNodeInBody(
      ProductionBodyId production_body_id,
      NextWordToShiftIndex next_word_to_shift_index) const override;

 private:
  /// @brief 产生式体名
  SymbolId body_symbol_id_;
};

/// @class OperatorNodeInterface production_node.h
/// @brief 表示运算符的类的基类
/// @details
/// 运算符支持区分双目运算符语义和单目运算符语义
/// 双目运算符语义使用时运算符所在的产生式体中两侧都应存在产生式
/// 例：
/// Example-> Example1 "+" Example2
/// 其中"+"为运算符，这里使用双目运算符语义
/// 单目运算符语义仅支持左侧单目运算符语义，不支持右侧单目运算符语义
/// 例：
/// Assignable -> "!" Assignable
/// 其中"!"为运算符，这里使用单目运算符语义
/// @attention 所有表示运算符的类都应从该类派生
class OperatorNodeInterface : public BaseProductionNode {
 public:
  enum class OperatorType {
    kBinary,     // 双目运算符
    kLeftUnary,  // 左侧单目运算符
    kBinaryLeftUnary  // 同时具有双目运算符语义和左侧单目运算符语义的运算符
  };
  /// @brief 获取产生式体内的产生式
  /// @param[in] production_body_id ：待获取的产生式体ID
  /// @param[in] next_word_to_shift_index ：待获取的子产生式位置
  /// @attention 终结节点调用该函数时production_body_id必须为0
  /// next_word_to_shift_index不是0时则返回NextWordToShiftIndex::InvalidId()
  virtual ProductionNodeId GetProductionNodeInBody(
      ProductionBodyId production_body_id,
      NextWordToShiftIndex next_word_to_shift_index) const final;

  /// @brief 设置运算符类型
  /// @param[in] operator_type ：新的运算符类型
  void SetOperatorType(OperatorType operator_type) {
    operator_type_ = operator_type;
  }
  /// @brief 获取运算符类型
  /// @return 返回运算符类型
  /// @retval OperatorType::kBinary ：双目运算符
  /// @retval OperatorType::kLeftUnary ：左侧单目运算符
  /// @retval OperatorType::kBinaryLeftUnary
  /// ：同时具有双目运算符与左侧单目运算符语义
  OperatorType GetOperatorType() const { return operator_type_; }

 protected:
  OperatorNodeInterface(OperatorType operator_type, SymbolId operator_symbol_id)
      : BaseProductionNode(ProductionNodeType::kOperatorNode,
                           operator_symbol_id),
        operator_type_(operator_type) {}
  OperatorNodeInterface(const OperatorNodeInterface&) = delete;
  OperatorNodeInterface& operator=(const OperatorNodeInterface&) = delete;
  OperatorNodeInterface(OperatorNodeInterface&& operator_production_node)
      : BaseProductionNode(std::move(operator_production_node)),
        operator_type_(std::move(operator_production_node.operator_type_)) {}
  OperatorNodeInterface& operator=(
      OperatorNodeInterface&& operator_production_node);

 private:
  OperatorType operator_type_;
};

/// @class BinaryOperatorNode production_node.h
/// @brief 表示双目运算符
class BinaryOperatorNode : public OperatorNodeInterface {
 public:
  BinaryOperatorNode(SymbolId operator_symbol_id,
                     OperatorAssociatityType binary_operator_associatity_type,
                     OperatorPriority binary_operator_priority)
      : OperatorNodeInterface(OperatorType::kBinary, operator_symbol_id),
        unary_operator_associatity_type_(binary_operator_associatity_type),
        unary_operator_priority_(binary_operator_priority) {}

  /// @brief 设置运算符结合性
  /// @param[in] type ：待设置的运算符结合性
  /// @note
  /// 运算符结合性分从左到右结合和从右到左结合
  void SetAssociatityType(OperatorAssociatityType type) {
    unary_operator_associatity_type_ = type;
  }
  /// @brief 获取运算符结合性
  /// @return 返回运算符结合性
  /// @retval OperatorAssociatityType::kLeftToRight ：从左到右结合
  /// @retval OperatorAssociatityType::kRightToLeft ：从右到左结合
  OperatorAssociatityType GetAssociatityType() const {
    return unary_operator_associatity_type_;
  }
  /// @brief 设置运算符优先级等级
  /// @param[in] priority ：待设置的运算符优先级等级
  /// @attention 运算符优先级与单词优先级意义不同
  /// 运算符优先级工作在语法分析，只有运算符拥有
  /// 单词优先级工作在词法分析，每个单词都有
  void SetPriority(OperatorPriority priority) {
    unary_operator_priority_ = priority;
  }
  /// @brief 获取运算符优先级
  /// @return 返回运算符优先级
  OperatorPriority GetPriority() const { return unary_operator_priority_; }

 private:
  /// @brief 运算符结合性
  OperatorAssociatityType unary_operator_associatity_type_;
  /// @brief 运算符优先级
  OperatorPriority unary_operator_priority_;
};

/// @class UnaryOperatorNode production_node.h
/// @brief 表示单目运算符
/// @attention 仅支持左侧单目运算符
class UnaryOperatorNode : public OperatorNodeInterface {
 public:
  UnaryOperatorNode(SymbolId operator_symbol_id,
                    OperatorAssociatityType unary_operator_associatity_type,
                    OperatorPriority unary_operator_priority)
      : OperatorNodeInterface(OperatorType::kLeftUnary, operator_symbol_id),
        unary_operator_associatity_type_(unary_operator_associatity_type),
        unary_operator_priority_(unary_operator_priority) {}

  /// @brief 设置运算符结合性
  /// @param[in] type ：待设置的运算符结合性
  /// @note
  /// 运算符结合性分从左到右结合和从右到左结合
  void SetAssociatityType(OperatorAssociatityType type) {
    unary_operator_associatity_type_ = type;
  }
  /// @brief 获取运算符结合性
  /// @return 返回运算符结合性
  /// @retval OperatorAssociatityType::kLeftToRight ：从左到右结合
  /// @retval OperatorAssociatityType::kRightToLeft ：从右到左结合
  OperatorAssociatityType GetAssociatityType() const {
    return unary_operator_associatity_type_;
  }
  /// @brief 设置运算符优先级等级
  /// @param[in] priority ：待设置的运算符优先级等级
  /// @attention 运算符优先级与单词优先级意义不同
  /// 运算符优先级工作在语法分析，只有运算符拥有
  /// 单词优先级工作在词法分析，每个单词都有
  void SetPriority(OperatorPriority priority) {
    unary_operator_priority_ = priority;
  }
  /// @brief 获取运算符优先级
  /// @return 返回运算符优先级
  OperatorPriority GetPriority() const { return unary_operator_priority_; }

 private:
  /// @brief 运算符结合性
  OperatorAssociatityType unary_operator_associatity_type_;
  /// @brief 运算符优先级
  OperatorPriority unary_operator_priority_;
};

/// @class BinaryUnaryOperatorNode production_node.h
/// @brief 表示同时具有双目运算符和单目运算符语义的运算符
/// @attention 仅支持左侧单目运算符
class BinaryUnaryOperatorNode : public OperatorNodeInterface {
 public:
  BinaryUnaryOperatorNode(
      SymbolId operator_symbol_id,
      OperatorAssociatityType binary_operator_associatity_type,
      OperatorPriority binary_operator_priority,
      OperatorAssociatityType unary_operator_associatity_type,
      OperatorPriority unary_operator_priority)
      : OperatorNodeInterface(OperatorType::kBinaryLeftUnary,
                              operator_symbol_id),
        binary_operator_associatity_type_(binary_operator_associatity_type),
        binary_operator_priority_(binary_operator_priority),
        unary_operator_associatity_type_(unary_operator_associatity_type),
        unary_operator_priority_(unary_operator_priority) {}

  /// @brief 设置双目运算符结合性
  /// @param[in] type ：待设置的运算符结合性
  /// @note
  /// 运算符结合性分从左到右结合和从右到左结合
  void SetBinaryOperatorAssociatityType(OperatorAssociatityType type) {
    binary_operator_associatity_type_ = type;
  }
  /// @brief 设置单目运算符结合性
  /// @param[in] type ：待设置的单目运算符结合性
  /// @note
  /// 运算符结合性分从左到右结合和从右到左结合
  void SetUnaryOperatorAssociatityType(OperatorAssociatityType type) {
    unary_operator_associatity_type_ = type;
  }
  /// @brief 获取双目运算符结合性
  /// @return 返回双目运算符结合性
  /// @retval OperatorAssociatityType::kLeftToRight ：从左到右结合
  /// @retval OperatorAssociatityType::kRightToLeft ：从右到左结合
  OperatorAssociatityType GetBinaryOperatorAssociatityType() const {
    return binary_operator_associatity_type_;
  }
  /// @brief 获取单目运算符结合性
  /// @return 返回单目运算符结合性
  /// @retval OperatorAssociatityType::kLeftToRight ：从左到右结合
  /// @retval OperatorAssociatityType::kRightToLeft ：从右到左结合
  OperatorAssociatityType GetUnaryOperatorAssociatityType() const {
    return unary_operator_associatity_type_;
  }
  /// @brief 设置双目运算符优先级等级
  /// @param[in] priority ：待设置的双目运算符优先级等级
  /// @attention 运算符优先级与单词优先级意义不同
  /// 运算符优先级工作在语法分析，只有运算符拥有
  /// 单词优先级工作在词法分析，每个单词都有
  void SetBinaryOperatorPriority(OperatorPriority priority) {
    binary_operator_priority_ = priority;
  }
  /// @brief 设置单目运算符优先级等级
  /// @param[in] priority ：待设置的单目运算符优先级等级
  /// @attention 运算符优先级与单词优先级意义不同
  /// 运算符优先级工作在语法分析，只有运算符拥有
  /// 单词优先级工作在词法分析，每个单词都有
  void SetUnaryOperatorPriority(OperatorPriority priority) {
    unary_operator_priority_ = priority;
  }
  /// @brief 获取双目运算符优先级
  /// @return 返回双目运算符优先级
  OperatorPriority GetBinaryOperatorPriority() const {
    return binary_operator_priority_;
  }
  /// @brief 获取单目运算符优先级
  /// @return 返回单目运算符优先级
  OperatorPriority GetUnaryOperatorPriority() const {
    return unary_operator_priority_;
  }

 private:
  /// @brief 双目运算符语义下的结合性
  OperatorAssociatityType binary_operator_associatity_type_;
  /// @brief 双目运算符语义下的优先级
  OperatorPriority binary_operator_priority_;
  /// @brief 单目运算符语义下的结合性
  OperatorAssociatityType unary_operator_associatity_type_;
  /// @brief 单目运算符语义下的优先级
  OperatorPriority unary_operator_priority_;
};

/// @class NonTerminalProductionNode production_node.h
/// @brief 表示非终结产生式
class NonTerminalProductionNode : public BaseProductionNode {
 public:
  /// @class NonTerminalProductionNode::ProductionBodyType production_node.h
  /// @brief 表示单个非终结产生式体
  struct ProductionBodyType {
    template <class BodyContainer>
    ProductionBodyType(BodyContainer&& production_body_,
                       ProcessFunctionClassId class_for_reduct_id_)
        : production_body(std::forward<BodyContainer>(production_body_)),
          class_for_reduct_id(class_for_reduct_id_) {
      cores_items_in_.resize(production_body.size() + 1);
    }

    /// @brief 产生式体
    std::vector<ProductionNodeId> production_body;
    /// @brief 每个产生式体对应的所有项所存在的项集
    /// @note
    /// 大小为production_body.size() + 1
    /// 访问时使用的下标例：
    /// 使用下标0：
    /// IdOrEquivence -> · IdOrEquivence "[" Num "]"
    /// 使用下标3：
    /// IdOrEquivence -> IdOrEquivence "[" Num · "]"
    /// 使用下标4：
    /// IdOrEquivence -> IdOrEquivence "[" Num "]" ·
    std::vector<std::list<ProductionItemSetId>> cores_items_in_;
    /// @brief 规约产生式使用的包装规约函数的类的实例化对象ID
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

  /// @brief 获取产生式体内的产生式
  /// @param[in] production_body_id ：待获取的产生式体ID
  /// @param[in] next_word_to_shift_index ：待获取的子产生式位置
  /// @attention 终结节点调用该函数时production_body_id必须为0
  /// next_word_to_shift_index不是0时则返回NextWordToShiftIndex::InvalidId()
  virtual ProductionNodeId GetProductionNodeInBody(
      ProductionBodyId production_body_id,
      NextWordToShiftIndex next_word_to_shift_index) const override;

  /// @brief 添加一个产生式体
  /// @tparam IdType 存储产生式体中各产生式ID的容器，仅支持vector
  /// @param[in] body ：待添加的产生式体
  /// @param[in] class_for_reduct_id
  /// ：包装规约该产生式体的函数的类的实例化对象ID
  /// @return 返回该产生式体在非终结节点内的ID
  /// @note body内的各产生式ID按书写顺序排列
  /// 该函数无检测重复功能
  template <class IdType>
  ProductionBodyId AddBody(IdType&& body,
                           ProcessFunctionClassId class_for_reduct_id);
  /// @brief 获取产生式的一个体
  /// @param[in] production_body_id ：要获取的产生式体ID
  /// @return 返回产生式体的const引用
  /// @note production_body_id必须对应存在的产生式体
  const ProductionBodyType& GetProductionBody(
      ProductionBodyId production_body_id) const {
    assert(production_body_id < nonterminal_bodys_.size());
    return nonterminal_bodys_[production_body_id];
  }
  /// @brief 设置给定产生式体ID规约使用的对象的ID
  /// @param[in] body_id ：待设置规约用对象ID的产生式体ID
  /// @param[in] class_for_reduct_id ：待设置的规约用对象ID
  void SetBodyProcessFunctionClassId(
      ProductionBodyId body_id, ProcessFunctionClassId class_for_reduct_id) {
    assert(body_id < nonterminal_bodys_.size());
    nonterminal_bodys_[body_id].class_for_reduct_id = class_for_reduct_id;
  }
  /// @brief 获取产生式体
  /// @param[in] body_id ：待获取的产生式体ID
  /// @return 返回获取到的产生式体数据结构const引用
  /// @note body_id必须对应有效的产生式体
  const ProductionBodyType& GetBody(ProductionBodyId body_id) const {
    return nonterminal_bodys_[body_id];
  }
  /// @brief 获取全部产生式体
  /// @return 返回存储产生式体的容器const引用
  const std::vector<ProductionBodyType>& GetAllBody() const {
    return nonterminal_bodys_;
  }
  /// @brief 获取全部有效的产生式体ID
  /// @return 返回存储全部有效的产生式体ID的vector容器
  std::vector<ProductionBodyId> GetAllBodyIds() const;
  /// @brief 设置该产生式不可以空规约
  void SetProductionShouldNotEmptyReduct() { could_empty_reduct_ = false; }
  /// @brief 设置该产生式可以空规约
  void SetProductionCouldBeEmptyRedut() { could_empty_reduct_ = true; }
  /// @brief 查询该产生式是否可以空规约
  /// @return 返回该产生式是否可以空规约
  /// @retval true ：该产生式可以空规约
  /// @retval false ：该产生式不可以空规约
  bool CouldBeEmptyReduct() const { return could_empty_reduct_; }
  /// @brief 添加产生式中某项所属的项集ID
  /// @param[in] body_id ：待添加项所属的产生式体ID
  /// @param[in] next_word_to_shift_index ：指向该项下一个移入的单词的位置
  /// @param[in] production_item_set_id ：该项所属的项集ID
  /// @details body_id与next_word_to_shift_index共同指定要添加所属项集ID的项
  /// @note 无去重功能，要求不与已添加过的的项集ID重复
  void AddProductionItemBelongToProductionItemSetId(
      ProductionBodyId body_id, NextWordToShiftIndex next_word_to_shift_index,
      ProductionItemSetId production_item_set_id);
  /// @brief 获取指定项所属的全部项集的ID
  /// @param[in] body_id ：指定项所属的产生式体ID
  /// @param[in] next_word_to_shift_index ：指向该项下一个移入单词的位置
  /// @return 返回存储该项所属的全部项集ID的容器
  /// @note body_id与next_word_to_shift_index共同指定一个项
  const std::list<ProductionItemSetId>&
  GetProductionItemSetIdFromProductionItem(
      ProductionBodyId body_id, NextWordToShiftIndex next_word_to_shift_index) {
    return nonterminal_bodys_[body_id]
        .cores_items_in_[next_word_to_shift_index];
  }
  /// @brief 获取指定产生式体包装规约用函数的类的实例化对象ID
  /// @return 返回包装规约用函数的类的实例化对象ID
  ProcessFunctionClassId GetBodyProcessFunctionClassId(
      ProductionBodyId body_id) const {
    assert(body_id < nonterminal_bodys_.size());
    return nonterminal_bodys_[body_id].class_for_reduct_id;
  }

 private:
  /// @brief 存储产生式体
  std::vector<ProductionBodyType> nonterminal_bodys_;
  /// @brief 标志该产生式是否可能为空
  bool could_empty_reduct_ = false;
};

/// @class EndNode production_node.h
/// @brief 表示文件尾
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
  /// @attention 该函数不应被调用，EndNode不存在该语义
  virtual ProductionNodeId GetProductionNodeInBody(
      ProductionBodyId production_body_id,
      NextWordToShiftIndex point_index) const override {
    assert(false);
    return ProductionNodeId::InvalidId();
  }
};
}  // namespace frontend::generator::syntax_generator

#endif  /// !GENERATOR_SYNTAXGENERATOR_PRODUCTION_NODE_H_