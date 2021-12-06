/// @file object_manager.h
/// @brief 管理对象，通过给定ID标识对象，ID与对象是一一映射的关系
/// @details
/// 该类的设计初衷为了通过整型标识对象，从而可以使用unordered容器来存储标识符提高效率
/// @attention
/// 为了支持序列化而使用了boost::serialization，编译时需要与boost库链接
#ifndef COMMON_OBJECT_MANAGER_H_
#define COMMON_OBJECT_MANAGER_H_

#include <assert.h>

#include <boost/serialization/export.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>

#include "Common/id_wrapper.h"

namespace frontend::common {

/// @class ObjectManager object_manager.h
/// @brief 管理对象，对象与ID一一对应
template <class T>
class ObjectManager {
 public:
  /// @brief 用来定义ObjectId类型的分发标签
  enum class WrapperLabel { kObjectId };
  /// @brief 标识对象的ID，与对象一一对应
  using ObjectId =
      ExplicitIdWrapper<size_t, WrapperLabel, WrapperLabel::kObjectId>;

  ObjectManager() {}
  ObjectManager(const ObjectManager&) = delete;
  ObjectManager(ObjectManager&&) = delete;
  ~ObjectManager();

  /// @class ObjectManager::Iterator object_manager.h
  /// @brief 该容器的迭代器
  class Iterator {
   public:
    Iterator() : manager_pointer_(nullptr), id_(ObjectId::InvalidId()) {}
    Iterator(ObjectManager<T>* manager_pointer, ObjectId id)
        : manager_pointer_(manager_pointer), id_(id) {}
    Iterator(const Iterator& iter)
        : manager_pointer_(iter.manager_pointer_), id_(iter.id_) {}

    /// @brief 获取该迭代器等效的ID
    /// @return 返回该迭代器等效的ID
    /// @note 如果该迭代器为超尾则返回ObjectId::InvalidId()
    ObjectId GetId() const { return id_; }
    /// @brief 设置该迭代器绑定到的容器
    /// @param[in] manager_pointer ：该迭代器绑定到的容器
    /// @attention 不允许输入nullptr
    void SetManagerPointer(ObjectManager<T>* manager_pointer) {
      assert(manager_pointer);
      manager_pointer_ = manager_pointer;
    }
    /// @brief 设置该迭代器等效的ID
    /// @param[in] id ：要设置的ID
    /// @attention 必须使用有效ID，并且已经设置绑定到的容器
    void SetId(ObjectId id);

    /// @brief 向后移动迭代器直至下一个有效对象或超尾，使用前缀++语义
    /// @return 返回移动后的迭代器的引用（*this）
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    /// 不允许对超尾调用该函数
    Iterator& operator++();
    /// @brief 向后移动迭代器直至下一个有效对象或超尾，使用后缀++语义
    /// @return 返回移动前的迭代器
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    /// 不允许对超尾调用该函数
    Iterator operator++(int);
    /// @brief 向前移动迭代器直至上一个有效对象，使用前缀--语义
    /// @return 返回移动后的迭代器的引用（*this）
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    /// 不允许对指向第一个有效对象的迭代器调用该函数
    Iterator& operator--();
    /// @brief 向前移动迭代器直至上一个有效对象，使用后缀--语义
    /// @return 返回移动前的迭代器
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    /// 不允许对指向第一个有效对象的迭代器调用该函数
    Iterator operator--(int);
    /// @brief 对迭代器解引用
    /// @return 返回迭代器指向的对象的引用
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    T& operator*() const;
    /// @brief 对迭代器解引用
    /// @return 返回迭代器指向的对象的指针
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    T* operator->() const;
    /// @brief 判断两个迭代器是否相同（与相同容器绑定且指向相同位置）
    /// @param[in] iter ：待判断的另一个迭代器
    /// @return 两个迭代器是否相同
    /// @retval true ：两个迭代器相同
    /// @retval false ：两个迭代器不同
    /// @note 允许传入*this
    bool operator==(const Iterator& iter) const {
      return manager_pointer_ == iter.manager_pointer_ && id_ == iter.id_;
    }
    /// @brief 判断两个迭代器是否不同（与不同容器绑定或指向不同位置）
    /// @param[in] iter ：待判断的另一个迭代器
    /// @return 两个迭代器是否不同
    /// @retval true ：两个迭代器不同
    /// @retval false ：两个迭代器相同
    /// @note 允许传入*this
    bool operator!=(const Iterator& iter) const {
      return !this->operator==(iter);
    }

