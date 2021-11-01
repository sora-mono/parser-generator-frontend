// 存储向外暴露的一些数据类型
#ifndef COMMON_ENUM_AND_TYPES_H_
#define COMMON_ENUM_AND_TYPES_H_

#include <boost/serialization/array.hpp>
#include <string>
#include <vector>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/object_manager.h"
#include "Common/unordered_struct_manager.h"

namespace frontend::generator::syntax_generator {
// ID包装器用来区别不同ID的枚举
enum class WrapperLabel {
  kPriorityLevel,
  kNextWordToShiftIndex,
  kSyntaxAnalysisTableEntryId,
  kProductionBodyId,
};
using OperatorAssociatityType = frontend::common::OperatorAssociatityType;
// 产生式节点类型：终结符号，运算符，非终结符号，文件尾节点
// 为了支持ClassfiyProductionNodes，允许自定义项的值
// 如果自定义项的值则所有项的值都必须小于sizeof(ProductionNodeType)
enum class ProductionNodeType {
  kTerminalNode,
  kOperatorNode,
  kNonTerminalNode,
  kEndNode
};
// 分析动作类型：规约，移入，移入和规约，报错，接受
enum class ActionType { kReduct, kShift, kShiftReduct, kError, kAccept };

// 前向声明类，用来获取管理该类的结构给出的标识ID
class BaseProductionNode;
class ProductionItemSet;
class ProcessFunctionInterface;
class SyntaxAnalysisTableEntry;

using frontend::common::ExplicitIdWrapper;
using frontend::common::ObjectManager;
using frontend::common::UnorderedStructManager;

// 运算符优先级，数字越大优先级越高，仅对运算符节点有效
// 与TailNodePriority意义不同，该优先级影响语法分析过程
// 当遇到连续的运算符时决定移入还是归并
using OperatorPriority =
    ExplicitIdWrapper<size_t, WrapperLabel, WrapperLabel::kPriorityLevel>;
// 非终结节点内产生式编号
using ProductionBodyId =
    ExplicitIdWrapper<size_t, WrapperLabel, WrapperLabel::kProductionBodyId>;
// 语法分析时一项中下一个移入的单词的位置
using NextWordToShiftIndex =
    ExplicitIdWrapper<size_t, WrapperLabel,
                      WrapperLabel::kNextWordToShiftIndex>;
// 语法分析表条目ID
using SyntaxAnalysisTableEntryId =
    ExplicitIdWrapper<size_t, WrapperLabel,
                      WrapperLabel::kSyntaxAnalysisTableEntryId>;
// 符号ID
using SymbolId =
    UnorderedStructManager<std::string, std::hash<std::string>>::ObjectId;
// 产生式节点ID
using ProductionNodeId = ObjectManager<BaseProductionNode>::ObjectId;
// 项集ID
using ProductionItemSetId = ObjectManager<ProductionItemSet>::ObjectId;
// 管理包装用户自定义函数和数据的类的已分配对象的容器
using ProcessFunctionClassManagerType = ObjectManager<ProcessFunctionInterface>;
// 包装用户自定义函数和数据的类的已分配对象ID
using ProcessFunctionClassId = ProcessFunctionClassManagerType::ObjectId;
// 语法分析表类型
using SyntaxAnalysisTableType = std::vector<SyntaxAnalysisTableEntry>;
}  // namespace frontend::generator::syntax_generator

namespace frontend::generator::dfa_generator {
namespace nfa_generator {
// 自定义类型的分发标签
enum class WrapperLabel { kTailNodePriority, kTailNodeId };
// 单词优先级，与运算符优先级不同
using WordPriority =
    frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                        WrapperLabel::kTailNodePriority>;
// 单词附属数据（在检测到相应单词时返回）
struct WordAttachedData {
  using ProductionNodeId =
      frontend::generator::syntax_generator::ProductionNodeId;
  using ProductionNodeType =
      frontend::generator::syntax_generator::ProductionNodeType;
  using OperatorAssociatityType =
      frontend::generator::syntax_generator::OperatorAssociatityType;
  using OperatorPriority =
      frontend::generator::syntax_generator::OperatorPriority;

  friend class boost::serialization::access;

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

  // 根据上一个操作是否为规约判断使用左侧单目运算符优先级还是双目运算符优先级
  // 返回获取到的结合类型和优先级
  std::pair<OperatorAssociatityType, OperatorPriority>
  GetAssociatityTypeAndPriority(bool is_last_operate_reduct) const;

  // 产生式节点ID
  // 应保证ID是唯一的，且一个ID对应的其余项唯一
  ProductionNodeId production_node_id = ProductionNodeId::InvalidId();
  // 节点类型
  ProductionNodeType node_type;
  // 以下三项仅对运算符有效，非运算符请使用默认值以保持==和!=语义
  // 双目运算符结合性
  OperatorAssociatityType binary_operator_associate_type =
      OperatorAssociatityType::kLeftToRight;
  // 双目运算符优先级
  OperatorPriority binary_operator_priority = OperatorPriority::InvalidId();
  // 左侧单目运算符结合性
  OperatorAssociatityType unary_operator_associate_type =
      OperatorAssociatityType::kLeftToRight;
  // 左侧单目运算符优先级
  OperatorPriority unary_operator_priority = OperatorPriority::InvalidId();
};
}  // namespace nfa_generator

// 管理转移表用，仅用于DfaGenerator，为了避免使用char作下标时使用负下标数组越界
// 可以直接使用CHAR_MIN~CHAR_MAX任意值访问
template <class BasicObjectType>
class TransformArrayManager {
 public:
  BasicObjectType& operator[](char c) {
    return transform_array_[(c + frontend::common::kCharNum) %
                            frontend::common::kCharNum];
  }
  void fill(const BasicObjectType& fill_object) {
    transform_array_.fill(fill_object);
  }

 private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& transform_array_;
  }

  std::array<BasicObjectType, frontend::common::kCharNum> transform_array_;
};
// 单词附属数据（在检测到相应单词时返回）
using WordAttachedData = nfa_generator::WordAttachedData;
// ID包装器用来区分不同ID的枚举
enum class WrapperLabel { kTransformArrayId };
// 状态转移表ID
using TransformArrayId =
    frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                        WrapperLabel::kTransformArrayId>;
// 状态转移表条目
using TransformArray = TransformArrayManager<TransformArrayId>;
// DFA配置类型
using DfaConfigType = std::vector<std::pair<TransformArray, WordAttachedData>>;
}  // namespace frontend::generator::dfa_generator
#endif  // !COMMON_ENUM_AND_TYPES_H_