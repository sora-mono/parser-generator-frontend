#ifndef GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
#define GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_

#include <any>
#include <array>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/multimap_object_manager.h"
#include "Common/unordered_struct_manager.h"

namespace frontend::generator::dfa_generator::nfa_generator {
class NfaGenerator {
 public:
  class NfaNode;
  // 自定义类型的分发标签
  enum class WrapperLabel { kTailNodePriority, kTailNodeId };
  // NfaNode在管理类中的ID
  using NfaNodeId = frontend::common::MultimapObjectManager<NfaNode>::ObjectId;
  // 符号优先级，与运算符优先级不同
  using WordPriority =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kTailNodePriority>;
  // 附属数据（在检测到相应单词时返回）
  struct WordAttachedData {
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);

    bool operator==(const WordAttachedData& saved_data) const {
      return (production_node_id == saved_data.production_node_id &&
              node_type == saved_data.node_type &&
              binary_operator_associate_type ==
                  saved_data.binary_operator_associate_type &&
              binary_operator_priority == saved_data.binary_operator_priority &&
              unary_operator_associate_type ==
                  saved_data.unary_operator_associate_type &&
              unary_operator_priority == saved_data.unary_operator_priority);
    }
    bool operator!=(const WordAttachedData& saved_data) const {
      return !operator==(saved_data);
    }

    // 根据上一个操作是否为规约判断使用左侧单目运算符优先级还是双目运算符优先级
    // 返回获取到的结合类型和优先级
    std::pair<frontend::common::OperatorAssociatityType, size_t>
    GetAssociatityTypeAndPriority(bool is_last_operate_reduct) const;

    // 产生式节点ID，前向声明无法引用嵌套类，所以无法引用源类型
    // 应保证ID是唯一的，且一个ID对应的其余项唯一
    size_t production_node_id;
    // 节点类型
    frontend::common::ProductionNodeType node_type;
    // 以下三项仅对运算符有效，非运算符请使用默认值以保持==和!=语义
    // 双目运算符结合性
    frontend::common::OperatorAssociatityType binary_operator_associate_type =
        frontend::common::OperatorAssociatityType::kLeftToRight;
    // 双目运算符优先级
    size_t binary_operator_priority = -1;
    // 左侧单目运算符结合性
    frontend::common::OperatorAssociatityType unary_operator_associate_type =
        frontend::common::OperatorAssociatityType::kLeftToRight;
    // 左侧单目运算符优先级
    size_t unary_operator_priority = -1;
  };
  // 尾节点数据，内容为该单词所附带的属性
  // 前半部分为用户定义数据，后半部分为单词优先级，数字越大优先级越高
  using TailNodeData = std::pair<WordAttachedData, WordPriority>;

  class NfaNode {
   public:
    NfaNode() {}
    NfaNode(const NfaNode& node)
        : nodes_forward(node.nodes_forward),
          conditionless_transfer_nodes_id(
              node.conditionless_transfer_nodes_id) {}
    NfaNode(NfaNode&& node)
        : nodes_forward(std::move(node.nodes_forward)),
          conditionless_transfer_nodes_id(
              std::move(node.conditionless_transfer_nodes_id)) {}

    // 根据转移条件获取转移到的节点ID
    // 如果不存在相应的转移结果则返回NfaNodeId::InvalidId()
    NfaNodeId GetForwardNodesId(char c_transfer);
    const std::unordered_set<NfaNodeId>& GetUnconditionTransferNodesIds()
        const {
      return conditionless_transfer_nodes_id;
    }
    const std::unordered_map<char, NfaNodeId>& GetConditionalTransfers() const {
      return nodes_forward;
    }
    // 设置条件转移节点
    void SetConditionTransfer(char c_transfer, NfaNodeId node_id);
    // 添加无条件转移节点
    void AddNoconditionTransfer(NfaNodeId node_id);
    // 移除一个转移条件节点，函数保证执行后不存在节点（无论原来是否存在)
    void RemoveConditionalTransfer(char c_treasfer);
    // 同上，移除一个无条件转移节点，函数保证执行后不存在节点（无论原来是否存在)
    void RemoveConditionlessTransfer(NfaNodeId node_id);
    // 移除所有无条件转移节点
    void RemoveAllConditionlessTransfer() {
      conditionless_transfer_nodes_id.clear();
    }
    bool MergeNodesWithManager(NfaNode& node_src);

