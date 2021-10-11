#ifndef GENERATOR_DFAGENERATOR_DFAGENERATOR_H_
#define GENERATOR_DFAGENERATOR_DFAGENERATOR_H_

#include <boost/serialization/array.hpp>
#include <boost/serialization/map.hpp>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/unordered_struct_manager.h"
#include "NfaGenerator/nfa_generator.h"

namespace frontend::parser::dfamachine {
class DfaMachine;
}

namespace frontend::generator::dfa_generator {
using frontend::common::ObjectManager;
using frontend::generator::dfa_generator::nfa_generator::NfaGenerator;
namespace common = frontend::common;

class DfaGenerator {
  struct IntermediateDfaNode;
  // 管理转移表用，仅用于DfaGenerator，为了避免使用char作下标时使用负下标
  // 可以直接使用CHAR_MIN~CHAR_MAX任意值访问
  template <class BasicObjectType>
  class TransformArrayManager {
   public:
    TransformArrayManager() {}

    BasicObjectType& operator[](char c) {
      return transform_array_[(c + frontend::common::kCharNum) %
                              frontend::common::kCharNum];
    }
    void fill(const BasicObjectType& fill_object) {
      transform_array_.fill(fill_object);
    }

   private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& transform_array_;
    }

    std::array<BasicObjectType, frontend::common::kCharNum> transform_array_;
  };

 public:
  // 尾节点数据
  using TailNodeData = NfaGenerator::TailNodeData;
  // 解析出单词后随单词返回的附属数据
  using WordAttachedData = TailNodeData::first_type;
  // 尾节点优先级，数字越大优先级越高，所有表达式体都有该参数
  // 与运算符节点的优先级意义不同
  // 该优先级决定在多个正则同时匹配成功时选择哪个正则得出词和附属数据
  using WordPriority = TailNodeData::second_type;
  // Nfa节点ID
  using NfaNodeId = NfaGenerator::NfaNodeId;
  // 子集构造法使用的集合的类型
  using SetType = std::unordered_set<NfaNodeId>;
  // 中间节点ID
  using IntermediateNodeId = ObjectManager<IntermediateDfaNode>::ObjectId;

  using SetManagerType = common::UnorderedStructManager<SetType>;
  // 子集构造法得到的子集的ID
  using SetId = SetManagerType::ObjectId;
  // 分发标签
  enum class WrapperLabel { kTransformArrayId };
  // 状态转移表ID
  using TransformArrayId =
      frontend::common::ExplicitIdWrapper<size_t, WrapperLabel,
                                          WrapperLabel::kTransformArrayId>;
  // 状态转移表
  using TransformArray = TransformArrayManager<TransformArrayId>;
  // DFA配置类型
  using DfaConfigType =
      std::vector<std::pair<TransformArray, WordAttachedData>>;

  DfaGenerator() = default;
  DfaGenerator(const DfaGenerator&) = delete;
  DfaGenerator(DfaGenerator&&) = delete;

  // 初始化
  void DfaInit();

  // 添加单词
  bool AddWord(const std::string& word, WordAttachedData&& word_attached_data,
               WordPriority priority_tag);
  // 添加正则
  bool AddRegexpression(const std::string& regex_str,
                        WordAttachedData&& regex_attached_data,
                        WordPriority regex_priority);
  // 设置遇到文件尾时返回的数据
  void SetEndOfFileSavedData(WordAttachedData&& saved_data) {
    file_end_saved_data_ = std::move(saved_data);
  }
  // 获取遇到文件尾时返回的数据
  const WordAttachedData& GetEndOfFileSavedData() {
    return file_end_saved_data_;
  }
  // 构建DFA
  bool DfaConstruct();
  // 构建最小化DFA
  bool DfaMinimize();
  // 重新进行完整构建过程
  bool DfaReconstrcut() { return DfaConstruct() && DfaMinimize(); }
  // 保存配置
  void SaveConfig() const;

 private:
  friend class DfaMachine;
  friend class boost::serialization::access;

