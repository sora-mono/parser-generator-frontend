/// @file dfa_generator.h
/// @brief DFA配置生成器
/// @details
/// DFA配置生成器使用子集构造法在NFA配置基础上构建DFA配置以提高速度
/// DFA Generator构建配置时先通过子集构造法生成中间节点（IntermediateDfaNode），
/// 每个中间节点对应子集构造法中唯一的一个集合；然后通过中间节点构造DFA转移表
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

/// @class DfaGenerator dfa_generator.h
/// @brief DFA配置生成器
class DfaGenerator {
  // @brief 前向声明中间节点以定义IntermediateNodeId
  struct IntermediateDfaNode;

 public:
  /// @brief Nfa节点ID
  using NfaNodeId = NfaGenerator::NfaNodeId;
  /// @brief 子集构造法使用的集合的类型
  using SetType = std::unordered_set<NfaNodeId>;
  /// @class UnorderedSetHasher dfa_generator.h
  /// @brief 哈希SetType集合的类，用于实例化UnorderedStructManager
  struct UnorderedSetHasher {
    size_t operator()(const SetType& set) const {
      size_t result = 1;
      for (const auto& item : set) {
        result *= item;
      }
      return result;
    }
  };
  /// @brief 尾节点数据
  using TailNodeData = NfaGenerator::TailNodeData;
  /// @brief 尾节点优先级
  /// @details 数字越大优先级越高
  /// 所有终结产生式的单词都有该参数
  /// 与运算符节点的优先级意义不同，该优先级决定在多个正则同时匹配成功时选择哪个
  /// 正则得出词和附属数据
  /// @note 普通单词使用优先级0，运算符使用优先级1，关键字使用优先级2
  using WordPriority = nfa_generator::WordPriority;
  /// @brief 解析出单词后随单词返回的附属数据
  using WordAttachedData = nfa_generator::WordAttachedData;
  /// @brief 管理子集构造法使用的集合的结构
  using SetManagerType =
      frontend::common::UnorderedStructManager<SetType, UnorderedSetHasher>;
  /// @brief 子集构造法使用的集合的ID
  using SetId = SetManagerType::ObjectId;
  /// @brief DFA中间节点ID
  using IntermediateNodeId = ObjectManager<IntermediateDfaNode>::ObjectId;

  DfaGenerator() = default;
  DfaGenerator(const DfaGenerator&) = delete;
  DfaGenerator(DfaGenerator&&) = delete;

  /// @brief 初始化
  /// @note 该函数可用于清除所有已添加单词
  void DfaInit();

  /// @brief 添加纯单词
  /// @param[in] word ：待添加的单词
  /// @param[in] word_attached_data ：单词的附属数据
  /// @param[in] priority_tag ：单词的优先级
  /// @return 返回是否添加成功
  /// @retval true ：添加成功
  /// @retval false ：添加失败
  /// @note 单词优先级定义见WordPriority说明
  /// @attention 该函数将word视为纯单词而不是正则表达式进行添加
  bool AddWord(const std::string& word, WordAttachedData&& word_attached_data,
               WordPriority priority_tag);
  /// @brief 添加单词的正则
  /// @param[in] regex_str ：待添加单词的正则表达式
  /// @param[in] regex_attached_data ：单词的附属数据
  /// @param[in] regex_priority ：单词的优先级
  /// @return 返回是否添加成功
  /// @retval true ：添加成功
  /// @retval false ：添加失败
  /// @note 单词优先级定义见WordPriority说明
  bool AddRegexpression(const std::string& regex_str,
                        WordAttachedData&& regex_attached_data,
                        WordPriority regex_priority);
  /// @brief 设置到达文件尾时返回的数据
  /// @param[in] saved_data ：到达文件尾返回的数据
  /// @attention 仅在未获取到任何单词且到达文件尾时返回
  /// 如果已经获取了一些字符后遇到文件尾则返回这些字符对应的单词
  void SetEndOfFileSavedData(WordAttachedData&& saved_data) {
    file_end_saved_data_ = std::move(saved_data);
  }
  /// @brief 获取到达文件尾时返回的数据
  /// @return 返回到达文件尾时返回的数据的const引用
  const WordAttachedData& GetEndOfFileSavedData() const {
    return file_end_saved_data_;
  }
  /// @brief 构建DFA配置
  /// @note 该函数仅生成DFA配置，不会自动保存配置到文件
  bool DfaConstruct();
  /// @brief 保存DFA配置
  void SaveConfig() const;

