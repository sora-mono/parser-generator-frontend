#include "NFA_generator.h"
#include<algorithm>

NFA_generator::node_handler_t NFA_generator::NFA_node::get_forward_nodes_index(char c_transfer)
{
	auto iter = m_umap_nodes_forward.find(c_transfer);
	if (iter == m_umap_nodes_forward.end())
	{
		return -1;
	}
	return iter->second;
}

inline const std::vector<NFA_generator::node_handler_t>&
NFA_generator::NFA_node::get_uncondition_transfer_nodes_index()
{
	return NFA_generator::NFA_node::m_vec_nocondition_transfer_nodes_index;
}

bool NFA_generator::NFA_node::add_transfer(char c_transfer, NFA_generator::node_handler_t node_index)
{
	auto iter = m_umap_nodes_forward.find(c_transfer);
	if (iter != m_umap_nodes_forward.end())
	{
		return false;
	}
	m_umap_nodes_forward[c_transfer] = node_index;
	return true;
}

inline bool NFA_generator::NFA_node::add_nocondition_transfer(node_handler_t node_index)
{
	m_vec_nocondition_transfer_nodes_index.push_back(node_index);
	return true;
}

bool NFA_generator::NFA_node::remove_transfer(char c_treasfer)
{
	auto iter = m_umap_nodes_forward.find(c_treasfer);
	if (iter != m_umap_nodes_forward.end())
	{
		m_umap_nodes_forward.erase(iter);
	}
	return true;
}

bool NFA_generator::NFA_node::remove_nocondition_transfer(node_handler_t node_index)
{
	if (node_index == -1)
	{
		m_vec_nocondition_transfer_nodes_index.clear();
	}
	else
	{
		for (auto iter = m_vec_nocondition_transfer_nodes_index.begin();
			iter != m_vec_nocondition_transfer_nodes_index.end(); iter++)
		{
			if (*iter == node_index)
			{
				m_vec_nocondition_transfer_nodes_index.erase(iter);
				break;
			}
		}
	}
	return true;
}

NFA_generator::NFA_generator()
{
	head_node_index = m_node_manager.emplace_node();	//添加头结点
}

bool NFA_generator::add_nocondition_transfer(node_handler_t node_tail_index, node_handler_t& node_head_index, bool allow_merge)
{
	NFA_node* node_tail = m_node_manager.get_node(node_tail_index);
	NFA_node* node_head = m_node_manager.get_node(node_head_index);
	if (node_tail == nullptr || node_head == nullptr)
	{
		return false;
	}
	if (allow_merge == true && node_tail->m_umap_nodes_forward.size() == 0 || node_head->m_umap_nodes_forward.size() == 0)
	{
		node_tail->m_umap_nodes_forward.merge(std::move(node_head->m_umap_nodes_forward));
		for (auto x : node_head->m_vec_nocondition_transfer_nodes_index)
		{
			node_tail->add_nocondition_transfer(x);
		}
		std::vector<node_handler_t>* vec_nocnodes_index = &node_tail->m_vec_nocondition_transfer_nodes_index;
		std::sort(vec_nocnodes_index->begin(), vec_nocnodes_index->end());
		auto new_end = std::unique(vec_nocnodes_index->begin(), vec_nocnodes_index->end());
		node_tail->m_vec_nocondition_transfer_nodes_index.erase(++new_end, vec_nocnodes_index->end());
		m_node_manager.remove_node(node_head_index);
	}
	else
	{
		node_tail->add_nocondition_transfer(node_head_index);
	}
	return true;
}

std::pair<NFA_generator::node_handler_t, NFA_generator::node_handler_t>
NFA_generator::regex_construct(std::istream& in, bool add_to_NFA_head, bool return_when_right_bracket)
{
	node_handler_t head_index = m_node_manager.emplace_node();
	node_handler_t tail_index = head_index;
	char c_now;
	in >> c_now;
	while (c_now != '\0' && in)
	{
		node_handler_t temp_head_index = -1, temp_tail_index = -1;
		switch (c_now)
		{
		case '[':
			in.putback(c_now);
			std::pair(temp_head_index, temp_tail_index) = create_switch_tree(in);
			add_nocondition_transfer(tail_index, temp_head_index, false);
			tail_index = temp_tail_index;
			break;
		case ']':
			throw std::runtime_error("regex_construct函数不应该处理]字符，应交给create_switch_tree处理");
			break;
		case '(':
			std::pair(temp_head_index, temp_tail_index) = regex_construct(in, false, true);		//自动处理)后的范围限定符
			add_nocondition_transfer(tail_index, temp_head_index, false);
			tail_index = temp_tail_index;
			break;
		case ')':
			in >> c_now;
			if (!in)
			{
				break;
			}
			switch (c_now)
			{
			case '*':
				
				temp_tail_index = m_node_manager.emplace_node();
				m_node_manager.get_node(tail_index)->add_nocondition_transfer(temp_tail_index);
				m_node_manager.get_node(temp_tail_index)->add_nocondition_transfer(tail_index);
				tail_index = temp_tail_index;
				break;
			case '+':
				temp_tail_index = m_node_manager.emplace_node();
				m_node_manager.get_node(temp_tail_index)->add_nocondition_transfer(tail_index);
				tail_index = temp_tail_index;
				break;
			case '?':
				temp_tail_index = m_node_manager.emplace_node();
				
					break;
			default:
				in.putback(c_now);
				break;
			}
			break;
		case '+':
			break;
		case '*':
			break;
		case '?':
			break;
		case '\\':
			break;
		default:
			break;
		}
		in >> c_now;
	}
	if (head_index != tail_index)
	{
		if (add_to_NFA_head == true)
		{
			m_node_manager.get_node(head_node_index)->add_nocondition_transfer(head_index);
			add_tail_node(tail_index);
		}
	}
	else
	{
		m_node_manager.remove_node(head_index);
		head_index = tail_index = -1;
	}
	return std::pair<node_handler_t, node_handler_t>(head_index, tail_index);
}

std::pair<NFA_generator::node_handler_t, NFA_generator::node_handler_t>
NFA_generator::word_construct(const std::string& str)
{
	return std::pair<node_handler_t, node_handler_t>();
}

std::pair<NFA_generator::node_handler_t, NFA_generator::node_handler_t>
NFA_generator::create_switch_tree(std::istream& in)
{
	return std::pair<node_handler_t, node_handler_t>();
}
