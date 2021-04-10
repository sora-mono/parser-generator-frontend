#include "DfaGenerator.h"

#include <queue>

bool DfaGenerator::AddKeyword(const std::string& str, TailNodeTag tail_node_tag,
                              PriorityTag priority_tag) {
  TailNodeData result = nfa_generator_.WordConstruct(
      str, TailNodeData(tail_node_tag, priority_tag));
  if (result == TailNodeData(-1, -1)) {
    return false;
  }
  return true;
}

bool DfaGenerator::AddRegexpression(const std::string& str,
                                    TailNodeTag tail_node_tag,
                                    PriorityTag priority_tag) {
  TailNodeData result = nfa_generator_.WordConstruct(
      str, TailNodeData(tail_node_tag, priority_tag));
  if (result == TailNodeData(-1, -1)) {
    return false;
  }
  return true;
}

bool DfaGenerator::SetIntermediateNodeTransform(
    IntermediateNodeHandler node_intermediate_src, char c_transform,
    IntermediateNodeHandler node_intermediate_dst) {
  return false;
}

std::pair<bool, DfaGenerator::DfaNodeHandler> DfaGenerator::InOrInsert(
    const std::unordered_set<DfaGenerator::NfaNodeHandler>& uset) {
  IntergalSetHashType set_hash_result = HashIntergalSet(uset);
  auto iter =
      set_hash_to_intermediate_node_handler_.equal_range(set_hash_result);
  if (iter.first != set_hash_to_intermediate_node_handler_.end() &&
      iter.first->first == set_hash_result) {
    IntermediateNodeHandler intermediate_node_handler = -1;
    for (auto iter_now = iter.first; iter_now != iter.second; iter_now++) {
      if (*GetSetNode(GetIntermediateNode(iter_now->second)->set_handler) ==
          uset) {
        intermediate_node_handler = iter_now->second;
        break;
      }
    }
    if (intermediate_node_handler != -1) {
      return std::make_pair(true, intermediate_node_handler);
    }
  }
  //该集合不存在已有集合中
  SetNodeHandler set_handler = node_manager_set_.EmplaceNode(uset);
  IntermediateNodeHandler intermediate_node_handler =
      node_manager_intermediate_node_.EmplaceNode(set_handler);
  set_hash_to_intermediate_node_handler_.insert(
      std::make_pair(set_hash_result, intermediate_node_handler));
  return std::make_pair(false, intermediate_node_handler);
}