 private:
  /// @brief 声明友元，允许序列化类访问成员
  friend class boost::serialization::access;

  /// @class PairOfIntermediateNodeIdAndWordAttachedDataHasher dfa_generator.h
  /// @brief 哈希中间节点ID和单词数据的结合体的结构，用于SubDataMinimize
  struct PairOfIntermediateNodeIdAndWordAttachedDataHasher {
    size_t operator()(const std::pair<IntermediateNodeId, WordAttachedData>&
                          data_to_hash) const {
      return data_to_hash.first * data_to_hash.second.production_node_id;
    }
  };

  /// @brief 序列化配置的函数
  /// @param[in,out] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  template <class Archive>
  void save(Archive& ar, const unsigned int version) const {
    ar << dfa_config_;
    ar << root_transform_array_id_;
    ar << file_end_saved_data_;
  }
  /// 分割save和load操作，DFA配置生成器只能执行save操作，不能执行load操作
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  /// @class IntermediateDfaNode dfa_generator.h
  /// @brief DFA配置生成器的中间节点
  /// @details
  /// 中间节点与子集构造法的集合一一对应，每个节点存储一个条件转移表
  struct IntermediateDfaNode {
    template <class AttachedData>
    IntermediateDfaNode(SetId handler = SetId::InvalidId(),
                        AttachedData&& word_attached_data_ = WordAttachedData())
        : word_attached_data(std::forward<AttachedData>(word_attached_data_)),
          set_handler(handler) {
      SetAllUntransable();
    }

    /// @brief 设置该节点在任何字符下均不可转移
    void SetAllUntransable() {
      forward_nodes.fill(IntermediateNodeId::InvalidId());
    }
    /// @brief 获取该节点对应集合的ID
    /// @return 返回该节点对应集合的ID
    SetId GetSetHandler() const { return set_handler; }
    /// @brief 设置该节点对应单词的数据
    /// @param[in] data ：对应单词数据
    /// @note 该函数仅接受WordAttachedData的const引用和右值引用
    template <class AttachedData>
    void SetWordAttachedData(AttachedData&& data) {
      word_attached_data = std::forward<AttachedData>(data);
    }

    /// @brief 该节点的转移条件，存储IntermediateDfaNode节点句柄
    /// @note 可以直接使用CHAR_MIN~CHAR_MAX任意值访问，不会存在下标错误
    TransformArrayManager<IntermediateNodeId> forward_nodes;
    /// @brief 该节点对应单词的数据
    WordAttachedData word_attached_data;
    /// 该节点对应的集合ID
    SetId set_handler;
  };

  /// @brief 构建最小化DFA
  /// @note 该函数在DfaConstruct操作后调用，合并相同转移项并填充DFA配置表
  bool DfaMinimize();
  /// @brief 获取中间节点引用
  /// @param[in] id ：中间节点ID
  /// @return 返回对应的中间节点引用
  /// @note id必须对应已存在的中间节点
  IntermediateDfaNode& GetIntermediateNode(IntermediateNodeId id) {
    return node_manager_intermediate_node_.GetObject(id);
  }
  /// @brief 获取中间节点const引用
  /// @param[in] id ：中间节点ID
  /// @return 返回对应的中间节点const引用
  /// @note id必须对应已存在的中间节点
  const IntermediateDfaNode& GetIntermediateNode(IntermediateNodeId id) const {
    return node_manager_intermediate_node_.GetObject(id);
  }
  /// @brief 获取子集构造法的集合的引用
  /// @param[in] set_id ：集合ID
  /// @return 返回集合的引用
  /// @note set_id必须对应已存在的集合
  SetType& GetSetObject(SetId set_id) {
    return node_manager_set_.GetObject(set_id);
  }
  /// @brief 获取NFA节点集合在给定字符下的转移结果
  /// @param[in] set_src ：转移起点的NFA节点ID的集合
  /// @param[in] c_transform ：转移条件
  /// @return 返回值前半部分为对应中间节点句柄，后半部分表示是否新创建了集合
  /// @retval std::pair(IntermediateNodeId::InvalidId(), false)
  /// ：该集合在给定条件下无法转移
  std::pair<IntermediateNodeId, bool> SetGoto(SetId set_src, char c_transform);
  /// @brief 查询DFA中间节点在给定条件下的转移情况
  /// @param[in] dfa_src ：转移起点的DFA中间节点
  /// @param[in] c_transform ：转移条件
  /// @return 返回转移到的DFA中间节点ID
  /// @retval IntermediateNodeId::InvalidId()
  /// ：该节点在c_transform条件下无法转移
  /// @note 该函数仅查询，SetGoto如果不存在会创建新集合和中间节点
  IntermediateNodeId IntermediateGoto(IntermediateNodeId dfa_src,
                                      char c_transform) const;
  /// @brief 设置DFA中间节点条件转移
  /// @param[in] node_intermediate_src ：源节点
  /// @param[in] c_transform ：转移条件
  /// @param[in] node_intermediate_dst ：目的节点
  /// @return 返回是否设置成功
  /// @retval true ：设置成功
  /// @retval false ：设置失败
  /// @note 一定返回true
  bool SetIntermediateNodeTransform(IntermediateNodeId node_intermediate_src,
                                    char c_transform,
                                    IntermediateNodeId node_intermediate_dst);
  /// @brief 获取集合对应的中间节点ID，如果不存在则创建新的中间节点
  /// @param[in] uset ：获取对应中间节点的集合
  /// @param[in] word_attached_data ：如果创建中间节点则该中间节点使用该单词数据
  /// @return
  /// 返回获取到的中间节点ID和是否新插入该集合（是否新建中间节点）
  /// @note IntermediateNodeSet仅支持SetType的const左值引用或右值引用
  template <class IntermediateNodeSet>
  std::pair<IntermediateNodeId, bool> InOrInsert(
      IntermediateNodeSet&& uset,
      WordAttachedData&& word_attached_data = WordAttachedData());

