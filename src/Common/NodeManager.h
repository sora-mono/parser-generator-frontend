#include <functional>
#include <stdexcept>
#include <vector>

#ifndef COMMON_NODE_MANAGER_H_
#define COMMON_NODE_MANAGER_H_

namespace common {

template <class T>
class NodeManager {
 public:
  using NodeId = size_t;

  NodeManager() {}
  NodeManager(const NodeManager&) = delete;
  NodeManager(NodeManager&&) = delete;
  ~NodeManager();

  class Iterator {
   public:
    Iterator() : manager_pointer_(nullptr), index_(-1) {}
    Iterator(NodeManager<T>* manager_pointer, size_t index)
        : manager_pointer_(manager_pointer), index_(index) {}
    Iterator(const Iterator& iter)
        : manager_pointer_(iter.manager_pointer_), index_(iter.index_) {}

    size_t GetId() { return index_; }
    void SetManagerPointer(const NodeManager<T>* manager_pointer) {
      manager_pointer_ = manager_pointer;
    }
    void SetIndex(size_t index);

    Iterator& operator++();
    Iterator operator++(int);
    Iterator& operator--();
    Iterator operator--(int);
    T& operator*();
    T* operator->();
    bool operator==(const Iterator& iter) {
      return manager_pointer_ == iter.manager_pointer_ && index_ == iter.index_;
    }
    bool operator!=(const Iterator& iter) { return !this->operator==(iter); }

   private:
    NodeManager<T>* manager_pointer_;
    size_t index_;
  };

  static bool DefaultMergeFunction2(T& node_dst, T& node_src) {
    return node_dst.MergeNode(node_src);
  }
  template <class Manager>
  static bool DefaultMergeFunction3(T& node_dst, T& node_src,
                                    Manager& manager) {
    return manager.MergeNodesWithManager(node_dst, node_src);
  }

  T* GetNode(NodeId index);
  bool IsSame(NodeId index1, NodeId index2) { return index1 == index2; }

  //系统自行选择最佳位置放置节点
  template <class... Args>
  NodeId EmplaceNode(Args&&... args);
  //在指定位置放置节点
  template <class... Args>
  NodeManager<T>::NodeId EmplaceNodeIndex(NodeId index, Args&&... args);
  //在指定位置放置指针
  NodeManager<T>::NodeId EmplacePointerIndex(NodeId index, T* pointer);
  //仅删除节点不释放节点内存
  T* RemovePointer(NodeId index);
  //删除节点并释放节点内存
  bool RemoveNode(NodeId index);
  //合并两个节点，合并成功会删除index_src节点
  bool MergeNodesWithManager(NodeId index_dst, NodeId index_src,
                             const std::function<bool(T&, T&)>& merge_function =
                                 DefaultMergeFunction2);
  //合并两个节点，合并成功会删除index_src节点
  template <class Manager>
  bool MergeNodesWithManager(NodeId index_dst, NodeId index_src,
                             Manager& manager,
                             const std::function<bool(T&, T&, Manager&)>&
                                 merge_function = DefaultMergeFunction3);
  void Swap(NodeManager& manager_other);

  bool SetNodeMergeAllowed(NodeId index);
  bool SetNodeMergeRefused(NodeId index);
  void SetAllNodesMergeAllowed();
  void SetAllNodesMergeRefused();

