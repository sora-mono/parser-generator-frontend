/// @file nfa_generator.h
/// @brief NFA配置生成器
/// @details
/// NFA Generator将输入的正则表达式转化为正则数据结构
/// 支持使用char的全部字符
/// 支持基础的正则格式有：单字符，[]，[]内使用-，()，*，+，?

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
/// @class NfaGenerator nfa_generator.h
/// @details
/// 该NFA配置生成器支持char的所有值作为NFA的转移字符，支持*，+，?限定重复次数
/// 该生成器设计初衷为了作为DFAGenerator的组件使用，所以未添加将配置写入文件功能
class NfaGenerator {
 private:
  /// @brief 存储NFA转移方法的节点
  class NfaNode;

 public:
  /// @brief NfaNode在管理类中的ID
  using NfaNodeId = frontend::common::MultimapObjectManager<NfaNode>::ObjectId;

  /// @brief 尾节点数据，内容为该单词所附带的属性
  /// @details 前半部分为用户定义数据，后半部分为单词优先级，数字越大优先级越高
  using TailNodeData = std::pair<WordAttachedData, WordPriority>;

 private:
  /// @class NfaNode nfa_generator.h
  /// @brief 表示正则的Nfa节点
  /// @details
  /// 1.NFA以链表的形式存储在内存中，每个节点具有两个成员，分别存储从该节点可以无条件
  /// 转移到的节点和移入某个字符后转移到的节点。
  /// 2.从一个节点可以无条件转移到的所有节点都与该节点等价，无论是可以从该节点直接无
  /// 条件转移还是间接无条件转移得到。
  /// 3.有条件转移模式用来表示单字符或[]模式，无条件转移模式用来表示*，+，?并用来连接
  /// 两个子部分，如两个()限定的子正则表达式，这样可以只扫描一次字符串形式的正则表达式
  /// 就构建出正则
  /// 4.无条件模式表示*，+，?的方法如下图所示：
  /// 图中=>代表有条件转移，->代表无条件转移，○代表节点
  /// ┌─┐          ┌─┐
  /// │  ↓          │  ↓
  /// ○=>○  ○=>○  ○=>○
  /// ↑  │  ↑  │
  /// └─┘  └─┘
  ///   *       +       *
  /// 5.无条件转移只存储可以直接转移到的节点，这样可以简化设计，同时便于修改；如果保存
  /// 所有可以无条件转移到的节点则在添加*，+，?时需要更新全部前面的可以无条件转移到源
  /// 节点的节点的无条件转移表，这样就需要追踪整个添加过程，大大增加复杂度
  /// 6.尾节点是携带有效返回数据（WordAttachedData）的节点，在状态转移到该节点后如果
  /// 终止转换过程，则获取到有效的单词（已经注册过的单词）。
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

    /// @brief 根据要移入的字母获取转移到的下一个节点ID
    /// @param[in] c_transfer ：要移入的字母
    /// @return 转移到的下一个节点ID
    /// @retval NfaNodeId::InvalidId() ：该节点不能移入给定字母
    NfaNodeId GetForwardNodeId(char c_transfer);
    /// @brief 获取所有可以无条件转移到的节点ID
    /// @return 可以无条件转移到的节点ID的集合
    const std::unordered_set<NfaNodeId>& GetUnconditionTransferNodesIds()
        const {
      return conditionless_transfer_nodes_id;
    }
    /// @brief 获取该节点全部转移条件和在转移条件下可以转移到的节点ID
    const std::unordered_map<char, NfaNodeId>& GetConditionalTransfers() const {
      return nodes_forward_;
    }
    /// @brief 设置条件转移条目
    /// @param[in] c_transfer ：转移条件（要移入的字符）
    /// @param[out] node_id ：转移后到达的节点ID
    /// @note 会覆盖原来的转移条件（如果存在）
    void SetConditionTransfer(char c_transfer, NfaNodeId node_id);
    /// @brief 添加无条件转移节点
    /// @param[in] node_id ：无条件转移到的节点ID
    /// @note node_id可以已经添加过
    void AddNoconditionTransfer(NfaNodeId node_id);
    /// @brief 移除一条转移条目
    /// @param[in] c_transfer ：要移除的转移条目的转移条件
    /// @return 返回移除的条目数目
    /// @retval 0 ：没有条目被移除
    /// @retval 1 ：移除了一条转移条目
    size_t RemoveConditionalTransfer(char c_transfer);
    /// @brief 移除一个无条件转移节点
    /// @param[in] node_id ：要移除的转移到的节点ID
    /// @return 返回移除的条目数目
    /// @retval 0 ：没有条目被移除
    /// @retval 1 ：移除了一条转移条目
    size_t RemoveConditionlessTransfer(NfaNodeId node_id);
    /// @brief 移除所有无条件转移节点
    void RemoveAllConditionlessTransfer() {
      conditionless_transfer_nodes_id.clear();
    }
    /// @brief 合并两个节点
    /// @param[in] node_src ：要合并进来的节点
    /// @return 返回合并是否成功
    /// @retval true ：合并成功
    /// @retval false ：node_src == this合并失败
    /// @details
    /// 合并要求：node_src所有条件转移条目必须在this中不存在
    /// 或与this中相同条件的转移条目转移到的节点ID相同
    /// @note 将node_src合并到this中
    bool MergeNodes(NfaNode* node_src);

