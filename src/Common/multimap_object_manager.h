/// @file multimap_object_manager.h
/// @brief 管理对象，支持对象的合并，允许多个ID对应一个对象
/// @details
/// 该文件定义的对象管理类与ObjectManager区别在于ObjectManagerID与对象一一对应
/// 多个MultimapObjectManagerID可以对应一个对象
/// @attention 由于使用了ObjectManager，使用该头文件时需要与boost库链接
#ifndef COMMON_MULTIMAP_OBJECT_MANAGER_H_
#define COMMON_MULTIMAP_OBJECT_MANAGER_H_

#include <assert.h>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "Common/id_wrapper.h"
#include "Common/object_manager.h"

namespace frontend::common {

/// @class MultimapObjectManager multimap_object_manager.h
/// @brief 管理对象，允许多个ID对应一个对象
/// @details
/// 在ObjectManager基础上支持对象的合并功能，并能够自动更新指向该对象的ID指向
/// 新的对象；ID对应的对象不曾参与合并过程或合并失败时保证ID与对象一一对应
template <class T>
class MultimapObjectManager {
 private:
  /// @brief ObjectManager中使用的ID（内部ID）
  using InsideId = ObjectManager<T>::ObjectId;

 public:
  /// @brief 用来生成MultimapObjectManagerID的枚举
  enum class WrapperLabel { kObjectId };
  /// @brief MultimapObjectManagerID的类型（外部ID）
  using ObjectId =
      ExplicitIdWrapper<size_t, WrapperLabel, WrapperLabel::kObjectId>;
  /// @brief 该容器的迭代器
  using Iterator = ObjectManager<T>::Iterator;
  /// @brief 该容器的const迭代器
  using ConstIterator = ObjectManager<T>::ConstIterator;

  MultimapObjectManager() {}
  MultimapObjectManager(const MultimapObjectManager&) = delete;
  MultimapObjectManager(MultimapObjectManager&&) = delete;
  ~MultimapObjectManager() {}

  /// @brief 获取指向有效对象的引用
  /// @param[in] object_id ：要获取的对象的ID
  /// @return 返回获取到的对象的引用
  T& GetObject(ObjectId object_id);
  /// @brief 获取指向有效对象的const引用
  /// @param[in] object_id ：要获取的对象的ID
  /// @return 返回获取到的对象的const引用
  /// @attention 入参对应的ID必须存在
  const T& GetObject(ObjectId object_id) const;
  /// @brief 通过一个id查询引用相同底层对象的所有id
  /// @param[in] object_id ：MultimapObjectManagerID
  /// @return 返回所有指向该对象的ID
  const std::unordered_set<ObjectId>& GetIdsReferringSameObject(
      ObjectId object_id) const {
    InsideId inside_id = GetInsideId(object_id);
    return GetIdsReferringSameObject(inside_id);
  }

  /// @brief 判断两个ID是否指向同一对象
  /// @param[in] id1 ：一个要判断的ID
  /// @param[in] id2 ：另一个要判断的ID
  /// @note 允许使用相同入参
  bool IsSame(ObjectId id1, ObjectId id2) const;

