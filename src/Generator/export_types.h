/// @file export_types.h
/// @brief 存储向外暴露的一些数据类型
#ifndef COMMON_ENUM_AND_TYPES_H_
#define COMMON_ENUM_AND_TYPES_H_

#include <boost/serialization/array.hpp>
#include <boost/serialization/utility.hpp>
#include <string>
#include <vector>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/id_wrapper_serializer.h"
#include "Common/object_manager.h"
#include "Common/unordered_struct_manager.h"

namespace frontend::generator::syntax_generator {
/// @brief ID包装器用来区别不同ID的枚举
enum class WrapperLabel {
  kPriorityLevel,
  kNextWordToShiftIndex,
  kSyntaxAnalysisTableEntryId,
  kProductionBodyId,
};
/// @brief 运算符结合性
using OperatorAssociatityType = frontend::common::OperatorAssociatityType;
/// @brief 产生式节点类型
/// @details
/// 为了支持ClassfiyProductionNodes，允许自定义项的值
/// 如果自定义项的值则所有项的值都必须小于sizeof(ProductionNodeType)
enum class ProductionNodeType {
  kTerminalNode,     ///< 终结产生式节点
  kOperatorNode,     ///< 运算符节点
  kNonTerminalNode,  ///< 非终结产生式节点
  kEndNode           ///< 文件尾节点
};
/// @brief 语法分析动作类型
enum class ActionType {
  kShift,        ///< 移入
  kReduct,       ///< 规约
  kShiftReduct,  ///< 移入或规约
  kError,        ///< 报错
  kAccept        ///< 接受
};

// 前向声明类，用来获取管理该类的结构给出的标识ID
class BaseProductionNode;
class ProductionItemSet;
class ProcessFunctionInterface;
class SyntaxAnalysisTableEntry;

using frontend::common::ExplicitIdWrapper;
using frontend::common::ObjectManager;
using frontend::common::UnorderedStructManager;

/// @brief 运算符优先级
/// @details
/// 1.数字越大优先级越高
/// 2.移入规约冲突且待移入节点为运算符时根据运算符优先级决定移入还是归并
/// 3.与TailNodePriority意义不同，该优先级影响语法分析过程
using OperatorPriority =
    ExplicitIdWrapper<size_t, WrapperLabel, WrapperLabel::kPriorityLevel>;
/// @brief 非终结节点产生式体编号
using ProductionBodyId =
    ExplicitIdWrapper<size_t, WrapperLabel, WrapperLabel::kProductionBodyId>;
/// @brief 项中下一个移入的单词的位置
using NextWordToShiftIndex =
    ExplicitIdWrapper<size_t, WrapperLabel,
                      WrapperLabel::kNextWordToShiftIndex>;
/// @brief 语法分析表条目ID
using SyntaxAnalysisTableEntryId =
    ExplicitIdWrapper<size_t, WrapperLabel,
                      WrapperLabel::kSyntaxAnalysisTableEntryId>;
/// @brief 符号ID
using SymbolId =
    UnorderedStructManager<std::string, std::hash<std::string>>::ObjectId;
/// @brief 产生式节点ID
using ProductionNodeId = ObjectManager<BaseProductionNode>::ObjectId;
/// @brief 项集ID
using ProductionItemSetId = ObjectManager<ProductionItemSet>::ObjectId;
/// @brief 管理包装规约函数的类的实例化对象的容器
using ProcessFunctionClassManagerType = ObjectManager<ProcessFunctionInterface>;
/// @brief 包装规约函数的类的实例化对象ID
using ProcessFunctionClassId = ProcessFunctionClassManagerType::ObjectId;
/// @brief 语法分析表类型
using SyntaxAnalysisTableType = std::vector<SyntaxAnalysisTableEntry>;
}  // namespace frontend::generator::syntax_generator

namespace frontend::generator::dfa_generator {
namespace nfa_generator {
/// @brief 自定义类型的分发标签
enum class WrapperLabel { kTailNodePriority, kTailNodeId };
/// @brief 单词优先级，与运算符优先级不同
using WordPriority =
    frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                        WrapperLabel::kTailNodePriority>;
/// @class WordAttachedData export_types.h
/// @brief 单词附属数据（在检测到相应单词时返回）
struct WordAttachedData {
  using ProductionNodeId =
      frontend::generator::syntax_generator::ProductionNodeId;
  using ProductionNodeType =
      frontend::generator::syntax_generator::ProductionNodeType;
  using OperatorAssociatityType =
      frontend::generator::syntax_generator::OperatorAssociatityType;
  using OperatorPriority =
      frontend::generator::syntax_generator::OperatorPriority;

