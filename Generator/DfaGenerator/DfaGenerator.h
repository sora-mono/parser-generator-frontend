#pragma once

#include <array>

#include "NfaGenerator/NfaGenerator.h"
#include "common.h"
#include<map>

class DfaGenerator {
  struct IntermediateDfaNode;

 public:
  using TailNodeTag = NfaGenerator::TailNodeData::first_type;
  using PriorityTag = NfaGenerator::TailNodeData::second_type;
  using TailNodeData = NfaGenerator::TailNodeData;
  using NfaNodeHandler = NfaGenerator::NodeHandler;
  using SetType = std::unordered_set<NfaNodeHandler>;
  using IntermediateNodeHandler = NodeManager<IntermediateDfaNode>::NodeHandler;
  using SetNodeHandler = NodeManager<SetType>::NodeHandler;

  using TransformArray = std::array<size_t,kchar_num>;

  //DFA配置类型
  using DfaConfigType =
      std::vector<std::pair<TransformArray, TailNodeTag>>;

  DfaGenerator() :head_node_intermediate_(-1) {}
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
    IntermediateDfaNode(SetNodeHandler handler = -1,
                        TailNodeData data = NfaGenerator::NotTailNodeTag)
        : tail_node_data(data), set_handler(handler) {
      SetAllUntransable();
    }

    //设置所有条件均不可转移
    void SetAllUntransable() { forward_nodes.fill(-1); }
    SetNodeHandler GetSetHandler() { return set_handler; }
    void SetTailNodeData(TailNodeData data) { tail_node_data = data; }

    //该节点的转移条件，存储IntermediateDfaNode节点句柄
    std::array<IntermediateNodeHandler, kchar_num> forward_nodes;
    TailNodeData tail_node_data;
    SetNodeHandler set_handler;  //该节点对应的子集闭包
  };
  IntermediateDfaNode* GetIntermediateNode(IntermediateNodeHandler handler) {
    return node_manager_intermediate_node_.GetNode(handler);
  }
  SetType* GetSetNode(SetNodeHandler handler) {
    return node_manager_set_.GetNode(handler);
  }
  //集合转移函数（子集构造法用），
  //返回值前半部分表示新集合是否已存在，后半部分为对应中间节点句柄
  std::pair<bool, IntermediateNodeHandler> SetGoto(SetNodeHandler set_src,
                                                   char c_transform);
  // DFA转移函数（最小化DFA用）
  IntermediateNodeHandler IntermediateGoto(IntermediateNodeHandler dfa_src, char c_transform);
  //设置DFA中间节点条件转移
  bool SetIntermediateNodeTransform(
      IntermediateNodeHandler node_intermediate_src, char c_transform,
      IntermediateNodeHandler node_intermediate_dst);
  //如果集合已存在则返回true，如果不存在则插入并返回false
  //返回的第一个参数为集合是否已存在
  //返回的第二个参数为对应中间节点句柄
  std::pair<bool, IntermediateNodeHandler> InOrInsert(
      const SetType& uset,
      TailNodeData tail_node_data = NfaGenerator::NotTailNodeTag);

  //对handlers数组中的句柄作最小化处理，从c_transform转移条件开始
  bool DfaMinimize(const std::vector<IntermediateNodeHandler>& handlers,
                   char c_transform);
  template<class T>
  bool DfaMinimizeGroupsRecursion(
      std::map<T, std::vector<IntermediateNodeHandler>> groups);

  //序列化保存DFA配置用
  template <class Archive>
  void Serialize(Archive& ar, const unsigned int version = 0);

  //DFA配置
  DfaConfigType dfa_config;
  //头结点序号
  size_t head_index;

  NfaGenerator nfa_generator_;
  //中间节点头结点句柄
  IntermediateNodeHandler head_node_intermediate_;
  //最终有效节点数
  size_t config_node_num;

  //存储DFA中间节点到最终标号的映射
  std::unordered_map<IntermediateNodeHandler, size_t>
      intermediate_node_to_final_node_;
  //存储中间节点
  NodeManager<IntermediateDfaNode> node_manager_intermediate_node_;
  //存储集合（子集构造法得到的）
  NodeManager<SetType> node_manager_set_;
  //存储集合的哈希到节点句柄的映射
  std::unordered_multimap<IntergalSetHashType, IntermediateNodeHandler>
      set_hash_to_intermediate_node_handler_;
};

template <class T>
inline bool DfaGenerator::DfaMinimizeGroupsRecursion(
    std::map<T, std::vector<IntermediateNodeHandler>> groups) {
  for (auto& p : groups) {
    std::vector<IntermediateNodeHandler>& vec = p.second;
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
        node_manager_intermediate_node_.RemoveNode(vec[i]);
      }
      ++config_node_num;
    }
  }
  return false;
}
