#include "dfa_generator.h"

#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/queue.hpp>
#include <fstream>
#include <sstream>

namespace frontend::generator::dfa_generator {
using frontend::common::kCharNum;
using frontend::generator::dfa_generator::nfa_generator::NfaGenerator;

void DfaGenerator::DfaInit() {
  dfa_config_.clear();
  root_index_ = TransformArrayId::InvalidId();
  file_end_saved_data_ = WordAttachedData();
  nfa_generator_.NfaInit();
  head_node_intermediate_ = IntermediateNodeId::InvalidId();
  config_node_num = 0;
  intermediate_node_to_final_node_.clear();
  node_manager_intermediate_node_.ObjectManagerInit();
  node_manager_set_.StructManagerInit();
  setid_to_intermediate_nodeid_.clear();
}

bool DfaGenerator::AddWord(const std::string& word,
                           WordAttachedData&& word_attached_data,
                           WordPriority word_priority) {
  auto [head_node_id, tail_node_id] = nfa_generator_.WordConstruct(
      word, TailNodeData(std::move(word_attached_data), word_priority));
  assert(head_node_id.IsValid() && tail_node_id.IsValid());
  return true;
}

bool DfaGenerator::AddRegexpression(const std::string& regex_str,
                                    WordAttachedData&& regex_attached_data,
                                    WordPriority regex_priority) {
  std::stringstream sstream(regex_str);
  auto [head_node_id, tail_node_id] = nfa_generator_.RegexConstruct(
      sstream, TailNodeData(std::move(regex_attached_data), regex_priority));
  assert(head_node_id.IsValid() && tail_node_id.IsValid());
  return true;
}

bool DfaGenerator::DfaConstruct() {
  nfa_generator_.MergeOptimization();
  NfaNodeId nfa_head_handler = nfa_generator_.GetHeadNfaNodeId();
  auto [set_now, tail_data] = nfa_generator_.Closure(nfa_head_handler);
  assert(tail_data == NfaGenerator::NotTailNodeTag);
  IntermediateNodeId intermediate_node_handler_now,
      intermediate_node_handler_temp;
  bool inserted;
  std::tie(intermediate_node_handler_now, inserted) = InOrInsert(set_now);
  assert(inserted);
  std::queue<IntermediateNodeId> q;
  q.push(intermediate_node_handler_now);
  head_node_intermediate_ = intermediate_node_handler_now;
  while (!q.empty()) {
    intermediate_node_handler_now = q.front();
    q.pop();
    SetId set_handler_now =
        GetIntermediateNode(intermediate_node_handler_now).set_handler;
    for (int i = CHAR_MIN; i <= CHAR_MAX; i++) {
      std::tie(intermediate_node_handler_temp, inserted) =
          SetGoto(set_handler_now, static_cast<char>(i));
      if (!intermediate_node_handler_temp.IsValid()) {
        // 该字符下不可转移
        continue;
      }
      GetIntermediateNode(intermediate_node_handler_now)
          .forward_nodes[static_cast<char>(i)] = intermediate_node_handler_temp;
      if (inserted) {
        // 新集合对应的中间节点以前不存在，插入队列等待处理
        q.push(intermediate_node_handler_temp);
      }
    }
  }
  node_manager_set_.StructManagerInit();
  node_manager_set_.ShrinkToFit();
  setid_to_intermediate_nodeid_.clear();
  return true;
}

bool DfaGenerator::DfaMinimize() {
  config_node_num = 0;  // 清零最终有效节点数
  std::vector<IntermediateNodeId> nodes;
  for (auto iter = node_manager_intermediate_node_.Begin();
       iter != node_manager_intermediate_node_.End(); ++iter) {
    nodes.push_back(iter.GetId());
  }
  DfaMinimize(nodes, CHAR_MIN);
  dfa_config_.resize(config_node_num);
  for (auto& p : dfa_config_) {
    p.first.fill(TransformArrayId::InvalidId());
  }
  std::vector<bool> logged_index(config_node_num, false);
  for (auto& p : intermediate_node_to_final_node_) {
    TransformArrayId index = p.second;
    IntermediateDfaNode& intermediate_node = GetIntermediateNode(p.first);
    if (logged_index[index] == false) {
      // 该状态转移表条目未配置
      for (int i = CHAR_MIN; i <= CHAR_MAX; i++) {
        // 寻找有效节点并设置语法分析表中对应项
        // 从最小的char开始
        IntermediateNodeId next_node_id =
            intermediate_node.forward_nodes[static_cast<char>(i)];
        if (next_node_id.IsValid()) {
          auto iter = intermediate_node_to_final_node_.find(next_node_id);
          assert(iter != intermediate_node_to_final_node_.end());
          dfa_config_[index].first[static_cast<char>(i)] = iter->second;
          dfa_config_[index].second = intermediate_node.tail_node_data.first;
        }
      }
      logged_index[index] = true;
    }
  }
  root_index_ =
      intermediate_node_to_final_node_.find(head_node_intermediate_)->second;
  head_node_intermediate_ = IntermediateNodeId::InvalidId();
  node_manager_intermediate_node_.ObjectManagerInit();
  node_manager_intermediate_node_.ShrinkToFit();
  intermediate_node_to_final_node_.clear();
  return true;
}

std::pair<DfaGenerator::IntermediateNodeId, bool> DfaGenerator::SetGoto(
    SetId set_src, char c_transform) {
  SetType set;
  TailNodeData tail_data(NfaGenerator::NotTailNodeTag);
  for (auto x : GetSetObject(set_src)) {
    auto [set_temp, tail_data_temp] = nfa_generator_.Goto(x, c_transform);
    if (set_temp.size() == 0) {
      continue;
    }
    set.merge(std::move(set_temp));
    // 不存在尾节点标记或新的标记优先级大于原来的标记则修改
    if (tail_data_temp != NfaGenerator::NotTailNodeTag) {
      if (tail_data == NfaGenerator::NotTailNodeTag ||
          tail_data_temp.second > tail_data.second) {
        tail_data = tail_data_temp;
      }
    }
  }
  if (set.size() == 0) {
    return std::make_pair(IntermediateNodeId::InvalidId(), false);
  }
  return InOrInsert(set, tail_data);
}

inline DfaGenerator::IntermediateNodeId DfaGenerator::IntermediateGoto(
    IntermediateNodeId handler_src, char c_transform) {
  return GetIntermediateNode(handler_src).forward_nodes[c_transform];
}

inline bool DfaGenerator::SetIntermediateNodeTransform(
    IntermediateNodeId node_intermediate_src, char c_transform,
    IntermediateNodeId node_intermediate_dst) {
  IntermediateDfaNode& node_src = GetIntermediateNode(node_intermediate_src);
  node_src.forward_nodes[c_transform] = node_intermediate_dst;
  return true;
}

std::pair<DfaGenerator::IntermediateNodeId, bool> DfaGenerator::InOrInsert(
    const SetType& uset, TailNodeData tail_node_data) {
  auto [setid, inserted] = node_manager_set_.AddObject(uset);
  IntermediateNodeId intermediate_node_id = IntermediateNodeId::InvalidId();
  if (inserted) {
    intermediate_node_id =
        node_manager_intermediate_node_.EmplaceObject(setid, tail_node_data);
    setid_to_intermediate_nodeid_.insert(
        std::make_pair(setid, intermediate_node_id));
  } else {
    auto iter = setid_to_intermediate_nodeid_.find(setid);
    assert(iter != setid_to_intermediate_nodeid_.end());
    intermediate_node_id = iter->second;
  }
  return std::make_pair(intermediate_node_id, inserted);
}

bool DfaGenerator::DfaMinimize(const std::vector<IntermediateNodeId>& handlers,
                               char c_transform) {
  // 存储不同分组，键值为当前转移条件下转移到的节点句柄
  std::map<std::pair<IntermediateNodeId, TailNodeData>,
           std::vector<IntermediateNodeId>>
      groups;
  // 存储当前转移条件下无法转移节点
  std::map<TailNodeData, std::vector<IntermediateNodeId>> no_next_group;
  for (auto handler : handlers) {
    IntermediateNodeId next_node_id = IntermediateGoto(handler, c_transform);
    IntermediateDfaNode& node = GetIntermediateNode(handler);
    if (next_node_id.IsValid()) {
      groups[std::make_pair(next_node_id, node.tail_node_data)].push_back(
          handler);
    } else {
      no_next_group[node.tail_node_data].push_back(handler);
    }
  }
  DfaMinimizeGroupsRecursion(groups, c_transform);
  DfaMinimizeGroupsRecursion(no_next_group, c_transform);
  return true;
}
void DfaGenerator::SaveConfig() const {
  std::ofstream ofile(frontend::common::kDfaConfigFileName);
  boost::archive::binary_oarchive oarchive(ofile);
  oarchive << *this;
}

}  // namespace frontend::generator::dfa_generator