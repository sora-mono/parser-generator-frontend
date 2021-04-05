#pragma once
#include"node_manager.h"
#include<unordered_map>
#include<unordered_set>
#include<stdexcept>

template<class T>
class multimap_node_manager
{
	using inside_index_t = node_manager<T>::node_handler_t;
public:
	using node_handler_t = size_t;

	multimap_node_manager() {}
	multimap_node_manager(const multimap_node_manager&) = delete;
	multimap_node_manager(multimap_node_manager&&) = delete;
	~multimap_node_manager() {}

	T* get_node(node_handler_t handler);
	const std::unordered_set<node_handler_t>& get_handlers_referring_same_node(node_handler_t handler);	//通过一个handler查询引用底层节点的所有handler
	bool is_same(node_handler_t handler1, node_handler_t handler2);
	template<class ...Args>
	node_handler_t emplace_node(Args&&...args);		//系统自行选择最佳位置放置节点
	T* remove_pointer(node_handler_t handler);	//仅删除节点
	bool remove_node(node_handler_t handler);		//删除节点并释放节点内存
	bool merge(node_handler_t handler_dst, node_handler_t handler_src,
		std::function<bool(T&, T&)>merge_function = node_manager<T>::default_merge_function_2);	//将handler_src合并到handler_dst，会调用函数merge_function
	template<class Manager>
	bool merge(
		node_handler_t handler_dst,
		node_handler_t handler_src,
		Manager& manager,
		const std::function<bool(T&, T&, Manager&)>& merge_function = node_manager<T>::default_merge_function_3);	//将handler_src合并到handler_dst，会调用函数merge_function
	void swap(multimap_node_manager& manager_other);		//交换两个类的内容

	bool set_merge_allowed(node_handler_t handler);		//设置节点允许合并标志
	bool set_merge_refused(node_handler_t handler);		//设置节点禁止合并标志
	void set_all_merge_allowed() { m_node_manager.set_all_merge_allowed(); }	//设置所有节点允许合并
	void set_all_merge_refused() { m_node_manager.set_all_merge_refused(); }	//设置所有节点禁止合并
	bool can_merge(node_handler_t handler);

	//暂时不写，优化释放后的空间太少
	//void remap_optimization();	//重映射所有的节点，消除node_manager空余空间

	void clear();			//清除并释放所有节点
	void clear_no_release();//清除但不释放所有节点
	void shrink_to_fit();	//调用成员变量的shrink_to_fit

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version = 0);		//序列化容器用

private:
	bool remap_handler(node_handler_t handler, inside_index_t inside_index);		//将handler指向的实际节点重映射为inside_index指向的节点，可以接受未映射的handler 
	node_handler_t create_handler() { return m_next_handler_index++; }
	size_t add_reference(node_handler_t handler, inside_index_t inside_index);		//增加该handler对inside_index节点的引用记录，返回增加后引用计数
	size_t remove_reference(node_handler_t handler, inside_index_t inside_index);	//移除该handler对inside_index节点的引用记录，返回移除后引用计数，引用计数为0时会自动删除节点
	bool pre_remove_inside_node(inside_index_t inside_index);		//处理移除内部节点前的步骤，如删除所有引用
	node_manager<T>::node_handler_t get_inside_index(node_handler_t handler);

	size_t m_next_handler_index = 0;		//下一个handler序号值，只增不减
	node_manager<T> m_node_manager;
	std::vector<std::unordered_set<node_handler_t>> m_referred_handlers;
	std::unordered_map<node_handler_t, inside_index_t>m_umap_handler_to_index;	//对底层m_node_manager handler的另一层封装，使本类支持多个handler指向同一个对象
};

template<class T>
inline node_manager<T>::node_handler_t multimap_node_manager<T>::get_inside_index(node_handler_t handler)
{
	auto iter = m_umap_handler_to_index.find(handler);
	if (iter == m_umap_handler_to_index.end())
	{
		return -1;
	}
	return iter->second;
}

