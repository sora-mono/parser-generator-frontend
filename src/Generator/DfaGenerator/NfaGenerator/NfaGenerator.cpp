#include "Generator/DfaGenerator/NfaGenerator/NfaGenerator.h"

#include <algorithm>
#include <queue>
#include <sstream>

namespace frontend::generator::dfagenerator::nfagenerator {

NfaGenerator::NodeId NfaGenerator::NfaNode::GetForwardNodesHandler(
    char c_transfer) {
  auto iter = nodes_forward.find(c_transfer);
  if (iter == nodes_forward.end()) {
    return -1;
  }
  return iter->second;
}

inline const std::unordered_set<NfaGenerator::NodeId>&
NfaGenerator::NfaNode::GetUnconditionTransferNodesHandler() {
  return NfaGenerator::NfaNode::conditionless_transfer_nodes_handler;
}

inline bool NfaGenerator::NfaNode::AddConditionTransfer(
    char c_condition, NfaGenerator::NodeId node_handler) {
  auto iter = nodes_forward.find(c_condition);
  if (iter != nodes_forward.end() &&
      iter->second != node_handler) {  //该条件下已有不同转移节点，不能覆盖
    return false;
  }
  nodes_forward[c_condition] = node_handler;
  return true;
}

inline bool NfaGenerator::NfaNode::AddNoconditionTransfer(NodeId node_handler) {
  conditionless_transfer_nodes_handler.insert(node_handler);
  return true;
}

inline bool NfaGenerator::NfaNode::RemoveConditionalTransfer(char c_treasfer) {
  auto iter = nodes_forward.find(c_treasfer);
  if (iter != nodes_forward.end()) {
    nodes_forward.erase(iter);
  }
  return true;
}

inline bool NfaGenerator::NfaNode::RemoveConditionlessTransfer(
    NodeId node_handler) {
  if (node_handler == -1) {
    conditionless_transfer_nodes_handler.clear();
  } else {
    auto iter = conditionless_transfer_nodes_handler.find(node_handler);
    if (iter != conditionless_transfer_nodes_handler.end()) {
      conditionless_transfer_nodes_handler.erase(iter);
    }
  }
  return true;
}

std::pair<std::unordered_set<typename NfaGenerator::NodeId>,
          typename NfaGenerator::TailNodeData>
NfaGenerator::Closure(NodeId handler) {
  std::unordered_set<NodeId> uset_temp =
      node_manager_.GetHandlersReferringSameNode(handler);
  TailNodeData tag(-1, -1);
  std::queue<NodeId> q;
  for (auto x : uset_temp) {
    q.push(x);
  }
  while (!q.empty()) {
    NodeId handler_now = q.front();
    q.pop();
    if (uset_temp.find(handler_now) != uset_temp.end()) {
      continue;
    }
    uset_temp.insert(handler_now);
    auto iter = tail_nodes_.find(GetNode(handler_now));  //判断是否为尾节点
    if (iter != tail_nodes_.end()) {
      if (tag == NotTailNodeTag) {  //以前无尾节点记录
        tag = iter->second;
      } else if (iter->second.second >
                 tag.second) {  //当前记录优先级大于以前的优先级
        tag = iter->second;
      } else if (iter->second.second == tag.second &&
                 iter->first != tag.first) {
        //两个尾节点标记优先级相同，对应尾节点不同
        throw std::runtime_error("两个尾节点具有相同优先级且不对应同一个节点");
      }
    }
    const std::unordered_set<NodeId> uset_nodes =
        node_manager_.GetHandlersReferringSameNode(handler_now);
    for (auto x : uset_nodes) {
      q.push(x);
    }
  }
  return std::pair<std::unordered_set<NodeId>, TailNodeData>(
      std::move(uset_temp), std::move(tag));
}

std::pair<std::unordered_set<typename NfaGenerator::NodeId>,
          typename NfaGenerator::TailNodeData>
NfaGenerator::Goto(NodeId handler_src, char c_transform) {
  NfaNode* pointer_node = GetNode(handler_src);
  if (pointer_node == nullptr) {
    return std::pair(std::unordered_set<NodeId>(), NotTailNodeTag);
  }
  NodeId handler = pointer_node->GetForwardNodesHandler(c_transform);
  if (handler == -1) {
    return std::pair(std::unordered_set<NodeId>(), NotTailNodeTag);
  }
  return Closure(handler);
}

bool NfaGenerator::NfaNode::MergeNodesWithManager(NfaNode& node_src) {
  if (&node_src == this) {  //相同节点合并则直接返回true
    return true;
  }
  if (nodes_forward.size() != 0 && node_src.nodes_forward.size() != 0) {
    bool CanMerge = true;
    for (auto& p : node_src.nodes_forward) {
      auto iter = nodes_forward.find(p.first);
      if (iter != nodes_forward.end()) {
        CanMerge = false;
        break;
      }
    }
    if (!CanMerge) {
      return false;
    }
  }
  nodes_forward.merge(node_src.nodes_forward);
  conditionless_transfer_nodes_handler.merge(
      node_src.conditionless_transfer_nodes_handler);
  return true;
}

NfaGenerator::NfaGenerator() {
  head_node_handler_ = node_manager_.EmplaceNode();  //添加头结点
}

inline const NfaGenerator::TailNodeData NfaGenerator::GetTailTag(
    NfaNode* pointer) {
  auto iter = tail_nodes_.find(pointer);
  if (iter == tail_nodes_.end()) {
    return NotTailNodeTag;
  }
  return iter->second;
}

inline const NfaGenerator::TailNodeData NfaGenerator::GetTailTag(
    NodeId handler) {
  return GetTailTag(GetNode(handler));
}

inline NfaGenerator::NfaNode* NfaGenerator::GetNode(NodeId handler) {
  return node_manager_.GetNode(handler);
}

std::pair<NfaGenerator::NodeId, NfaGenerator::NodeId>
NfaGenerator::RegexConstruct(std::istream& in, const TailNodeData& tag,
                             bool add_to_NFA_head,
                             bool return_when_right_bracket) {
  NodeId head_handler = node_manager_.EmplaceNode();
  NodeId tail_handler = head_handler;
  NodeId pre_tail_handler = head_handler;
  char c_now;
  in >> c_now;
  while (c_now != '\0' && in) {
    NodeId temp_head_handler = -1, temp_tail_handler = -1;
    switch (c_now) {
      case '[':
        in.putback(c_now);
        std::pair(temp_head_handler, temp_tail_handler) = CreateSwitchTree(in);
        if (temp_head_handler == -1 || temp_tail_handler == -1) {
          throw std::invalid_argument("非法正则");
        }
        GetNode(tail_handler)->AddNoconditionTransfer(temp_tail_handler);
        pre_tail_handler = tail_handler;
        tail_handler = temp_tail_handler;
        break;
      case ']':
        throw std::runtime_error(
            "regex_construct函数不应该处理]字符，应交给create_switch_tree处理");
        break;
      case '(':
        std::pair(temp_head_handler, temp_tail_handler) =
            RegexConstruct(in, NotTailNodeTag, false, true);
        if (temp_head_handler == -1 || temp_tail_handler == -1) {
          throw std::invalid_argument("非法正则");
        }
        GetNode(tail_handler)->AddNoconditionTransfer(temp_head_handler);
        pre_tail_handler = tail_handler;
        tail_handler = temp_tail_handler;
        break;
      case ')':
        in >> c_now;
        if (!in) {
          c_now = '\0';
          break;
        }
        switch (c_now) {
          case '*':
            temp_tail_handler = node_manager_.EmplaceNode();
            GetNode(tail_handler)->AddNoconditionTransfer(temp_tail_handler);
            GetNode(temp_tail_handler)->AddNoconditionTransfer(head_handler);
            GetNode(head_handler)->AddNoconditionTransfer(temp_tail_handler);
            pre_tail_handler = tail_handler;
            tail_handler = temp_tail_handler;
            break;
          case '+':
            temp_tail_handler = node_manager_.EmplaceNode();
            GetNode(temp_tail_handler)->AddNoconditionTransfer(head_handler);
            GetNode(tail_handler)->AddNoconditionTransfer(head_handler);
            pre_tail_handler = tail_handler;
            tail_handler = temp_tail_handler;
            break;
          case '?':
            temp_tail_handler = node_manager_.EmplaceNode();
            GetNode(tail_handler)->AddNoconditionTransfer(temp_tail_handler);
            GetNode(head_handler)->AddNoconditionTransfer(temp_tail_handler);
            pre_tail_handler = tail_handler;
            tail_handler = temp_tail_handler;
            break;
          default:
            in.putback(c_now);
            break;
        }
        if (return_when_right_bracket) {
          c_now = '\0';
        }
        break;
      case '+':  //仅对单个字符生效
        temp_tail_handler = node_manager_.EmplaceNode();
        GetNode(tail_handler)->AddNoconditionTransfer(temp_tail_handler);
        GetNode(pre_tail_handler)->AddNoconditionTransfer(temp_tail_handler);
        pre_tail_handler = tail_handler;
        tail_handler = temp_tail_handler;
        break;
      case '*':  //仅对单个字符生效
        temp_tail_handler = node_manager_.EmplaceNode();
        GetNode(tail_handler)->AddNoconditionTransfer(temp_tail_handler);
        GetNode(pre_tail_handler)->AddNoconditionTransfer(temp_tail_handler);
        GetNode(temp_tail_handler)->AddNoconditionTransfer(pre_tail_handler);
        pre_tail_handler = tail_handler;
        tail_handler = temp_tail_handler;
        break;
      case '?':
        temp_tail_handler = node_manager_.EmplaceNode();
        GetNode(tail_handler)->AddNoconditionTransfer(temp_tail_handler);
        GetNode(pre_tail_handler)->AddNoconditionTransfer(temp_tail_handler);
        pre_tail_handler = tail_handler;
        tail_handler = pre_tail_handler;
        break;
      case '\\':
        in >> c_now;
        if (!in || c_now == '\0') {
          throw std::invalid_argument("非法正则");
        }
        temp_tail_handler = node_manager_.EmplaceNode();
        GetNode(tail_handler)->AddConditionTransfer(c_now, temp_tail_handler);
        pre_tail_handler = tail_handler;
        tail_handler = temp_tail_handler;
        break;
      default:
        temp_tail_handler = node_manager_.EmplaceNode();
        GetNode(tail_handler)->AddConditionTransfer(c_now, temp_tail_handler);
        pre_tail_handler = tail_handler;
        tail_handler = temp_tail_handler;
        break;
    }
    if (c_now != '\0') {
      in >> c_now;
    }
  }
  if (head_handler != tail_handler) {
    if (add_to_NFA_head) {
      GetNode(head_node_handler_)->AddNoconditionTransfer(head_handler);
      AddTailNode(tail_handler, tag);
    }
  } else {
    node_manager_.RemoveNode(head_handler);
    head_handler = tail_handler = -1;
  }
  return std::pair<NodeId, NodeId>(head_handler, tail_handler);
}

std::pair<NfaGenerator::NodeId, NfaGenerator::NodeId>
NfaGenerator::WordConstruct(const std::string& str, const TailNodeData& tag) {
  if (str.size() == 0) {
    return std::pair(-1, -1);
  }
  NodeId head_handler = node_manager_.EmplaceNode();
  NodeId tail_handler = head_handler;
  for (auto c : str) {
    NodeId temp_handler = node_manager_.EmplaceNode();
    GetNode(tail_handler)->AddConditionTransfer(c, temp_handler);
    tail_handler = temp_handler;
  }
  GetNode(head_node_handler_)->AddNoconditionTransfer(head_handler);
  AddTailNode(tail_handler, tag);
  return std::pair(head_handler, tail_handler);
}

void NfaGenerator::MergeOptimization() {
  node_manager_.SetAllMergeAllowed();
  std::queue<NodeId> q;
  for (auto x :
       GetNode(head_node_handler_)->conditionless_transfer_nodes_handler) {
    q.push(x);
  }
  while (!q.empty()) {
    NodeId handler_now = q.front();
    q.pop();
    bool merged = false;
    bool CanMerge = node_manager_.CanMerge(handler_now);
    if (!CanMerge) {
      continue;
    }
    for (auto x : GetNode(handler_now)->conditionless_transfer_nodes_handler) {
      merged |= node_manager_.MergeNodesWithManager<NfaGenerator>(
          handler_now, x, *this, MergeNfaNodesWithGenerator);
      q.push(x);
    }
    if (merged) {
      q.push(handler_now);
    } else {
      node_manager_.SetNodeMergeRefused(handler_now);
    }
  }
}

void NfaGenerator::Clear() {
  head_node_handler_ = -1;
  tail_nodes_.clear();
  node_manager_.Clear();
}

inline bool NfaGenerator::RemoveTailNode(NfaNode* pointer) {
  if (pointer == nullptr) {
    return false;
  }
  tail_nodes_.erase(pointer);
  return true;
}

bool NfaGenerator::AddTailNode(NfaNode* pointer, const TailNodeData& tag) {
  if (pointer == nullptr) {
    return false;
  }
  tail_nodes_.insert(std::pair(pointer, tag));
  return false;
}

std::pair<NfaGenerator::NodeId, NfaGenerator::NodeId>
NfaGenerator::CreateSwitchTree(std::istream& in) {
  NodeId head_handler = node_manager_.EmplaceNode();
  NodeId tail_handler = node_manager_.EmplaceNode();
  char c_now, c_pre;
  in >> c_now;
  c_pre = c_now;
  NfaNode* p_node = GetNode(head_handler);
  while (in && c_now != ']') {
    switch (c_now) {
      case '-':
        for (char c = c_pre; c != c_now; c++) {
          p_node->AddConditionTransfer(c, tail_handler);
        }
        p_node->AddConditionTransfer(c_now);
        break;
      case '\\':
        in >> c_now;
        if (!in || c_now == '\0') {
          throw std::invalid_argument("非法正则");
        }
        p_node->AddConditionTransfer(c_now, tail_handler);
        break;
      default:
        p_node->AddConditionTransfer(c_now, tail_handler);
        break;
    }
    c_pre = c_now;
    in >> c_now;
  }
  if (c_now != ']' || !in) {
    throw std::invalid_argument("非法正则");
  }
  if (head_handler == tail_handler) {
    node_manager_.RemoveNode(head_handler);
    throw std::invalid_argument("[]中为空");
    return std::pair(-1, -1);
  }

  in >> c_now;
  if (!in) {
    return std::pair(head_handler, tail_handler);
  }
  switch (c_now) {
    case '*':
      GetNode(head_handler)->AddNoconditionTransfer(tail_handler);
      GetNode(tail_handler)->AddNoconditionTransfer(head_handler);
      break;
    case '+':
      GetNode(tail_handler)->AddNoconditionTransfer(head_handler);
      break;
    case '?':
      GetNode(head_handler)->AddNoconditionTransfer(tail_handler);
      break;
    default:
      in.putback(c_now);
      break;
  }
  return std::pair(head_handler, tail_handler);
}

bool MergeNfaNodesWithGenerator(NfaGenerator::NfaNode& node_dst,
                                NfaGenerator::NfaNode& node_src,
                                NfaGenerator& generator) {
  NfaGenerator::TailNodeData dst_tag = generator.GetTailTag(&node_dst);
  NfaGenerator::TailNodeData src_tag = generator.GetTailTag(&node_src);
  if (dst_tag != NfaGenerator::NotTailNodeTag &&
      src_tag != NfaGenerator::NotTailNodeTag &&
      dst_tag.second == src_tag.second && dst_tag.first != src_tag.first) {
    throw std::runtime_error("两个尾节点具有相同优先级且不对应同一个节点");
  }
  bool result = node_dst.MergeNodesWithManager(node_src);
  if (result) {
    if (src_tag != NfaGenerator::NotTailNodeTag) {
      generator.RemoveTailNode(&node_src);
      generator.AddTailNode(&node_dst, src_tag);
    }
    return true;
  } else {
    return false;
  }
}
}  // namespace frontend::generator::dfagenerator::nfagenerator
#endif  // !GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR
