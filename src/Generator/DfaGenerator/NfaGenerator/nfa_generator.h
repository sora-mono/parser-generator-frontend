#ifndef GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
#define GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_

#include <any>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/multimap_object_manager.h"
#include "Common/unordered_struct_manager.h"

namespace frontend::generator::dfa_generator::nfa_generator {
class NfaGenerator {
 public:
  struct NfaNode;
  // 自定义类型的分发标签
  enum class WrapperLabel { kTailNodePriority, kTailNodeId };
  // NfaNode在管理类中的ID
  using NfaNodeId = frontend::common::MultimapObjectManager<NfaNode>::ObjectId;
  // 符号优先级
  using WordPriority =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kTailNodePriority>;
  // 附属数据（在检测到相应单词时返回）
  struct WordAttachedData {
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& production_node_id;
      ar& node_type;
      ar& process_function_class_id_;
#ifdef USE_AMBIGUOUS_GRAMMAR
      ar& associate_type;
      ar& operator_priority;
#endif  // USE_AMBIGUOUS_GRAMMAR
    }

    bool operator==(const WordAttachedData& saved_data) const {
      return (
          production_node_id == saved_data.production_node_id &&
          node_type == saved_data.node_type &&
          process_function_class_id_ == saved_data.process_function_class_id_
#ifdef USE_AMBIGUOUS_GRAMMAR
          && associate_type == saved_data.associate_type &&
          operator_priority == saved_data.operator_priority
#endif  // USE_AMBIGUOUS_GRAMMAR
      );
    }
    bool operator!=(const WordAttachedData& saved_data) const {
      return !operator==(saved_data);
    }
    // 产生式节点ID，前向声明无法引用嵌套类，所以无法引用源类型
    // 应保证ID是唯一的，且一个ID对应的其余项唯一
    size_t production_node_id;
    // 节点类型
    frontend::common::ProductionNodeType node_type;
    // 包装用户定义的函数类的对象的ID
    size_t process_function_class_id_;

    //以下两项仅对二义性文法的操作符有效
#ifdef USE_AMBIGUOUS_GRAMMAR
    // 结合性
    frontend::common::OperatorAssociatityType associate_type;
    // 运算符优先级
    size_t operator_priority;
#endif  // USE_AMBIGUOUS_GRAMMAR
  };
  // 前半部分为用户定义数据，后半部分为优先级，数字越大优先级越高
  using TailNodeData = std::pair<WordAttachedData, WordPriority>;

  // 合并两个NFA节点
  // TODO 了解这个友元函数是否必要
  friend bool MergeNfaNodesWithGenerator(NfaNode& node_dst, NfaNode& node_src,
                                         NfaGenerator& generator);

  // TODO 将该struct改为class
  struct NfaNode {
    NfaNode() {}
    NfaNode(const NfaNode& node)
        : nodes_forward(node.nodes_forward),
          conditionless_transfer_nodes_id(
              node.conditionless_transfer_nodes_id) {}
    NfaNode(NfaNode&& node)
        : nodes_forward(std::move(node.nodes_forward)),
          conditionless_transfer_nodes_id(
              std::move(node.conditionless_transfer_nodes_id)) {}

    NfaNodeId GetForwardNodesId(char c_transfer);
    const std::unordered_set<NfaNodeId>& GetUnconditionTransferNodesId();

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
  // 解析正则并添加到已有NFA中，返回生成的自动机的头结点和尾节点，自动处理结尾的范围限制符号
  std::pair<NfaNodeId, NfaNodeId> RegexConstruct(
      std::istream& in, const TailNodeData& tag, const bool add_to_NFA_head = true,
      const bool return_when_right_bracket = false);
  // 添加一个由字符串构成的NFA，自动处理结尾的范围限制符号
  std::pair<NfaNodeId, NfaNodeId> WordConstruct(const std::string& str,
                                                const TailNodeData& tag);
  // 合并优化，降低节点数以降低子集构造法集合大小，直接使用NFA也可以降低成本
  void MergeOptimization();

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
  // 生成可选字符序列，会读取]后的*,+,?等限定符
  std::pair<NfaNodeId, NfaNodeId> CreateSwitchTree(std::istream& in);

  // 所有NFA的头结点
  NfaNodeId head_node_id_;
  // 该set用来存储所有尾节点和对应单词的信息
  std::unordered_map<NfaNode*, TailNodeData> tail_nodes_;
  frontend::common::MultimapObjectManager<NfaNode> node_manager_;
};

bool MergeNfaNodesWithGenerator(NfaGenerator::NfaNode& node_dst,
                                NfaGenerator::NfaNode& node_src,
                                NfaGenerator& generator);
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

}  // namespace frontend::generator::dfa_generator::nfa_generator

#endif  // !GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