   private:
    /// @brief 记录转移条件与转移到的节点，一个条件仅允许对应一个节点
    std::unordered_map<char, NfaNodeId> nodes_forward_;
    /// @brief 存储无条件转移节点
    std::unordered_set<NfaNodeId> conditionless_transfer_nodes_id;
  };

 public:
  NfaGenerator() {}
  NfaGenerator(const NfaGenerator&) = delete;
  NfaGenerator(NfaGenerator&&) = delete;

  /// @brief 获取NFA节点头结点ID
  /// @return 返回NFA节点头结点ID
  NfaNodeId GetHeadNfaNodeId() const { return head_node_id_; }
  /// @brief 根据NFA节点ID获取尾节点数据
  /// @param[in] node_id ：要获取尾节点数据的NFA节点ID
  /// @return 返回获取到的尾节点数据
  /// @retval ：kNotTailNodeTag node_id不是尾节点时返回该内容
  const TailNodeData& GetTailNodeData(NfaNodeId node_id);
  /// @brief 根据NFA节点ID获取NFA节点
  /// @param[in] node_id ：NFA节点ID
  /// @return 返回获取到的NFA节点
  /// @attention node_id必须对应已存在的NFA节点
  NfaNode& GetNfaNode(NfaNodeId node_id) {
    return node_manager_.GetObject(node_id);
  }
  /// @brief 解析正则
  /// @param[in] tail_node_data ：获取到该单词时返回的数据
  /// @param[in] raw_regex_string ：表示单词的正则表达式
  /// @param[in,out] next_character_index
  /// ：下一个读取的raw_regex_string中的单词的位置
  /// @param[in] add_to_nfa_head
  /// ：构建完成后是否设置NFA头结点到生成的结构链头结点的无条件转移路径
  /// @return 返回生成的NFA结构链的头结点ID和尾节点ID
  /// @details
  /// next_character_index决定调用该函数时从哪里开始读取raw_regex_string中的字符
  /// add_to_nfa_head用于内部遇到'('时递归调用该函数时防止将子正则作为有效单词
  /// 添加到配置中，第一次调用应使用true（默认值），递归调用时应使用false
  /// 如果next_character_index指向的字符左侧为'('，则读取到与之匹配的')'时返回
  /// （读取到第一个')'时返回，因为递归调用时会自动处理对应的')'），否则读取到
  /// raw_regex_string末尾时返回
  /// 如果遇到')'，返回时不自动处理')'后的范围限制符号（?、+、*等）
  /// @note 返回时next_character_index指向下一个要读取的字符
  /// add_to_nfa_head == false时tail_node_data可以提供任意参数
  /// @attention 如果next_character_index >= raw_regex_string.size()
  /// 则返回值两部分都为NfaNodeId::InvalidId()
  std::pair<NfaNodeId, NfaNodeId> RegexConstruct(
      TailNodeData&& tail_node_data, const std::string& raw_regex_string,
      size_t&& next_character_index = 0, const bool add_to_nfa_head = true);
  /// @brief 添加一个纯单词
  /// @param[in] str ：单词的字符串
  /// @param[in] word_attached_data
  /// @return 返回生成的NFA结构链的头结点ID和尾节点ID
  /// @details
  /// 该函数与RegexConstruct区别在于它将str视为纯单词而非正则表达式来添加
  /// @return 返回生成的NFA结构链的头结点和尾节点
  std::pair<NfaNodeId, NfaNodeId> WordConstruct(
      const std::string& str, TailNodeData&& word_attached_data);
  /// @brief 合并优化，降低节点数和转移路径以降低子集构造法集合大小
  /// @details
  /// 合并优化由两部分组成，一部分为删除当前处理节点与等效节点重复的转移项
  /// 另一部分为当前处理节点仅存在无条件转移到等效节点的转移条目时合并两个节点
  /// 直接使用NFA也可以降低成本
  void MergeOptimization();

  /// @brief 获取给定NFA节点的所有等效节点ID（包含自身）
  /// @param[in] node_id ：要获取所有等效节点的NFA节点ID
  /// @return 前半部分为所有等效节点的集合，后半部分为这些等效节点代表的单词的
  /// 附属数据
  /// @note 返回值后半部分为所有可能代表的单词中最高优先级的单词的数据
  /// 如果所有等效节点都无法代表任何单词，那么后半部分返回kNotTailNodeTag
  std::pair<std::unordered_set<NfaNodeId>, TailNodeData> Closure(
      NfaNodeId node_id);
  /// @brief 获取给定NFA节点在转移条件下可以到达的等效节点
  /// @param[in] id_src ：源节点
  /// @param[in] c_transform ：转移条件
  /// @return 前半部分为所有等效节点的集合，后半部分为这些等效节点代表的单词的
  /// 附属数据
  /// @details
  /// 该函数获取转移后达到的NfaNodeId，然后返回该ID的Closure结果
  std::pair<std::unordered_set<NfaNodeId>, TailNodeData> Goto(NfaNodeId id_src,
                                                              char c_transform);
  /// @brief NFA初始化
  /// @note 可以通过调用该函数清空已有的NFA配置
  void NfaInit();

  /// @brief 非尾节点标记
  static const TailNodeData kNotTailNodeTag;

 private:
  /// @brief 移除尾节点信息
  /// @param[in] tail_node_id ：要移除尾节点信息的节点ID
  /// @return 返回删除的条目个数
  /// @retval 0 ：未删除任何条目（node_id指定的节点不存在或不为尾节点）
  /// @retval 1 ：删除了已有的一个条目
  size_t RemoveTailNode(NfaNodeId tail_node_id) {
    return tail_nodes_.erase(tail_node_id);
  }
  /// @brief 设置尾节点信息
  /// @tparam TailNodeDataType ：尾节点数据类型，仅支持const
  /// TailNodeData&和TailNodeData&&
  /// @param[in] node_id ：待设置为尾节点的节点ID
  /// @param[in] tail_node_data ：尾节点数据
  /// @return 是否设置成功
  /// @retval true ：设置成功
  /// @retval false ：设置失败
  /// 已存在node_id的尾节点记录，需要先调用RemoveTailNode然后再调用该函数
  template <class TailNodeDataType>
  bool SetTailNode(NfaNodeId node_id, TailNodeDataType&& tail_node_data) {
    return tail_nodes_
        .emplace(node_id, std::forward<TailNodeDataType>(tail_node_data))
        .second;
  }
  /// @brief 根据输入生成可选字符结构
  /// @param[in] raw_regex_string ：表示单词的正则表达式字符串
  /// @param[in,out] next_character_index ：指向下一个读取的字符位置的下标的指针
  /// @return 返回生成的NFA数据结构的头结点ID和尾节点ID
  /// @details
  /// 1.raw_regex_string内对应[]部分必须为有效正则
  ///   否则在std::cerr输出错误信息后退出
  /// 2.next_character_index调用时解引用的值应为'['右侧第一个字符的下标
  ///   成功调用后解引用的值为']'右侧第一个字符的下标
  /// 3.不会读取]后的*,+,?等限定符
  /// 4.例：raw_regex_string == "[a-zA-Z_]" next_character_index == 1
  std::pair<NfaNodeId, NfaNodeId> CreateSwitchTree(
      const std::string& raw_regex_string, size_t* const next_character_index);

  /// @brief NFA头结点ID
  NfaNodeId head_node_id_;
  /// @brief 储存尾节点ID到尾节点数据的映射
  std::unordered_map<NfaNodeId, TailNodeData> tail_nodes_;
  /// @brief 储存NFA节点
  frontend::common::MultimapObjectManager<NfaNode> node_manager_;
};

}  // namespace frontend::generator::dfa_generator::nfa_generator

#endif  /// !GENERATOR_DFAGENERATOR_NFAGENERATOR_NFAGENERATOR_H_
