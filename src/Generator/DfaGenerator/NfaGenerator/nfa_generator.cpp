#include "nfa_generator.h"

#include <format>
#include <queue>

namespace frontend::generator::dfa_generator::nfa_generator {

NfaGenerator::NfaNodeId NfaGenerator::NfaNode::GetForwardNodesId(
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

inline void NfaGenerator::NfaNode::RemoveConditionalTransfer(char c_treasfer) {
  auto iter = nodes_forward_.find(c_treasfer);
  if (iter != nodes_forward_.end()) {
    nodes_forward_.erase(iter);
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
  if (nodes_forward_.size() != 0 && node_src.nodes_forward_.size() != 0) {
    bool CanBeSourceInMerge = true;
    for (auto& p : node_src.nodes_forward_) {
      auto iter = nodes_forward_.find(p.first);
      if (iter != nodes_forward_.end() && iter->second != p.second) {
        CanBeSourceInMerge = false;
        break;
      }
    }
    if (!CanBeSourceInMerge) {
      return false;
    }
  }
  nodes_forward_.merge(node_src.nodes_forward_);
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
std::pair<NfaGenerator::NfaNodeId, NfaGenerator::NfaNodeId>
NfaGenerator::RegexConstruct(const TailNodeData& tail_node_data,
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
          std::cerr << std::format("非法正则 {:}\n{: >{}}", raw_regex_string,
                                   '^', next_character_index + 9)
                    << std::endl;
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
            RegexConstruct(NotTailNodeTag, raw_regex_string,
                           std::move(next_character_index), false);
        if (!(temp_head_id.IsValid() && temp_tail_id.IsValid())) [[unlikely]] {
          std::cerr << std::format("非法正则 {:}\n{: >{}}", raw_regex_string,
                                   '^', next_character_index + 9)
                    << std::endl;
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
          std::cerr << std::format("非法正则 {:}\n{: >{}}", raw_regex_string,
                                   '^', next_character_index + 9)
                    << std::endl;
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
      AddTailNode(tail_id, tail_node_data);
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
NfaGenerator::CreateSwitchTree(const std::string& raw_regex_string,
                               size_t* next_character_index) {
  NfaNodeId head_id = node_manager_.EmplaceObject();
  NfaNodeId tail_id = node_manager_.EmplaceObject();
  NfaNode& head_node = GetNfaNode(head_id);
  // 初始化成不是']'的值就可以，从而可以进入循环
  char character_pre;
  char character_now = '[';
  while (character_now != ']') {
    if (*next_character_index >= raw_regex_string.size()) [[unlikely]] {
      std::cerr << std::format("非法正则 {:}\n{: >{}}", raw_regex_string, '^',
                               *next_character_index + 9)
                << std::endl;
      exit(-1);
    }
    character_pre = character_now;
    character_now = raw_regex_string[*next_character_index];
    ++*next_character_index;
    switch (character_now) {
      case '-': {
        // 读入另一端的字符
        if (*next_character_index >= raw_regex_string.size()) [[unlikely]] {
          std::cerr << std::format("非法正则 {:}\n{: >{}}", raw_regex_string,
                                   '^', *next_character_index + 9)
                    << std::endl;
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
          std::cerr << std::format("非法正则 {:}\n{: >{}}", raw_regex_string,
                                   '^', *next_character_index + 9)
                    << std::endl;
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
    std::cerr << std::format("非法正则 {:}\n{: >{}}\n[]中为空",
                             raw_regex_string, '^', *next_character_index + 9)
              << std::endl;
    exit(-1);
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

}  // namespace frontend::generator::dfa_generator::nfa_generator