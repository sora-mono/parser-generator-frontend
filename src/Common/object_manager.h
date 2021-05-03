#include <assert.h>

#include <functional>
#include <stdexcept>
#include <vector>

#include "Common/id_wrapper.h"

#ifndef COMMON_OBJECT_MANAGER_H_
#define COMMON_OBJECT_MANAGER_H_
//TODO 给可以加const的内容加const
namespace frontend::common {

template <class T>
class ObjectManager {
 public:
  enum class WrapperLabel { kObjectId };
  using ObjectId =
      ExplicitIdWrapper<size_t, WrapperLabel, WrapperLabel::kObjectId>;

  ObjectManager() {}
  ObjectManager(const ObjectManager&) = delete;
  ObjectManager(ObjectManager&&) = delete;
  ~ObjectManager();

  class Iterator {
   public:
    Iterator() : manager_pointer_(nullptr), id_(ObjectId::InvalidId()) {}
    Iterator(ObjectManager<T>* manager_pointer, ObjectId id)
        : manager_pointer_(manager_pointer), id_(id) {}
    Iterator(const Iterator& iter)
        : manager_pointer_(iter.manager_pointer_), id_(iter.id_) {}

    ObjectId GetId() { return id_; }
    void SetManagerPointer(const ObjectManager<T>* manager_pointer) {
      manager_pointer_ = manager_pointer;
    }
    void SetId(ObjectId id);

    Iterator& operator++();
    Iterator operator++(int);
    Iterator& operator--();
    Iterator operator--(int);
    T& operator*();
    T* operator->();
    bool operator==(const Iterator& iter) {
      return manager_pointer_ == iter.manager_pointer_ && id_ == iter.id_;
    }
    bool operator!=(const Iterator& iter) { return !this->operator==(iter); }

   private:
    ObjectManager<T>* manager_pointer_;
    ObjectId id_;
  };

  static bool DefaultMergeFunction2(T& node_dst, T& node_src) {
    return node_dst.MergeNode(node_src);
  }
  template <class Manager>
  static bool DefaultMergeFunction3(T& node_dst, T& node_src,
                                    Manager& manager) {
    return manager.MergeNodes(node_dst, node_src);
  }
  //获取节点引用
  T& GetObject(ObjectId id);
  T& operator[](ObjectId id);


  //系统自行选择最佳位置放置对象
  template <class ObjectType = T, class... Args>
  ObjectId EmplaceObject(Args&&... args);

  //仅删除节点不释放节点内存，返回指向对象的指针
  T* RemoveObjectNoDelete(ObjectId id);
  //删除节点并释放节点内存
  bool RemoveObject(ObjectId id);

  //合并两个对象，合并成功会删除id_src节点
  bool MergeObjects(ObjectId id_dst, ObjectId id_src,
                    const std::function<bool(T&, T&)>& merge_function =
                        DefaultMergeFunction2);
  //合并两个对象，合并成功会删除id_src节点
  template <class Manager>
  bool MergeObjectsWithManager(ObjectId id_dst, ObjectId id_src,
                               Manager& manager,
                               const std::function<bool(T&, T&, Manager&)>&
                                   merge_function = DefaultMergeFunction3);
  //查询ID对应的节点是否可合并
  bool CanMerge(ObjectId id) {
    assert(id < nodes_can_merge.size());
    return nodes_can_merge[id];
  }

  bool SetObjectMergeAllowed(ObjectId id);
  bool SetObjectMergeRefused(ObjectId id);
  void SetAllObjectsMergeAllowed();
  void SetAllObjectsMergeRefused();

  //判断两个ID是否相等
  bool IsSame(ObjectId id1, ObjectId id2) { return id1 == id2; }

  //交换两个容器
  void Swap(ObjectManager& manager_other);

  //容器大小，包含未分配节点的指针
  size_t Size() { return nodes_.size(); }
  //容器实际持有的对象数量
  size_t ItemSize();

  //清除并释放所有节点
  void Clear();
  //清除但不释放所有节点
  void ClearNoRelease();
  //调用成员变量的shrink_to_fit
  void ShrinkToFit();

  Iterator End() { return Iterator(this, ObjectId(nodes_.size())); }
  Iterator Begin();

 private:
  friend class Iterator;

  //在指定位置放置节点
  template <class ObjectType = T, class... Args>
  ObjectId EmplaceObjectIndex(ObjectId id, Args&&... args);
  //在指定位置放置指针
  ObjectId EmplacePointerIndex(ObjectId id, T* pointer);

