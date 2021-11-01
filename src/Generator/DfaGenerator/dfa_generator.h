#ifndef GENERATOR_DFAGENERATOR_DFAGENERATOR_H_
#define GENERATOR_DFAGENERATOR_DFAGENERATOR_H_

#include <boost/serialization/array.hpp>
#include <boost/serialization/map.hpp>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/object_manager.h"
#include "Common/unordered_struct_manager.h"
#include "Generator/export_types.h"
#include "NfaGenerator/nfa_generator.h"

namespace frontend::generator::dfa_generator {
using frontend::common::ObjectManager;
using frontend::generator::dfa_generator::nfa_generator::NfaGenerator;

class DfaGenerator {
  struct IntermediateDfaNode;

 public:
  // 尾节点数据
  using TailNodeData = NfaGenerator::TailNodeData;
  // 解析出单词后随单词返回的附属数据
  using WordAttachedData = nfa_generator::WordAttachedData;
  // 尾节点优先级，数字越大优先级越高，所有表达式体都有该参数
  // 与运算符节点的优先级意义不同
  // 该优先级决定在多个正则同时匹配成功时选择哪个正则得出词和附属数据
  using WordPriority = nfa_generator::WordPriority;
  // Nfa节点ID
  using NfaNodeId = NfaGenerator::NfaNodeId;
  // 子集构造法使用的集合的类型
  using SetType = std::unordered_set<NfaNodeId>;
  // 管理集合的结构
  using SetManagerType = frontend::common::UnorderedStructManager<SetType>;
  // 子集构造法得到的子集的ID
  using SetId = SetManagerType::ObjectId;
  // 中间节点ID
  using IntermediateNodeId = ObjectManager<IntermediateDfaNode>::ObjectId;

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
  // 保存配置
  void SaveConfig() const;

 private:
  friend class boost::serialization::access;

  // 用来哈希中间节点ID和单词数据的结合体的结构，用于SubDataMinimize
  struct PairOfIntermediateNodeIdAndWordAttachedDataHasher {
    size_t operator()(const std::pair<IntermediateNodeId, WordAttachedData>&
                          data_to_hash) const {
      return data_to_hash.first * data_to_hash.second.production_node_id;
    }
  };

  template <class Archive>
  void save(Archive& ar, const unsigned int version) const {
    ar << dfa_config_;
    ar << root_transform_array_id_;
    ar << file_end_saved_data_;
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  struct IntermediateDfaNode {
    template <class AttachedData>
    IntermediateDfaNode(SetId handler = SetId::InvalidId(),
                        AttachedData&& word_attached_data_ = WordAttachedData())
        : word_attached_data(std::forward<AttachedData>(word_attached_data_)),
          set_handler(handler) {
      SetAllUntransable();
    }

    // 设置所有条件均不可转移
    void SetAllUntransable() {
      forward_nodes.fill(IntermediateNodeId::InvalidId());
    }
    SetId GetSetHandler() { return set_handler; }
    void SetWordAttachedData(const WordAttachedData& data) {
      word_attached_data = data;
    }

    // 该节点的转移条件，存储IntermediateDfaNode节点句柄
    // 可以直接使用CHAR_MIN~CHAR_MAX任意值访问
    TransformArrayManager<IntermediateNodeId> forward_nodes;
    WordAttachedData word_attached_data;
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
  template <class IntermediateNodeSet>
  std::pair<IntermediateNodeId, bool> InOrInsert(
      IntermediateNodeSet&& uset,
      WordAttachedData&& word_attached_data = WordAttachedData());

  // 对handlers数组中的句柄作最小化处理，从CHAR_MIN转移条件开始递增直到CHAR_MAX
  // 第二个参数表示起始条件
  void SubDfaMinimize(std::list<IntermediateNodeId>&& handlers,
                      char c_transform = CHAR_MIN);

  // DFA配置，写入文件
  DfaConfigType dfa_config_;
  // 头结点序号，写入文件
  TransformArrayId root_transform_array_id_;
  // 遇到文件尾时返回的数据，写入文件
  WordAttachedData file_end_saved_data_;

  NfaGenerator nfa_generator_;
  // 中间节点头结点句柄
  IntermediateNodeId head_node_intermediate_;
  // 最终有效节点数
  size_t transform_array_size = 0;

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

// 如果集合已存在则返回true，如果不存在则插入并返回false
// 返回的第一个参数为对应中间节点句柄
// 返回的第二个参数为集合是否已存在

template <class IntermediateNodeSet>
inline std::pair<DfaGenerator::IntermediateNodeId, bool>
DfaGenerator::InOrInsert(IntermediateNodeSet&& uset,
                         WordAttachedData&& word_attached_data) {
  auto [setid, inserted] =
      node_manager_set_.AddObject(std::forward<IntermediateNodeSet>(uset));
  IntermediateNodeId intermediate_node_id = IntermediateNodeId::InvalidId();
  if (inserted) {
    intermediate_node_id = node_manager_intermediate_node_.EmplaceObject(
        setid, std::move(word_attached_data));
    setid_to_intermediate_nodeid_.insert(
        std::make_pair(setid, intermediate_node_id));
  } else {
    auto iter = setid_to_intermediate_nodeid_.find(setid);
    assert(iter != setid_to_intermediate_nodeid_.end());
    intermediate_node_id = iter->second;
  }
  return std::make_pair(intermediate_node_id, inserted);
}

}  // namespace frontend::generator::dfa_generator
#endif  // !GENERATOR_DFAGENERATOR_DFAGENERATOR_H_