   private:
    friend class NfaGenerator;

    std::unordered_set<NfaNodeId>& GetUnconditionTransferNodesIds() {
      return conditionless_transfer_nodes_id;
    }
    std::unordered_map<char, NfaNodeId>& GetConditionalTransfers() {
      return nodes_forward;
    }
    // 记录转移条件与转移到的节点，一个条件仅允许对应一个节点
    std::unordered_map<char, NfaNodeId> nodes_forward;
    // 存储无条件转移节点
    std::unordered_set<NfaNodeId> conditionless_transfer_nodes_id;
  };

  NfaGenerator() {}
  NfaGenerator(const NfaGenerator&) = delete;
  NfaGenerator(NfaGenerator&&) = delete;

  NfaNodeId GetHeadNfaNodeId() { return head_node_id_; }

  const TailNodeData GetTailNodeData(NfaNode* pointer);
  const TailNodeData GetTailNodeData(NfaNodeId production_node_id);
  NfaNode& GetNfaNode(NfaNodeId production_node_id) {
    return node_manager_.GetObject(production_node_id);
  }
  // 解析正则并添加到已有NFA中，返回生成的自动机的头结点和尾节点
  // 自动处理结尾的范围限制符号
  std::pair<NfaNodeId, NfaNodeId> RegexConstruct(
      std::istream& in, const TailNodeData& tag,
      const bool add_to_NFA_head = true,
      const bool return_when_right_bracket = false);
  // 添加一个由字符串构成的NFA，自动处理结尾的范围限制符号
  std::pair<NfaNodeId, NfaNodeId> WordConstruct(const std::string& str,
                                                TailNodeData&& tail_node_data);
  // 合并优化，降低节点数和分支路径以降低子集构造法集合大小
  // 直接使用NFA也可以降低成本
  void MergeOptimization();

  // 获取给定NFA节点的所有等效节点ID（包含自身）
  std::pair<std::unordered_set<NfaNodeId>, TailNodeData> Closure(
      NfaNodeId production_node_id);
  // 返回goto后的节点的闭包
  std::pair<std::unordered_set<NfaNodeId>, TailNodeData> Goto(NfaNodeId id_src,
                                                              char c_transform);
  // NFA初始化
  void NfaInit();

  // 非尾节点标记
  static const TailNodeData NotTailNodeTag;

 private:
  bool RemoveTailNode(NfaNode* pointer);
  bool AddTailNode(NfaNode* pointer, const TailNodeData& tag);
  bool AddTailNode(NfaNodeId production_node_id, const TailNodeData& tag) {
    return AddTailNode(&GetNfaNode(production_node_id), tag);
  }
  // 根据输入生成可选字符序列，会读取]后的*,+,?等限定符
  // 输入的字符流第一个字符应为'['右侧字符
  // 例：a-zA-Z_]
  // 返回的第一个参数为头结点ID，第二个参数为尾节点ID
  std::pair<NfaNodeId, NfaNodeId> CreateSwitchTree(std::istream& in);
  // 将node_src合并到node_dst中
  // 返回是否合并成功
  static bool MergeNfaNodes(NfaGenerator::NfaNode& node_dst,
                            NfaGenerator::NfaNode& node_src,
                            NfaGenerator& nfa_generator);

  // 所有NFA的头结点
  NfaNodeId head_node_id_;
  // 该set用来存储所有尾节点和对应单词的信息
  std::unordered_map<NfaNode*, TailNodeData> tail_nodes_;
  frontend::common::MultimapObjectManager<NfaNode> node_manager_;
};

// 为了SavedData可以参与排序，实际该结构并没有逻辑顺序
// 使用需满足结构体内注释的条件
inline bool operator<(const NfaGenerator::WordAttachedData& left,
                      const NfaGenerator::WordAttachedData& right) {
  return left.production_node_id < right.production_node_id;
}
inline bool operator>(const NfaGenerator::WordAttachedData& left,
                      const NfaGenerator::WordAttachedData& right) {
  return left.production_node_id > right.production_node_id;
}

template <class Archive>
inline void NfaGenerator::WordAttachedData::serialize(
    Archive& ar, const unsigned int version) {
  ar& production_node_id;
  ar& node_type;
  ar& binary_operator_associate_type;
  ar& binary_operator_priority;
  ar& unary_operator_priority;
}

}  // namespace frontend::generator::dfa_generator::nfa_generator

#endif  // !GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
