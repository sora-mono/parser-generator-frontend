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
  root_transform_array_id_ = TransformArrayId::InvalidId();
  file_end_saved_data_ = WordAttachedData();
  nfa_generator_.NfaInit();
  head_node_intermediate_ = IntermediateNodeId::InvalidId();
  transform_array_size = 0;
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
  auto [head_node_id, tail_node_id] = nfa_generator_.RegexConstruct(
      TailNodeData(std::move(regex_attached_data), regex_priority), regex_str);
  assert(head_node_id.IsValid() && tail_node_id.IsValid());
  return true;
}

bool DfaGenerator::DfaConstruct() {
  nfa_generator_.MergeOptimization();
  NfaNodeId nfa_head_handler = nfa_generator_.GetHeadNfaNodeId();
  auto&& [set_now, tail_data] = nfa_generator_.Closure(nfa_head_handler);
  assert(tail_data == NfaGenerator::NotTailNodeTag);
  auto [root_itermediate_node_id, inserted] = InOrInsert(std::move(set_now));
  assert(inserted);
  std::queue<IntermediateNodeId> q;
  q.push(root_itermediate_node_id);
  head_node_intermediate_ = root_itermediate_node_id;
  while (!q.empty()) {
    IntermediateNodeId intermediate_node_handler_now = q.front();
    q.pop();
    SetId set_handler_now =
        GetIntermediateNode(intermediate_node_handler_now).set_handler;
    for (int i = CHAR_MIN; i <= CHAR_MAX; i++) {
      auto [transformed_intermediate_node_id, inserted] =
          SetGoto(set_handler_now, static_cast<char>(i));
      if (!transformed_intermediate_node_id.IsValid()) {
        // 该字符下不可转移
        continue;
      }
      // 设置转移条件
      GetIntermediateNode(intermediate_node_handler_now)
          .forward_nodes[static_cast<char>(i)] =
          transformed_intermediate_node_id;
      if (inserted) {
        // 新集合对应的中间节点以前不存在，插入队列等待处理
        q.push(transformed_intermediate_node_id);
      }
    }
  }
  node_manager_set_.StructManagerInit();
  node_manager_set_.ShrinkToFit();
  setid_to_intermediate_nodeid_.clear();
  return true;
}

bool DfaGenerator::DfaMinimize() {
  transform_array_size = 0;  // 清零最终有效节点数
  std::list<IntermediateNodeId> nodes;
  for (auto iter = node_manager_intermediate_node_.Begin();
       iter != node_manager_intermediate_node_.End(); ++iter) {
    nodes.push_back(iter.GetId());
  }
  SubDfaMinimize(std::move(nodes));
  dfa_config_.resize(transform_array_size);
  for (auto& p : dfa_config_) {
    // 填充整个转移表为无法转移状态
    p.first.fill(TransformArrayId::InvalidId());
  }
  // 标记转移表条目是否完成构建
  std::vector<bool> logged_index(transform_array_size, false);
  for (auto& p : intermediate_node_to_final_node_) {
    TransformArrayId index = p.second;
    IntermediateDfaNode& intermediate_node = GetIntermediateNode(p.first);
    if (logged_index[index] == false) {
      // 该状态转移表条目未配置
      dfa_config_[index].second = intermediate_node.word_attached_data;
      for (int i = CHAR_MIN; i <= CHAR_MAX; i++) {
        // 寻找有效节点并设置语法分析表中对应项
        // 从最小的char开始
        IntermediateNodeId next_node_id =
            intermediate_node.forward_nodes[static_cast<char>(i)];
        if (next_node_id.IsValid()) [[unlikely]] {
          // 该条件下可以转移，查询转移到的中间节点对应转移表条目ID
          auto iter = intermediate_node_to_final_node_.find(next_node_id);
          assert(iter != intermediate_node_to_final_node_.end());
          dfa_config_[index].first[static_cast<char>(i)] = iter->second;
        }
      }
      logged_index[index] = true;
    }
  }
#ifdef _DEBUG
  // 检查每个转移表条目都被填写过
  for (bool logged : logged_index) {
    assert(logged);
  }
#endif  // _DEBUG
  root_transform_array_id_ =
      intermediate_node_to_final_node_.find(head_node_intermediate_)->second;
  assert(root_transform_array_id_.IsValid());
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
    if (set_temp.empty()) [[likely]] {
      continue;
    }
    set.merge(std::move(set_temp));
    // 不存在尾节点标记或新的标记优先级大于原来的标记则修改
    if (tail_data_temp != NfaGenerator::NotTailNodeTag) [[unlikely]] {
      if (tail_data == NfaGenerator::NotTailNodeTag ||
          tail_data_temp.second > tail_data.second) {
        tail_data = tail_data_temp;
      }
    }
  }
  if (set.empty()) {
    return std::make_pair(IntermediateNodeId::InvalidId(), false);
  } else {
    return InOrInsert(std::move(set), std::move(tail_data.first));
  }
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

void DfaGenerator::SubDfaMinimize(std::list<IntermediateNodeId>&& handlers,
                                  char c_transform) {
  // 存储不同分组，键值为当前转移条件下转移到的节点句柄
  // 无法转移的节点键值前半部分使用IntermediateNodeId::InvalidId()
  std::unordered_map<std::pair<IntermediateNodeId, WordAttachedData>,
                     std::list<IntermediateNodeId>,
                     PairOfIntermediateNodeIdAndWordAttachedDataHasher>
      transform_node_ids;
  for (auto handler : handlers) {
    IntermediateNodeId next_node_id = IntermediateGoto(handler, c_transform);
    IntermediateDfaNode& node = GetIntermediateNode(handler);
    transform_node_ids[std::make_pair(next_node_id, node.word_attached_data)]
        .push_back(handler);
  }
  if (c_transform == CHAR_MAX) [[unlikely]] {
    // 最后一个转移条件已经判断，应填写中间节点到转移表条目的映射表
    for (auto& group : transform_node_ids) {
      for (auto& intermediate_node_id : group.second) {
        // 当前具有的转移表条目数为新建条目的下标
        intermediate_node_to_final_node_.emplace(intermediate_node_id,
                                                 transform_array_size);
      }
      ++transform_array_size;
    }
  } else {
    // 未达到最后条件（CHAR_MAX），应继续对数目大于1的组分类
    // 修改转移条件
    ++c_transform;
    for (auto& group : transform_node_ids) {
      if (group.second.size() > 1) {
        // 组内成员大于1个，需要继续分类
        SubDfaMinimize(std::move(group.second), c_transform);
      } else {
        // 组内只有一个成员，直接填写中间节点到转移表条目的映射表
        intermediate_node_to_final_node_.emplace(group.second.front(),
                                                 transform_array_size);
        ++transform_array_size;
      }
    }
  }
}

void DfaGenerator::SaveConfig() const {
  std::ofstream ofile(frontend::common::kDfaConfigFileName,
                      std::ios_base::binary | std::ios_base::out);
  // oarchive要在config_file析构前析构，否则文件不完整在反序列化时会抛异常
  {
    boost::archive::binary_oarchive oarchive(ofile);
    oarchive << *this;
  }
}

}  // namespace frontend::generator::dfa_generator