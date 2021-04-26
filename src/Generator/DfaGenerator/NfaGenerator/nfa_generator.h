#pragma once
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Common/multimap_node_manager.h"
#include "Common/unordered_struct_manager.h"

#ifndef GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
#define GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_

namespace frontend::generator::dfagenerator::nfagenerator {
class NfaGenerator {
 public:
  struct NfaNode;
  using NodeId = common::MultimapNodeManager<NfaNode>::NodeId;
  using NodeGather = size_t;
  using PriorityTag = size_t;
  //前半部分为tag序号，后半部分为优先级，数字越大优先级越高
  using TailNodeData = std::pair<size_t, PriorityTag>;

  friend bool MergeNfaNodesWithGenerator(NfaNode& node_dst, NfaNode& node_src,
                                         NfaGenerator& generator);

  struct NfaNode {
    NfaNode() {}
    NfaNode(const NfaNode& node)
        : nodes_forward(node.nodes_forward),
          conditionless_transfer_nodes_handler(
              node.conditionless_transfer_nodes_handler) {}
    NfaNode(NfaNode&& node)
        : nodes_forward(std::move(node.nodes_forward)),
          conditionless_transfer_nodes_handler(
              std::move(node.conditionless_transfer_nodes_handler)) {}

    NodeId GetForwardNodesHandler(char c_transfer);
    const std::unordered_set<NodeId>& GetUnconditionTransferNodesHandler();

    //添加条件转移节点，遇到已存在节点会返回false
    bool AddConditionTransfer(char c_transfer, NodeId node_handler);
    //添加无条件转移节点
    bool AddNoconditionTransfer(NodeId node_handler);
    //移除一个转移条件节点，函数保证执行后不存在节点（无论原来是否存在)
    bool RemoveConditionalTransfer(char c_treasfer);
    //同上，移除一个无条件节点，输入-1代表清除所有
    bool RemoveConditionlessTransfer(NodeId node_handler);

    bool MergeNodesWithManager(NfaNode& node_src);
    //记录转移条件与前向节点，一个条件仅允许对应一个节点
    std::unordered_map<char, NodeGather> nodes_forward;
    //存储无条件转移节点
    std::unordered_set<NodeId> conditionless_transfer_nodes_handler;
  };

  NfaGenerator() : head_node_handler_(-1) {}
  NfaGenerator(const NfaGenerator&) = delete;
  NfaGenerator(NfaGenerator&&) = delete;

  NodeId GetHeadNodeId() { return head_node_handler_; }

  const TailNodeData GetTailTag(NfaNode* pointer);
  const TailNodeData GetTailTag(NodeId handler);
  NfaNode* GetNode(NodeId handler);
  //解析正则并添加到已有NFA中，返回生成的自动机的头结点和尾节点，自动处理结尾的范围限制符号
  std::pair<NodeId, NodeId> RegexConstruct(
      std::istream& in, const TailNodeData& tag, bool add_to_NFA_head = true,
      bool return_when_right_bracket = false);
  //添加一个由字符串构成的NFA，自动处理结尾的范围限制符号
  std::pair<NodeId, NodeId> WordConstruct(const std::string& str,
                                          const TailNodeData& tag);
  //合并优化，降低节点数以降低子集构造法集合大小，直接使用NFA也可以降低成本
  void MergeOptimization();

  std::pair<std::unordered_set<NodeId>, TailNodeData> Closure(NodeId handler);
  //返回goto后的节点的闭包
  std::pair<std::unordered_set<NodeId>, TailNodeData> Goto(NodeId handler_src,
                                                           char c_transform);

  //清除已有NFA
  void Clear();

  //序列化容器用
  template <class Archive>
  void Serialize(Archive& ar, const unsigned int version = 0);

  //非尾节点标记
  const static TailNodeData NotTailNodeTag;

 private:
  bool RemoveTailNode(NfaNode* pointer);
  bool AddTailNode(NfaNode* pointer, const TailNodeData& tag);
  bool AddTailNode(NodeId handler, const TailNodeData& tag) {
    return AddTailNode(GetNode(handler), tag);
  }
  //生成可选字符序列，会读取]后的*,+,?等限定符
  std::pair<NodeId, NodeId> CreateSwitchTree(std::istream& in);

  //所有NFA的头结点
  NodeId head_node_handler_;
  //该set用来存储所有尾节点和对应单词的tag
  std::unordered_map<NfaNode*, TailNodeData> tail_nodes_;
  common::MultimapNodeManager<NfaNode> node_manager_;
};

bool MergeNfaNodesWithGenerator(NfaGenerator::NfaNode& node_dst,
                                NfaGenerator::NfaNode& node_src,
                                NfaGenerator& generator);
const NfaGenerator::TailNodeData NfaGenerator::NotTailNodeTag =
    NfaGenerator::TailNodeData(-1, -1);
}  // namespace frontend::generator::dfagenerator::nfagenerator
#endif  // !GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
