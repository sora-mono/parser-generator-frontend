#pragma once

#include <functional>
#include <vector>

template <class T>
class NodeManager {
 public:
  using NodeHandler = size_t;

  NodeManager() {}
  NodeManager(const NodeManager&) = delete;
  NodeManager(NodeManager&&) = delete;
  ~NodeManager();

  static bool DefaultMergeFunction2(T& node_dst, T& node_src) {
    return node_dst.MergeNode(node_src);
  }
  template <class Manager>
  static bool DefaultMergeFunction3(T& node_dst, T& node_src,
                                    Manager& manager) {
    return manager.MergeNodesWithManager(node_dst, node_src);
  }

  T* GetNode(NodeHandler index);
  bool IsSame(NodeHandler index1, NodeHandler index2) {
    return index1 == index2;
  }

  //系统自行选择最佳位置放置节点
  template <class... Args>
  NodeHandler EmplaceNode(Args&&... args);
  //在指定位置放置节点
  template <class... Args>
  NodeManager<T>::NodeHandler EmplaceNodeIndex(NodeHandler index,
                                               Args&&... args);
  //在指定位置放置指针
  NodeManager<T>::NodeHandler EmplacePointerIndex(NodeHandler index,
                                                  T* pointer);
  //仅删除节点不释放节点内存
  T* RemovePointer(NodeHandler index);
  //删除节点并释放节点内存
  bool RemoveNode(NodeHandler index);
  //合并两个节点，合并成功会删除index_src节点
  bool MergeNodesWithManager(NodeHandler index_dst, NodeHandler index_src,
                             const std::function<bool(T&, T&)>& merge_function =
                                 DefaultMergeFunction2);
  //合并两个节点，合并成功会删除index_src节点
  template <class Manager>
  bool MergeNodesWithManager(NodeHandler index_dst, NodeHandler index_src,
                             Manager& manager,
                             const std::function<bool(T&, T&, Manager&)>&
                                 merge_function = DefaultMergeFunction3);
  void Swap(NodeManager& manager_other);

  bool SetNodeMergeAllowed(NodeHandler index);
  bool SetNodeMergeRefused(NodeHandler index);
  void SetAllNodesMergeAllowed();
  void SetAllNodesMergeRefused();

  inline bool CanMerge(NodeHandler index) {
    return index < nodes_can_merge.size() ? nodes_can_merge[index] : false;
  }
  size_t Size() { return nodes_.size(); }
  size_t ItemSize();
  //清除并释放所有节点
  void Clear();
  //清除但不释放所有节点
  void ClearNoRelease();
  //调用成员变量的shrink_to_fit
  void ShrinkToFit();

  //序列化容器用
  template <class Archive>
  void Serialize(Archive& ar, const unsigned int version = 0);

 private:
  std::vector<T*> nodes_;
  //优化用数组，存放被删除节点对应的ID
  std::vector<NodeHandler> removed_ids_;
  //存储信息表示是否允许合并节点
  std::vector<bool> nodes_can_merge;
};

template <class T>
NodeManager<T>::~NodeManager() {
  for (T* ptr : nodes_) {
    delete ptr;
  }
}

template <class T>
inline T* NodeManager<T>::GetNode(NodeHandler index) {
  if (index > nodes_.size()) {
    return nullptr;
  } else {
    return nodes_[index];
  }
}

template <class T>
inline bool NodeManager<T>::RemoveNode(NodeHandler index) {
  if (index > nodes_.size()) {
    return false;
  }
  if (nodes_[index] == nullptr) {
    return true;
  }
  removed_ids_.push_back(index);  //添加到已删除ID中
  delete nodes_[index];
  nodes_[index] = nullptr;
  nodes_can_merge[index] = false;
  while (nodes_.size() > 0 && nodes_.back() == nullptr) {
    nodes_.pop_back();  //清理末尾无效ID
    nodes_can_merge.pop_back();
  }
  return true;
}

template <class T>
inline T* NodeManager<T>::RemovePointer(NodeHandler index) {
  if (index > nodes_.size()) {
    return nullptr;
  }
  T* temp_pointer = nodes_[index];
  if (temp_pointer == nullptr) {
    return nullptr;
  }
  removed_ids_.push_back(index);
  nodes_can_merge[index] = false;
  while (nodes_.size() > 0 && nodes_.back() == nullptr) {
    nodes_.pop_back();  //清理末尾无效ID
    nodes_can_merge.pop_back();
  }
  return temp_pointer;
}

template <class T>
template <class... Args>
inline NodeManager<T>::NodeHandler NodeManager<T>::EmplaceNode(Args&&... args) {
  NodeHandler index = -1;
  if (removed_ids_.size() != 0) {
    while (index == -1 && removed_ids_.size() != 0)  //查找有效ID
    {
      if (removed_ids_.back() < nodes_.size()) {
        index = removed_ids_.back();
      }
      removed_ids_.pop_back();
    }
  }
  if (index == -1)  //无有效已删除ID
  {
    index = nodes_.size();
  }

  return EmplaceNodeIndex(index, std::forward<Args>(args)...);  //返回插入位置
}

