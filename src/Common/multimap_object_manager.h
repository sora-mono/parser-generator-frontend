#ifndef COMMON_MULTIMAP_OBJECT_MANAGER_H_
#define COMMON_MULTIMAP_OBJECT_MANAGER_H_

#include <assert.h>

#include <algorithm>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/unordered_set.hpp>
#include <stdexcept>

#include "Common/id_wrapper.h"
#include "Common/object_manager.h"

namespace frontend::common {

template <class T>
class MultimapObjectManager {
 private:
  using InsideId = ObjectManager<T>::ObjectId;

 public:
  enum class WrapperLabel { kObjectId };
  using ObjectId =
      ExplicitIdWrapper<size_t, WrapperLabel, WrapperLabel::kObjectId>;
  using Iterator = ObjectManager<T>::Iterator;

  MultimapObjectManager() {}
  MultimapObjectManager(const MultimapObjectManager&) = delete;
  MultimapObjectManager(MultimapObjectManager&&) = delete;
  ~MultimapObjectManager() {}
  // 获取指向有效对象的引用
  T& GetObject(ObjectId production_node_id);
  // 通过一个id查询引用相同底层对象的所有id
  const std::unordered_set<ObjectId>& GetIdsReferringSameObject(
      ObjectId object_id) {
    InsideId inside_id = GetInsideId(object_id);
    return GetIdsReferringSameObject(inside_id);
  }

  bool IsSame(ObjectId id1, ObjectId id2);

  // 系统自行选择最佳位置放置对象
  template <class... Args>
  ObjectId EmplaceObject(Args&&... args);
  // 仅删除对象不释放指针指向的内存，返回管理的指针
  T* RemoveObjectNoDelete(ObjectId production_node_id);
  // 删除对象并释放对象内存
  bool RemoveObject(ObjectId production_node_id);
  // 将id_src合并到id_dst，
  // 合并成功则删除id_src对应的对象（二者不同前提下）
  bool MergeObjects(ObjectId id_dst, ObjectId id_src,
                    std::function<bool(T&, T&)> merge_function =
                        ObjectManager<T>::DefaultMergeFunction2);
  template <class Manager>
  bool MergeObjectsWithManager(
      ObjectId id_dst, ObjectId id_src, Manager& manager,
      const std::function<bool(T&, T&, Manager&)>& merge_function =
          ObjectManager<T>::DefaultMergeFunction3);
  // 交换两个类的内容
  void Swap(MultimapObjectManager& manager_other);
  // 设置对象允许合并标志
  bool SetObjectMergeAllowed(ObjectId production_node_id);
  // 设置对象禁止合并标志
  bool SetObjectMergeRefused(ObjectId production_node_id);
  // 设置所有对象允许合并，不会对未分配对象设置允许标记
  void SetAllObjectsMergeAllowed() {
    node_manager_.SetAllObjectsMergeAllowed();
  }
  // 设置所有对象禁止合并
  void SetAllObjectsMergeRefused() {
    node_manager_.SetAllObjectsMergeRefused();
  }
  // 返回id对应对象可合并状态
  bool CanMerge(ObjectId production_node_id);

  // 暂时不写，优化释放后的空间太少
  // void remap_optimization();	// 重映射所有的对象，消除node_manager空余空间

  // 初始化，如果容器中存在对象则全部释放
  void MultimapObjectManagerInit();
  // 清除但不释放所有对象
  void ClearNoRelease();
  // 调用成员变量的shrink_to_fit
  void ShrinkToFit();

  // 返回对象个数（包括已删除但仍保留index的对象）
  size_t Size() { return node_manager_.Size(); }
  // 返回实际对象个数
  size_t ItemSize() { return node_manager_.ItemSize(); }

  // 序列化容器用
  template <class Archive>
  void Serialize(Archive& ar, const unsigned int version = 0);

  Iterator Begin() { return node_manager_.Begin(); }
  Iterator End() { return node_manager_.End(); }

  T& operator[](ObjectId production_node_id);

