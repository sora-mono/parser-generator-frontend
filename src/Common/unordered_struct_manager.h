#include <functional>
#include <unordered_map>

#include "Common/node_manager.h"
#include "Common/common.h"

#ifndef COMMON_UNORDERED_STRUCT_MANAGER_H_
#define COMMON_UNORDERED_STRUCT_MANAGER_H_

namespace frontend::common {

template <class T>
struct DefaultHasher {
  size_t DoHash(const T& object) {
    auto iter = object.begin();
    size_t result = 1;
    while (iter != object.end()) {
      result *= *iter;
      ++iter;
    }
    return result;
  }
};
//该类用于建立stl未提供hash方法的结构的哈希存储

template <class StructType, class Hasher = DefaultHasher<StructType>>
class UnorderedStructManager {
 public:
  using NodeId = NodeManager<std::string>::NodeId;
  using ObjectHashType = size_t;

  UnorderedStructManager() {}
  ~UnorderedStructManager() {}

  //返回指向管理的对象的指针
  StructType* GetObjectPtr(NodeId id) { return node_manager_.GetNode(id); }
  //返回值前半部分为对象是否已存在，后半部分为对象ID
  template <class T>
  std::pair<bool, NodeId> AddObject(T&& object);
  NodeId GetObjectId(const StructType& object);
  bool RemoveObject(const StructType& object);
  void Clear() {
    node_manager_.Clear();
    hash_to_id_.clear();
  }
  void ShrinkToFit() { node_manager_.ShrinkToFit(); }
  std::string* operator[](NodeId id) { return GetObjectPtr(id); }

 private:
  NodeManager<StructType> node_manager_;
  std::unordered_multimap<ObjectHashType, NodeId> hash_to_id_;
};

template <class StructType, class Hasher>
template <class T>
inline std::pair<bool,
                 typename UnorderedStructManager<StructType, Hasher>::NodeId>
UnorderedStructManager<StructType, Hasher>::AddObject(T&& object) {
  Hasher hasher;
  ObjectHashType hashed_object = hasher.DoHash(object);
  NodeId id = GetObjectId(object);
  if (id == -1) {
    id = node_manager_.EmplaceNode(std::forward<T>(object));
    hash_to_id_.insert(std::make_pair(hashed_object, id));
    return std::make_pair(false, id);
  } else {
    return std::make_pair(true, id);
  }
}

template <class StructType, class Hasher>
UnorderedStructManager<StructType, Hasher>::NodeId
UnorderedStructManager<StructType, Hasher>::GetObjectId(
    const StructType& object) {
  Hasher hasher;
  ObjectHashType hashed_string = hasher.DoHash(object);
  auto [iter_begin, iter_end] = hash_to_id_.equal_range(hashed_string);
  if (iter_begin == hash_to_id_.end()) {
    return -1;
  } else {
    do {
      if (*node_manager_.GetNode(iter_begin->second) == object) {
        return iter_begin->second;
      }
      ++iter_begin;
    } while (iter_begin != iter_end);
    return -1;
  }
}

template <class StructType, class Hasher>
bool UnorderedStructManager<StructType, Hasher>::RemoveObject(
    const StructType& object) {
  NodeId id = GetObjectId(object);
  if (id != -1) {
    return node_manager_.RemoveNode(id);
  }
  return true;
}

}  // namespace frontend::common
#endif  // !COMMON_UNORDERED_STRUCT_MANAGER