   private:
    /// @brief 迭代器绑定到的容器
    ObjectManager<T>* manager_pointer_;
    /// @brief 迭代器指向的对象位置
    ObjectId id_;
  };
  /// @class ObjectManager::ConstIterator object_manager.h
  /// @brief 该容器的const迭代器
  class ConstIterator {
   public:
    ConstIterator() : manager_pointer_(nullptr), id_(ObjectId::InvalidId()) {}
    ConstIterator(const ObjectManager<T>* manager_pointer, ObjectId id)
        : manager_pointer_(manager_pointer), id_(id) {}
    ConstIterator(const Iterator& iterator)
        : manager_pointer_(iterator.manager_pointer), id_(iterator.id_) {}
    ConstIterator(const ConstIterator& iter)
        : manager_pointer_(iter.manager_pointer_), id_(iter.id_) {}

    /// @brief 获取该迭代器等效的ID
    /// @return 返回该迭代器等效的ID
    /// @note 如果该迭代器为超尾则返回ObjectId::InvalidId()
    ObjectId GetId() const { return id_; }
    /// @brief 设置该迭代器绑定到的容器
    /// @param[in] manager_pointer ：该迭代器绑定到的容器
    /// @attention 不允许输入nullptr
    void SetManagerPointer(const ObjectManager<T>* manager_pointer) {
      manager_pointer_ = manager_pointer;
    }
    /// @brief 设置该迭代器等效的ID
    /// @param[in] id ：要设置的ID
    /// @attention 必须使用有效ID，并且已经设置绑定到的容器
    void SetId(ObjectId id) {
      assert(manager_pointer_ != nullptr &&
             id < manager_pointer_->nodes_.size());
      id_ = id;
    }

    /// @brief 向后移动迭代器直至下一个有效对象或超尾，使用前缀++语义
    /// @return 返回移动后的迭代器的引用（*this）
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    /// 不允许对超尾调用该函数
    ConstIterator& operator++();
    /// @brief 向后移动迭代器直至下一个有效对象或超尾，使用后缀++语义
    /// @return 返回移动前的迭代器
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    /// 不允许对超尾调用该函数
    ConstIterator operator++(int);
    /// @brief 向前移动迭代器直至上一个有效对象，使用前缀--语义
    /// @return 返回移动后的迭代器的引用（*this）
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    /// 不允许对指向第一个有效对象的迭代器调用该函数
    ConstIterator& operator--();
    /// @brief 向前移动迭代器直至上一个有效对象，使用后缀--语义
    /// @return 返回移动前的迭代器
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    /// 不允许对指向第一个有效对象的迭代器调用该函数
    ConstIterator operator--(int);
    /// @brief 对迭代器解引用
    /// @return 返回迭代器指向的对象的const引用
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    const T& operator*() const { return manager_pointer_->GetObject(id_); }
    /// @brief 对迭代器解引用
    /// @return 返回迭代器指向的对象的const指针
    /// @attention 调用该函数前必须已经设置等效ID和绑定到的容器
    const T* operator->() const { return &manager_pointer_->GetObject(id_); }
    /// @brief 判断两个迭代器是否相同（与相同容器绑定且指向相同位置）
    /// @param[in] iter ：待判断的另一个迭代器
    /// @return 两个迭代器是否相同
    /// @retval true ：两个迭代器相同
    /// @retval false ：两个迭代器不同
    /// @note 允许传入*this
    bool operator==(const ConstIterator& iter) const {
      return manager_pointer_ == iter.manager_pointer_ && id_ == iter.id_;
    }
    /// @brief 判断两个迭代器是否不同（与不同容器绑定或指向不同位置）
    /// @param[in] iter ：待判断的另一个迭代器
    /// @return 两个迭代器是否不同
    /// @retval true ：两个迭代器不同
    /// @retval false ：两个迭代器相同
    /// @note 允许传入*this
    bool operator!=(const ConstIterator& iter) const {
      return !this->operator==(iter);
    }