  /// @brief 创建对象并自动选择最佳位置放置对象
  /// @param[in] args ：对象构建所需要的所有参数
  /// @return 返回创建的对象的ID
  template <class... Args>
  ObjectId EmplaceObject(Args&&... args);
  /// @details 仅删除对象记录但不释放指针指向的内存
  /// @param[in] object_id ：要释放的对象ID
  /// @return 返回指向对象的指针
  /// @attention 必须保证对象存在，不可以忽略返回值，否则会导致内存泄漏
  [[nodiscard]] T* RemoveObjectNoDelete(ObjectId object_id);
  /// @details 删除对象并释放对象内存
  /// @param[in] object_id ：要删除的对象的ID
  /// @retval true ：一定返回true
  /// @attention 要删除的对象必须存在
  bool RemoveObject(ObjectId object_id);
  /// @brief 将id_src对应的节点合并到id_dst对应的节点
  /// @param[in] id_dst ：合并后保留的节点的ID
  /// @param[in] id_src ：合并后删除的节点的ID
  /// @param[in] merge_function ：用来控制合并过程的函数
  /// 第一个参数为id_dst对应的对象，第二个参数为id_src对应的对象
  /// @return 返回是否合并成功
  /// @retval ：true 合并成功
  /// @retval ：false 合并同一个对象或merge_function返回了false
  /// @details
  /// 如果合并成功则删除id_src对应的节点并重映射id_src指向id_dst指向的节点
  /// 如果合并同一个对象或merge_function返回了false则不会删除节点也不会重映射
  /// @attention
  /// 合并同一个对象时不调用merge_function直接返回false
  bool MergeObjects(ObjectId id_dst, ObjectId id_src,
                    std::function<bool(T&, T&)> merge_function =
                        ObjectManager<T>::DefaultMergeFunction2);
  /// @brief 将id_src对应的节点合并到id_dst对应的节点，允许管理器参与
  /// @param[in] id_dst ：合并后保留的节点的ID
  /// @param[in] id_src ：合并后删除的节点的ID
  /// @param[in,out] manager ：管理合并的对象，在调用merge_function时传入
  /// @param[in] merge_function ：用来控制合并过程的函数
  /// 第一个参数为id_dst对应的对象，第二个参数为id_src对应的对象
  /// 第三个参数为管理合并过程的管理器
  /// @return 返回是否合并成功
  /// @retval ：true 合并成功
  /// @retval ：false 合并同一个对象或merge_function返回了false
  /// @details
  /// 如果合并成功则删除id_src对应的节点并重映射id_src指向id_dst指向的节点
  /// 如果合并同一个对象或merge_function返回了false则不会删除节点也不会重映射
  /// @attention
  /// 合并同一个对象时不调用merge_function直接返回false
  template <class Manager>
  bool MergeObjectsWithManager(
      ObjectId id_dst, ObjectId id_src, Manager& manager,
      const std::function<bool(T&, T&, Manager&)>& merge_function =
          ObjectManager<T>::DefaultMergeFunction3);
  /// @brief 交换两个类的内容
  /// @param[in,out] manager_other ：指向另一个管理器的指针
  /// @note 允许传入this，但是会执行完整的交换流程
  /// @attention 不允许使用空指针
  void Swap(MultimapObjectManager* manager_other);
  /// @brief 设置对象允许合并标志
  /// @param[in] object_id ：要设置允许合并的对象的ID
  /// @attention 入参必须对应已存在的对象
  bool SetObjectCanBeSourceInMerge(ObjectId object_id);
  /// @brief 设置对象禁止合并标志
  /// @param[in] object_id ：要设置禁止合并的对象的ID
  /// @attention 入参必须对应已存在的对象
  bool SetObjectCanNotBeSourceInMerge(ObjectId object_id);
  /// @brief 设置所有对象允许合并
  /// @attention 不会对未分配对象的空穴设置允许标记
  void SetAllObjectsCanBeSourceInMerge() {
    node_manager_.SetAllObjectsCanBeSourceInMerge();
  }
  /// @brief 设置所有对象禁止合并
  /// @note 所有对象（包括未分配对象的空穴都会被设置）
  void SetAllObjectsCanNotBeSourceInMerge() {
    node_manager_.SetAllObjectsCanNotBeSourceInMerge();
  }
  /// @brief 返回入参对应对象是否可合并
  /// @param[in] object_id ：需要查询是否可合并的对象
  /// @attention 入参必须对应存在的对象
  bool CanBeSourceInMerge(ObjectId object_id) const;

  /// @brief 初始化，如果容器中存在对象则全部释放
  void MultimapObjectManagerInit();
  /// @brief 清除所有对象但不释放
  /// @note 仅清除成员，不会释放成员占用的内存
  void ClearNoRelease();
  /// @brief 调用成员变量的shrink_to_fit降低内存使用量
  void ShrinkToFit();

  /// @brief 获取容器能存储的对象个数
  /// @return 返回容器能存储的对象个数
  size_t Capacity() const { return node_manager_.Size(); }
  /// @brief 返回容器中实际存储的有效对象个数
  /// @return 返回容器中实际存储的有效对象个数
  size_t ItemSize() const { return node_manager_.ItemSize(); }

  /// @brief 序列化容器
  /// @param[in] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  template <class Archive>
  void Serialize(Archive& ar, const unsigned int version = 0);