  //序列化容器用
  template <class Archive>
  void Serialize(Archive& ar, const unsigned int version = 0);
  //获取当前最佳可用id
  ObjectId GetBestEmptyIndex();
  //添加已移除的id
  void AddRemovedIndex(ObjectId id) { removed_ids_.push_back(id); }

  std::vector<T*> nodes_;
  //优化用数组，存放被删除节点对应的ID
  std::vector<ObjectId> removed_ids_;
  //存储信息表示是否允许合并节点
  std::vector<bool> nodes_can_merge;
};

template <class T>
ObjectManager<T>::~ObjectManager() {
  for (T* ptr : nodes_) {
    delete ptr;
  }
}

template <class T>
inline T& ObjectManager<T>::GetObject(ObjectId id) {
  assert(id < nodes_.size() && nodes_[id] != nullptr);
  return *nodes_[id];
}

template <class T>
inline bool ObjectManager<T>::RemoveObject(ObjectId id) {
  T* removed_pointer = RemoveObjectNoDelete(id);
  delete removed_pointer;
  return true;
}

template <class T>
inline T* ObjectManager<T>::RemoveObjectNoDelete(ObjectId id) {
  assert(id < nodes_.size());
  T* temp_pointer = nodes_[id];
  assert(temp_pointer != nullptr);
  AddRemovedIndex(id);
  nodes_can_merge[id] = false;
  while (nodes_.size() > 0 && nodes_.back() == nullptr) {
    nodes_.pop_back();  //清理末尾无效ID
    nodes_can_merge.pop_back();
  }
  return temp_pointer;
}

template <class T>
template <class ObjectType, class... Args>
inline ObjectManager<T>::ObjectId ObjectManager<T>::EmplaceObject(
    Args&&... args) {
  ObjectId id = GetBestEmptyIndex();
  T* object_pointer = new ObjectType(std::forward<Args>(args)...);
  ObjectId result = EmplacePointerIndex(id, object_pointer);
  assert(result != ObjectId::InvalidId());
  return result;
}

template <class T>
template <class ObjectType, class... Args>
inline ObjectManager<T>::ObjectId ObjectManager<T>::EmplaceObjectIndex(
    ObjectId id, Args&&... args) {
  T* pointer = new ObjectType(std::forward<Args>(args)...);
  ObjectId result = EmplacePointerIndex(id, pointer);
  assert(result != ObjectId::InvalidId());
  return result;
}

template <class T>
inline ObjectManager<T>::ObjectId ObjectManager<T>::EmplacePointerIndex(
    ObjectId id, T* pointer) {
  size_t size_old = nodes_.size();
  if (id >= size_old) {
    nodes_.resize(id + 1, nullptr);
    nodes_can_merge.resize(id + 1, false);
    for (size_t i = size_old; i < id; i++) {
      removed_ids_.push_back(ObjectId(i));
    }
  }
  //不可以覆盖已有非空且不同的指针
  assert(!(nodes_[id] != nullptr && nodes_[id] != pointer));
  nodes_[id] = pointer;
  return id;
}

template <class T>
inline void ObjectManager<T>::Swap(ObjectManager& manager_other) {
  nodes_.swap(manager_other.nodes_);
  removed_ids_.swap(manager_other.removed_ids_);
  nodes_can_merge.swap(manager_other.nodes_can_merge);
}

template <class T>
inline bool ObjectManager<T>::SetObjectMergeAllowed(ObjectId id) {
  assert(id < nodes_.size() && nodes_[id] != nullptr);
  nodes_can_merge[id] = true;
  return true;
}

template <class T>
inline bool ObjectManager<T>::SetObjectMergeRefused(ObjectId id) {
  assert(id < nodes_.size() && nodes_[id] != nullptr);
  nodes_can_merge[id] = false;
  return true;
}

template <class T>
inline void ObjectManager<T>::SetAllObjectsMergeAllowed() {
  for (size_t id = 0; id < nodes_.size(); id++) {
    if (nodes_[id] != nullptr) {
      nodes_can_merge[id] = true;
    }
  }
}

template <class T>
inline void ObjectManager<T>::SetAllObjectsMergeRefused() {
  std::vector<bool> vec_temp(nodes_can_merge.size(), false);
  nodes_can_merge.Swap(vec_temp);
}

template <class T>
inline bool ObjectManager<T>::MergeObjects(
    ObjectId id_dst, ObjectId id_src,
    const std::function<bool(T&, T&)>& merge_function) {
  T& object_dst = GetObject(id_dst);
  T& object_src = GetObject(id_src);
  if (!CanMerge(id_dst) || !CanMerge(id_src)) {
    //两个节点至少有一个不可合并
    return false;
  }
  if (!merge_function(object_dst, object_src)) {
    //合并失败
    return false;
  }
  RemoveObject(id_src);
  return true;
}

template <class T>
template <class Manager>
bool ObjectManager<T>::MergeObjectsWithManager(
    typename ObjectManager<T>::ObjectId id_dst,
    typename ObjectManager<T>::ObjectId id_src, Manager& manager,
    const std::function<bool(T&, T&, Manager&)>& merge_function) {
  T& object_dst = GetObject(id_dst);
  T& object_src = GetObject(id_src);
  if (!CanMerge(id_dst) || !CanMerge(id_src)) {
    //两个节点至少有一个不可合并
    return false;
  }
  if (!merge_function(object_dst, object_src, manager)) {
    //合并失败
    return false;
  }
  RemoveObject(id_src);
  return true;
}

template <class T>
inline void ObjectManager<T>::Clear() {
  for (auto p : nodes_) {
    delete p;
  }
  nodes_.clear();
  removed_ids_.clear();
}

template <class T>
inline void ObjectManager<T>::ClearNoRelease() {
  nodes_.clear();
  removed_ids_.clear();
}

template <class T>
inline void ObjectManager<T>::ShrinkToFit() {
  while (nodes_.size() > 0 && nodes_.back() == nullptr) {
    nodes_.pop_back();
    nodes_can_merge.pop_back();
  }
  nodes_.shrink_to_fit();
  removed_ids_.shrink_to_fit();
}

template <class T>
inline size_t ObjectManager<T>::ItemSize() {
  size_t count = 0;
  for (auto p : nodes_) {
    if (p != nullptr) {
      ++count;
    }
  }
  return count;
}

template <class T>
inline ObjectManager<T>::ObjectId ObjectManager<T>::GetBestEmptyIndex() {
  ObjectId id = ObjectId::InvalidId();
  if (removed_ids_.size() != 0) {
    while (id == ObjectId::InvalidId() &&
           removed_ids_.size() != 0)  //查找有效ID
    {
      if (removed_ids_.back() < nodes_.size()) {
        id = removed_ids_.back();
      }
      removed_ids_.pop_back();
    }
  }
  if (id == ObjectId::InvalidId())  //无有效已删除ID
  {
    id = ObjectId(nodes_.size());
  }
  return id;
}

template <class T>
inline T& ObjectManager<T>::operator[](ObjectId id) {
  assert(id < nodes_.size());
  return *nodes_[id];
}

template <class T>
inline void ObjectManager<T>::Iterator::SetId(ObjectId id) {
  assert(manager_pointer_ != nullptr && id < manager_pointer_->nodes_.size());
  id_ = id;
}

template <class T>
inline ObjectManager<T>::Iterator ObjectManager<T>::Begin() {
  ObjectId id(0);
  while (id <= nodes_.size() && nodes_[id] == nullptr) {
    ++id;
  }
  return Iterator(this, id);
}

template <class T>
inline ObjectManager<T>::Iterator& ObjectManager<T>::Iterator::operator++() {
  ObjectId id_temp = id_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    ++id_temp;
  } while (id_temp < nodes.size() && nodes[id_temp] == nullptr);
  assert(id_temp < nodes.size());
  id_ = id_temp;
  return *this;
}