  inline bool CanMerge(NodeId index) {
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

  T* operator[](NodeId index);
  Iterator End() { return Iterator(this, nodes_.size()); }
  Iterator Begin();

 private:
  friend class Iterator;
  std::vector<T*> nodes_;
  //优化用数组，存放被删除节点对应的ID
  std::vector<NodeId> removed_ids_;
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
inline T* NodeManager<T>::GetNode(NodeId index) {
  if (index > nodes_.size()) {
    return nullptr;
  } else {
    return nodes_[index];
  }
}

template <class T>
inline bool NodeManager<T>::RemoveNode(NodeId index) {
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
inline T* NodeManager<T>::RemovePointer(NodeId index) {
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
inline NodeManager<T>::NodeId NodeManager<T>::EmplaceNode(Args&&... args) {
  NodeId index = -1;
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
inline NodeManager<T>::NodeId NodeManager<T>::EmplaceNodeIndex(NodeId index,
                                                               Args&&... args) {
  T* pointer = new T(std::forward<Args>(args)...);
  bool result = EmplacePointerIndex(index, pointer);
  if (!result) {
    delete pointer;
  }
  return result;
}

template <class T>
inline NodeManager<T>::NodeId NodeManager<T>::EmplacePointerIndex(NodeId index,
                                                                  T* pointer) {
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
inline bool NodeManager<T>::SetNodeMergeAllowed(NodeId index) {
  if (index > nodes_.size() || nodes_can_merge[index] == nullptr) {
    return false;
  }
  nodes_can_merge[index] = true;
  return true;
}

template <class T>
inline bool NodeManager<T>::SetNodeMergeRefused(NodeId index) {
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
    NodeId index_dst, NodeId index_src,
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
    typename NodeManager<T>::NodeId index_dst,
    typename NodeManager<T>::NodeId index_src, Manager& manager,
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
  nodes_.shrink_to_fit();
  removed_ids_.shrink_to_fit();
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

template <class T>
inline T* NodeManager<T>::operator[](size_t index) {
  if (index >= nodes_.size()) {
    throw std::invalid_argument("index越界");
  }
  return nodes_[index];
}

template <class T>
inline void NodeManager<T>::Iterator::SetIndex(size_t index) {
  if (manager_pointer_ == nullptr) {
    throw std::runtime_error("未绑定NodeManager");
  }
  if (index > manager_pointer_->nodes_.size()) {
    throw std::invalid_argument("index越界");
  }
  index_ = index;
}

template <class T>
inline NodeManager<T>::Iterator NodeManager<T>::Begin() {
  size_t index = 0;
  while (index != nodes_.size() && GetNode(index) == nullptr) {
    ++index;
  }
  return Iterator(this, index);
}

template <class T>
inline NodeManager<T>::Iterator& NodeManager<T>::Iterator::operator++() {
  size_t index_temp = index_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    ++index_temp;
  } while (index_temp < nodes.size() && nodes[index_temp] == nullptr);
  if (index_temp >= nodes.size()) {
    throw std::runtime_error("该节点后无非空节点");
  } else {
    index_ = index_temp;
    return *this;
  }
}

template <class T>
inline NodeManager<T>::Iterator NodeManager<T>::Iterator::operator++(int) {
  size_t index_temp = index_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    ++index_temp;
  } while (index_temp < nodes.size() && nodes[index_temp] == nullptr);
  if (index_temp >= nodes.size()) {
    throw std::runtime_error("该节点后无非空节点");
  }
  std::swap(index_temp, index_);
  return Iterator(manager_pointer_, index_temp);
}

template <class T>
inline NodeManager<T>::Iterator& NodeManager<T>::Iterator::operator--() {
  size_t index_temp = index_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    --index_temp;
  } while (index_temp != -1 && nodes[index_temp] == nullptr);
  if (index_temp == -1) {
    throw std::runtime_error("该节点前无非空节点");
  } else {
    index_ = index_temp;
    return *this;
  }
}

template <class T>
inline NodeManager<T>::Iterator NodeManager<T>::Iterator::operator--(int) {
  size_t index_temp = index_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    --index_temp;
  } while (index_temp != -1 && nodes[index_temp] == nullptr);
  if (index_temp == -1) {
    throw std::runtime_error("该节点前无非空节点");
  } else {
    std::swap(index_temp, index_);
    return Iterator(manager_pointer_, index_temp);
  }
}

template <class T>
inline T& NodeManager<T>::Iterator::operator*() {
  if (index_ >= manager_pointer_->nodes_.size()) {
    throw std::runtime_error("无效解引用");
  }
  return *manager_pointer_->nodes_[index_];
}

template <class T>
inline T* NodeManager<T>::Iterator::operator->() {
  if (index_ >= manager_pointer_->nodes_.size()) {
    throw std::runtime_error("无效解引用");
  }
  return manager_pointer_->nodes_[index_];
}

}  // namespace common
#endif  // !COMMON_COMMON_NODE_MANAGER