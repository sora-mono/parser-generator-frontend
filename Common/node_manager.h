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

	bool default_merge_function(T& node_dst, T& node_src) { return node_dst.merge(node_src); }

	T* get_node(node_handler_t index);
	bool is_same(node_handler_t index1, node_handler_t index2) { return index1 == index2; }
	template<class ...Args>
	node_handler_t emplace_node(Args&&...args);		//系统自行选择最佳位置放置节点
	template<class ...Args>
	node_manager<T>::node_handler_t emplace_node_index(node_handler_t index, Args&&...args);	//在指定位置放置节点
	node_manager<T>::node_handler_t emplace_pointer_index(node_handler_t index, T* pointer);	//在指定位置放置指针
	T* remove_pointer(node_handler_t index);	//仅删除节点
	bool remove_node(node_handler_t index);		//删除节点并释放节点内存
	bool merge(node_handler_t index_dst, node_handler_t index_src, std::function<bool(T&, T&)> merge_function = default_merge_function);	//合并两个节点，合并成功会删除index_src节点
	void swap(node_manager& manager_other);
	size_t size() { return m_vec_nodes.size(); }
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version = 0);		//序列化容器用
	void clear();			//清除并释放所有节点
	void clear_no_release();//清除但不释放所有节点
	void shrink_to_fit();	//调用成员变量的shrink_to_fit

private:
	std::vector<T*> m_vec_nodes;
	std::vector<node_handler_t>m_vec_removed_ids;	//优化用数组，存放被删除节点对应的ID
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
	if (m_vec_nodes[index] != nullptr)
	{
		m_vec_removed_ids.push_back(index);	//添加到已删除ID中
	}
	delete m_vec_nodes[index];
	m_vec_nodes[index] = nullptr;
	remove_pointer(index);
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
	if (temp_pointer != nullptr)
	{
		m_vec_removed_ids.push_back(index);
	}
	while (m_vec_nodes.size() > 0 && m_vec_nodes.back() == nullptr)
	{
		m_vec_nodes.pop_back();		//清理末尾无效ID
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
	return emplace_pointer_index(index, pointer);
}

template<class T>
inline node_manager<T>::node_handler_t node_manager<T>::emplace_pointer_index(node_handler_t index, T* pointer)
{
	size_t size_old = m_vec_nodes.size();
	if (index >= size_old)
	{
		m_vec_nodes.resize(index + 1, nullptr);
		for (size_t i = size_old; i < index; i++)
		{
			m_vec_removed_ids.push_back(i);
		}
	}
	if (m_vec_nodes[index] == nullptr)
	{
		return -1;
	}
	m_vec_nodes[index] = pointer;
	return index;
}

template<class T>
inline void node_manager<T>::swap(node_manager& manager_other)
{
	m_vec_nodes.swap(manager_other.m_vec_nodes);
	m_vec_removed_ids.swap(manager_other.m_vec_removed_ids);
}

template<class T>
inline bool node_manager<T>::merge(node_handler_t index_dst, node_handler_t index_src, std::function<bool(T&, T&)> merge_function)
{
	if (index_dst >= m_vec_nodes.size()
		|| index_src >= m_vec_nodes.size()
		|| m_vec_nodes[index_dst] == nullptr
		|| m_vec_nodes[index_src] == nullptr)
	{
		return false;
	}
	if (!m_vec_nodes[index_dst]->merge(m_vec_nodes[index_src]))
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
	m_vec_nodes.shrink_to_fit();
	m_vec_removed_ids.shrink_to_fit();
}