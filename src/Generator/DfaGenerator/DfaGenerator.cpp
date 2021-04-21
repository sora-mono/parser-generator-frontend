#include "Generator/DfaGenerator/DfaGenerator.h"

#include <queue>
#include <sstream>

namespace frontend::generator::dfagenerator {
using frontend::common::kCharNum;
using frontend::generator::dfagenerator::nfagenerator::NfaGenerator;

bool DfaGenerator::AddKeyword(const std::string& str, TailNodeTag tail_node_tag,
                              PriorityTag priority_tag) {
  TailNodeData result = nfa_generator_.WordConstruct(
      str, TailNodeData(tail_node_tag, priority_tag));
  if (result == NfaGenerator::NotTailNodeTag) {
    return false;
  }
  return true;
}

bool DfaGenerator::AddRegexpression(const std::string& str,
                                    TailNodeTag tail_node_tag,
                                    PriorityTag priority_tag) {
  std::stringstream sstream(str);
  TailNodeData result = nfa_generator_.RegexConstruct(
      sstream, TailNodeData(tail_node_tag, priority_tag));
  if (result == NfaGenerator::NotTailNodeTag) {
    return false;
  }
  return true;
}

bool DfaGenerator::DfaConstruct() {
  NfaNodeId nfa_head_handler = nfa_generator_.GetHeadNodeId();
  SetType set_now;
  TailNodeData tail_data;
  std::pair(set_now, tail_data) = nfa_generator_.Closure(nfa_head_handler);
  assert(tail_data == NfaGenerator::NotTailNodeTag);
  IntermediateNodeId intermediate_node_handler_now,
      intermediate_node_handler_temp;
  bool result;
  std::pair(result, intermediate_node_handler_now) = InOrInsert(set_now);
  assert(result == false);
  std::queue<IntermediateNodeId> q;
  q.push(intermediate_node_handler_now);
  head_node_intermediate_ = intermediate_node_handler_now;
  while (!q.empty()) {
    intermediate_node_handler_now = q.front();
    q.pop();
    SetId set_handler_now =
        GetIntermediateNode(intermediate_node_handler_now)->set_handler;
    for (size_t i = 0; i < kCharNum; i++) {
      std::pair(result, intermediate_node_handler_temp) =
          SetGoto(set_handler_now, char(i + CHAR_MIN));
      if (intermediate_node_handler_temp == -1) {
        //该字符下不可转移
        continue;
      }
      GetIntermediateNode(intermediate_node_handler_now)->forward_nodes[i] =
          intermediate_node_handler_temp;
      if (result == false) {  //如果新集合以前不存在则插入队列等待处理
        q.push(
            GetIntermediateNode(intermediate_node_handler_temp)->set_handler);
      }
    }
  }
  node_manager_set_.Clear();
  node_manager_set_.ShrinkToFit();
  setid_to_intermediate_nodeid_.clear();
  return true;
}

bool DfaGenerator::DfaMinimize() {
  config_node_num = 0;  //清零最终有效节点数
  std::vector<IntermediateNodeId> nodes;
  for (auto iter = node_manager_intermediate_node_.Begin();
       iter != node_manager_intermediate_node_.End(); ++iter) {
    nodes.push_back(iter.GetId());
  }
  DfaMinimize(nodes, CHAR_MIN);
  dfa_config.resize(config_node_num);
  for (auto& p : dfa_config) {
    p.first.fill(-1);
    p.second = -1;
  }
  std::vector<bool> logged_index(config_node_num, false);
  for (auto& p : intermediate_node_to_final_node_) {
    size_t index = p.second;
    IntermediateDfaNode* intermediate_node = GetIntermediateNode(p.first);
    assert(intermediate_node != nullptr);
    if (logged_index[index] == false) {
      //该节点未配置
      for (size_t i = 0; i < kCharNum; i++) {
        if (intermediate_node->forward_nodes[i] != -1) {
          auto iter = intermediate_node_to_final_node_.find(
              intermediate_node->forward_nodes[i]);
          assert(iter != intermediate_node_to_final_node_.end());
          dfa_config[index].first[i] = iter->second;
          dfa_config[index].second = intermediate_node->tail_node_data.first;
        }
      }
      logged_index[index] = true;
    }
  }
  head_index =
      intermediate_node_to_final_node_.find(head_node_intermediate_)->second;
  head_node_intermediate_ = -1;
  node_manager_intermediate_node_.Clear();
  node_manager_intermediate_node_.ShrinkToFit();
  intermediate_node_to_final_node_.clear();
  return true;
}

std::pair<bool, DfaGenerator::IntermediateNodeId> DfaGenerator::SetGoto(
    SetId set_src, char c_transform) {
  SetType set;
  TailNodeData tail_data(-1, -1);
  for (auto x : *GetSetNode(set_src)) {
    auto [set_temp, tail_data_temp] = nfa_generator_.Goto(x, c_transform);
    if (set_temp.size() == 0) {
      continue;
    }
    set.merge(std::move(set_temp));
    //不存在尾节点标记或新的标记优先级大于原来的标记则修改
    if (tail_data_temp != NfaGenerator::NotTailNodeTag) {
      if (tail_data == NfaGenerator::NotTailNodeTag ||
          tail_data_temp.second > tail_data.second) {
        tail_data = tail_data_temp;
      }
    }
  }
  if (set.size() == 0) {
    return std::pair(false, -1);
  }
  return InOrInsert(set, tail_data);
}

inline DfaGenerator::IntermediateNodeId DfaGenerator::IntermediateGoto(
    IntermediateNodeId handler_src, char c_transform) {
  return GetIntermediateNode(handler_src)
      ->forward_nodes[size_t(c_transform + kCharNum) % kCharNum];
}

inline bool DfaGenerator::SetIntermediateNodeTransform(
    IntermediateNodeId node_intermediate_src, char c_transform,
    IntermediateNodeId node_intermediate_dst) {
  IntermediateDfaNode* node_src = GetIntermediateNode(node_intermediate_src);
  node_src->forward_nodes[size_t(c_transform + kCharNum) % kCharNum] =
      node_intermediate_dst;
  return true;
}

std::pair<bool, DfaGenerator::IntermediateNodeId> DfaGenerator::InOrInsert(
    const SetType& uset, TailNodeData tail_node_data) {
  auto [set_already_exist, setid] = node_manager_set_.AddObject(uset);
  if (set_already_exist) {
    auto iter = setid_to_intermediate_nodeid_.find(setid);
    assert(iter != setid_to_intermediate_nodeid_.end());
    return std::make_pair(true, iter->second);
  } else {
    IntermediateNodeId intermediate_nodeid =
        node_manager_intermediate_node_.EmplaceNode(setid, tail_node_data);
    setid_to_intermediate_nodeid_.insert(
        std::make_pair(setid, intermediate_nodeid));
    return std::make_pair(false, intermediate_nodeid);
  }
}

bool DfaGenerator::DfaMinimize(const std::vector<IntermediateNodeId>& handlers,
                               char c_transform) {
  //存储不同分组，键值为当前转移条件下转移到的节点句柄
  std::map<std::pair<IntermediateNodeId, TailNodeData>,
           std::vector<IntermediateNodeId>>
      groups;
  //存储当前转移条件下无法转移节点
  std::map<TailNodeData, std::vector<IntermediateNodeId>> no_next_group;
  for (auto h : handlers) {
    IntermediateNodeId next_node_handler = IntermediateGoto(h, c_transform);
    IntermediateDfaNode* node = GetIntermediateNode(h);
    assert(node != nullptr);
    if (next_node_handler == -1) {
      no_next_group[node->tail_node_data].push_back(h);
    } else {
      groups[std::make_pair(next_node_handler, node->tail_node_data)].push_back(
          h);
    }
  }
  DfaMinimizeGroupsRecursion(groups, c_transform);
  DfaMinimizeGroupsRecursion(no_next_group, c_transform);
  return true;
}
}  // namespace frontend::generator::dfagenerator