template <class T>
inline ObjectManager<T>::Iterator ObjectManager<T>::Iterator::operator++(int) {
  ObjectId id_temp = id_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    ++id_temp;
  } while (id_temp < nodes.size() && nodes[id_temp] == nullptr);
  assert(id_temp < nodes.size());
  std::swap(id_temp, id_);
  return Iterator(manager_pointer_, id_temp);
}

template <class T>
inline ObjectManager<T>::Iterator& ObjectManager<T>::Iterator::operator--() {
  ObjectId id_temp = id_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    --id_temp;
  } while (id_temp != ObjectId::InvalidId() && nodes[id_temp] == nullptr);
  assert(id_temp != ObjectId::InvalidId());
  id_ = id_temp;
  return *this;
}

template <class T>
inline ObjectManager<T>::Iterator ObjectManager<T>::Iterator::operator--(int) {
  size_t id_temp = id_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    --id_temp;
  } while (id_temp != ObjectId::InvalidId() && nodes[id_temp] == nullptr);
  assert(id_temp != ObjectId::InvalidId());
  std::swap(id_temp, id_);
  return Iterator(manager_pointer_, id_temp);
}

template <class T>
inline T& ObjectManager<T>::Iterator::operator*() {
  return *manager_pointer_->GetObject(id_);
}

template <class T>
inline T* ObjectManager<T>::Iterator::operator->() {
  return manager_pointer_->GetObject(id_);
}

}  // namespace frontend::common
#endif  // !COMMON_COMMON_NODE_MANAGER