  /// @brief 获取指向容器第一个对象的迭代器
  /// @return 返回指向容器第一个对象的迭代器
  /// @note 如果第一个对象不存在则返回End()
  Iterator Begin() const { return node_manager_.Begin(); }
  /// @brief 返回指向容器超尾的迭代器
  Iterator End() const { return node_manager_.End(); }
  /// @brief 获取指向超尾的const迭代器
  /// @return 返回指向超尾的const迭代器
  ConstIterator ConstEnd() const { return node_manager_.ConstEnd(); }
  /// @brief 获取指向第一个有效节点的const迭代器
  /// @return 返回指向第一个有效节点的const迭代器
  /// @attention 如果不存在有效节点则返回ConstEnd()
  ConstIterator ConstBegin() const { return node_manager_.ConstBegin(); }

  /// @brief 使用对象的ID获取对象引用
  /// @param[in] object_id ：要获取引用的对象ID
  /// @return 返回获取到的对象引用
  /// @attention 入参对应的对象必须存在
  T& operator[](ObjectId object_id);
  /// @brief 适用对象的ID获取对象const引用
  /// @param[in] object_id ：要获取const引用的对象ID
  /// @return 返回获取到的对象引用
  /// @attention 入参对应的对象必须存在
  const T& operator[](ObjectId object_id) const;

 private:
  /// @brief 通过一个ID查询所有引用相同底层对象的ID（包含自身）
  /// @param[in] inside_id ：node_manager_使用的ID
  /// @return 返回存储所有引用相同底层对象的ID的集合
  const std::unordered_set<ObjectId>& GetIdsReferringSameObject(
      InsideId inside_id) const {
    return objectids_referring_same_object.at(inside_id);
  }
  /// @brief 将object_id指向的对象的内部ID重映射为inside_id指向的对象
  /// @param[in] object_id ：要重映射的外部ID
  /// @param[in] inside_id ：新内部ID
  /// @note 可以接受未映射的object_id
  /// @attention inside_id必须指向存在的对象
  bool RemapId(ObjectId object_id, InsideId inside_id);
  /// @brief 生成一个未被使用的外部ID
  /// @return 返回生成的外部ID
  /// @attention 如果超出了数据类型最大存储能力则会回环
  ObjectId CreateId();
  /// @brief 增加一条外部ID对内部对象的引用记录
  /// @param[in] object_id ：使用的外部ID
  /// @param[in] inside_id ：指向有效对象的内部ID
  /// @return 返回增加后inside_id指向的对象的外部ID引用计数
  /// @attention inside_id必须指向存在的对象
  size_t AddReference(ObjectId object_id, InsideId inside_id);
  /// @brief 移除一条外部ID对内部对象的引用记录
  /// @param[in] object_id ：使用的外部ID
  /// @param[out] inside_id ：指向有效对象的内部ID
  /// @return 返回删除引用后对象的引用计数
  /// @note 允许传入未引用该内部对象的外部ID
  /// @attention inside_id必须指向存在的对象，引用计数为0时会自动删除该对象
  size_t RemoveReference(ObjectId object_id, InsideId inside_id);
  /// @brief 处理移除内部对象前的步骤，如删除所有句柄到底层ID的引用
  /// @param[in] inside_id ：要移除的对象的内部ID
  /// @return 一定返回true
  /// @attention 该函数不会移除底层对象
  bool PreRemoveInsideObject(InsideId inside_id);
  /// @brief 获取外部ID对应的内部ID
  /// @param[in] object_id ：外部ID
  /// @return 返回外部ID对应的内部ID
  /// @attention 外部ID必须指向存在的对象
  InsideId GetInsideId(ObjectId object_id) const;