   private:
    /// @brief 迭代器绑定到的容器
    const ObjectManager<T>* manager_pointer_;
    /// @brief 迭代器指向的对象位置
    ObjectId id_;
  };

  /// @brief 默认用来合并两个对象的函数
  /// @param[in] node_dst ：合并到的对象，该对象在成功合并后会被保留
  /// @param[in] node_src ：被合并的对象，该对象在成功合并后会被删除
  /// @return 返回合并是否成功
  /// @retval true ：合并成功
  /// @retval false ：合并失败
  static bool DefaultMergeFunction2(T& node_dst, T& node_src) {
    return node_dst.MergeNode(node_src);
  }
  /// @brief 默认用来在管理器参与下合并两个对象的函数
  /// @param[in] node_dst ：合并到的对象，该对象在成功合并后会被保留
  /// @param[in] node_src ：被合并的对象，该对象在成功合并后会被删除
  /// @param[in,out] manager ：上层管理器，控制合并过程
  /// @return 返回合并是否成功
  /// @retval true ：合并成功
  /// @retval false ：合并失败
  template <class Manager>
  static bool DefaultMergeFunction3(T& node_dst, T& node_src,
                                    Manager& manager) {
    return manager.MergeNodes(node_dst, node_src);
  }
  /// @brief 获取对象的const引用
  /// @param[in] id ：要获取const引用的对象的ID
  /// @return 返回获取到的对象的const引用
  /// @attention id必须指向有效的的对象
  const T& GetObject(ObjectId id) const;
  /// @brief 获取对象的引用
  /// @param[in] id ：要获取引用的对象的ID
  /// @return 返回获取到的对象的引用
  /// @attention id必须指向有效的的对象
  T& GetObject(ObjectId id);
  /// @brief 获取对象的const引用
  /// @param[in] id ：要获取const引用的对象的ID
  /// @return 返回获取到的对象的const引用
  /// @note 与GetObject等效
  /// @attention id必须指向有效的的对象
  const T& operator[](ObjectId id) const;
  /// @brief 获取对象的引用
  /// @param[in] id ：要获取引用的对象的ID
  /// @return 返回获取到的对象的引用
  /// @note 与GetObject等效
  /// @attention id必须指向有效的的对象
  T& operator[](ObjectId id);

  /// @brief 系统自行选择最佳位置放置对象
  /// @param[in,out] args ：对象构造函数使用的参数
  /// @return 返回唯一对应于对象的ID
  template <class ObjectType = T, class... Args>
  ObjectId EmplaceObject(Args&&... args);

  /// @brief 仅删除节点记录但不释放节点内存
  /// @param[in] id ：要删除的对象的ID
  /// @return 返回指向对象的指针
  /// @attention 入参必须指向有效的对象
  [[nodiscard]] T* RemoveObjectNoDelete(ObjectId id);
  /// @brief 删除节点并释放节点内存（默认删除语义）
  /// @param[in] id ：要删除的对象的ID
  /// @return 一定返回true
  /// @attention 入参必须指向有效的对象
  bool RemoveObject(ObjectId id);