 private:
  // 通过一个id查询引用相同底层对象的所有id
  const std::unordered_set<ObjectId>& GetIdsReferringSameObject(
      InsideId inside_id) {
    return objectids_referring_same_object[inside_id];
  }
  // 将id指向的实际对象重映射为inside_id指向的对象
  // 可以接受未映射的id
  bool RemapId(ObjectId production_node_id, InsideId inside_id);
  // 生成一个可用于分配的ID
  ObjectId CreateId();
  // 增加一条该id对inside_id对象的引用记录，返回增加后引用计数
  size_t AddReference(ObjectId production_node_id, InsideId inside_id);
  // 移除该id对inside_id对象的引用记录，返回移除后引用计数
  // 引用计数为0时会自动删除对象
  size_t RemoveReference(ObjectId production_node_id, InsideId inside_id);
  // 处理移除内部对象前的步骤，如删除所有句柄到底层ID的引用
  bool PreRemoveInsideObject(InsideId inside_id);
  // 获取句柄对应的底层对象ID
  InsideId GetInsideId(ObjectId production_node_id);

  // 下一个id序号值，只增不减
  ObjectId next_id_index_ = ObjectId(0);
  ObjectManager<T> node_manager_;
  // 储存指向同一个底层对象的id
  std::unordered_map<InsideId, std::unordered_set<ObjectId>>
      objectids_referring_same_object;
  // 对底层id的另一层封装，使本类支持多个id指向同一个对象
  std::unordered_map<ObjectId, InsideId> id_to_index_;
};

template <class T>
inline typename MultimapObjectManager<T>::InsideId
MultimapObjectManager<T>::GetInsideId(ObjectId production_node_id) {
  auto iter = id_to_index_.find(production_node_id);
  assert(iter != id_to_index_.end());
  return iter->second;
}

template <class T>
inline T& MultimapObjectManager<T>::GetObject(ObjectId production_node_id) {
  InsideId inside_id = GetInsideId(production_node_id);
  return node_manager_.GetObject(inside_id);
}

template <class T>
inline bool MultimapObjectManager<T>::IsSame(ObjectId id1, ObjectId id2) {
  InsideId inside_id1 = GetInsideId(id1);
  InsideId inside_id2 = GetInsideId(id2);
  return inside_id1 == inside_id2;
}

template <class T>
template <class... Args>
inline MultimapObjectManager<T>::ObjectId
MultimapObjectManager<T>::EmplaceObject(Args&&... args) {
  ObjectId production_node_id = CreateId();
  InsideId inside_id = node_manager_.EmplaceObject(std::forward<Args>(args)...);
  AddReference(production_node_id, inside_id);
  id_to_index_[production_node_id] = inside_id;
  return production_node_id;
}

template <class T>
inline bool MultimapObjectManager<T>::SetObjectMergeAllowed(
    ObjectId production_node_id) {
  InsideId inside_id = GetInsideId(production_node_id);
  return node_manager_.SetObjectMergeAllowed(inside_id);
}

template <class T>
inline bool MultimapObjectManager<T>::SetObjectMergeRefused(
    ObjectId production_node_id) {
  InsideId inside_id = GetInsideId(production_node_id);
  return node_manager_.SetObjectMergeRefused(inside_id);
}

template <class T>
inline bool MultimapObjectManager<T>::CanMerge(ObjectId production_node_id) {
  InsideId index = GetInsideId(production_node_id);
  return node_manager_.CanMerge(index);
}

template <class T>
inline bool MultimapObjectManager<T>::MergeObjects(
    ObjectId id_dst, ObjectId id_src,
    std::function<bool(T&, T&)> merge_function) {
  InsideId inside_id_dst = GetInsideId(id_dst);
  InsideId inside_id_src = GetInsideId(id_src);
  if (!node_manager_.MergeObjects(inside_id_dst, inside_id_src,
                                  merge_function)) {
    return false;
  }
  RemapId(id_src, inside_id_dst);
  return true;
}

template <class T>
template <class Manager>
inline bool MultimapObjectManager<T>::MergeObjectsWithManager(
    typename MultimapObjectManager<T>::ObjectId id_dst,
    typename MultimapObjectManager<T>::ObjectId id_src, Manager& manager,
    const std::function<bool(T&, T&, Manager&)>& merge_function) {
  InsideId inside_id_dst = GetInsideId(id_dst);
  InsideId inside_id_src = GetInsideId(id_src);
  if (!node_manager_.MergeObjectsWithManager(inside_id_dst, inside_id_src,
                                             manager, merge_function)) {
    return false;
  }
  RemapId(id_src, inside_id_dst);
  return true;
}

template <class T>
inline bool MultimapObjectManager<T>::PreRemoveInsideObject(
    InsideId inside_id) {
  assert(inside_id < node_manager_.Size());
  const auto& ids_referring_same_inside_id =
      GetIdsReferringSameObject(inside_id);
  if (ids_referring_same_inside_id.size() != 0) {
    for (ObjectId production_node_id : ids_referring_same_inside_id) {
      auto iter = id_to_index_.find(production_node_id);
      id_to_index_.erase(iter);
    }
    objectids_referring_same_object[inside_id].clear();
  }
  return true;
}

template <class T>
inline T* MultimapObjectManager<T>::RemoveObjectNoDelete(
    ObjectId production_node_id) {
  InsideId inside_id = GetInsideId(production_node_id);
  PreRemoveInsideObject(inside_id);
  return node_manager_.RemovePointer(inside_id);
}

template <class T>
inline bool MultimapObjectManager<T>::RemoveObject(
    ObjectId production_node_id) {
  InsideId inside_id = GetInsideId(production_node_id);
  PreRemoveInsideObject(inside_id);
  node_manager_.RemoveObject(inside_id);
  return true;
}

template <class T>
inline void MultimapObjectManager<T>::Swap(
    MultimapObjectManager& manager_other) {
  std::swap(next_id_index_, manager_other.next_id_index_);
  node_manager_.Swap(manager_other.node_manager_);
  objectids_referring_same_object.Swap(
      manager_other.objectids_referring_same_object);
  id_to_index_.Swap(manager_other.id_to_index_);
}

template <class T>
inline bool MultimapObjectManager<T>::RemapId(ObjectId production_node_id,
                                              InsideId inside_id) {
  assert(inside_id < node_manager_.Size());
  InsideId inside_id_old = GetInsideId(production_node_id);
  AddReference(production_node_id, inside_id);
  id_to_index_[production_node_id] = inside_id;
  return true;
}

template <class T>
inline MultimapObjectManager<T>::ObjectId MultimapObjectManager<T>::CreateId() {
  ObjectId return_id = next_id_index_;
  next_id_index_ = ObjectId(return_id + 1);
  return return_id;
}

template <class T>
inline size_t MultimapObjectManager<T>::AddReference(
    ObjectId production_node_id, InsideId inside_id) {
  assert(inside_id < node_manager_.Size());
  objectids_referring_same_object[inside_id].insert(production_node_id);
  return objectids_referring_same_object[inside_id].size();
}

template <class T>
inline size_t MultimapObjectManager<T>::RemoveReference(
    ObjectId production_node_id, InsideId inside_id) {
  assert(inside_id < node_manager_.Size());
  auto& ref_uset = objectids_referring_same_object[inside_id];
  auto iter = ref_uset.find(production_node_id);
  if (iter != ref_uset.end()) {
    if (ref_uset.size() == 1) {
      node_manager_.RemoveObject(inside_id);
    }
    ref_uset.erase(iter);
  }
  return ref_uset.size();
}

template <class T>
inline void MultimapObjectManager<T>::MultimapObjectManagerInit() {
  next_id_index_ = ObjectId(0);
  node_manager_.ObjectManagerInit();
  objectids_referring_same_object.clear();
  id_to_index_.clear();
}

template <class T>
inline void MultimapObjectManager<T>::ClearNoRelease() {
  next_id_index_ = ObjectId(0);
  node_manager_.ClearNoRelease();
  objectids_referring_same_object.clear();
  id_to_index_.clear();
}

template <class T>
inline void MultimapObjectManager<T>::ShrinkToFit() {
  node_manager_.ShrinkToFit();
  objectids_referring_same_object.shrink_to_fit();
}

template <class T>
inline T& MultimapObjectManager<T>::operator[](ObjectId production_node_id) {
  return GetObject(production_node_id);
}

}  // namespace frontend::common
#endif  // !COMMON_COMMON_MULTIMAP_NODE_MANAGER