  /// @brief 对中间节点进行分类
  /// @param[in] node_ids ：待分类的节点集合
  /// @param[in] c_transform ：本轮分类使用的转移字符
  /// @details
  /// 该函数每次调用都对当前中间节点根据在当前转移字符下的转移结果进行分类
  /// 分类方法：
  /// 每轮分类后得到>=1个子集合，如果子集合内含有多个节点且c_transform!=CHAR_MIN
  /// 则使用c_transform+1和每个含有多个元素的集合调用该函数进行下一轮分类
  /// 否则每个子集合映射为一条DFA转移表条目，这个集合内所有节点等效
  /// 调用方式：
  /// 初次调用时c_transform代表第一个开始判断的字符
  /// 每轮调用后如果需要进行下一轮分类则自动调用，无需手动控制
  /// @note 每一轮转移结果都相同的中间节点为等效节点，可以合并
  /// 该函数会填写intermediate_node_to_final_node_和transform_array_size_
  /// @attention node_ids所有元素不允许重复
  void IntermediateNodeClassify(std::list<IntermediateNodeId>&& node_ids,
                                char c_transform = CHAR_MIN);

  /// @brief DFA配置
  /// @note 写入配置文件
  DfaConfigType dfa_config_;
  /// @brief DFA转移表初始条目序号
  /// @note 写入配置文件
  TransformArrayId root_transform_array_id_;
  /// @brief 遇到文件尾时返回的数据
  /// @note 写入配置文件
  WordAttachedData file_end_saved_data_;

  /// @brief NFA配置生成器
  NfaGenerator nfa_generator_;
  /// @brief 中间节点初始节点ID
  IntermediateNodeId root_intermediate_node_id_;
  /// @brief DFA转移表条目数
  size_t transform_array_size_ = 0;

  /// @brief 存储DFA中间节点到转移表条目的映射
  std::unordered_map<IntermediateNodeId, TransformArrayId>
      intermediate_node_to_final_node_;
  /// @brief 存储中间节点
  ObjectManager<IntermediateDfaNode> node_manager_intermediate_node_;
  /// @brief 管理中间节点对应的集合
  SetManagerType node_manager_set_;
  /// @brief 存储集合ID到中间节点ID的映射
  std::unordered_map<SetId, IntermediateNodeId> setid_to_intermediate_nodeid_;
};

template <class IntermediateNodeSet>
inline std::pair<DfaGenerator::IntermediateNodeId, bool>
DfaGenerator::InOrInsert(IntermediateNodeSet&& uset,
                         WordAttachedData&& word_attached_data) {
  auto [setid, inserted] =
      node_manager_set_.EmplaceObject(std::forward<IntermediateNodeSet>(uset));
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
#endif  /// !GENERATOR_DFAGENERATOR_DFAGENERATOR_H_