  /// @brief 下一个id序号值，只增不减
  ObjectId next_id_index_ = ObjectId(0);
  /// @brief 真正管理对象的容器
  ObjectManager<T> node_manager_;
  /// @brief 储存引用内部ID对应对象的外部ID
  std::unordered_map<InsideId, std::unordered_set<ObjectId>>
      objectids_referring_same_object;
  /// @brief 存储外部ID到内部ID的映射
  std::unordered_map<ObjectId, InsideId> id_to_index_;
};

template <class T>
inline typename MultimapObjectManager<T>::InsideId
MultimapObjectManager<T>::GetInsideId(ObjectId object_id) const {
  auto iter = id_to_index_.find(object_id);
  assert(iter != id_to_index_.end());
  return iter->second;
}

template <class T>
inline T& MultimapObjectManager<T>::GetObject(ObjectId object_id) {
  InsideId inside_id = GetInsideId(object_id);
  return node_manager_.GetObject(inside_id);
}
template <class T>
inline const T& MultimapObjectManager<T>::GetObject(
    ObjectId object_id) const {
  InsideId inside_id = GetInsideId(object_id);
  return node_manager_.GetObject(inside_id);
}

template <class T>
inline bool MultimapObjectManager<T>::IsSame(ObjectId id1, ObjectId id2) const {
  InsideId inside_id1 = GetInsideId(id1);
  InsideId inside_id2 = GetInsideId(id2);
  return inside_id1 == inside_id2;
}

template <class T>
template <class... Args>
inline MultimapObjectManager<T>::ObjectId
MultimapObjectManager<T>::EmplaceObject(Args&&... args) {
  ObjectId object_id = CreateId();
  InsideId inside_id = node_manager_.EmplaceObject(std::forward<Args>(args)...);
  AddReference(object_id, inside_id);
  id_to_index_[object_id] = inside_id;
  return object_id;
}

template <class T>
inline bool MultimapObjectManager<T>::SetObjectCanBeSourceInMerge(
    ObjectId object_id) {
  InsideId inside_id = GetInsideId(object_id);
  return node_manager_.SetObjectCanBeSourceInMerge(inside_id);
}

template <class T>
inline bool MultimapObjectManager<T>::SetObjectCanNotBeSourceInMerge(
    ObjectId object_id) {
  InsideId inside_id = GetInsideId(object_id);
  return node_manager_.SetObjectCanNotBeSourceInMerge(inside_id);
}

template <class T>
inline bool MultimapObjectManager<T>::CanBeSourceInMerge(
    ObjectId object_id) const {
  InsideId index = GetInsideId(object_id);
  return node_manager_.CanBeSourceInMerge(index);
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
    for (ObjectId object_id : ids_referring_same_inside_id) {
      auto iter = id_to_index_.find(object_id);
      id_to_index_.erase(iter);
    }
    objectids_referring_same_object[inside_id].clear();
  }
  return true;
}

template <class T>
inline T* MultimapObjectManager<T>::RemoveObjectNoDelete(
    ObjectId object_id) {
  InsideId inside_id = GetInsideId(object_id);
  PreRemoveInsideObject(inside_id);
  return node_manager_.RemovePointer(inside_id);
}

template <class T>
inline bool MultimapObjectManager<T>::RemoveObject(
    ObjectId object_id) {
  InsideId inside_id = GetInsideId(object_id);
  PreRemoveInsideObject(inside_id);
  node_manager_.RemoveObject(inside_id);
  return true;
}

template <class T>
inline void MultimapObjectManager<T>::Swap(
    MultimapObjectManager* manager_other) {
  std::swap(next_id_index_, manager_other->next_id_index_);
  node_manager_.Swap(manager_other->node_manager_);
  objectids_referring_same_object.Swap(
      manager_other->objectids_referring_same_object);
  id_to_index_.Swap(manager_other->id_to_index_);
}

template <class T>
inline bool MultimapObjectManager<T>::RemapId(ObjectId object_id,
                                              InsideId inside_id) {
  assert(inside_id < node_manager_.Size());
  InsideId inside_id_old = GetInsideId(object_id);
  AddReference(object_id, inside_id);
  id_to_index_[object_id] = inside_id;
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
    ObjectId object_id, InsideId inside_id) {
  assert(inside_id < node_manager_.Size());
  objectids_referring_same_object[inside_id].insert(object_id);
  return objectids_referring_same_object[inside_id].size();
}

template <class T>
inline size_t MultimapObjectManager<T>::RemoveReference(
    ObjectId object_id, InsideId inside_id) {
  assert(inside_id < node_manager_.Size());
  auto& ref_uset = objectids_referring_same_object[inside_id];
  auto iter = ref_uset.find(object_id);
  if (iter != ref_uset.end()) [[likely]] {
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
inline T& MultimapObjectManager<T>::operator[](ObjectId object_id) {
  return GetObject(object_id);
}
template <class T>
inline const T& MultimapObjectManager<T>::operator[](
    ObjectId object_id) const {
  return GetObject(object_id);
}

}  // namespace frontend::common
#endif  /// !COMMON_COMMON_MULTIMAP_NODE_MANAGER