  /// @brief 合并两个对象，合并成功会删除id_src对象
  /// @param[in] id_dst ：要合并到的对象（合并后保留）
  /// @param[in] id_src ：用来合并的对象（成功合并后删除）
  /// @param[in] merge_function
  /// ：控制合并过程的函数，第一个参数为id_dst对应的对象
  /// 第二个参数为id_src对应的对象
  /// @return 返回是否合并成功
  /// @retval false
  /// ：合并同一个对象或id_src对应的节点不允许合并或merge_function返回false
  /// @retval true ：合并成功
  /// @attention 合并失败则不会删除源对象，id_dst和id_src必须指向已存在的对象
  bool MergeObjects(ObjectId id_dst, ObjectId id_src,
                    const std::function<bool(T&, T&)>& merge_function =
                        DefaultMergeFunction2);
  /// @brief 合并两个对象，合并成功会删除id_src节点
  /// @param[in] id_dst ：要合并到的对象（合并后保留）
  /// @param[in] id_src ：用来合并的对象（成功合并后删除）
  /// @param[in] manager ：合并管理器，在调用merge_function时使用
  /// @param[in] merge_function
  /// ：控制合并过程的函数，第一个参数为id_dst对应的对象
  /// 第二个参数为id_src对应的对象，第三个参数为传入的manager
  /// @return 返回是否合并成功
  /// @retval false
  /// ：合并同一个对象或id_src对应的节点不允许合并或merge_function返回false
  /// @retval true ：合并成功
  /// @attention 合并失败则不会删除源对象，id_dst和id_src必须指向已存在的对象
  template <class Manager>
  bool MergeObjectsWithManager(ObjectId id_dst, ObjectId id_src,
                               Manager& manager,
                               const std::function<bool(T&, T&, Manager&)>&
                                   merge_function = DefaultMergeFunction3);
  /// @brief 查询ID对应的节点是否可合并
  /// @param[in] id ：待查询的节点ID
  /// @return 给定节点是否可合并
  /// @retval true ：可以合并
  /// @retval false ：不可以合并
  /// @attention 传入的参数必须指向已存在的对象
  bool CanBeSourceInMerge(ObjectId id) const {
    assert(id < nodes_can_be_source_in_merge.size() && nodes_[id] != nullptr);
    return nodes_can_be_source_in_merge[id];
  }
  /// @brief 设置给定对象在合并时可以作为源对象（合并成功则源对象被释放）
  /// @param[in] id ：待设置的对象ID
  /// @return 一定返回true
  /// @attention 传入的参数必须指向已存在的对象
  bool SetObjectCanBeSourceInMerge(ObjectId id);
  /// 设置给定对象在合并时不能作为源对象（合并成功则源对象被释放）
  /// @param[in] id ：待设置的对象ID
  /// @return 一定返回true
  /// @attention 传入的参数必须指向已存在的对象
  bool SetObjectCanNotBeSourceInMerge(ObjectId id);
  /// @brief 设置所有对象在合并时均可以作为源对象（合并成功则源对象被释放）
  void SetAllObjectsCanBeSourceInMerge();
  /// @brief 设置所有对象在合并时均不可以作为源对象（合并成功则源对象被释放）
  void SetAllObjectsCanNotBeSourceInMerge();

  /// @brief 判断两个ID是否指向同一对象
  /// @param[in] id1 ：要判断的一个ID
  /// @param[in] id2 ：要判断的另一个ID
  /// @return 返回两个ID是否指向相同对象
  /// @retval true ：两个ID指向相同对象
  /// @retval false ：两个ID指向不同对象
  /// @note 允许输入相同ID
  bool IsSame(ObjectId id1, ObjectId id2) const { return id1 == id2; }

  /// @brief 交换两个容器的内容
  /// @param[in,out] manager_other ：要交换的另一个容器
  /// @note 允许传入this，但会执行完整的交换流程
  /// @attention 不允许传入空指针
  void Swap(ObjectManager* manager_other);

  /// @brief 获取容器能存储的对象个数
  /// @return 返回容器能存储的对象个数
  size_t Capacity() const { return nodes_.size(); }
  /// @brief 获取容器实际存储的有效对象数量
  /// @return 返回容器中实际存储的有效对象个数
  size_t Size() const;

  /// @brief 初始化，如果容器中存在节点则全部释放
  void ObjectManagerInit();
  /// @brief 清除所有节点记录但不释放
  void ClearNoRelease();
  /// @brief 调用成员变量的shrink_to_fit来节省空间
  void ShrinkToFit();