template<class T>
inline T* multimap_node_manager<T>::get_node(node_handler_t handler)
{
	inside_index_t inside_index = get_inside_index(handler);
	if (inside_index == -1)
	{
		return nullptr;
	}
	else
	{
		return m_node_manager.get_node(inside_index);
	}
}

template<class T>
inline const std::unordered_set<typename multimap_node_manager<T>::node_handler_t>&
multimap_node_manager<T>::get_handlers_referring_same_node(node_handler_t handler)
{
	inside_index_t inside_index = get_inside_index(handler);
	if (inside_index == -1)
	{
		throw std::invalid_argument("句柄无效");
	}
	return m_referred_handlers[inside_index];
}

template<class T>
inline bool multimap_node_manager<T>::is_same(node_handler_t handler1, node_handler_t handler2)
{
	inside_index_t inside_index1 = get_inside_index(handler1);
	if (inside_index1 == -1)
	{
		throw std::runtime_error("Invalid handler1");
	}
	inside_index_t inside_index2 = get_inside_index(handler2);
	if (inside_index2 == -1)
	{
		throw std::runtime_error("Invalid handler2");
	}
	return inside_index1 == inside_index2;
}

template<class T>
template<class ...Args>
inline multimap_node_manager<T>::node_handler_t
multimap_node_manager<T>::emplace_node(Args && ...args)
{
	node_handler_t handler = create_handler();
	inside_index_t inside_index = m_node_manager.emplace_node(std::forward<Args>(args)...);
	if (inside_index == -1)
	{
		return -1;
	}
	if (add_reference(handler, inside_index) == -1)
	{//添加引用记录失败
		m_node_manager.remove_node(inside_index);
		return -1;
	}
	m_umap_handler_to_index[handler] = inside_index;

	return handler;
}

template<class T>
inline bool multimap_node_manager<T>::set_merge_allowed(node_handler_t handler)
{
	inside_index_t inside_index = get_inside_index(handler);
	if (inside_index == -1)
	{
		return false;
	}
	return m_node_manager.set_merge_allowed(inside_index);
}

template<class T>
inline bool multimap_node_manager<T>::set_merge_refused(node_handler_t handler)
{
	inside_index_t inside_index = get_inside_index(handler);
	if (inside_index == -1)
	{
		return false;
	}
	return m_node_manager.set_merge_refused(inside_index);
}

template<class T>
inline bool multimap_node_manager<T>::can_merge(node_handler_t handler)
{
	inside_index_t index = get_inside_index(handler);
	if (index == -1)
	{
		return false;
	}
	return m_node_manager.can_merge(index);
}

template<class T>
inline bool multimap_node_manager<T>::merge(node_handler_t handler_dst, node_handler_t handler_src, std::function<bool(T&, T&)>merge_function)
{
	inside_index_t inside_index_dst = get_inside_index(handler_dst);
	inside_index_t inside_index_src = get_inside_index(handler_src);
	if (inside_index_dst == -1 || inside_index_src == -1)
	{
		return false;
	}
	if (!m_node_manager.merge(inside_index_dst, inside_index_src, merge_function))
	{
		return false;
	}
	remap_handler(handler_src, inside_index_dst);
	return true;
}

template<class T>
template<class Manager>
inline bool multimap_node_manager<T>::merge(
	typename multimap_node_manager<T>::node_handler_t handler_dst,
	typename multimap_node_manager<T>::node_handler_t handler_src,
	Manager& manager,
	const std::function<bool(T&, T&, Manager&)>& merge_function)
{
	inside_index_t inside_index_dst = get_inside_index(handler_dst);
	inside_index_t inside_index_src = get_inside_index(handler_src);
	if (inside_index_dst == -1 || inside_index_src == -1)
	{
		return false;
	}
	if (!m_node_manager.merge(inside_index_dst, inside_index_src, manager, merge_function))
	{
		return false;
	}
	remap_handler(handler_src, inside_index_dst);
	return true;
}

