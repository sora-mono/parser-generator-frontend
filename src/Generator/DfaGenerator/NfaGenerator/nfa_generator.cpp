#include "nfa_generator.h"

#include <format>
#include <queue>

#define ENABLE_LOG
#include "Logger/logger.h"

namespace frontend::generator::dfa_generator::nfa_generator {

NfaGenerator::NfaNodeId NfaGenerator::NfaNode::GetForwardNodeId(
    char c_transfer) {
  auto iter = nodes_forward_.find(c_transfer);
  if (iter == nodes_forward_.end()) {
    return NfaNodeId::InvalidId();
  } else {
    return iter->second;
  }
}

inline void NfaGenerator::NfaNode::SetConditionTransfer(
    char c_condition, NfaGenerator::NfaNodeId node_id) {
  auto [iter, inserted] = nodes_forward_.emplace(c_condition, node_id);
  assert(inserted || iter->second == node_id);
}

inline void NfaGenerator::NfaNode::AddNoconditionTransfer(NfaNodeId node_id) {
  auto [iter, inserted] = conditionless_transfer_nodes_id.insert(node_id);
  assert(inserted);
}

inline size_t NfaGenerator::NfaNode::RemoveConditionalTransfer(
    char c_transfer) {
  return nodes_forward_.erase(c_transfer);
}

inline size_t NfaGenerator::NfaNode::RemoveConditionlessTransfer(
    NfaNodeId node_id) {
  return conditionless_transfer_nodes_id.erase(node_id);
}

std::pair<std::unordered_set<typename NfaGenerator::NfaNodeId>,
          typename NfaGenerator::TailNodeData>
NfaGenerator::Closure(NfaNodeId node_id) {
  std::unordered_set<NfaNodeId> result_set;
  TailNodeData word_attached_data(kNotTailNodeTag);
  std::queue<NfaNodeId> q;
  for (auto x : node_manager_.GetIdsReferringSameObject(node_id)) {
    q.push(x);
  }
  while (!q.empty()) {
    NfaNodeId id_now = q.front();
    q.pop();
    if (result_set.find(id_now) != result_set.end()) {
      continue;
    }
    auto [result_iter, inserted] = result_set.insert(id_now);
    assert(inserted);
    auto iter = tail_nodes_.find(id_now);
    // 判断是否为尾节点
    if (iter != tail_nodes_.end()) {
      TailNodeData& tail_node_data_new = iter->second;
      WordPriority priority_old = word_attached_data.second;
      WordPriority priority_new = tail_node_data_new.second;
      if (word_attached_data == kNotTailNodeTag) {
        // 以前无尾节点记录
        word_attached_data = tail_node_data_new;
      } else if (priority_new > priority_old) {
        // 当前记录优先级大于以前的优先级
        word_attached_data = tail_node_data_new;
      } else if (priority_new == priority_old &&
                 tail_node_data_new.first != word_attached_data.first) {
        // 两个尾节点标记优先级相同，对应尾节点不同
        // 输出错误信息
        LOG_ERROR(
            "NFA Generator",
            std::format(
                "NfaGenerator "
                "Error:"
                "存在两个正则表达式在相同的输入下均可获取单词，且单词优先级"
                "相同，产生歧义，请检查终结节点定义/运算符定义部分"))
        assert(false);
        exit(-1);
      }
    }
    for (auto x : GetNfaNode(id_now).GetUnconditionTransferNodesIds()) {
      q.push(x);
    }
  }
  return std::make_pair(std::move(result_set), std::move(word_attached_data));
}

std::pair<std::unordered_set<typename NfaGenerator::NfaNodeId>,
          typename NfaGenerator::TailNodeData>
NfaGenerator::Goto(NfaNodeId id_src, char c_transform) {
  NfaNode& pointer_node = GetNfaNode(id_src);
  NfaNodeId node_id = pointer_node.GetForwardNodeId(c_transform);
  if (!node_id.IsValid()) {
    return std::make_pair(std::unordered_set<NfaNodeId>(), kNotTailNodeTag);
  }
  return Closure(node_id);
}

void NfaGenerator::NfaInit() {
  tail_nodes_.clear();
  node_manager_.MultimapObjectManagerInit();
  head_node_id_ = node_manager_.EmplaceObject();  // 添加头结点
}

bool NfaGenerator::NfaNode::MergeNodes(NfaNode* node_src) {
  if (node_src == this) {
    // 相同节点合并则直接返回false
    return false;
  }
  // 检查是否可以合并
  // node_src所有条件转移条目必须在this中不存在
  // 或与this中相同条件的转移条目转移到的节点ID相同
  if (GetConditionalTransfers().size() != 0 &&
      node_src->GetConditionalTransfers().size() != 0) {
    for (const auto& transform : node_src->GetConditionalTransfers()) {
      NfaNodeId next_node_id = GetForwardNodeId(transform.first);
      if (!next_node_id.IsValid() && next_node_id != transform.second) {
        return false;
      }
    }
  }
  nodes_forward_.merge(node_src->nodes_forward_);
  conditionless_transfer_nodes_id.merge(
      node_src->conditionless_transfer_nodes_id);
  return true;
}

inline const NfaGenerator::TailNodeData& NfaGenerator::GetTailNodeData(
    NfaNodeId node_id) {
  auto iter = tail_nodes_.find(node_id);
  if (iter != tail_nodes_.end()) [[likely]] {
    return iter->second;
  } else {
    return kNotTailNodeTag;
  }
}
std::pair<NfaGenerator::NfaNodeId, NfaGenerator::NfaNodeId>
NfaGenerator::RegexConstruct(TailNodeData&& tail_node_data,
                             const std::string& raw_regex_string,
                             size_t&& next_character_index,
                             const bool add_to_nfa_head) {
  NfaNodeId head_id = node_manager_.EmplaceObject();
  NfaNodeId tail_id = head_id;
  NfaNodeId pre_tail_id = head_id;
  if (next_character_index >= raw_regex_string.size()) [[unlikely]] {
    return std::make_pair(head_id, tail_id);
  }
  char c_now;
  while (next_character_index < raw_regex_string.size()) {
    c_now = raw_regex_string[next_character_index];
    ++next_character_index;
    switch (c_now) {
      case '[': {
        auto [temp_head_id, temp_tail_id] =
            CreateSwitchTree(raw_regex_string, &next_character_index);
        if (!(temp_head_id.IsValid() && temp_tail_id.IsValid())) [[unlikely]] {
          LOG_ERROR("NFA Generator",
                    std::format("非法正则 {:}\n{: >{}}", raw_regex_string, '^',
                                next_character_index + 9))
          exit(-1);
        }
        GetNfaNode(tail_id).AddNoconditionTransfer(temp_head_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
      } break;
      case ']':
        // regex_construct函数不应该处理]字符，应交给create_switch_tree处理
        assert(false);
        break;
      case '(': {
        auto [temp_head_id, temp_tail_id] =
            RegexConstruct(std::move(tail_node_data), raw_regex_string,
                           std::move(next_character_index), false);
        if (!(temp_head_id.IsValid() && temp_tail_id.IsValid())) [[unlikely]] {
          LOG_ERROR("NFA Generator",
                    std::format("非法正则 {:}\n{: >{}}", raw_regex_string, '^',
                                next_character_index + 9))
          exit(-1);
        }
        GetNfaNode(tail_id).AddNoconditionTransfer(temp_head_id);
        pre_tail_id = tail_id;
        tail_id = temp_tail_id;
      } break;
      case ')':
        return std::make_pair(head_id, tail_id);
        break;
      case '+':  // 对单个字符和组合结构均生效
        GetNfaNode(tail_id).AddNoconditionTransfer(pre_tail_id);
        break;
      case '*':  // 对单个字符和组合结构均生效
        GetNfaNode(tail_id).AddNoconditionTransfer(pre_tail_id);
        GetNfaNode(pre_tail_id).AddNoconditionTransfer(tail_id);
        break;
      case '?':  // 对单个字符和组合结构均生效
        GetNfaNode(pre_tail_id).AddNoconditionTransfer(tail_id);
        break;
      case '\\':  // 仅对单个字符生效
        if (next_character_index >= raw_regex_string.size()) [[unlikely]] {
          LOG_ERROR("NFA Generator",
                    std::format("非法正则 {:}\n{: >{}}", raw_regex_string, '^',
                                next_character_index + 9))
          exit(-1);
        }
        c_now = raw_regex_string[next_character_index];
        ++next_character_index;
        pre_tail_id = tail_id;
        tail_id = node_manager_.EmplaceObject();
        GetNfaNode(pre_tail_id).SetConditionTransfer(c_now, tail_id);
        break;
      case '.':  // 仅对单个字符生效
        pre_tail_id = tail_id;
        tail_id = node_manager_.EmplaceObject();
        for (char transform_char = CHAR_MIN; transform_char != CHAR_MAX;
             ++transform_char) {
          GetNfaNode(pre_tail_id).SetConditionTransfer(transform_char, tail_id);
        }
        GetNfaNode(pre_tail_id).SetConditionTransfer(CHAR_MAX, tail_id);
        break;
      default:
        pre_tail_id = tail_id;
        tail_id = node_manager_.EmplaceObject();
        GetNfaNode(pre_tail_id).SetConditionTransfer(c_now, tail_id);
        break;
    }
  }
  if (head_id != tail_id) [[likely]] {
    if (add_to_nfa_head) {
      GetNfaNode(head_node_id_).AddNoconditionTransfer(head_id);
      SetTailNode(tail_id, std::move(tail_node_data));
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
  SetTailNode(tail_id, std::move(word_attached_data));
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
      for (auto iter = node_now.GetUnconditionTransferNodesIds().cbegin();
           iter != node_now.GetUnconditionTransferNodesIds().cend();) {
        if (equal_node_unconditional_transfer_node_ids.find(*iter) !=
            equal_node_unconditional_transfer_node_ids.end()) {
          // 当前节点转移表中的项在等价节点中存在
          // 移除该项
          if (*iter != equal_node_id) [[likely]] {
            // 如果存在等价节点的无条件自环节点则不能移除
            // 否则会失去指向等价节点的记录，导致正则不完整、内存泄漏等
            ++iter;
            node_now.RemoveConditionlessTransfer(*iter);
            // continue防止额外前移iter
            continue;
          }
        }
        ++iter;
      }
      // 处理条件转移表
      const auto& equal_node_conditional_transfers =
          equal_node.GetConditionalTransfers();
      for (auto iter = node_now.GetConditionalTransfers().cbegin();
           iter != node_now.GetConditionalTransfers().cend();) {
        auto equal_node_iter =
            equal_node_conditional_transfers.find(iter->first);
        if (equal_node_iter != equal_node_conditional_transfers.cend() &&
            iter->second == equal_node_iter->second) {
          // 当前节点转移表中的项在等价节点中存在
          // 移除该项
          ++iter;
          node_now.RemoveConditionalTransfer(iter->first);
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
    // 检查是否可以将node_now与node_now唯一可无条件转移到的节点合并
    if (node_now.GetConditionalTransfers().empty() &&
        node_now.GetUnconditionTransferNodesIds().size() == 1) [[unlikely]] {
      // 只剩一条无条件转移路径，该节点可以与无条件转移到的节点合并
      NfaNodeId dst_node_id =
          *node_now.GetUnconditionTransferNodesIds().cbegin();
      const NfaGenerator::TailNodeData& dst_tag = GetTailNodeData(dst_node_id);
      const NfaGenerator::TailNodeData& src_tag = GetTailNodeData(id_now);
      if (dst_tag != NfaGenerator::kNotTailNodeTag &&
          src_tag != NfaGenerator::kNotTailNodeTag &&
          dst_tag.second == src_tag.second && dst_tag.first != src_tag.first) {
        LOG_ERROR("NFA Generator",
                  std::format("两个尾节点具有相同优先级且不对应同一个节点"))
        exit(-1);
      }
      bool result = GetNfaNode(dst_node_id).MergeNodes(&GetNfaNode(id_now));
      if (result) {
        if (src_tag != NfaGenerator::kNotTailNodeTag) {
          SetTailNode(dst_node_id, src_tag);
          RemoveTailNode(id_now);
        }
        bool result = node_manager_.MergeObjects(
            dst_node_id, id_now,
            [](NfaNode&, NfaNode&) -> bool { return true; });
        assert(result);
      }

    } else {
      // 没有执行任何操作，不存在从该节点开始的合并操作
      // 设置该节点在合并时不能作为源节点
      node_manager_.SetObjectCanNotBeSourceInMerge(id_now);
    }
  }
}

// @brief 添加尾节点信息

std::pair<NfaGenerator::NfaNodeId, NfaGenerator::NfaNodeId>
NfaGenerator::CreateSwitchTree(const std::string& raw_regex_string,
                               size_t* const next_character_index) {
  NfaNodeId head_id = node_manager_.EmplaceObject();
  NfaNodeId tail_id = node_manager_.EmplaceObject();
  NfaNode& head_node = GetNfaNode(head_id);
  char character_pre;
  // 初始化成不是']'的值从而可以进入循环
  char character_now = '[';
  while (character_now != ']') {
    if (*next_character_index >= raw_regex_string.size()) [[unlikely]] {
      LOG_ERROR("NFA Generator",
                std::format("非法正则 {:}\n{: >{}}", raw_regex_string, '^',
                            *next_character_index + 9))
      exit(-1);
    }
    character_pre = character_now;
    character_now = raw_regex_string[*next_character_index];
    ++*next_character_index;
    switch (character_now) {
      case '-': {
        // 读入另一端的字符
        if (*next_character_index >= raw_regex_string.size()) [[unlikely]] {
          LOG_ERROR("NFA Generator",
                    std::format("非法正则 {:}\n{: >{}}", raw_regex_string, '^',
                                *next_character_index + 9))
          exit(-1);
        }
        character_now = raw_regex_string[*next_character_index];
        ++*next_character_index;
        if (character_now == ']') [[unlikely]] {
          // [+-]这种正则表达式
          break;
        }
        char bigger_character = std::max(character_now, character_pre);
        for (char smaller_character = std::min(character_pre, character_now);
             smaller_character < bigger_character; smaller_character++) {
          head_node.SetConditionTransfer(smaller_character, tail_id);
        }
        // 防止bigger_character == CHAR_MAX导致回环，无法退出上面循环
        head_node.SetConditionTransfer(bigger_character, tail_id);
      } break;
      case '\\':
        if (*next_character_index >= raw_regex_string.size()) [[unlikely]] {
          LOG_ERROR("NFA Generator",
                    std::format("非法正则 {:}\n{: >{}}", raw_regex_string, '^',
                                *next_character_index + 9))
          exit(-1);
        }
        character_now = raw_regex_string[*next_character_index];
        ++*next_character_index;
        head_node.SetConditionTransfer(character_now, tail_id);
        break;
      case ']':
        break;
      default:
        head_node.SetConditionTransfer(character_now, tail_id);
        break;
    }
  }
  if (head_node.GetConditionalTransfers().empty()) [[unlikely]] {
    node_manager_.RemoveObject(head_id);
    LOG_ERROR("NFA Generator",
              std::format("非法正则 {:}\n{: >{}}\n[]中为空", raw_regex_string,
                          '^', *next_character_index + 9))
    exit(-1);
  }
  return std::make_pair(head_id, tail_id);
}

const NfaGenerator::TailNodeData NfaGenerator::kNotTailNodeTag =
    NfaGenerator::TailNodeData(WordAttachedData(), WordPriority::InvalidId());

}  // namespace frontend::generator::dfa_generator::nfa_generator