  /// @brief 获取指向超尾的迭代器
  /// @return 返回指向超尾的迭代器
  Iterator End() { return Iterator(this, ObjectId(nodes_.size())); }
  /// @brief 获取指向第一个有效节点的迭代器
  /// @return 返回指向第一个有效节点的迭代器
  /// @attention 如果不存在有效节点则返回End()
  Iterator Begin();
  /// @brief 获取指向超尾的const迭代器
  /// @return 返回指向超尾的const迭代器
  ConstIterator ConstEnd() const {
    return ConstIterator(this, ObjectId(nodes_.size()));
  }
  /// @brief 获取指向第一个有效节点的const迭代器
  /// @return 返回指向第一个有效节点的const迭代器
  /// @attention 如果不存在有效节点则返回ConstEnd()
  ConstIterator ConstBegin() const;

 private:
  /// @brief 声明友元以允许boost-serialization库访问内部成员
  friend class boost::serialization::access;

  /// @brief 序列化容器
  /// @param[in,out] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& nodes_;
    ar& removed_ids_;
    ar& nodes_can_be_source_in_merge;
  }

  /// @brief 在指定位置放置对象
  /// @tparam ObjectType ：放置的对象类型，默认使用容器声明时指定的对象类型
  /// 允许使用该类型的派生类类型
  /// @param[in] id ：要放置对象的位置，可以为大于等于nodes_.size()的值
  /// @param[in] args ：对象的构造函数参数
  /// @return 返回id
  template <class ObjectType = T, class... Args>
  ObjectId EmplaceObjectIndex(ObjectId id, Args&&... args);
  /// @brief 在指定位置放置指针
  /// @param[in] id ：放置对象的位置
  /// @param[in] pointer ：对象的指针
  /// @return 返回id
  /// @attention 不允许传入空指针
  ObjectId EmplacePointerIndex(ObjectId id, T* pointer);

  /// @brief 获取当前最佳可用ID
  /// @return 返回当前最佳可用ID
  /// @note 优先选择removed_ids_中的ID，如果不存在则返回nodes_.size()
  ObjectId GetBestEmptyIndex();
  /// @brief 添加已移除的ID
  /// @param[in] id ：已移除的对象对应的ID
  void AddRemovedIndex(ObjectId id) { removed_ids_.push_back(id); }

  /// @brief 存放指向节点的指针，节点不存在则置为nullptr
  std::vector<T*> nodes_;
  /// @brief 存放所有被删除节点对应的ID（可能大于nodes_.size()）
  std::vector<ObjectId> removed_ids_;
  /// @brief 存储节点在合并时是否允许作为源节点
  std::vector<bool> nodes_can_be_source_in_merge;
};

template <class T>
ObjectManager<T>::~ObjectManager() {
  for (T* ptr : nodes_) {
    delete ptr;
  }
}

