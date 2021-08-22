#ifndef COMMON_UNORDERED_STRUCT_MANAGER_H_
#define COMMON_UNORDERED_STRUCT_MANAGER_H_

#include <boost/serialization/unordered_map.hpp>
#include <functional>

#include "Common/id_wrapper.h"
#include "Common/object_manager.h"

namespace frontend::common {

template <class T>
struct DefaultHasher {
  size_t operator()(const T& object) {
    size_t result = 1;
    for (auto x : object) {
      result *= x;
    }
    return result;
  }
};

// TODO 设置使用std::string时的特化
// 该类用于建立stl未提供hash方法的结构的哈希存储

template <class StructType, class Hasher = DefaultHasher<StructType>>
class UnorderedStructManager {
 public:
  enum class WrapperLabel { kObjectHashType };
  using ObjectId = ObjectManager<StructType>::ObjectId;
  using ObjectHashType =
      ExplicitIdWrapper<size_t, WrapperLabel, WrapperLabel::kObjectHashType>;

  UnorderedStructManager() {}
  ~UnorderedStructManager() {}

  // 返回指向管理的对象的引用
  StructType& GetObject(ObjectId production_node_id) {
    return node_manager_.GetObject(production_node_id);
  }
  // 返回值前半部分为对象ID，后半部分为是否执行了插入操作
  template <class T>
  std::pair<ObjectId, bool> AddObject(T&& object);
  ObjectId GetObjectId(const StructType& object);
  bool RemoveObject(const StructType& object);
  // 初始化，如果容器中存在对象则全部释放
  void StructManagerInit() {
    node_manager_.ObjectManagerInit();
    hash_to_id_.clear();
  }
  void ShrinkToFit() { node_manager_.ShrinkToFit(); }
  std::string& operator[](ObjectId production_node_id) {
    return GetObject(production_node_id);
  }

 private:
  ObjectHashType DoHash(const StructType& object) {
    Hasher hasher;
    return ObjectHashType(hasher(object));
  }

  ObjectManager<StructType> node_manager_;
  std::unordered_multimap<ObjectHashType, ObjectId> hash_to_id_;
};

template <class StructType, class Hasher>
template <class T>
inline std::pair<typename UnorderedStructManager<StructType, Hasher>::ObjectId,
                 bool>
UnorderedStructManager<StructType, Hasher>::AddObject(T&& object) {
  ObjectHashType hashed_object(DoHash(object));
  ObjectId production_node_id = GetObjectId(object);
  if (production_node_id == ObjectId::InvalidId()) {
    production_node_id = node_manager_.EmplaceObject(std::forward<T>(object));
    hash_to_id_.insert(std::make_pair(hashed_object, production_node_id));
    return std::make_pair(production_node_id, true);
  } else {
    return std::make_pair(production_node_id, false);
  }
}

template <class StructType, class Hasher>
UnorderedStructManager<StructType, Hasher>::ObjectId
UnorderedStructManager<StructType, Hasher>::GetObjectId(
    const StructType& object) {
  ObjectHashType hashed_string(DoHash(object));
  auto [iter_begin, iter_end] = hash_to_id_.equal_range(hashed_string);
  ObjectId return_id = ObjectId::InvalidId();
  while (iter_begin != iter_end) {
    if (node_manager_.GetObject(iter_begin->second) == object) {
      return_id = iter_begin->second;
    }
    ++iter_begin;
  }
  return return_id;
}

template <class StructType, class Hasher>
bool UnorderedStructManager<StructType, Hasher>::RemoveObject(
    const StructType& object) {
  ObjectId production_node_id = GetObjectId(object);
  if (production_node_id != ObjectId::InvalidId()) {
    auto [iter_begin, iter_end] = hash_to_id_.equal_range(DoHash(object));
    while (iter_begin != iter_end) {
      if (node_manager_.GetObject(iter_begin->second) == object) {
        hash_to_id_.erase(iter_begin);
        break;
      }
      ++iter_begin;
    }
    return node_manager_.RemoveNode(production_node_id);
  }
  return true;
}

}  // namespace frontend::common
#endif  // !COMMON_UNORDERED_STRUCT_MANAGER