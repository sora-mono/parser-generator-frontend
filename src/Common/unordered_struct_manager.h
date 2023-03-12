/// @file unordered_struct_manager.h
/// @brief 将多元素结构哈希存储的类
/// @note 该类仅支持插入对象和全部清空，不支持删除单个对象
#ifndef COMMON_UNORDERED_STRUCT_MANAGER_H_
#define COMMON_UNORDERED_STRUCT_MANAGER_H_

#include <unordered_set>
#include <vector>

#include "Common/id_wrapper.h"

namespace frontend::common {

/// TODO 设置使用std::string时的特化
/// @brief 建立stl未提供hash方法的结构的哈希存储,通过ID一一标识对象
/// @class UnorderedStructManager unordered_struct_manager.h
/// @tparam StructType ：要管理的对象类型
/// @tparam Hasher ：哈希StructType的函数的包装类
template <class StructType, class Hasher = std::hash<StructType>>
class UnorderedStructManager {
 public:
  enum class WrapperLabel { kObjectHashType };
  /// @brief 标识对象的ID
  using ObjectId = ObjectManager<StructType>::ObjectId;
  /// @brief 对象hash后得到的类型
  using ObjectHashType =
      ExplicitIdWrapper<size_t, WrapperLabel, WrapperLabel::kObjectHashType>;

  UnorderedStructManager() {}
  ~UnorderedStructManager() {}

  /// @brief 返回指向管理的对象的引用
  /// @param[in] object_id ：要获取的对象的ID
  /// @return 返回object_id对应的对象的引用
  /// @attention 入参必须对应存在的对象
  StructType& GetObject(ObjectId object_id) {
    return const_cast<StructType&>(
        static_cast<const UnorderedStructManager&>(*this).GetObject(
            object_id));
  }
  /// @brief 返回指向管理的对象的const引用
  /// @param[in] object_id ：要获取的对象的ID
  /// @return 返回object_id对应的对象的const引用
  /// @attention 入参必须对应存在的对象
  const StructType& GetObject(ObjectId object_id) const {
    return *id_to_object_[object_id];
  }
  /// @brief 插入一个待管理的对象
  /// @param[in,out] args ：构建待管理的对象的参数
  /// @return 前半部分为对象ID，后半部分为是否执行了插入操作
  /// @note 如果提供const左值引用则会复制一份对象存储，
  /// 如果提供右值引用则会移动构造该对象
  /// 如果待插入对象已存在则会返回已存在的对象的ID
  /// @attention 该类不会管理传入的参数，真正管理的是根据传入参数构造的新对象
  template <class... Args>
  std::pair<ObjectId, bool> EmplaceObject(Args&&... args);
  /// @brief 通过指向对象的指针获取对象的ID
  /// @param[in] object_pointer ：指向（存储于node_manager_中的）对象的指针
  /// @return 返回对象的ID
  /// @retval ObjectId::InvalidId() ：对象不存在
  ObjectId GetObjectIdFromObjectPointer(const StructType* object_pointer) const;
  /// @brief 根据对象获取其ID
  /// @param[in] object ：要获取ID的对象
  /// @return 获取到的对象ID
  /// @retval ObjectId::InvalidId() ：该对象不存在
  ObjectId GetObjectIdFromObject(const StructType& object) const;
  /// @brief 初始化容器
  /// @note 如果容器中存在对象则全部释放
  void StructManagerInit() {
    node_manager_.clear();
    id_to_object_.clear();
  }
  /// @brief 根据对象ID获取对象引用
  /// @param[in] object_id ：要获取引用的对象ID
  /// @return 返回获取到的对象引用
  /// @note 语义同GetObject
  /// @attention 入参必须对应存在的对象
  StructType& operator[](ObjectId object_id) {
    return GetObject(object_id);
  }
  /// @brief 根据对象ID获取对象const引用
  /// @param[in] object_id ：要获取引用的对象ID
  /// @return 返回获取到的对象const引用
  /// @note 语义同GetObject
  /// @attention 入参必须对应存在的对象
  const StructType& operator[](ObjectId object_id) const {
    return GetObject(object_id);
  }

 private:
  /// @brief 存储所有管理的对象
  std::unordered_set<StructType, Hasher> node_manager_;
  /// @brief 存储管理的对象的地址到ID的映射
  std::unordered_map<const StructType*, ObjectId> object_pointer_to_id_;
  /// @brief 存储ID到指向管理的对象的迭代器映射
  std::vector<typename std::unordered_set<StructType, Hasher>::iterator>
      id_to_object_;
};

template <class StructType, class Hasher>
template <class... Args>
inline std::pair<typename UnorderedStructManager<StructType, Hasher>::ObjectId,
                 bool>
UnorderedStructManager<StructType, Hasher>::EmplaceObject(Args&&... args) {
  auto [iter, inserted] = node_manager_.emplace(std::forward<Args>(args)...);
  if (!inserted) [[unlikely]] {
    return std::make_pair(GetObjectIdFromObjectPointer(&*iter), false);
  } else {
    ObjectId object_id(id_to_object_.size());
    id_to_object_.emplace_back(std::move(iter));
    object_pointer_to_id_.emplace(&*iter, object_id);
    return std::make_pair(object_id, true);
  }
}

template <class StructType, class Hasher>
UnorderedStructManager<StructType, Hasher>::ObjectId
UnorderedStructManager<StructType, Hasher>::GetObjectIdFromObject(
    const StructType& object) const {
  auto iter = node_manager_.find(object);
  if (iter == node_manager_.end()) {
    return ObjectId::InvalidId();
  } else {
    return GetObjectIdFromObjectPointer(&*iter);
  }
}
template <class StructType, class Hasher>
inline UnorderedStructManager<StructType, Hasher>::ObjectId
UnorderedStructManager<StructType, Hasher>::GetObjectIdFromObjectPointer(
    const StructType* object_pointer) const {
  auto iter = object_pointer_to_id_.find(object_pointer);
  if (iter != object_pointer_to_id_.end()) {
    return iter->second;
  } else {
    return ObjectId::InvalidId();
  }
}

}  // namespace frontend::common
#endif  /// !COMMON_UNORDERED_STRUCT_MANAGER