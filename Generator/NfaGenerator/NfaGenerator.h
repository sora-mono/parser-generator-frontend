#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "MultimapNodeManager.h"

class NfaGenerator {
 public:
  struct NfaNode;
  using NodeHandler = MultimapNodeManager<NfaNode>::NodeHandler;
  using NodeGather = size_t;
  //前半部分为tag序号，后半部分为优先级，数字越大优先级越高
  using TailNodeTag = std::pair<size_t, size_t>;
  friend bool MergeNfaNodesWithGenerator(NfaNode& node_dst, NfaNode& node_src,
                                         NfaGenerator& generator);

  struct NfaNode {
    NfaNode() {}
    NfaNode(const NfaNode& node)
        : nodes_forward_(node.nodes_forward_),
          conditionless_transfer_nodes_handler_(
              node.conditionless_transfer_nodes_handler_) {}
    NfaNode(NfaNode&& node)
        : nodes_forward_(std::move(node.nodes_forward_)),
          conditionless_transfer_nodes_handler_(
              std::move(node.conditionless_transfer_nodes_handler_)) {}

    NodeHandler GetForwardNodesHandler(char c_transfer);
    const std::unordered_set<NodeHandler>& GetUnconditionTransferNodesHandler();

    //添加条件转移节点，遇到已存在节点会返回false
    bool AddConditionTransfer(char c_transfer, NodeHandler node_handler);
    //添加无条件转移节点
    bool AddNoconditionTransfer(NodeHandler node_handler);
    //移除一个转移条件节点，函数保证执行后不存在节点（无论原来是否存在)
    bool RemoveConditionalTransfer(char c_treasfer);
    //同上，移除一个无条件节点，输入-1代表清除所有
    bool RemoveConditionlessTransfer(NodeHandler node_handler);

    bool MergeNodesWithManager(NfaNode& node_src);
    //记录转移条件与前向节点，一个条件仅允许对应一个节点
    std::unordered_map<char, NodeGather> nodes_forward_;
    //存储无条件转移节点
    std::unordered_set<NodeHandler> conditionless_transfer_nodes_handler_;
  };

  NfaGenerator();
  NfaGenerator(const NfaGenerator&) = delete;
  NfaGenerator(NfaGenerator&&) = delete;

  const TailNodeTag get_tail_tag(NfaNode* pointer);
  const TailNodeTag get_tail_tag(NodeHandler handler);
  NfaNode* GetNode(NodeHandler handler);
  //解析正则并添加到已有NFA中，返回生成的自动机的头结点和尾节点，自动处理结尾的范围限制符号
  std::pair<NodeHandler, NodeHandler> regex_construct(
      std::istream& in, const TailNodeTag& tag, bool add_to_NFA_head,
      bool return_when_right_bracket);
  //添加一个由字符串构成的NFA，自动处理结尾的范围限制符号
  std::pair<NodeHandler, NodeHandler> word_construct(const std::string& str,
                                                     const TailNodeTag& tag);
  //合并优化，降低节点数以降低子集构造法集合大小，直接使用NFA也可以降低成本
  void merge_optimization();

  std::pair<std::unordered_set<NodeHandler>, TailNodeTag> Closure(
      NodeHandler handler);
  //返回goto后的节点的闭包
  std::pair<std::unordered_set<NodeHandler>, TailNodeTag> Goto(
      NodeHandler handler_src, char c_transform);
  //序列化容器用
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version = 0);

 private:
  bool remove_tail_node(NfaNode* pointer);
  bool add_tail_node(NfaNode* pointer, const TailNodeTag& tag);
  bool add_tail_node(NodeHandler handler, const TailNodeTag& tag) {
    return add_tail_node(GetNode(handler), tag);
  }
  //生成可选字符序列，会读取]后的*,+,?等限定符
  std::pair<NodeHandler, NodeHandler> create_switch_tree(std::istream& in);

  //所有NFA的头结点
  NodeHandler head_node_handler_;
  //该set用来存储所有尾节点和对应单词的tag
  std::unordered_map<NfaNode*, TailNodeTag> tail_nodes_;
  MultimapNodeManager<NfaNode> node_manager_;
};

bool MergeNfaNodesWithGenerator(NfaGenerator::NfaNode& node_dst,
                                NfaGenerator::NfaNode& node_src,
                                NfaGenerator& generator);