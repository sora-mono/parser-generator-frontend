#pragma once

#include<vector>
#include<functional>

template<class T>
class node_manager
{
public:
	using node_handler_t = size_t;

	node_manager() {}
	node_manager(const node_manager&) = delete;
	node_manager(node_manager&&) = delete;
	~node_manager();

	static bool default_merge_function_2(T& node_dst, T& node_src) { return node_dst.merge(node_src); }
	template<class Manager>
	static bool default_merge_function_3(T& node_dst, T& node_src, Manager& manager) { return manager.merge(node_dst, node_src); }

	T* get_node(node_handler_t index);
	bool is_same(node_handler_t index1, node_handler_t index2) { return index1 == index2; }
	template<class ...Args>
	node_handler_t emplace_node(Args&&...args);		//系统自行选择最佳位置放置节点
	template<class ...Args>
	node_manager<T>::node_handler_t emplace_node_index(node_handler_t index, Args&&...args);	//在指定位置放置节点
	node_manager<T>::node_handler_t emplace_pointer_index(node_handler_t index, T* pointer);	//在指定位置放置指针
	T* remove_pointer(node_handler_t index);	//仅删除节点
	bool remove_node(node_handler_t index);		//删除节点并释放节点内存
	bool merge(node_handler_t index_dst, node_handler_t index_src, std::function<bool(T&, T&)> merge_function = default_merge_function_2);	//合并两个节点，合并成功会删除index_src节点
	template<class Manager>
	bool merge(node_handler_t index_dst, node_handler_t index_src, Manager& manager,
		const std::function<bool(T&, T&, Manager&)>& merge_function = default_merge_function_3);	//合并两个节点，合并成功会删除index_src节点
	void swap(node_manager& manager_other);
	bool set_merge_allowed(node_handler_t index);
	bool set_merge_refused(node_handler_t index);
	void set_all_merge_allowed();
	void set_all_merge_refused();
	inline bool can_merge(node_handler_t index) { return index < m_vec_can_merge.size() ? m_vec_can_merge[index] : false; }
	size_t size() { return m_vec_nodes.size(); }

	void clear();			//清除并释放所有节点
	void clear_no_release();//清除但不释放所有节点
	void shrink_to_fit();	//调用成员变量的shrink_to_fit

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version = 0);		//序列化容器用
private:
	std::vector<T*> m_vec_nodes;
	std::vector<node_handler_t>m_vec_removed_ids;	//优化用数组，存放被删除节点对应的ID
	std::vector<bool>m_vec_can_merge;		//存储信息表示是否允许合并节点
};

template<class T>
node_manager<T>::~node_manager()
{
	for (T* ptr : m_vec_nodes)
	{
		delete ptr;
	}
}

template<class T>
inline T* node_manager<T>::get_node(node_handler_t index)
{
	if (index > m_vec_nodes.size())
	{
		return nullptr;
	}
	else
	{
		return m_vec_nodes[index];
	}
	// TODO: 在此处插入 return 语句
}

template<class T>
inline bool node_manager<T>::remove_node(node_handler_t index)
{
	if (index > m_vec_nodes.size())
	{
		return false;
	}
	if (m_vec_nodes[index] == nullptr)
	{
		return true;
	}
	m_vec_removed_ids.push_back(index);	//添加到已删除ID中
	delete m_vec_nodes[index];
	m_vec_nodes[index] = nullptr;
	m_vec_can_merge[index] = false;
	while (m_vec_nodes.size() > 0 && m_vec_nodes.back() == nullptr)
	{
		m_vec_nodes.pop_back();		//清理末尾无效ID
		m_vec_can_merge.pop_back();
	}
	return true;
}

template<class T>
inline T* node_manager<T>::remove_pointer(node_handler_t index)
{
	if (index > m_vec_nodes.size())
	{
		return nullptr;
	}
	T* temp_pointer = m_vec_nodes[index];
	if (temp_pointer == nullptr)
	{
		return nullptr;
	}
	m_vec_removed_ids.push_back(index);
	m_vec_can_merge[index] = false;
	while (m_vec_nodes.size() > 0 && m_vec_nodes.back() == nullptr)
	{
		m_vec_nodes.pop_back();		//清理末尾无效ID
		m_vec_can_merge.pop_back();
	}
	return temp_pointer;
}

template<class T>
template<class ...Args>
inline node_manager<T>::node_handler_t node_manager<T>::emplace_node(Args && ...args)
{
	node_handler_t index = -1;
	if (m_vec_removed_ids.size() != 0)
	{
		while (index == -1 && m_vec_removed_ids.size() != 0)	//查找有效ID
		{
			if (m_vec_removed_ids.back() < m_vec_nodes.size())
			{
				index = m_vec_removed_ids.back();
			}
			m_vec_removed_ids.pop_back();
		}
	}
	if (index == -1)	//无有效已删除ID
	{
		index = m_vec_nodes.size();
	}

	return emplace_node_index(index, std::forward<Args>(args)...);	//返回插入位置
}