  template <class Archive>
  void save(Archive& ar, const unsigned int version) const {
    ar << dfa_config_;
    ar << root_index_;
    ar << file_end_saved_data_;
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  struct IntermediateDfaNode {
    IntermediateDfaNode(SetId handler = SetId::InvalidId(),
                        TailNodeData data = NfaGenerator::NotTailNodeTag)
        : tail_node_data(data), set_handler(handler) {
      SetAllUntransable();
    }

    // 设置所有条件均不可转移
    void SetAllUntransable() {
      forward_nodes.fill(IntermediateNodeId::InvalidId());
    }
    SetId GetSetHandler() { return set_handler; }
    void SetTailNodeData(TailNodeData data) { tail_node_data = data; }

    // 该节点的转移条件，存储IntermediateDfaNode节点句柄
    // 可以直接使用CHAR_MIN~CHAR_MAX任意值访问
    TransformArrayManager<IntermediateNodeId> forward_nodes;
    TailNodeData tail_node_data;
    // 该节点对应的子集闭包
    SetId set_handler;
  };

  IntermediateDfaNode& GetIntermediateNode(IntermediateNodeId handler) {
    return node_manager_intermediate_node_.GetObject(handler);
  }
  SetType& GetSetObject(SetId production_node_id) {
    return node_manager_set_.GetObject(production_node_id);
  }
  // 集合转移函数（子集构造法用），
  // 返回值前半部分为对应中间节点句柄，后半部分表示是否新创建了集合
  // 如果无法转移则返回std::make_pair(IntermediateNodeId::InvalidId(),false)
  std::pair<IntermediateNodeId, bool> SetGoto(SetId set_src, char c_transform);
  // DFA转移函数（最小化DFA用）
  IntermediateNodeId IntermediateGoto(IntermediateNodeId dfa_src,
                                      char c_transform);
  // 设置DFA中间节点条件转移
  bool SetIntermediateNodeTransform(IntermediateNodeId node_intermediate_src,
                                    char c_transform,
                                    IntermediateNodeId node_intermediate_dst);
  // 如果集合已存在则返回true，如果不存在则插入并返回false
  // 返回的第一个参数为对应中间节点句柄
  // 返回的第二个参数为集合是否已存在
  std::pair<IntermediateNodeId, bool> InOrInsert(
      const SetType& uset,
      TailNodeData tail_node_data = NfaGenerator::NotTailNodeTag);

  // 对handlers数组中的句柄作最小化处理，从c_transform转移条件开始
  bool DfaMinimize(const std::vector<IntermediateNodeId>& handlers,
                   char c_transform);
  // DfaMinimize的子过程，处理它生成的分类表
  // 并对每一个需要继续分类的组调用DfaMinimize()
  // 分类完成的节点会添加旧节点ID到新节点ID的映射
  // 会添加保留的节点到新节点的映射
  // 并删除多余的等效节点，仅保留一个
  template <class IdType>
  bool DfaMinimizeGroupsRecursion(
      const std::map<IdType, std::vector<IntermediateNodeId>>& groups,
      char c_transform);

  // DFA配置，写入文件
  DfaConfigType dfa_config_;
  // 头结点序号，写入文件
  TransformArrayId root_index_;
  // 遇到文件尾时返回的数据，写入文件
  WordAttachedData file_end_saved_data_;

  NfaGenerator nfa_generator_;
  // 中间节点头结点句柄
  IntermediateNodeId head_node_intermediate_;
  // 最终有效节点数
  size_t config_node_num = 0;

  // 存储DFA中间节点到最终标号的映射
  std::unordered_map<IntermediateNodeId, TransformArrayId>
      intermediate_node_to_final_node_;
  // 存储中间节点
  ObjectManager<IntermediateDfaNode> node_manager_intermediate_node_;
  // 子集构造法使用的集合的管理器
  SetManagerType node_manager_set_;
  // 存储集合的哈希到节点句柄的映射
  std::unordered_map<SetId, IntermediateNodeId> setid_to_intermediate_nodeid_;
};

template <class IdType>
inline bool DfaGenerator::DfaMinimizeGroupsRecursion(
    const std::map<IdType, std::vector<IntermediateNodeId>>& groups,
    char c_transform) {
  for (auto& p : groups) {
    const std::vector<IntermediateNodeId>& vec = p.second;
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

}  // namespace frontend::generator::dfa_generator
#endif  // !GENERATOR_DFAGENERATOR_DFAGENERATOR_H_
