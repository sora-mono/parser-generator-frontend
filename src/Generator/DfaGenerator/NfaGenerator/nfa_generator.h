#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Common/id_wrapper.h"
#include "Common/multimap_object_manager.h"
#include "Common/unordered_struct_manager.h"

#ifndef GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
#define GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_

namespace frontend::generator::dfa_generator::nfa_generator {
class NfaGenerator {
 public:
  struct NfaNode;
  //自定义类型的分发标签
  enum class WrapperLabel { kTailNodePriority, kTailNodeId };
  // NfaNode在管理类中的ID
  using NfaNodeId = frontend::common::MultimapObjectManager<NfaNode>::ObjectId;
  //符号优先级
  using TailNodePriority =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kTailNodePriority>;
  //尾节点ID（由用户传入，在检测到相应节点时返回）
  using TailNodeId =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kTailNodeId>;
  //前半部分为tag序号，后半部分为优先级，数字越大优先级越高
  using TailNodeData = std::pair<TailNodeId, TailNodePriority>;

  //合并两个NFA节点
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

    //设置条件转移节点
    void SetConditionTransfer(char c_transfer, NfaNodeId node_id);
    //添加无条件转移节点
    void AddNoconditionTransfer(NfaNodeId node_id);
    //移除一个转移条件节点，函数保证执行后不存在节点（无论原来是否存在)
    void RemoveConditionalTransfer(char c_treasfer);
    //同上，移除一个无条件转移节点，函数保证执行后不存在节点（无论原来是否存在)
    void RemoveConditionlessTransfer(NfaNodeId node_id);
    //移除所有无条件转移节点
    void RemoveAllConditionlessTransfer() {
      conditionless_transfer_nodes_id.clear();
    }

    bool MergeNodesWithManager(NfaNode& node_src);
    //记录转移条件与转移到的节点，一个条件仅允许对应一个节点
    std::unordered_map<char, NfaNodeId> nodes_forward;
    //存储无条件转移节点
    std::unordered_set<NfaNodeId> conditionless_transfer_nodes_id;
  };

  NfaGenerator();
  NfaGenerator(const NfaGenerator&) = delete;
  NfaGenerator(NfaGenerator&&) = delete;

  NfaNodeId GetHeadNfaNodeId() { return head_node_id_; }

  const TailNodeData GetTailTag(NfaNode* pointer);
  const TailNodeData GetTailTag(NfaNodeId id);
  NfaNode& GetNfaNode(NfaNodeId id) {
    return node_manager_.GetObject(id);
  }
  //解析正则并添加到已有NFA中，返回生成的自动机的头结点和尾节点，自动处理结尾的范围限制符号
  std::pair<NfaNodeId, NfaNodeId> RegexConstruct(
      std::istream& in, const TailNodeData& tag, bool add_to_NFA_head = true,
      bool return_when_right_bracket = false);
  //添加一个由字符串构成的NFA，自动处理结尾的范围限制符号
  std::pair<NfaNodeId, NfaNodeId> WordConstruct(const std::string& str,
                                                const TailNodeData& tag);
  //合并优化，降低节点数以降低子集构造法集合大小，直接使用NFA也可以降低成本
  void MergeOptimization();

  std::pair<std::unordered_set<NfaNodeId>, TailNodeData> Closure(
      NfaNodeId id);
  //返回goto后的节点的闭包
  std::pair<std::unordered_set<NfaNodeId>, TailNodeData> Goto(
      NfaNodeId id_src, char c_transform);

  //清除已有NFA
  void Clear();

  //序列化容器用
  template <class Archive>
  void Serialize(Archive& ar, const unsigned int version = 0);

  //非尾节点标记
  static const TailNodeData NotTailNodeTag;

 private:
  bool RemoveTailNode(NfaNode* pointer);
  bool AddTailNode(NfaNode* pointer, const TailNodeData& tag);
  bool AddTailNode(NfaNodeId id, const TailNodeData& tag) {
    return AddTailNode(&GetNfaNode(id), tag);
  }
  //生成可选字符序列，会读取]后的*,+,?等限定符
  std::pair<NfaNodeId, NfaNodeId> CreateSwitchTree(std::istream& in);

  //所有NFA的头结点
  NfaNodeId head_node_id_;
  //该set用来存储所有尾节点和对应单词的tag
  std::unordered_map<NfaNode*, TailNodeData> tail_nodes_;
  common::MultimapObjectManager<NfaNode> node_manager_;
};

bool MergeNfaNodesWithGenerator(NfaGenerator::NfaNode& node_dst,
                                NfaGenerator::NfaNode& node_src,
                                NfaGenerator& generator);

}  // namespace frontend::generator::dfa_generator::nfa_generator
#endif  // !GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