template<class T>
template<class ...Args>
inline node_manager<T>::node_handler_t node_manager<T>::emplace_node_index(node_handler_t index, Args && ...args)
{
	T* pointer = new T(std::forward<Args>(args)...);
	bool result = emplace_pointer_index(index, pointer);
	if (!result)
	{
		delete pointer;
	}
	return result;
}

template<class T>
inline node_manager<T>::node_handler_t node_manager<T>::emplace_pointer_index(node_handler_t index, T* pointer)
{
	size_t size_old = m_vec_nodes.size();
	if (index >= size_old)
	{
		m_vec_nodes.resize(index + 1, nullptr);
		m_vec_can_merge.resize(index + 1, false);
		for (size_t i = size_old; i < index; i++)
		{
			m_vec_removed_ids.push_back(i);
		}
	}
	if (m_vec_nodes[index] != nullptr && pointer != m_vec_nodes[index])
	{//不可以覆盖已有非空且不同的指针
		return -1;
	}
	m_vec_nodes[index] = pointer;
	m_vec_can_merge[index] = true;
	return index;
}

template<class T>
inline void node_manager<T>::swap(node_manager& manager_other)
{
	m_vec_nodes.swap(manager_other.m_vec_nodes);
	m_vec_removed_ids.swap(manager_other.m_vec_removed_ids);
	m_vec_can_merge.swap(manager_other.m_vec_can_merge);
}

template<class T>
inline bool node_manager<T>::set_merge_allowed(node_handler_t index)
{
	if (index > m_vec_nodes.size() || m_vec_can_merge[index] == nullptr)
	{
		return false;
	}
	m_vec_can_merge[index] = true;
	return true;
}

template<class T>
inline bool node_manager<T>::set_merge_refused(node_handler_t index)
{
	if (index > m_vec_can_merge.size() || m_vec_nodes[index] == nullptr)
	{
		return false;
	}
	m_vec_can_merge[index] = false;
	return true;
}

template<class T>
inline void node_manager<T>::set_all_merge_allowed()
{
	for (size_t index = 0; index < m_vec_nodes.size(); index++)
	{
		if (m_vec_nodes[index] != nullptr)
		{
			m_vec_can_merge[index] = true;
		}
	}
}

template<class T>
inline void node_manager<T>::set_all_merge_refused()
{
	std::vector<bool>vec_temp(m_vec_can_merge.size(), false);
	m_vec_can_merge.swap(vec_temp);
}

template<class T>
inline bool node_manager<T>::merge(node_handler_t index_dst, node_handler_t index_src, std::function<bool(T&, T&)> merge_function)
{
	if (index_dst >= m_vec_nodes.size()
		|| index_src >= m_vec_nodes.size()
		|| m_vec_nodes[index_dst] == nullptr
		|| m_vec_nodes[index_src] == nullptr
		|| !can_merge(index_dst)
		|| !can_merge(index_src))
	{
		return false;
	}
	if (!merge_function(*m_vec_nodes[index_dst], *m_vec_nodes[index_src]))
	{
		return false;
	}
	remove_node(index_src);
	return true;
}

template<class T>
template<class Manager>
bool node_manager<T>::merge(
	typename node_manager<T>::node_handler_t index_dst,
	typename node_manager<T>::node_handler_t index_src,
	Manager& manager,
	const std::function<bool(T&, T&, Manager&)>& merge_function)
{
	if (index_dst >= m_vec_nodes.size()
		|| index_src >= m_vec_nodes.size()
		|| m_vec_nodes[index_dst] == nullptr
		|| m_vec_nodes[index_src] == nullptr
		|| !can_merge(index_dst)
		|| !can_merge(index_src))
	{
		return false;
	}
	if (!merge_function(*m_vec_nodes[index_dst], *m_vec_nodes[index_src], manager))
	{
		return false;
	}
	remove_node(index_src);
	return true;
}

template<class T>
inline void node_manager<T>::clear()
{
	for (auto p : m_vec_nodes)
	{
		delete p;
	}
	m_vec_nodes.clear();
	m_vec_removed_ids.clear();
}

template<class T>
inline void node_manager<T>::clear_no_release()
{
	m_vec_nodes.clear();
	m_vec_removed_ids.clear();
}

template<class T>
inline void node_manager<T>::shrink_to_fit()
{
	while (m_vec_nodes.size() > 0 && m_vec_nodes.back() == nullptr)
	{
		m_vec_nodes.pop_back();
		m_vec_can_merge.pop_back();
	}
	m_vec_nodes.shrink_to_fit();
	m_vec_removed_ids.shrink_to_fit();
}