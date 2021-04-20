#include <assert.h>

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "Common/NodeManager.h"

#ifndef COMMON_MULTIMAP_NODE_MANAGER_H_
#define COMMON_MULTIMAP_NODE_MANAGER_H_

namespace common {

template <class T>
class MultimapNodeManager {
 public:
  using NodeId = size_t;

  class Iterator {
   public:
    Iterator() {}
    Iterator(const Iterator& iter) : iter_(iter.iter_) {}

    //获取对应ID
    const std::unordered_set<NodeId>& GetId() {
      assert(iter_.GetId() < referred_handlers_.size());
      return referred_handlers_[iter_.GetId()];
    }

    Iterator& operator++();
    Iterator operator++(int);
    Iterator& operator--();
    Iterator operator--(int);
    T& operator*();
    T* operator->();
    bool operator==(const Iterator& iter) { return iter_ == iter.iter_; }
    bool operator!=(const Iterator& iter) { return !this->operator==(iter); }

   private:
    //设置绑定的NodeManager对象
    void SetManagerPointer(NodeManager<T>* manager_pointer) {
      iter_.SetManagerPointer(manager_pointer);
    }
    friend class MultimapNodeManager;
    Iterator(NodeManager<T>::Iterator& iter) : iter_(iter) {}
    void SetHandler(NodeManager<T>::NodeId handler);
    NodeManager<T>::Iterator iter_;
  };

  MultimapNodeManager() {}
  MultimapNodeManager(const MultimapNodeManager&) = delete;
  MultimapNodeManager(MultimapNodeManager&&) = delete;
  ~MultimapNodeManager() {}

  T* GetNode(NodeId handler);
  //通过一个handler查询引用底层节点的所有handler
  const std::unordered_set<NodeId>& GetHandlersReferringSameNode(
      NodeId handler);
  bool IsSame(NodeId handler1, NodeId handler2);

  //系统自行选择最佳位置放置节点
  template <class... Args>
  NodeId EmplaceNode(Args&&... args);
  //仅删除节点不释放指针指向的内存，返回管理的指针
  T* RemovePointer(NodeId handler);
  //删除节点并释放节点内存
  bool RemoveNode(NodeId handler);
  //将handler_src合并到handler_dst，
  //合并成功则删除handler_src对应的节点（二者不同前提下）
  bool MergeNodes(NodeId handler_dst, NodeId handler_src,
                  std::function<bool(T&, T&)> merge_function =
                      NodeManager<T>::DefaultMergeFunction2);
  template <class Manager>
  bool MergeNodesWithManager(
      NodeId handler_dst, NodeId handler_src, Manager& manager,
      const std::function<bool(T&, T&, Manager&)>& merge_function =
          NodeManager<T>::DefaultMergeFunction3);
  //交换两个类的内容
  void Swap(MultimapNodeManager& manager_other);
  //设置节点允许合并标志
  bool SetNodeMergeAllowed(NodeId handler);
  //设置节点禁止合并标志
  bool SetNodeMergeRefused(NodeId handler);
  //设置所有节点允许合并，不会对未分配节点设置允许标记
  void SetAllMergeAllowed() { node_manager_.SetAllNodesMergeAllowed(); }
  //设置所有节点禁止合并
  void SetAllNodesMergeRefused() { node_manager_.SetAllNodesMergeRefused(); }
  //返回handler对应节点可合并状态
  bool CanMerge(NodeId handler);

  //暂时不写，优化释放后的空间太少
  // void remap_optimization();	//重映射所有的节点，消除node_manager空余空间

  //清除并释放所有节点
  void Clear();
  //清除但不释放所有节点
  void ClearNoRelease();
  //调用成员变量的shrink_to_fit
  void ShrinkToFit();

  //返回节点个数（包括已删除但仍保留index的节点）
  size_t Size() { return node_manager_.Size(); }
  //返回实际节点个数
  size_t ItemSize() { return node_manager_.ItemSize(); }

  //序列化容器用
  template <class Archive>
  void Serialize(Archive& ar, const unsigned int version = 0);

  Iterator Begin() { return Iterator(node_manager_.Begin); }
  Iterator End() { return Iterator(node_manager_.End()); }

  T* operator[](NodeId handler);

 private:
  using InsideIndex = NodeManager<T>::NodeId;
  //将handler指向的实际节点重映射为inside_index指向的节点，
  //可以接受未映射的handler
  bool RemapHandler(NodeId handler, InsideIndex inside_index);
  NodeId CreateHandler() { return next_handler_index_++; }
  //增加该handler对inside_index节点的引用记录，返回增加后引用计数
  size_t AddReference(NodeId handler, InsideIndex inside_index);
  //移除该handler对inside_index节点的引用记录，返回移除后引用计数，
  //引用计数为0时会自动删除节点
  size_t RemoveReference(NodeId handler, InsideIndex inside_index);
  //处理移除内部节点前的步骤，如删除所有引用
  bool PreRemoveInsideNode(InsideIndex inside_index);
  InsideIndex GetInsideIndex(NodeId handler);

