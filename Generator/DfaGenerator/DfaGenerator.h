#pragma once

#include <array>

#include "NfaGenerator/NfaGenerator.h"
#include "common.h"

class DfaGenerator {
  struct DfaNode;
  struct IntermediateDfaNode;
 public:
  using TailNodeTag = NfaGenerator::TailNodeData::first_type;
  using PriorityTag = NfaGenerator::TailNodeData::second_type;
  using TailNodeData = NfaGenerator::TailNodeData;
  using NfaNodeHandler = NfaGenerator::NodeHandler;
  using SetType = std::unordered_set<NfaNodeHandler>;
  using DfaNodeHandler = NodeManager<DfaNode>::NodeHandler;
  using IntermediateNodeHandler = NodeManager<IntermediateDfaNode>::NodeHandler;
  using SetNodeHandler = NodeManager<SetType>::NodeHandler;

  DfaGenerator() : head_node_(-1) {}
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
  //将配置写入Config/DFA Config文件夹下DFA.conf
  bool ConfigConstruct();

 private:
  struct IntermediateDfaNode {
    //该节点的转移条件，存储IntermediateDfaNode节点句柄
    std::array<IntermediateNodeHandler, kchar_num> forward_nodes;
    SetNodeHandler set_handler;  //该节点对应的子集闭包
    IntermediateDfaNode(SetNodeHandler handler) : set_handler(handler) {}
    //设置所有条件均不可转移
    void SetAllUntransable();
  };
  struct DfaNode {
    std::array<DfaNodeHandler, kchar_num> forward_nodes;
  };
  IntermediateDfaNode* GetIntermediateNode(IntermediateNodeHandler handler) {
    return node_manager_intermediate_node_.GetNode(handler);
  }
  DfaNode* GetDfaNode(DfaNodeHandler handler) {
    return node_manager_dfa_node_.GetNode(handler);
  }
 SetType* GetSetNode(SetNodeHandler handler) {
    return node_manager_set_.GetNode(handler);
  }
  //集合转移函数（子集构造法用）
  SetNodeHandler SetGoto(SetNodeHandler set_src, char c_transform);
  // DFA转移函数（最小化DFA用）
  DfaNodeHandler DfaGoto(DfaNodeHandler dfa_src, char c_transform);
  //设置DFA中间节点条件转移
  bool SetIntermediateNodeTransform(IntermediateNodeHandler node_intermediate_src,
                                    char c_transform,
                                    IntermediateNodeHandler node_intermediate_dst);
  //设置最终DFA节点条件转移
  bool SetFinalNodeTransform(DfaNodeHandler node_final_src, char c_transform,
                             DfaNodeHandler node_final_dst);
  //如果集合已存在则返回true，如果不存在则插入并返回false
  //返回的第二个参数为对应中间节点句柄
  std::pair<bool, DfaNodeHandler> InOrInsert(
      const std::unordered_set<NfaNodeHandler>& uset);
  DfaNodeHandler head_node_;
  NfaGenerator nfa_generator_;
  //存储DFA中间节点
  NodeManager<IntermediateDfaNode> node_manager_intermediate_node_;
  //存储最小DFA节点
  NodeManager<DfaNode> node_manager_dfa_node_;
  //存储集合（子集构造法得到的）
  NodeManager<SetType> node_manager_set_;
  //存储集合的哈希到节点句柄的映射
  std::unordered_multimap<IntergalSetHashType, DfaNodeHandler>
      set_hash_to_intermediate_node_handler_;
};
