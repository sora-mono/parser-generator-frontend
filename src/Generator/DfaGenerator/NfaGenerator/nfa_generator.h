#ifndef GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
#define GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Common/common.h"
#include "Common/id_wrapper.h"
#include "Common/multimap_object_manager.h"
#include "Common/unordered_struct_manager.h"
#include "Generator/export_types.h"

namespace frontend::generator::dfa_generator::nfa_generator {
class NfaGenerator {
 public:
  class NfaNode;

  // NfaNode在管理类中的ID
  using NfaNodeId = frontend::common::MultimapObjectManager<NfaNode>::ObjectId;

  // 尾节点数据，内容为该单词所附带的属性
  // 前半部分为用户定义数据，后半部分为单词优先级，数字越大优先级越高
  using TailNodeData = std::pair<WordAttachedData, WordPriority>;

  class NfaNode {
   public:
    NfaNode() {}
    NfaNode(const NfaNode& node)
        : nodes_forward_(node.nodes_forward_),
          conditionless_transfer_nodes_id(
              node.conditionless_transfer_nodes_id) {}
    NfaNode(NfaNode&& node)
        : nodes_forward_(std::move(node.nodes_forward_)),
          conditionless_transfer_nodes_id(
              std::move(node.conditionless_transfer_nodes_id)) {}

    // 根据转移条件获取转移到的节点ID
    // 如果不存在相应的转移结果则返回NfaNodeId::InvalidId()
    NfaNodeId GetForwardNodesId(char c_transfer);
    const std::unordered_set<NfaNodeId>& GetUnconditionTransferNodesIds()
        const {
      return conditionless_transfer_nodes_id;
    }
    const std::unordered_map<char, NfaNodeId>& GetConditionalTransfers() const {
      return nodes_forward_;
    }
    // 设置条件转移节点
    void SetConditionTransfer(char c_transfer, NfaNodeId node_id);
    // 添加无条件转移节点
    void AddNoconditionTransfer(NfaNodeId node_id);
    // 移除一个转移条件节点，函数保证执行后不存在节点（无论原来是否存在)
    void RemoveConditionalTransfer(char c_treasfer);
    // 同上，移除一个无条件转移节点，函数保证执行后不存在节点（无论原来是否存在)
    void RemoveConditionlessTransfer(NfaNodeId node_id);
    // 移除所有无条件转移节点
    void RemoveAllConditionlessTransfer() {
      conditionless_transfer_nodes_id.clear();
    }
    bool MergeNodesWithManager(NfaNode& node_src);

   private:
    friend class NfaGenerator;

    std::unordered_set<NfaNodeId>& GetUnconditionTransferNodesIds() {
      return conditionless_transfer_nodes_id;
    }
    std::unordered_map<char, NfaNodeId>& GetConditionalTransfers() {
      return nodes_forward_;
    }
    // 记录转移条件与转移到的节点，一个条件仅允许对应一个节点
    std::unordered_map<char, NfaNodeId> nodes_forward_;
    // 存储无条件转移节点
    std::unordered_set<NfaNodeId> conditionless_transfer_nodes_id;
  };

  NfaGenerator() {}
  NfaGenerator(const NfaGenerator&) = delete;
  NfaGenerator(NfaGenerator&&) = delete;

  NfaNodeId GetHeadNfaNodeId() { return head_node_id_; }

  const TailNodeData GetTailNodeData(NfaNode* pointer);
  const TailNodeData GetTailNodeData(NfaNodeId production_node_id);
  NfaNode& GetNfaNode(NfaNodeId production_node_id) {
    return node_manager_.GetObject(production_node_id);
  }
  // 解析正则并添加到已有NFA中，返回生成的自动机的头结点和尾节点
  // 作为外部接口使用时只需填写前两个参数，其余参数使用默认值
  // regex为正则表达式字符串，next_character_index为下一个读取的字符的位置
  // add_to_nfa_head代表构建字符串后是否添加到NFA头结点中
  // 读取到与调用时给定的next_character_index左侧'('匹配的')'时返回
  // （如果有，否则在next_character_index到达字符串尾部时返回）
  // 返回时不自动处理结尾的范围限制符号（?、+、*等）
  // 如果输入空串则仅返回一个节点（头结点和尾节点相同）
  std::pair<NfaNodeId, NfaNodeId> RegexConstruct(
      const TailNodeData& tail_node_data, const std::string& raw_regex_string,
      size_t&& next_character_index = 0, const bool add_to_nfa_head = true);
  // 添加一个由字符串构成的NFA，自动处理结尾的范围限制符号
  std::pair<NfaNodeId, NfaNodeId> WordConstruct(
      const std::string& str, TailNodeData&& word_attached_data);
  // 合并优化，降低节点数和分支路径以降低子集构造法集合大小
  // 直接使用NFA也可以降低成本
  void MergeOptimization();

  // 获取给定NFA节点的所有等效节点ID（包含自身）
  // 返回所有等效节点的集合和这些等效节点代表的单词附属数据（可能无效）
  std::pair<std::unordered_set<NfaNodeId>, TailNodeData> Closure(
      NfaNodeId production_node_id);
  // 返回goto后的节点的闭包
  std::pair<std::unordered_set<NfaNodeId>, TailNodeData> Goto(NfaNodeId id_src,
                                                              char c_transform);
  // NFA初始化
  void NfaInit();

  // 非尾节点标记
  static const TailNodeData NotTailNodeTag;

 private:
  bool RemoveTailNode(NfaNode* pointer);
  bool AddTailNode(NfaNode* pointer, const TailNodeData& tag);
  bool AddTailNode(NfaNodeId production_node_id, const TailNodeData& tag) {
    return AddTailNode(&GetNfaNode(production_node_id), tag);
  }
  // 根据输入生成可选字符序列，不会读取]后的*,+,?等限定符
  // next_character_index应指向'['右侧第一个字符
  // 例：raw_regex_string == "[a-zA-Z_]" next_character_index == 1
  // 返回的第一个参数为头结点ID，第二个参数为尾节点ID
  std::pair<NfaNodeId, NfaNodeId> CreateSwitchTree(
      const std::string& raw_regex_string, size_t* next_character_index);
  // 将node_src合并到node_dst中
  // 返回是否合并成功
  static bool MergeNfaNodes(NfaGenerator::NfaNode& node_dst,
                            NfaGenerator::NfaNode& node_src,
                            NfaGenerator& nfa_generator);

  // 所有NFA的头结点
  NfaNodeId head_node_id_;
  // 该set用来存储所有尾节点和对应单词的信息
  std::unordered_map<const NfaNode*, TailNodeData> tail_nodes_;
  frontend::common::MultimapObjectManager<NfaNode> node_manager_;
};

// 为了SavedData可以参与排序，实际该结构并没有逻辑顺序
// 使用需满足结构体内注释的条件
inline bool operator<(const WordAttachedData& left,
                      const WordAttachedData& right) {
  return left.production_node_id < right.production_node_id;
}
inline bool operator>(const WordAttachedData& left,
                      const WordAttachedData& right) {
  return left.production_node_id > right.production_node_id;
}

}  // namespace frontend::generator::dfa_generator::nfa_generator

#endif  // !GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
