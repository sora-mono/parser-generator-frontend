#pragma once
#include <assert.h>

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "NodeManager.h"
template <class T>
class MultimapNodeManager {
 public:
  using NodeHandler = size_t;

  MultimapNodeManager() {}
  MultimapNodeManager(const MultimapNodeManager&) = delete;
  MultimapNodeManager(MultimapNodeManager&&) = delete;
  ~MultimapNodeManager() {}

  T* GetNode(NodeHandler handler);
  //通过一个handler查询引用底层节点的所有handler
  const std::unordered_set<NodeHandler>& GetHandlersReferringSameNode(
      NodeHandler handler);
  bool IsSame(NodeHandler handler1, NodeHandler handler2);

  //系统自行选择最佳位置放置节点
  template <class... Args>
  NodeHandler EmplaceNode(Args&&... args);
  //仅删除节点不释放指针指向的内存，返回管理的指针
  T* RemovePointer(NodeHandler handler);
  //删除节点并释放节点内存
  bool RemoveNode(NodeHandler handler);
  //将handler_src合并到handler_dst，
  //合并成功则删除handler_src对应的节点（二者不同前提下）
  bool MergeNodes(NodeHandler handler_dst, NodeHandler handler_src,
                  std::function<bool(T&, T&)> merge_function =
                      NodeManager<T>::DefaultMergeFunction2);
  template <class Manager>
  bool MergeNodesWithManager(
      NodeHandler handler_dst, NodeHandler handler_src, Manager& manager,
      const std::function<bool(T&, T&, Manager&)>& merge_function =
          NodeManager<T>::DefaultMergeFunction3);
  //交换两个类的内容
  void Swap(MultimapNodeManager& manager_other);
  //设置节点允许合并标志
  bool SetNodeMergeAllowed(NodeHandler handler);
  //设置节点禁止合并标志
  bool SetNodeMergeRefused(NodeHandler handler);
  //设置所有节点允许合并，不会对未分配节点设置允许标记
  void SetAllMergeAllowed() { node_manager_.SetAllNodesMergeAllowed(); }
  //设置所有节点禁止合并
  void SetAllNodesMergeRefused() { node_manager_.SetAllNodesMergeRefused(); }
  //返回handler对应节点可合并状态
  bool CanMerge(NodeHandler handler);

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

 private:
  using InsideIndex = NodeManager<T>::NodeHandler;
  //将handler指向的实际节点重映射为inside_index指向的节点，
  //可以接受未映射的handler
  bool RemapHandler(NodeHandler handler, InsideIndex inside_index);
  NodeHandler CreateHandler() { return next_handler_index_++; }
  //增加该handler对inside_index节点的引用记录，返回增加后引用计数
  size_t AddReference(NodeHandler handler, InsideIndex inside_index);
  //移除该handler对inside_index节点的引用记录，返回移除后引用计数，
  //引用计数为0时会自动删除节点
  size_t RemoveReference(NodeHandler handler, InsideIndex inside_index);
  //处理移除内部节点前的步骤，如删除所有引用
  bool PreRemoveInsideNode(InsideIndex inside_index);
  InsideIndex GetInsideIndex(NodeHandler handler);

  //下一个handler序号值，只增不减
  size_t next_handler_index_ = 0;
  NodeManager<T> node_manager_;
  //储存指向同一个底层节点的handler
  std::vector<std::unordered_set<NodeHandler>> referred_handlers_;
  //对底层handler的另一层封装，使本类支持多个handler指向同一个对象
  std::unordered_map<NodeHandler, InsideIndex> handler_to_index_;
};

template <class T>
inline typename MultimapNodeManager<T>::InsideIndex
MultimapNodeManager<T>::GetInsideIndex(NodeHandler handler) {
  auto iter = handler_to_index_.find(handler);
  if (iter == handler_to_index_.end()) {
    return -1;
  }
  return iter->second;
}

template <class T>
inline T* MultimapNodeManager<T>::GetNode(NodeHandler handler) {
  InsideIndex inside_index = GetInsideIndex(handler);
  if (inside_index == -1) {
    return nullptr;
  } else {
    return node_manager_.GetNode(inside_index);
  }
}

template <class T>
inline const std::unordered_set<typename MultimapNodeManager<T>::NodeHandler>&
MultimapNodeManager<T>::GetHandlersReferringSameNode(NodeHandler handler) {
  InsideIndex inside_index = GetInsideIndex(handler);
  if (inside_index == -1) {
    throw std::invalid_argument("句柄无效");
  }
  return referred_handlers_[inside_index];
}

template <class T>
inline bool MultimapNodeManager<T>::IsSame(NodeHandler handler1,
                                           NodeHandler handler2) {
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
inline MultimapNodeManager<T>::NodeHandler MultimapNodeManager<T>::EmplaceNode(
    Args&&... args) {
  NodeHandler handler = CreateHandler();
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
inline bool MultimapNodeManager<T>::SetNodeMergeAllowed(NodeHandler handler) {
  InsideIndex inside_index = GetInsideIndex(handler);
  if (inside_index == -1) {
    return false;
  }
  return node_manager_.SetNodeMergeAllowed(inside_index);
}

template <class T>
inline bool MultimapNodeManager<T>::SetNodeMergeRefused(NodeHandler handler) {
  InsideIndex inside_index = GetInsideIndex(handler);
  if (inside_index == -1) {
    return false;
  }
  return node_manager_.SetNodeMergeRefused(inside_index);
}

template <class T>
inline bool MultimapNodeManager<T>::CanMerge(NodeHandler handler) {
  InsideIndex index = GetInsideIndex(handler);
  if (index == -1) {
    return false;
  }
  return node_manager_.CanMerge(index);
}

template <class T>
inline bool MultimapNodeManager<T>::MergeNodes(
    NodeHandler handler_dst, NodeHandler handler_src,
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
    typename MultimapNodeManager<T>::NodeHandler handler_dst,
    typename MultimapNodeManager<T>::NodeHandler handler_src, Manager& manager,
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
inline T* MultimapNodeManager<T>::RemovePointer(NodeHandler handler) {
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
inline bool MultimapNodeManager<T>::RemoveNode(NodeHandler handler) {
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
inline bool MultimapNodeManager<T>::RemapHandler(NodeHandler handler,
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
inline size_t MultimapNodeManager<T>::AddReference(NodeHandler handler,
                                                   InsideIndex inside_index) {
  if (inside_index >= node_manager_.Size()) {
    return -1;
  }
  referred_handlers_[inside_index].insert(handler);
  return referred_handlers_[inside_index].size();
}

template <class T>
inline size_t MultimapNodeManager<T>::RemoveReference(
    NodeHandler handler, InsideIndex inside_index) {
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