template <class T>
inline const T& ObjectManager<T>::GetObject(ObjectId id) const {
  assert(id < nodes_.size() && nodes_[id] != nullptr);
  return *nodes_[id];
}
template <class T>
inline T& frontend::common::ObjectManager<T>::GetObject(ObjectId id) {
  return const_cast<T&>(
      static_cast<const ObjectManager<T>&>(*this).GetObject(id));
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
  nodes_[id] = nullptr;
  assert(temp_pointer != nullptr);
  AddRemovedIndex(id);
  nodes_can_be_source_in_merge[id] = false;
  while (nodes_.size() > 0 && nodes_.back() == nullptr) {
    nodes_.pop_back();  /// 清理末尾无效ID
    nodes_can_be_source_in_merge.pop_back();
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
  assert(result.IsValid());
  return result;
}

template <class T>
template <class ObjectType, class... Args>
inline ObjectManager<T>::ObjectId ObjectManager<T>::EmplaceObjectIndex(
    ObjectId id, Args&&... args) {
  T* pointer = new ObjectType(std::forward<Args>(args)...);
  ObjectId result = EmplacePointerIndex(id, pointer);
  assert(result.IsValid());
  return result;
}

template <class T>
inline ObjectManager<T>::ObjectId ObjectManager<T>::EmplacePointerIndex(
    ObjectId id, T* pointer) {
  assert(pointer != nullptr);
  size_t size_old = nodes_.size();
  if (id >= size_old) {
    nodes_.resize(id + 1, nullptr);
    nodes_can_be_source_in_merge.resize(id + 1, false);
    for (size_t i = size_old; i < id; i++) {
      removed_ids_.push_back(ObjectId(i));
    }
  }
  /// 不可以覆盖已有非空且不同的指针
  assert(!(nodes_[id] != nullptr && nodes_[id] != pointer));
  nodes_[id] = pointer;
  return id;
}

template <class T>
inline void ObjectManager<T>::Swap(ObjectManager* manager_other) {
  nodes_.swap(manager_other->nodes_);
  removed_ids_.swap(manager_other->removed_ids_);
  nodes_can_be_source_in_merge.swap(
      manager_other->nodes_can_be_source_in_merge);
}

template <class T>
inline bool ObjectManager<T>::SetObjectCanBeSourceInMerge(ObjectId id) {
  assert(id < nodes_.size() && nodes_[id] != nullptr);
  nodes_can_be_source_in_merge[id] = true;
  return true;
}

template <class T>
inline bool ObjectManager<T>::SetObjectCanNotBeSourceInMerge(ObjectId id) {
  assert(id < nodes_.size() && nodes_[id] != nullptr);
  nodes_can_be_source_in_merge[id] = false;
  return true;
}

template <class T>
inline void ObjectManager<T>::SetAllObjectsCanBeSourceInMerge() {
  for (size_t id = 0; id < nodes_.size(); id++) {
    if (nodes_[id] != nullptr) {
      nodes_can_be_source_in_merge[id] = true;
    }
  }
}

template <class T>
inline void ObjectManager<T>::SetAllObjectsCanNotBeSourceInMerge() {
  std::vector<bool> vec_temp(nodes_can_be_source_in_merge.size(), false);
  nodes_can_be_source_in_merge.Swap(vec_temp);
}

template <class T>
inline bool ObjectManager<T>::MergeObjects(
    ObjectId id_dst, ObjectId id_src,
    const std::function<bool(T&, T&)>& merge_function) {
  // 检查是否合并同一个节点
  if (id_dst == id_src) [[unlikely]] {
    return false;
  }
  T& object_dst = GetObject(id_dst);
  T& object_src = GetObject(id_src);
  if (!CanBeSourceInMerge(id_src)) {
    /// 给定源节点不允许作为合并时的源节点
    return false;
  }
  if (!merge_function(object_dst, object_src)) {
    /// 合并失败
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
  if (!CanBeSourceInMerge(id_src)) {
    /// 给定源节点不允许作为合并时的源节点
    return false;
  }
  if (!merge_function(object_dst, object_src, manager)) {
    /// 合并失败
    return false;
  }
  RemoveObject(id_src);
  return true;
}

template <class T>
inline void ObjectManager<T>::ObjectManagerInit() {
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
    nodes_can_be_source_in_merge.pop_back();
  }
  nodes_.shrink_to_fit();
  removed_ids_.shrink_to_fit();
}

template <class T>
inline size_t ObjectManager<T>::Size() const {
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
    while (!id.IsValid() && removed_ids_.size() != 0)  /// 查找有效ID
    {
      if (removed_ids_.back() < nodes_.size()) {
        id = removed_ids_.back();
      }
      removed_ids_.pop_back();
    }
  }
  if (!id.IsValid())  /// 无有效已删除ID
  {
    id = ObjectId(nodes_.size());
  }
  return id;
}