  /// @brief 允许序列化类访问
  friend class boost::serialization::access;

  /// @brief 序列化配置的函数
  /// @param[in,out] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& production_node_id;
    ar& node_type;
    ar& binary_operator_associate_type;
    ar& binary_operator_priority;
    ar& unary_operator_associate_type;
    ar& unary_operator_priority;
  }

  bool operator==(const WordAttachedData& saved_data) const;

  bool operator!=(const WordAttachedData& saved_data) const {
    return !operator==(saved_data);
  }

  /// @brief 获取结合性和运算符优先级
  /// @return 前半部分为结合性，后半部分为运算符优先级
  /// @details 根据上一个操作是否为规约确定使用左侧单目还是双目运算符语义
  std::pair<OperatorAssociatityType, OperatorPriority>
  GetAssociatityTypeAndPriority(bool is_last_operate_reduct) const;

  /// @brief 产生式节点ID
  /// @attention ID是唯一的，且一个ID对应的其余数据唯一
  ProductionNodeId production_node_id = ProductionNodeId::InvalidId();
  /// @brief 节点类型
  ProductionNodeType node_type;
  /// @brief 双目运算符结合性
  /// @note 仅对运算符有效，非运算符请使用默认值以保持==和!=语义
  OperatorAssociatityType binary_operator_associate_type =
      OperatorAssociatityType::kLeftToRight;
  /// @brief 双目运算符优先级
  /// @note 仅对运算符有效，非运算符请使用默认值以保持==和!=语义
  OperatorPriority binary_operator_priority = OperatorPriority::InvalidId();
  /// @brief 左侧单目运算符结合性
  /// @note 仅对运算符有效，非运算符请使用默认值以保持==和!=语义
  OperatorAssociatityType unary_operator_associate_type =
      OperatorAssociatityType::kLeftToRight;
  /// @brief 左侧单目运算符优先级
  /// @note 仅对运算符有效，非运算符请使用默认值以保持==和!=语义
  OperatorPriority unary_operator_priority = OperatorPriority::InvalidId();
};
}  // namespace nfa_generator

/// @class TransformArrayManager export_types.h
/// @brief 管理转移表
/// @tparam BasicObjectType ：内部对象类型
/// @details
/// 1.仅用于DfaGenerator，为了避免使用char作下标时使用负下标数组越界
/// 2.存储frontend::common::kCharNum个BasicObjectType类型的对象
/// @note 可以直接使用CHAR_MIN~CHAR_MAX任意值访问
template <class BasicObjectType>
class TransformArrayManager {
 public:
  const BasicObjectType& operator[](char c) const {
    return transform_array_[(c + frontend::common::kCharNum) %
                            frontend::common::kCharNum];
  }
  BasicObjectType& operator[](char c) {
    return const_cast<BasicObjectType&>(
        static_cast<const TransformArrayManager&>(*this).operator[](c));
  }
  /// @brief 填充转移表
  /// @param[in] fill_object ：用来填充的对象
  void fill(const BasicObjectType& fill_object) {
    transform_array_.fill(fill_object);
  }

 private:
  /// @brief 允许序列化类访问
  friend class boost::serialization::access;

  /// @brief 序列化配置的函数
  /// @param[in,out] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& transform_array_;
  }

  /// @brief 转移表
  std::array<BasicObjectType, frontend::common::kCharNum> transform_array_;
};
/// @brief 单词附属数据（在检测到相应单词时返回）
using WordAttachedData = nfa_generator::WordAttachedData;
/// @brief ID包装器用来区分不同ID的枚举
enum class WrapperLabel { kTransformArrayId };
/// @brief 转移表ID
using TransformArrayId =
    frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                        WrapperLabel::kTransformArrayId>;
/// @brief 转移表条目
using TransformArray = TransformArrayManager<TransformArrayId>;
/// @brief DFA配置类型
using DfaConfigType = std::vector<std::pair<TransformArray, WordAttachedData>>;
}  // namespace frontend::generator::dfa_generator
#endif  /// !COMMON_ENUM_AND_TYPES_H_