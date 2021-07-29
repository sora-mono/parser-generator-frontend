#include "nfa_generator.h"

#include <algorithm>
#include <queue>
#include <sstream>

#include "Generator/DfaGenerator/NfaGenerator/nfa_generator.h"
#include "nfa_generator.h"

namespace frontend::generator::dfa_generator::nfa_generator {

NfaGenerator::NfaNodeId NfaGenerator::NfaNode::GetForwardNodesId(
    char c_transfer) {
  auto iter = nodes_forward.find(c_transfer);
  assert(iter != nodes_forward.end());
  return iter->second;
}

inline const std::unordered_set<NfaGenerator::NfaNodeId>&
NfaGenerator::NfaNode::GetUnconditionTransferNodesId() {
  return NfaGenerator::NfaNode::conditionless_transfer_nodes_id;
}

inline void NfaGenerator::NfaNode::SetConditionTransfer(
    char c_condition, NfaGenerator::NfaNodeId node_id) {
  nodes_forward[c_condition] = node_id;
}

inline void NfaGenerator::NfaNode::AddNoconditionTransfer(NfaNodeId node_id) {
  conditionless_transfer_nodes_id.insert(node_id);
}

inline void NfaGenerator::NfaNode::RemoveConditionalTransfer(char c_treasfer) {
  auto iter = nodes_forward.find(c_treasfer);
  if (iter != nodes_forward.end()) {
    nodes_forward.erase(iter);
  }
}

inline void NfaGenerator::NfaNode::RemoveConditionlessTransfer(
    NfaNodeId node_id) {
  auto iter = conditionless_transfer_nodes_id.find(node_id);
  if (iter != conditionless_transfer_nodes_id.end()) {
    conditionless_transfer_nodes_id.erase(iter);
  }
}

std::pair<std::unordered_set<typename NfaGenerator::NfaNodeId>,
          typename NfaGenerator::TailNodeData>
NfaGenerator::Closure(NfaNodeId production_node_id) {
  std::unordered_set<NfaNodeId> uset_temp =
      node_manager_.GetIdsReferringSameObject(production_node_id);
  TailNodeData tail_node_data(NotTailNodeTag);
  std::queue<NfaNodeId> q;
  for (auto x : uset_temp) {
    q.push(x);
  }
  while (!q.empty()) {
    NfaNodeId id_now = q.front();
    q.pop();
    if (uset_temp.find(id_now) != uset_temp.end()) {
      continue;
    }
    uset_temp.insert(id_now);
    auto iter = tail_nodes_.find(&GetNfaNode(id_now));
    // 判断是否为尾节点
    if (iter != tail_nodes_.end()) {
      TailNodeData& tail_node_data_new = iter->second;
      WordPriority priority_old = tail_node_data.second;
      WordPriority priority_new = tail_node_data_new.second;

      if (tail_node_data == NotTailNodeTag) {
        // 以前无尾节点记录
        tail_node_data = tail_node_data_new;
      } else if (priority_new > priority_old) {
        // 当前记录优先级大于以前的优先级
        tail_node_data = tail_node_data_new;
      } else if (priority_new == priority_old &&
                 tail_node_data_new.first != tail_node_data.first) {
        // TODO 去除异常
        // 两个尾节点标记优先级相同，对应尾节点不同
        throw std::runtime_error("两个尾节点具有相同优先级且不对应同一个节点");
      }
    }
    const std::unordered_set<NfaNodeId>& uset_nodes =
        node_manager_.GetIdsReferringSameObject(id_now);
    for (auto x : uset_nodes) {
      q.push(x);
    }
  }
  return std::pair<std::unordered_set<NfaNodeId>, TailNodeData>(
      std::move(uset_temp), std::move(tail_node_data));
}

std::pair<std::unordered_set<typename NfaGenerator::NfaNodeId>,
          typename NfaGenerator::TailNodeData>
NfaGenerator::Goto(NfaNodeId id_src, char c_transform) {
  NfaNode& pointer_node = GetNfaNode(id_src);
  NfaNodeId production_node_id = pointer_node.GetForwardNodesId(c_transform);
  if (!production_node_id.IsValid()) {
    return std::pair(std::unordered_set<NfaNodeId>(), NotTailNodeTag);
  }
  return Closure(production_node_id);
}

void NfaGenerator::NfaInit() {
  head_node_id_ = NfaNodeId::InvalidId();
  tail_nodes_.clear();
  node_manager_.MultimapObjectManagerInit();
  head_node_id_ = node_manager_.EmplaceObject();  // 添加头结点
}

bool NfaGenerator::NfaNode::MergeNodesWithManager(NfaNode& node_src) {
  if (&node_src == this) {  // 相同节点合并则直接返回true
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
  conditionless_transfer_nodes_id.merge(
      node_src.conditionless_transfer_nodes_id);
  return true;
}

inline const NfaGenerator::TailNodeData NfaGenerator::GetTailNodeData(
    NfaNode* pointer) {
  auto iter = tail_nodes_.find(pointer);
  if (iter == tail_nodes_.end()) {
    return NotTailNodeTag;
  }
  return iter->second;
}

inline const NfaGenerator::TailNodeData NfaGenerator::GetTailNodeData(
    NfaNodeId production_node_id) {
  return GetTailNodeData(&GetNfaNode(production_node_id));
}
// TODO 将流改成printf等函数
std::pair<NfaGenerator::NfaNodeId, NfaGenerator::NfaNodeId>
NfaGenerator::RegexConstruct(std::istream& in, const TailNodeData& tag,
                             const bool add_to_NFA_head,
                             const bool return_when_right_bracket) {
  NfaNodeId head_id = node_manager_.EmplaceObject();
  NfaNodeId tail_id = head_id;
  NfaNodeId pre_tail_id = head_id;
  char c_now;
  in >> c_now;
  while (c_now != '\0' && in) {
    NfaNodeId temp_head_id = NfaNodeId::InvalidId(),
              temp_tail_id = NfaNodeId::InvalidId();
    switch (c_now) {
      case '[':
        in.putback(c_now);
        std::pair(temp_head_id, temp_tail_id) = CreateSwitchTree(in);
        if (!(temp_head_id.IsValid() && temp_tail_id.IsValid())) {
          throw std::invalid_argument("非法正则");
        }
        GetNfaNode(tail_id).AddNoconditionTransfer(temp_tail_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
        break;
      case ']':
        throw std::runtime_error(
            "regex_construct函数不应该处理]字符，应交给create_switch_tree处理");
        break;
      case '(':
        std::pair(temp_head_id, temp_tail_id) =
            RegexConstruct(in, NotTailNodeTag, false, true);
        if (!(temp_head_id.IsValid() && temp_tail_id.IsValid())) {
          throw std::invalid_argument("非法正则");
        }
        GetNfaNode(tail_id).AddNoconditionTransfer(temp_head_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
        break;
      case ')':
        in >> c_now;
        if (!in) {
          c_now = '\0';
          break;
        }
        switch (c_now) {
          case '*':
            temp_tail_id = node_manager_.EmplaceObject();
            GetNfaNode(tail_id).AddNoconditionTransfer(temp_tail_id);
            GetNfaNode(temp_tail_id).AddNoconditionTransfer(head_id);
            GetNfaNode(head_id).AddNoconditionTransfer(temp_tail_id);
            pre_tail_id = tail_id;
            tail_id = temp_tail_id;
            break;
          case '+':
            temp_tail_id = node_manager_.EmplaceObject();
            GetNfaNode(temp_tail_id).AddNoconditionTransfer(head_id);
            GetNfaNode(tail_id).AddNoconditionTransfer(head_id);
            pre_tail_id = tail_id;
            tail_id = temp_tail_id;
            break;
          case '?':
            temp_tail_id = node_manager_.EmplaceObject();
            GetNfaNode(tail_id).AddNoconditionTransfer(temp_tail_id);
            GetNfaNode(head_id).AddNoconditionTransfer(temp_tail_id);
            pre_tail_id = tail_id;
            tail_id = temp_tail_id;
            break;
          default:
            in.putback(c_now);
            break;
        }
        if (return_when_right_bracket) {
          c_now = '\0';
        }
        break;
      case '+':  // 仅对单个字符生效
        temp_tail_id = node_manager_.EmplaceObject();
        GetNfaNode(tail_id).AddNoconditionTransfer(temp_tail_id);
        GetNfaNode(pre_tail_id).AddNoconditionTransfer(temp_tail_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
        break;
      case '*':  // 仅对单个字符生效
        temp_tail_id = node_manager_.EmplaceObject();
        GetNfaNode(tail_id).AddNoconditionTransfer(temp_tail_id);
        GetNfaNode(pre_tail_id).AddNoconditionTransfer(temp_tail_id);
        GetNfaNode(temp_tail_id).AddNoconditionTransfer(pre_tail_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
        break;
      case '?':  // 仅对单个字符生效
        temp_tail_id = node_manager_.EmplaceObject();
        GetNfaNode(tail_id).AddNoconditionTransfer(temp_tail_id);
        GetNfaNode(pre_tail_id).AddNoconditionTransfer(temp_tail_id);
        pre_tail_id = tail_id;
        tail_id = pre_tail_id;
        break;
      case '\\': // 仅对单个字符生效
        in >> c_now;
        if (!in || c_now == '\0') {
          throw std::invalid_argument("非法正则");
        }
        temp_tail_id = node_manager_.EmplaceObject();
        GetNfaNode(tail_id).SetConditionTransfer(c_now, temp_tail_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
        break;
      case '.':  // 仅对单个字符生效
        temp_tail_id = node_manager_.EmplaceObject();
        for (char transform_char = CHAR_MIN; transform_char != CHAR_MAX;
             ++transform_char) {
          GetNfaNode(tail_id).SetConditionTransfer(transform_char,
                                                   temp_tail_id);
        }
        GetNfaNode(tail_id).SetConditionTransfer(CHAR_MAX, temp_tail_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
        break;
      default:
        temp_tail_id = node_manager_.EmplaceObject();
        GetNfaNode(tail_id).SetConditionTransfer(c_now, temp_tail_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
        break;
    }
    if (c_now != '\0') {
      in >> c_now;
    }
  }
  if (head_id != tail_id) {
    if (add_to_NFA_head) {
      GetNfaNode(head_node_id_).AddNoconditionTransfer(head_id);
      AddTailNode(tail_id, tag);
    }
  } else {
    node_manager_.RemoveObject(head_id);
    head_id = tail_id = NfaNodeId::InvalidId();
  }
  return std::make_pair(head_id, tail_id);
}

std::pair<NfaGenerator::NfaNodeId, NfaGenerator::NfaNodeId>
NfaGenerator::WordConstruct(const std::string& str, const TailNodeData& tag) {
  assert(str.size() != 0);
  NfaNodeId head_id = node_manager_.EmplaceObject();
  NfaNodeId tail_id = head_id;
  for (auto c : str) {
    NfaNodeId temp_id = node_manager_.EmplaceObject();
    GetNfaNode(tail_id).SetConditionTransfer(c, temp_id);
    tail_id = temp_id;
  }
  GetNfaNode(head_node_id_).AddNoconditionTransfer(head_id);
  AddTailNode(tail_id, tag);
  return std::make_pair(head_id, tail_id);
}

void NfaGenerator::MergeOptimization() {
  node_manager_.SetAllObjectsMergeAllowed();
  std::queue<NfaNodeId> q;
  for (auto x : GetNfaNode(head_node_id_).conditionless_transfer_nodes_id) {
    q.push(x);
  }
  while (!q.empty()) {
    NfaNodeId id_now = q.front();
    q.pop();
    bool merged = false;
    bool CanMerge = node_manager_.CanMerge(id_now);
    if (!CanMerge) {
      continue;
    }
    for (auto x : GetNfaNode(id_now).conditionless_transfer_nodes_id) {
      merged |= node_manager_.MergeObjectsWithManager<NfaGenerator>(
          id_now, x, *this, MergeNfaNodesWithGenerator);
      q.push(x);
    }
    if (merged) {
      q.push(id_now);
    } else {
      node_manager_.SetObjectMergeRefused(id_now);
    }
  }
}

inline bool NfaGenerator::RemoveTailNode(NfaNode* pointer) {
  if (pointer == nullptr) {
    return false;
  }
  tail_nodes_.erase(pointer);
  return true;
}

bool NfaGenerator::AddTailNode(NfaNode* pointer, const TailNodeData& tag) {
  assert(pointer != nullptr);
  tail_nodes_.insert(std::make_pair(pointer, tag));
  return false;
}

std::pair<NfaGenerator::NfaNodeId, NfaGenerator::NfaNodeId>
NfaGenerator::CreateSwitchTree(std::istream& in) {
  NfaNodeId head_id = node_manager_.EmplaceObject();
  NfaNodeId tail_id = node_manager_.EmplaceObject();
  char c_now, c_pre;
  in >> c_now;
  c_pre = c_now;
  NfaNode& p_node = GetNfaNode(head_id);
  while (in && c_now != ']') {
    switch (c_now) {
      case '-':
        for (char c = c_pre; c != c_now; c++) {
          p_node.SetConditionTransfer(c, tail_id);
        }
        p_node.SetConditionTransfer(c_now, tail_id);
        break;
      case '\\':
        in >> c_now;
        if (!in || c_now == '\0') {
          throw std::invalid_argument("非法正则");
        }
        p_node.SetConditionTransfer(c_now, tail_id);
        break;
      default:
        p_node.SetConditionTransfer(c_now, tail_id);
        break;
    }
    c_pre = c_now;
    in >> c_now;
  }
  if (c_now != ']' || !in) {
    throw std::invalid_argument("非法正则");
  }
  if (head_id == tail_id) {
    node_manager_.RemoveObject(head_id);
    throw std::invalid_argument("[]中为空");
  }

  in >> c_now;
  if (!in) {
    return std::pair(head_id, tail_id);
  }
  switch (c_now) {
    case '*':
      GetNfaNode(head_id).AddNoconditionTransfer(tail_id);
      GetNfaNode(tail_id).AddNoconditionTransfer(head_id);
      break;
    case '+':
      GetNfaNode(tail_id).AddNoconditionTransfer(head_id);
      break;
    case '?':
      GetNfaNode(head_id).AddNoconditionTransfer(tail_id);
      break;
    default:
      in.putback(c_now);
      break;
  }
  return std::pair(head_id, tail_id);
}

bool MergeNfaNodesWithGenerator(NfaGenerator::NfaNode& node_dst,
                                NfaGenerator::NfaNode& node_src,
                                NfaGenerator& generator) {
  const NfaGenerator::TailNodeData& dst_tag =
      generator.GetTailNodeData(&node_dst);
  const NfaGenerator::TailNodeData& src_tag =
      generator.GetTailNodeData(&node_src);
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

const NfaGenerator::TailNodeData NfaGenerator::NotTailNodeTag =
    NfaGenerator::TailNodeData(WordAttachedData(),
                               WordPriority::InvalidId());

}  // namespace frontend::generator::dfa_generator::nfa_generator