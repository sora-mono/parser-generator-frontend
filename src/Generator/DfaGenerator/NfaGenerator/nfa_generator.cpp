#include "nfa_generator.h"

#include <algorithm>
#include <format>
#include <queue>
#include <sstream>

#include "Generator/DfaGenerator/NfaGenerator/nfa_generator.h"
#include "nfa_generator.h"

namespace frontend::generator::dfa_generator::nfa_generator {

NfaGenerator::NfaNodeId NfaGenerator::NfaNode::GetForwardNodesId(
    char c_transfer) {
  auto iter = nodes_forward.find(c_transfer);
  if (iter == nodes_forward.end()) {
    return NfaNodeId::InvalidId();
  } else {
    return iter->second;
  }
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
  std::unordered_set<NfaNodeId> result_set;
  TailNodeData word_attached_data(NotTailNodeTag);
  std::queue<NfaNodeId> q;
  for (auto x : node_manager_.GetIdsReferringSameObject(production_node_id)) {
    q.push(x);
  }
  // 清空等效节点集合，否则队列中所有节点都在集合内，导致
  result_set.clear();
  while (!q.empty()) {
    NfaNodeId id_now = q.front();
    q.pop();
    if (result_set.find(id_now) != result_set.end()) {
      continue;
    }
    auto [result_iter, inserted] = result_set.insert(id_now);
    assert(inserted);
    const auto& nfa_node = GetNfaNode(id_now);
    auto iter = tail_nodes_.find(&nfa_node);
    // 判断是否为尾节点
    if (iter != tail_nodes_.end()) {
      TailNodeData& tail_node_data_new = iter->second;
      WordPriority priority_old = word_attached_data.second;
      WordPriority priority_new = tail_node_data_new.second;
      if (word_attached_data == NotTailNodeTag) {
        // 以前无尾节点记录
        word_attached_data = tail_node_data_new;
      } else if (priority_new > priority_old) {
        // 当前记录优先级大于以前的优先级
        word_attached_data = tail_node_data_new;
      } else if (priority_new == priority_old &&
                 tail_node_data_new.first != word_attached_data.first) {
        // 两个尾节点标记优先级相同，对应尾节点不同
        // 输出错误信息
        std::cerr
            << std::format(
                   "NfaGenerator "
                   "Error:"
                   "存在两个正则表达式在相同的输入下均可获取单词，且单词优先级"
                   "相同，产生歧义，请检查终结节点定义/运算符定义部分")
            << std::endl;
        assert(false);
        exit(-1);
      }
    }
    for (auto x : nfa_node.GetUnconditionTransferNodesIds()) {
      q.push(x);
    }
  }
  return std::make_pair(std::move(result_set), std::move(word_attached_data));
}

std::pair<std::unordered_set<typename NfaGenerator::NfaNodeId>,
          typename NfaGenerator::TailNodeData>
NfaGenerator::Goto(NfaNodeId id_src, char c_transform) {
  NfaNode& pointer_node = GetNfaNode(id_src);
  NfaNodeId production_node_id = pointer_node.GetForwardNodesId(c_transform);
  if (!production_node_id.IsValid()) {
    return std::make_pair(std::unordered_set<NfaNodeId>(), NotTailNodeTag);
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
    bool CanBeSourceInMerge = true;
    for (auto& p : node_src.nodes_forward) {
      auto iter = nodes_forward.find(p.first);
      if (iter != nodes_forward.end() && iter->second != p.second) {
        CanBeSourceInMerge = false;
        break;
      }
    }
    if (!CanBeSourceInMerge) {
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
      case '[': {
        std::tie(temp_head_id, temp_tail_id) = CreateSwitchTree(in);
        if (!(temp_head_id.IsValid() && temp_tail_id.IsValid())) {
          throw std::invalid_argument("非法正则");
        }
        GetNfaNode(tail_id).AddNoconditionTransfer(temp_head_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
      } break;
      case ']':
        throw std::runtime_error(
            "regex_construct函数不应该处理]字符，应交给create_switch_tree处理");
        break;
      case '(': {
        std::tie(temp_head_id, temp_tail_id) =
            RegexConstruct(in, NotTailNodeTag, false, true);
        if (!(temp_head_id.IsValid() && temp_tail_id.IsValid())) {
          throw std::invalid_argument("非法正则");
        }
        GetNfaNode(tail_id).AddNoconditionTransfer(temp_head_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
      } break;
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
      case '\\':  // 仅对单个字符生效
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
NfaGenerator::WordConstruct(const std::string& str,
                            TailNodeData&& word_attached_data) {
  assert(str.size() != 0);
  NfaNodeId head_id = node_manager_.EmplaceObject();
  NfaNodeId tail_id = head_id;
  for (auto c : str) {
    NfaNodeId temp_id = node_manager_.EmplaceObject();
    GetNfaNode(tail_id).SetConditionTransfer(c, temp_id);
    tail_id = temp_id;
  }
  GetNfaNode(head_node_id_).AddNoconditionTransfer(head_id);
  AddTailNode(tail_id, std::move(word_attached_data));
  return std::make_pair(head_id, tail_id);
}

void NfaGenerator::MergeOptimization() {
  node_manager_.SetAllObjectsCanBeSourceInMerge();
  std::queue<NfaNodeId> q;
  q.push(GetHeadNfaNodeId());
  while (!q.empty()) {
    NfaNodeId id_now = q.front();
    NfaNode& node_now = GetNfaNode(id_now);
    q.pop();
    if (!node_manager_.CanBeSourceInMerge(id_now)) {
      continue;
    }
    // 检查每一个与当前处理节点等价的节点（就是当前节点可以无条件转移到的节点）
    // 如果当前节点转移表中的项在等价节点中存在则可以从当前节点转移表中删除
    for (auto equal_node_id :
         GetNfaNode(id_now).GetUnconditionTransferNodesIds()) {
      if (equal_node_id == id_now) [[unlikely]] {
        // 跳过转移到自己的情况
        continue;
      }
      auto& equal_node = GetNfaNode(equal_node_id);
      // 处理无条件转移表
      const auto& equal_node_unconditional_transfer_node_ids =
          equal_node.GetUnconditionTransferNodesIds();
      for (auto iter = node_now.GetUnconditionTransferNodesIds().begin();
           iter != node_now.GetUnconditionTransferNodesIds().end();) {
        if (equal_node_unconditional_transfer_node_ids.find(*iter) !=
            equal_node_unconditional_transfer_node_ids.end()) {
          // 当前节点转移表中的项在等价节点中存在
          // 移除该项
          if (*iter != equal_node_id) [[likely]] {
            // 如果存在等价节点的无条件自环节点则不能移除
            // 否则会失去指向等价节点的记录，导致正则不完整、内存泄漏等
            iter = node_now.GetUnconditionTransferNodesIds().erase(iter);
            // continue防止额外前移iter
            continue;
          }
        }
        ++iter;
      }
      // 处理条件转移表
      const auto& equal_node_conditional_transfers =
          equal_node.GetConditionalTransfers();
      for (auto iter = node_now.GetConditionalTransfers().begin();
           iter != node_now.GetConditionalTransfers().end();) {
        auto equal_node_iter =
            equal_node_conditional_transfers.find(iter->first);
        if (equal_node_iter != equal_node_conditional_transfers.end() &&
            iter->second == equal_node_iter->second) {
          // 当前节点转移表中的项在等价节点中存在
          // 移除该项
          iter = node_now.GetConditionalTransfers().erase(iter);
          // continue防止额外前移iter
          continue;
        }
        ++iter;
      }
      // 压入等效节点等待处理
      q.push(equal_node_id);
      // 压入当前节点的所有可以条件转移到的节点等待处理
      for (const auto& conditional_transfer :
           node_now.GetConditionalTransfers()) {
        q.push(conditional_transfer.second);
      }
    }
    if (node_now.GetConditionalTransfers().empty() &&
        node_now.GetUnconditionTransferNodesIds().size() == 1) [[unlikely]] {
      // 只剩一条无条件转移路径，该节点可以与无条件转移到的节点合并
      bool result = node_manager_.MergeObjectsWithManager<NfaGenerator>(
          *node_now.GetUnconditionTransferNodesIds().begin(), id_now, *this,
          MergeNfaNodes);
      assert(result);
    } else {
      // 没有执行任何操作，不存在从该节点开始的合并操作
      // 设置该节点在合并时不能作为源节点
      node_manager_.SetObjectCanNotBeSourceInMerge(id_now);
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
  char character_now, character_pre;
  in >> character_now;
  character_pre = character_now;
  NfaNode& head_node = GetNfaNode(head_id);
  while (in && character_now != ']') {
    switch (character_now) {
      case '-':
        // 读入另一端的字符
        in >> character_now;
        for (char bigger_character = std::max(character_now, character_pre),
                  smaller_character = std::min(character_pre, character_now);
             smaller_character != character_now; smaller_character++) {
          head_node.SetConditionTransfer(smaller_character, tail_id);
        }
        head_node.SetConditionTransfer(character_now, tail_id);
        break;
      case '\\':
        in >> character_now;
        if (!in || character_now == '\0') {
          throw std::invalid_argument("非法正则");
        }
        head_node.SetConditionTransfer(character_now, tail_id);
        break;
      default:
        head_node.SetConditionTransfer(character_now, tail_id);
        break;
    }
    character_pre = character_now;
    in >> character_now;
  }
  if (character_now != ']' || !in) {
    throw std::invalid_argument("非法正则");
  }
  if (head_node.GetConditionalTransfers().empty()) {
    node_manager_.RemoveObject(head_id);
    throw std::invalid_argument("[]中为空");
  }

  in >> character_now;
  if (!in) {
    return std::make_pair(head_id, tail_id);
  }
  switch (character_now) {
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
      in.putback(character_now);
      break;
  }
  return std::make_pair(head_id, tail_id);
}

bool NfaGenerator::MergeNfaNodes(NfaGenerator::NfaNode& node_dst,
                                 NfaGenerator::NfaNode& node_src,
                                 NfaGenerator& nfa_generator) {
  const NfaGenerator::TailNodeData dst_tag =
      nfa_generator.GetTailNodeData(&node_dst);
  const NfaGenerator::TailNodeData src_tag =
      nfa_generator.GetTailNodeData(&node_src);
  if (dst_tag != NfaGenerator::NotTailNodeTag &&
      src_tag != NfaGenerator::NotTailNodeTag &&
      dst_tag.second == src_tag.second && dst_tag.first != src_tag.first) {
    throw std::runtime_error("两个尾节点具有相同优先级且不对应同一个节点");
  }
  bool result = node_dst.MergeNodesWithManager(node_src);
  if (result) {
    if (src_tag != NfaGenerator::NotTailNodeTag) {
      nfa_generator.RemoveTailNode(&node_src);
      nfa_generator.AddTailNode(&node_dst, src_tag);
    }
    return true;
  } else {
    return false;
  }
}

const NfaGenerator::TailNodeData NfaGenerator::NotTailNodeTag =
    NfaGenerator::TailNodeData(WordAttachedData(), WordPriority::InvalidId());

// 根据上一个操作是否为规约判断使用左侧单目运算符优先级还是双目运算符优先级
// 返回获取到的结合类型和优先级

std::pair<frontend::common::OperatorAssociatityType, size_t>
NfaGenerator::WordAttachedData::GetAssociatityTypeAndPriority(
    bool is_last_operate_reduct) const {
  assert(node_type == frontend::common::ProductionNodeType::kOperatorNode);
  if (binary_operator_priority != -1) {
    if (unary_operator_priority != -1) {
      // 两种语义均存在
      if (is_last_operate_reduct) {
        // 上次操作为规约，应使用左侧单目运算符语义
        return std::make_pair(unary_operator_associate_type,
                              unary_operator_priority);
      } else {
        // 上次操作为移入，应使用双目运算符语义
        return std::make_pair(binary_operator_associate_type,
                              binary_operator_priority);
      }
    } else {
      // 仅存在双目运算符语义，直接返回
      return std::make_pair(binary_operator_associate_type,
                            binary_operator_priority);
    }
  } else {
    // 仅存在单目运算符语义，直接返回
    return std::make_pair(unary_operator_associate_type,
                          unary_operator_priority);
  }
}

}  // namespace frontend::generator::dfa_generator::nfa_generator