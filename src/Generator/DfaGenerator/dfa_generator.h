#pragma once

#include <array>
#include <map>

#include "Common/common.h"
#include "Common/unordered_struct_manager.h"
#include "NfaGenerator/nfa_generator.h"

#ifndef GENERATOR_DFAGENERATOR_DFAGENERATOR_H_
#define GENERATOR_DFAGENERATOR_DFAGENERATOR_H_

namespace frontend::generator::dfagenerator {
using frontend::common::ObjectManager;
using frontend::generator::dfagenerator::nfagenerator::NfaGenerator;
namespace common = frontend::common;

class DfaGenerator {
  struct IntermediateDfaNode;

 public:
  using TailNodeTag = NfaGenerator::TailNodeData::first_type;
  using PriorityTag = NfaGenerator::TailNodeData::second_type;
  using TailNodeData = NfaGenerator::TailNodeData;
  using NfaNodeId = NfaGenerator::ObjectId;
  using SetType = std::unordered_set<NfaNodeId>;
  using IntermediateNodeId = ObjectManager<IntermediateDfaNode>::ObjectId;

  struct IntergalSetHasher {
    common::IntergalSetHashType DoHash(
        const std::unordered_set<NfaNodeId>& set) {
      return common::HashIntergalUnorderedSet(set);
    }
  };

  using SetManagerType =
      common::UnorderedStructManager<SetType, IntergalSetHasher>;
  using SetId = SetManagerType::ObjectId;
  using TransformArray = std::array<size_t, common::kCharNum>;
  // DFA配置类型
  using DfaConfigType = std::vector<std::pair<TransformArray, TailNodeTag>>;

  DfaGenerator() : head_node_intermediate_(-1) {}
  DfaGenerator(const DfaGenerator&) = delete;
  DfaGenerator(DfaGenerator&&) = delete;

  //添加关键字
  bool AddKeyword(const std::string& str, TailNodeTag tail_node_tag,
                  PriorityTag priority_tag);
  //添加正则
  bool AddRegexpression(const std::string& str, TailNodeTag tail_node_tag,
                        PriorityTag priority_tag);
  //构建DFA
  bool DfaConstruct();
  //构建最小化DFA
  bool DfaMinimize();
  //重新进行完整构建过程
  bool DfaReconstrcut() { return DfaConstruct() && DfaMinimize(); }
  //将配置写入Config/DFA Config文件夹下DFA.conf
  bool ConfigConstruct();

 private:
  struct IntermediateDfaNode {
    IntermediateDfaNode(SetId handler = -1,
                        TailNodeData data = NfaGenerator::NotTailNodeTag)
        : tail_node_data(data), set_handler(handler) {
      SetAllUntransable();
    }

    //设置所有条件均不可转移
    void SetAllUntransable() { forward_nodes.fill(-1); }
    SetId GetSetHandler() { return set_handler; }
    void SetTailNodeData(TailNodeData data) { tail_node_data = data; }

    //该节点的转移条件，存储IntermediateDfaNode节点句柄
    std::array<IntermediateNodeId, common::kCharNum> forward_nodes;
    TailNodeData tail_node_data;
    SetId set_handler;  //该节点对应的子集闭包
  };
  IntermediateDfaNode& GetIntermediateNode(IntermediateNodeId handler) {
    return node_manager_intermediate_node_.GetObject(handler);
  }
  SetType& GetSetObject(SetId id) { return node_manager_set_.GetObject(id); }
  //集合转移函数（子集构造法用），
  //返回值前半部分表示新集合是否已存在，后半部分为对应中间节点句柄
  std::pair<bool, IntermediateNodeId> SetGoto(SetId set_src, char c_transform);
  // DFA转移函数（最小化DFA用）
  IntermediateNodeId IntermediateGoto(IntermediateNodeId dfa_src,
                                      char c_transform);
  //设置DFA中间节点条件转移
  bool SetIntermediateNodeTransform(IntermediateNodeId node_intermediate_src,
                                    char c_transform,
                                    IntermediateNodeId node_intermediate_dst);
  //如果集合已存在则返回true，如果不存在则插入并返回false
  //返回的第一个参数为集合是否已存在
  //返回的第二个参数为对应中间节点句柄
  std::pair<bool, IntermediateNodeId> InOrInsert(
      const SetType& uset,
      TailNodeData tail_node_data = NfaGenerator::NotTailNodeTag);

  //对handlers数组中的句柄作最小化处理，从c_transform转移条件开始
  bool DfaMinimize(const std::vector<IntermediateNodeId>& handlers,
                   char c_transform);
  template <class T>
  bool DfaMinimizeGroupsRecursion(
      std::map<T, std::vector<IntermediateNodeId>> groups, char c_transform);

  //序列化保存DFA配置用
  template <class Archive>
  void Serialize(Archive& ar, const unsigned int version = 0);

  // DFA配置
  DfaConfigType dfa_config;
  //头结点序号
  size_t head_index;

  NfaGenerator nfa_generator_;
  //中间节点头结点句柄
  IntermediateNodeId head_node_intermediate_;
  //最终有效节点数
  size_t config_node_num;

  //存储DFA中间节点到最终标号的映射
  std::unordered_map<IntermediateNodeId, size_t>
      intermediate_node_to_final_node_;
  //存储中间节点
  ObjectManager<IntermediateDfaNode> node_manager_intermediate_node_;
  //存储集合的哈希到节点句柄的映射
  SetManagerType node_manager_set_;
  std::unordered_map<SetId, IntermediateNodeId> setid_to_intermediate_nodeid_;
};

template <class T>
inline bool DfaGenerator::DfaMinimizeGroupsRecursion(
    std::map<T, std::vector<IntermediateNodeId>> groups, char c_transform) {
  for (auto& p : groups) {
    std::vector<IntermediateNodeId>& vec = p.second;
    if (vec.size() == 1) {
      intermediate_node_to_final_node_.insert(
          std::make_pair(vec.front(), config_node_num));
      ++config_node_num;
      continue;
    }
    if (c_transform != CHAR_MAX) {
      DfaMinimize(vec, c_transform + 1);
    } else {
      intermediate_node_to_final_node_.insert(
          std::make_pair(vec[0], config_node_num));
      for (size_t i = 1; i < vec.size(); i++) {
        intermediate_node_to_final_node_.insert(
            std::make_pair(vec[i], config_node_num));
        node_manager_intermediate_node_.RemoveObject(vec[i]);
      }
      ++config_node_num;
    }
  }
  return true;
}

}  // namespace frontend::generator::dfagenerator
#endif  // !GENERATOR_DFAGENERATOR_DFAGENERATOR_H_