template <class T>
template <class... Args>
inline NodeManager<T>::NodeHandler NodeManager<T>::EmplaceNodeIndex(
    NodeHandler index, Args&&... args) {
  T* pointer = new T(std::forward<Args>(args)...);
  bool result = EmplacePointerIndex(index, pointer);
  if (!result) {
    delete pointer;
  }
  return result;
}

template <class T>
inline NodeManager<T>::NodeHandler NodeManager<T>::EmplacePointerIndex(
    NodeHandler index, T* pointer) {
  size_t size_old = nodes_.size();
  if (index >= size_old) {
    nodes_.resize(index + 1, nullptr);
    nodes_can_merge.resize(index + 1, false);
    for (size_t i = size_old; i < index; i++) {
      removed_ids_.push_back(i);
    }
  }
  if (nodes_[index] != nullptr &&
      pointer != nodes_[index]) {  //不可以覆盖已有非空且不同的指针
    return -1;
  }
  nodes_[index] = pointer;
  nodes_can_merge[index] = true;
  return index;
}

template <class T>
inline void NodeManager<T>::Swap(NodeManager& manager_other) {
  nodes_.Swap(manager_other.nodes_);
  removed_ids_.Swap(manager_other.removed_ids_);
  nodes_can_merge.Swap(manager_other.nodes_can_merge);
}

template <class T>
inline bool NodeManager<T>::SetNodeMergeAllowed(NodeHandler index) {
  if (index > nodes_.size() || nodes_can_merge[index] == nullptr) {
    return false;
  }
  nodes_can_merge[index] = true;
  return true;
}

template <class T>
inline bool NodeManager<T>::SetNodeMergeRefused(NodeHandler index) {
  if (index > nodes_can_merge.size() || nodes_[index] == nullptr) {
    return false;
  }
  nodes_can_merge[index] = false;
  return true;
}

template <class T>
inline void NodeManager<T>::SetAllNodesMergeAllowed() {
  for (size_t index = 0; index < nodes_.size(); index++) {
    if (nodes_[index] != nullptr) {
      nodes_can_merge[index] = true;
    }
  }
}

template <class T>
inline void NodeManager<T>::SetAllNodesMergeRefused() {
  std::vector<bool> vec_temp(nodes_can_merge.size(), false);
  nodes_can_merge.Swap(vec_temp);
}

template <class T>
inline bool NodeManager<T>::MergeNodesWithManager(
    NodeHandler index_dst, NodeHandler index_src,
    const std::function<bool(T&, T&)>& merge_function) {
  if (index_dst >= nodes_.size() || index_src >= nodes_.size() ||
      nodes_[index_dst] == nullptr || nodes_[index_src] == nullptr ||
      !CanMerge(index_dst) || !CanMerge(index_src)) {
    return false;
  }
  if (!merge_function(*nodes_[index_dst], *nodes_[index_src])) {
    return false;
  }
  RemoveNode(index_src);
  return true;
}

template <class T>
template <class Manager>
bool NodeManager<T>::MergeNodesWithManager(
    typename NodeManager<T>::NodeHandler index_dst,
    typename NodeManager<T>::NodeHandler index_src, Manager& manager,
    const std::function<bool(T&, T&, Manager&)>& merge_function) {
  if (index_dst >= nodes_.size() || index_src >= nodes_.size() ||
      nodes_[index_dst] == nullptr || nodes_[index_src] == nullptr ||
      !CanMerge(index_dst) || !CanMerge(index_src)) {
    return false;
  }
  if (!merge_function(*nodes_[index_dst], *nodes_[index_src], manager)) {
    return false;
  }
  RemoveNode(index_src);
  return true;
}

template <class T>
inline void NodeManager<T>::Clear() {
  for (auto p : nodes_) {
    delete p;
  }
  nodes_.clear();
  removed_ids_.clear();
}

template <class T>
inline void NodeManager<T>::ClearNoRelease() {
  nodes_.clear();
  removed_ids_.clear();
}

template <class T>
inline void NodeManager<T>::ShrinkToFit() {
  while (nodes_.size() > 0 && nodes_.back() == nullptr) {
    nodes_.pop_back();
    nodes_can_merge.pop_back();
  }
  nodes_.ShrinkToFit();
  removed_ids_.ShrinkToFit();
}

template <class T>
inline size_t NodeManager<T>::ItemSize() {
  size_t count = 0;
  for (auto p : nodes_) {
    if (p != nullptr) {
      ++count;
    }
  }
  return count;
}