template<class T>
inline bool multimap_node_manager<T>::pre_remove_inside_node(inside_index_t inside_index)
{
	if (inside_index >= m_node_manager.size())
	{
		return false;
	}
	if (m_referred_handlers[inside_index].size() != 0)
	{
		for (auto x : m_referred_handlers[inside_index])
		{
			auto iter = m_umap_handler_to_index.find(x);
			m_umap_handler_to_index.erase(iter);
		}
		m_referred_handlers[inside_index].clear();
	}
	return true;
}

template<class T>
inline T* multimap_node_manager<T>::remove_pointer(node_handler_t handler)
{
	inside_index_t inside_index = get_inside_index(handler);
	if (inside_index == -1)
	{
		return nullptr;
	}
	if (!pre_remove_inside_node(inside_index))
	{
		return nullptr;
	}
	return m_node_manager.remove_pointer(inside_index);
}

template<class T>
inline bool multimap_node_manager<T>::remove_node(node_handler_t handler)
{
	inside_index_t inside_index = get_inside_index(handler);
	if (inside_index == -1)
	{
		return true;
	}
	pre_remove_inside_node(inside_index);
	m_node_manager.remove_node(inside_index);
	return true;
}

template<class T>
inline void multimap_node_manager<T>::swap(multimap_node_manager& manager_other)
{
	std::swap(m_next_handler_index, manager_other.m_next_handler_index);
	m_node_manager.swap(manager_other.m_node_manager);
	m_referred_handlers.swap(manager_other.m_referred_handlers);
	m_umap_handler_to_index.swap(manager_other.m_umap_handler_to_index);
}

template<class T>
inline bool multimap_node_manager<T>::remap_handler(node_handler_t handler, inside_index_t inside_index)
{
	if (inside_index >= m_node_manager.size())
	{
		return false;
	}
	inside_index_t inside_index_old = get_inside_index(handler);
	if (inside_index == inside_index_old)
	{
		return true;
	}
	if (inside_index_old != -1)
	{
		if (remove_reference(handler, inside_index_old) == -1)
		{//仅在handler对应的inside_index>m_node_manager.size()时发生，但是所有节点正常删除时会连带删除m_umap_handler_to_index的记录
			throw std::runtime_error("数据结构错乱");
		}
	}
	if (add_reference(handler, inside_index) == -1)
	{
		if (inside_index_old != -1)
		{
			add_reference(handler, inside_index_old);
		}
		return false;
	}
	m_umap_handler_to_index[handler] = inside_index;
	return true;
}

template<class T>
inline size_t multimap_node_manager<T>::add_reference(node_handler_t handler, inside_index_t inside_index)
{
	if (inside_index >= m_node_manager.size())
	{
		return -1;
	}
	m_referred_handlers[inside_index].insert(handler);
	return m_referred_handlers[inside_index].size();
}

template<class T>
inline size_t multimap_node_manager<T>::remove_reference(node_handler_t handler, inside_index_t inside_index)
{
	if (inside_index >= m_node_manager.size())
	{
		return -1;
	}
	auto& ref_uset = m_referred_handlers[inside_index];
	auto iter = ref_uset.find(handler);
	if (iter != ref_uset.end())
	{
		if (ref_uset.size() == 1)
		{
			m_node_manager.remove_node(inside_index);
		}
		ref_uset.erase(iter);
	}
	return ref_uset.size();
}

template<class T>
inline void multimap_node_manager<T>::clear()
{
	m_next_handler_index = 0;
	m_node_manager.clear();
	m_referred_handlers.clear();
	m_umap_handler_to_index.clear();
}

template<class T>
inline void multimap_node_manager<T>::clear_no_release()
{
	m_next_handler_index = 0;
	m_node_manager.clear_no_release();
	m_referred_handlers.clear();
	m_umap_handler_to_index.clear();
}

template<class T>
inline void multimap_node_manager<T>::shrink_to_fit()
{
	m_node_manager.shrink_to_fit();
	m_referred_handlers.shrink_to_fit();
}