  //下一个handler序号值，只增不减
  size_t next_handler_index_ = 0;
  NodeManager<T> node_manager_;
  //储存指向同一个底层节点的handler
  std::vector<std::unordered_set<NodeId>> referred_handlers_;
  //对底层handler的另一层封装，使本类支持多个handler指向同一个对象
  std::unordered_map<NodeId, InsideIndex> handler_to_index_;
};

template <class T>
inline typename MultimapNodeManager<T>::InsideIndex
MultimapNodeManager<T>::GetInsideIndex(NodeId handler) {
  auto iter = handler_to_index_.find(handler);
  if (iter == handler_to_index_.end()) {
    return -1;
  }
  return iter->second;
}

template <class T>
inline T* MultimapNodeManager<T>::GetNode(NodeId handler) {
  InsideIndex inside_index = GetInsideIndex(handler);
  if (inside_index == -1) {
    return nullptr;
  } else {
    return node_manager_.GetNode(inside_index);
  }
}

template <class T>
inline const std::unordered_set<typename MultimapNodeManager<T>::NodeId>&
MultimapNodeManager<T>::GetHandlersReferringSameNode(NodeId handler) {
  InsideIndex inside_index = GetInsideIndex(handler);
  if (inside_index == -1) {
    throw std::invalid_argument("句柄无效");
  }
  return referred_handlers_[inside_index];
}

template <class T>
inline bool MultimapNodeManager<T>::IsSame(NodeId handler1, NodeId handler2) {
  InsideIndex inside_index1 = GetInsideIndex(handler1);
  if (inside_index1 == -1) {
    throw std::runtime_error("Invalid handler1");
  }
  InsideIndex inside_index2 = GetInsideIndex(handler2);
  if (inside_index2 == -1) {
    throw std::runtime_error("Invalid handler2");
  }
  return inside_index1 == inside_index2;
}

template <class T>
template <class... Args>
inline MultimapNodeManager<T>::NodeId MultimapNodeManager<T>::EmplaceNode(
    Args&&... args) {
  NodeId handler = CreateHandler();
  InsideIndex inside_index =
      node_manager_.EmplaceNode(std::forward<Args>(args)...);
  if (inside_index == -1) {
    return -1;
  }
  if (AddReference(handler, inside_index) == -1) {  //添加引用记录失败
    node_manager_.RemoveNode(inside_index);
    return -1;
  }
  handler_to_index_[handler] = inside_index;

  return handler;
}

template <class T>
inline bool MultimapNodeManager<T>::SetNodeMergeAllowed(NodeId handler) {
  InsideIndex inside_index = GetInsideIndex(handler);
  if (inside_index == -1) {
    return false;
  }
  return node_manager_.SetNodeMergeAllowed(inside_index);
}

template <class T>
inline bool MultimapNodeManager<T>::SetNodeMergeRefused(NodeId handler) {
  InsideIndex inside_index = GetInsideIndex(handler);
  if (inside_index == -1) {
    return false;
  }
  return node_manager_.SetNodeMergeRefused(inside_index);
}

template <class T>
inline bool MultimapNodeManager<T>::CanMerge(NodeId handler) {
  InsideIndex index = GetInsideIndex(handler);
  if (index == -1) {
    return false;
  }
  return node_manager_.CanMerge(index);
}

template <class T>
inline bool MultimapNodeManager<T>::MergeNodes(
    NodeId handler_dst, NodeId handler_src,
    std::function<bool(T&, T&)> merge_function) {
  InsideIndex inside_index_dst = GetInsideIndex(handler_dst);
  InsideIndex inside_index_src = GetInsideIndex(handler_src);
  if (inside_index_dst == -1 || inside_index_src == -1) {
    return false;
  }
  if (!node_manager_.MergeNodesWithManager(inside_index_dst, inside_index_src,
                                           merge_function)) {
    return false;
  }
  RemapHandler(handler_src, inside_index_dst);
  return true;
}

template <class T>
template <class Manager>
inline bool MultimapNodeManager<T>::MergeNodesWithManager(
    typename MultimapNodeManager<T>::NodeId handler_dst,
    typename MultimapNodeManager<T>::NodeId handler_src, Manager& manager,
    const std::function<bool(T&, T&, Manager&)>& merge_function) {
  InsideIndex inside_index_dst = GetInsideIndex(handler_dst);
  InsideIndex inside_index_src = GetInsideIndex(handler_src);
  if (inside_index_dst == -1 || inside_index_src == -1) {
    return false;
  }
  if (!node_manager_.MergeNodesWithManager(inside_index_dst, inside_index_src,
                                           manager, merge_function)) {
    return false;
  }
  RemapHandler(handler_src, inside_index_dst);
  return true;
}

template <class T>
inline bool MultimapNodeManager<T>::PreRemoveInsideNode(
    InsideIndex inside_index) {
  if (inside_index >= node_manager_.Size()) {
    return false;
  }
  if (referred_handlers_[inside_index].size() != 0) {
    for (auto x : referred_handlers_[inside_index]) {
      auto iter = handler_to_index_.find(x);
      handler_to_index_.erase(iter);
    }
    referred_handlers_[inside_index].clear();
  }
  return true;
}

template <class T>
inline T* MultimapNodeManager<T>::RemovePointer(NodeId handler) {
  InsideIndex inside_index = GetInsideIndex(handler);
  if (inside_index == -1) {
    return nullptr;
  }
  if (!PreRemoveInsideNode(inside_index)) {
    return nullptr;
  }
  return node_manager_.RemovePointer(inside_index);
}

template <class T>
inline bool MultimapNodeManager<T>::RemoveNode(NodeId handler) {
  InsideIndex inside_index = GetInsideIndex(handler);
  if (inside_index == -1) {
    return true;
  }
  PreRemoveInsideNode(inside_index);
  node_manager_.RemoveNode(inside_index);
  return true;
}

template <class T>
inline void MultimapNodeManager<T>::Swap(MultimapNodeManager& manager_other) {
  std::swap(next_handler_index_, manager_other.next_handler_index_);
  node_manager_.Swap(manager_other.node_manager_);
  referred_handlers_.Swap(manager_other.referred_handlers_);
  handler_to_index_.Swap(manager_other.handler_to_index_);
}

template <class T>
inline bool MultimapNodeManager<T>::RemapHandler(NodeId handler,
                                                 InsideIndex inside_index) {
  if (inside_index >= node_manager_.Size()) {
    return false;
  }
  InsideIndex inside_index_old = GetInsideIndex(handler);
  if (inside_index == inside_index_old) {
    return true;
  }
  if (inside_index_old != -1) {
    size_t result = RemoveReference(handler, inside_index_old);
    assert(result != -1);
  }
  if (AddReference(handler, inside_index) == -1) {
    if (inside_index_old != -1) {
      AddReference(handler, inside_index_old);
    }
    return false;
  }
  handler_to_index_[handler] = inside_index;
  return true;
}

template <class T>
inline size_t MultimapNodeManager<T>::AddReference(NodeId handler,
                                                   InsideIndex inside_index) {
  if (inside_index >= node_manager_.Size()) {
    return -1;
  }
  referred_handlers_[inside_index].insert(handler);
  return referred_handlers_[inside_index].size();
}

template <class T>
inline size_t MultimapNodeManager<T>::RemoveReference(
    NodeId handler, InsideIndex inside_index) {
  if (inside_index >= node_manager_.Size()) {
    return -1;
  }
  auto& ref_uset = referred_handlers_[inside_index];
  auto iter = ref_uset.find(handler);
  if (iter != ref_uset.end()) {
    if (ref_uset.size() == 1) {
      node_manager_.RemoveNode(inside_index);
    }
    ref_uset.erase(iter);
  }
  return ref_uset.size();
}

template <class T>
inline void MultimapNodeManager<T>::Clear() {
  next_handler_index_ = 0;
  node_manager_.Clear();
  referred_handlers_.clear();
  handler_to_index_.clear();
}

template <class T>
inline void MultimapNodeManager<T>::ClearNoRelease() {
  next_handler_index_ = 0;
  node_manager_.ClearNoRelease();
  referred_handlers_.clear();
  handler_to_index_.clear();
}

template <class T>
inline void MultimapNodeManager<T>::ShrinkToFit() {
  node_manager_.ShrinkToFit();
  referred_handlers_.shrink_to_fit();
}

template <class T>
inline T* MultimapNodeManager<T>::operator[](NodeId handler) {
  return GetNode(handler);
}

template <class T>
inline void MultimapNodeManager<T>::Iterator::SetHandler(
    NodeManager<T>::NodeId handler) {
  auto index_iter = handler_to_index_.find(handler);
  if (index_iter == handler_to_index_.end()) {
    throw std::invalid_argument("无效句柄");
  }
  iter_.SetIndex(index_iter->second);
}

template <class T>
inline MultimapNodeManager<T>::Iterator&
MultimapNodeManager<T>::Iterator::operator++() {
  ++iter_;
  return *this;
}

template <class T>
inline MultimapNodeManager<T>::Iterator
MultimapNodeManager<T>::Iterator::operator++(int) {
  Iterator iter_temp = *this;
  ++iter_;
  return iter_temp;
}

template <class T>
inline MultimapNodeManager<T>::Iterator&
MultimapNodeManager<T>::Iterator::operator--() {
  --iter_;
  return *this;
}

template <class T>
inline MultimapNodeManager<T>::Iterator
MultimapNodeManager<T>::Iterator::operator--(int) {
  Iterator iter_temp = *this;
  --iter_;
  return *this;
}

template <class T>
inline T& MultimapNodeManager<T>::Iterator::operator*() {
  return *iter_;
}

template <class T>
inline T* MultimapNodeManager<T>::Iterator::operator->() {
  return iter_.operator->();
}

}  // namespace common
#endif  // !COMMON_COMMON_MULTIMAP_NODE_MANAGER