template <class T>
inline const T& ObjectManager<T>::operator[](ObjectId id) const {
  assert(id < nodes_.size());
  return GetObject(id);
}
template <class T>
inline T& ObjectManager<T>::operator[](ObjectId id) {
  return const_cast<T&>(
      static_cast<const ObjectManager<T>&>(*this).operator[](id));
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
inline ObjectManager<T>::ConstIterator ObjectManager<T>::ConstBegin() const {
  ObjectId id(0);
  while (id <= nodes_.size() && nodes_[id] == nullptr) {
    ++id;
  }
  return ConstIterator(this, id);
}

template <class T>
inline ObjectManager<T>::Iterator& ObjectManager<T>::Iterator::operator++() {
  assert(*this != manager_pointer_->End());
  auto& nodes = manager_pointer_->nodes_;
  do {
    ++id_;
  } while (id_ < nodes.size() && nodes[id_] == nullptr);
  if (id_ >= nodes.size()) [[likely]] {
    *this = manager_pointer_->End();
  }
  return *this;
}

template <class T>
inline ObjectManager<T>::Iterator ObjectManager<T>::Iterator::operator++(int) {
  assert(*this != manager_pointer_->End());
  ObjectId id_temp = id_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    ++id_temp;
  } while (id_temp < nodes.size() && nodes[id_temp] == nullptr);
  if (id_temp < nodes.size()) [[likely]] {
    std::swap(id_temp, id_);
    return Iterator(manager_pointer_, id_temp);
  } else {
    *this = manager_pointer_->End();
    return manager_pointer_->End();
  }
}

template <class T>
inline ObjectManager<T>::Iterator& ObjectManager<T>::Iterator::operator--() {
  assert(id_.IsValid() && *this != manager_pointer_->End());
  auto& nodes = manager_pointer_->nodes_;
  do {
    --id_;
    assert(id_ != -1);
  } while (nodes[id_] == nullptr);
  return *this;
}

template <class T>
inline ObjectManager<T>::Iterator ObjectManager<T>::Iterator::operator--(int) {
  assert(id_.IsValid() && *this != manager_pointer_->End());
  size_t temp_id = id_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    --temp_id;
    assert(temp_id != -1);
  } while (nodes[temp_id] == nullptr);
  std::swap(temp_id, id_);
  return Iterator(manager_pointer_, temp_id);
}

template <class T>
inline T& ObjectManager<T>::Iterator::operator*() const {
  return manager_pointer_->GetObject(id_);
}

template <class T>
inline T* ObjectManager<T>::Iterator::operator->() const {
  return &manager_pointer_->GetObject(id_);
}

template <class T>
inline ObjectManager<T>::ConstIterator&
ObjectManager<T>::ConstIterator::operator++() {
  assert(*this != manager_pointer_->ConstEnd());
  auto& nodes = manager_pointer_->nodes_;
  do {
    ++id_;
  } while (id_ < nodes.size() && nodes[id_] == nullptr);
  if (id_ >= nodes.size()) [[likely]] {
    *this = manager_pointer_->ConstEnd();
  }
  return *this;
}

template <class T>
inline ObjectManager<T>::ConstIterator
ObjectManager<T>::ConstIterator::operator++(int) {
  assert(*this != manager_pointer_->ConstEnd());
  ObjectId id_temp = id_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    ++id_temp;
  } while (id_temp < nodes.size() && nodes[id_temp] == nullptr);
  if (id_temp < nodes.size()) [[likely]] {
    std::swap(id_temp, id_);
    return ConstIterator(manager_pointer_, id_temp);
  } else {
    *this = manager_pointer_->ConstEnd();
    return manager_pointer_->ConstEnd();
  }
}

template <class T>
inline ObjectManager<T>::ConstIterator&
ObjectManager<T>::ConstIterator::operator--() {
  assert(id_.IsValid() && *this != manager_pointer_->End());
  auto& nodes = manager_pointer_->nodes_;
  do {
    --id_;
    assert(id_ != -1);
  } while (nodes[id_] == nullptr);
  return *this;
}

template <class T>
inline ObjectManager<T>::ConstIterator
ObjectManager<T>::ConstIterator::operator--(int) {
  assert(id_.IsValid() && *this != manager_pointer_->End());
  size_t temp_id = id_;
  auto& nodes = manager_pointer_->nodes_;
  do {
    --temp_id;
    assert(temp_id != -1);
  } while (nodes[temp_id] == nullptr);
  std::swap(temp_id, id_);
  return ConstIterator(manager_pointer_, temp_id);
}
}  // namespace frontend::common
#endif  /// !COMMON_COMMON_NODE_MANAGER