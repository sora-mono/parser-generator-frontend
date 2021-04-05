#include "NFA_generator.h"
#include<algorithm>
#include<sstream>
#include<queue>

NFA_generator::node_handler_t NFA_generator::NFA_node::get_forward_nodes_handler(char c_transfer)
{
	auto iter = m_umap_nodes_forward.find(c_transfer);
	if (iter == m_umap_nodes_forward.end())
	{
		return -1;
	}
	return iter->second;
}

inline const std::unordered_set<NFA_generator::node_handler_t>&
NFA_generator::NFA_node::get_uncondition_transfer_nodes_handler()
{
	return NFA_generator::NFA_node::m_uset_nocondition_transfer_nodes_handler;
}

inline bool NFA_generator::NFA_node::add_condition_transfer(char c_condition, NFA_generator::node_handler_t node_handler)
{
	auto iter = m_umap_nodes_forward.find(c_condition);
	if (iter != m_umap_nodes_forward.end() && iter->second != node_handler)
	{//该条件下已有不同转移节点，不能覆盖
		return false;
	}
	m_umap_nodes_forward[c_condition] = node_handler;
	return true;
}

inline bool NFA_generator::NFA_node::add_nocondition_transfer(node_handler_t node_handler)
{
	m_uset_nocondition_transfer_nodes_handler.insert(node_handler);
	return true;
}

inline bool NFA_generator::NFA_node::remove_condition_transfer(char c_treasfer)
{
	auto iter = m_umap_nodes_forward.find(c_treasfer);
	if (iter != m_umap_nodes_forward.end())
	{
		m_umap_nodes_forward.erase(iter);
	}
	return true;
}

inline bool NFA_generator::NFA_node::remove_nocondition_transfer(node_handler_t node_handler)
{
	if (node_handler == -1)
	{
		m_uset_nocondition_transfer_nodes_handler.clear();
	}
	else
	{
		auto iter = m_uset_nocondition_transfer_nodes_handler.find(node_handler);
		if (iter != m_uset_nocondition_transfer_nodes_handler.end())
		{
			m_uset_nocondition_transfer_nodes_handler.erase(iter);
		}
	}
	return true;
}

std::pair<std::unordered_set<typename NFA_generator::node_handler_t>, typename NFA_generator::tail_node_tag>
NFA_generator::closure(node_handler_t handler)
{
	std::unordered_set<node_handler_t> uset_temp = m_node_manager.get_handlers_referring_same_node(handler);
	tail_node_tag tag(-1,-1);
	std::queue<node_handler_t>q;
	for (auto x : uset_temp)
	{
		q.push(x);
	}
	while (!q.empty())
	{
		node_handler_t handler_now = q.front();
		q.pop();
		if (uset_temp.find(handler_now)!=uset_temp.end())
		{
			continue;
		}
		uset_temp.insert(handler_now);
		auto iter = m_umap_tail_nodes.find(get_node(handler_now));	//判断是否为尾节点
		if (iter!=m_umap_tail_nodes.end())
		{
			if (tag == tail_node_tag(-1,-1))
			{//以前无尾节点记录
				tag = iter->second;
			}
			else if (iter->second.second>tag.second)
			{//当前记录优先级大于以前的优先级
				tag = iter->second;
			}
		}
		const std::unordered_set<node_handler_t> uset_nodes = m_node_manager.get_handlers_referring_same_node(handler_now);
		for (auto x : uset_nodes)
		{
			q.push(x);
		}
	}
	return std::pair<std::unordered_set<node_handler_t>, tail_node_tag>(std::move(uset_temp),std::move(tag));
}

std::pair<std::unordered_set<typename NFA_generator::node_handler_t>, typename NFA_generator::tail_node_tag>
NFA_generator::GOTO(node_handler_t handler_src, char c_transform)
{
	NFA_node* pointer_node = get_node(handler_src);
	if (pointer_node == nullptr)
	{
		return std::pair(std::unordered_set<node_handler_t>(), tail_node_tag(-1,-1));
	}
	node_handler_t handler = pointer_node->get_forward_nodes_handler(c_transform);
	if (handler == -1)
	{
		return std::pair(std::unordered_set<node_handler_t>(), tail_node_tag(-1, -1));
	}
	return closure(handler);
}

bool NFA_generator::NFA_node::merge(NFA_node& node_src)
{
	if (&node_src == this)
	{//相同节点合并则直接返回true
		return true;
	}
	if (m_umap_nodes_forward.size() != 0 && node_src.m_umap_nodes_forward.size() != 0)
	{
		bool can_merge = true;
		for (auto& p : node_src.m_umap_nodes_forward)
		{
			auto iter = m_umap_nodes_forward.find(p.first);
			if (iter != m_umap_nodes_forward.end())
			{
				can_merge = false;
				break;
			}
		}
		if (!can_merge)
		{
			return false;
		}
	}
	m_umap_nodes_forward.merge(node_src.m_umap_nodes_forward);
	m_uset_nocondition_transfer_nodes_handler.merge(node_src.m_uset_nocondition_transfer_nodes_handler);
	return true;
}

NFA_generator::NFA_generator()
{
	head_node_handler = m_node_manager.emplace_node();	//添加头结点
}

inline const NFA_generator::tail_node_tag NFA_generator::get_tail_tag(NFA_node* pointer)
{
	auto iter = m_umap_tail_nodes.find(pointer);
	if (iter == m_umap_tail_nodes.end())
	{
		return tail_node_tag(-1, -1);
	}
	return iter->second;
}

inline const NFA_generator::tail_node_tag NFA_generator::get_tail_tag(node_handler_t handler)
{
	return get_tail_tag(get_node(handler));
}

inline NFA_generator::NFA_node* NFA_generator::get_node(node_handler_t handler)
{
	return m_node_manager.get_node(handler);
}

std::pair<NFA_generator::node_handler_t, NFA_generator::node_handler_t>
NFA_generator::regex_construct(std::istream& in, const tail_node_tag& tag, bool add_to_NFA_head, bool return_when_right_bracket)
{
	node_handler_t head_handler = m_node_manager.emplace_node();
	node_handler_t tail_handler = head_handler;
	node_handler_t pre_tail_handler = head_handler;
	char c_now;
	in >> c_now;
	while (c_now != '\0' && in)
	{
		node_handler_t temp_head_handler = -1, temp_tail_handler = -1;
		switch (c_now)
		{
		case '[':
			in.putback(c_now);
			std::pair(temp_head_handler, temp_tail_handler) = create_switch_tree(in);
			if (temp_head_handler == -1 || temp_tail_handler == -1)
			{
				throw std::invalid_argument("非法正则");
			}
			get_node(tail_handler)->add_nocondition_transfer(temp_tail_handler);
			pre_tail_handler = tail_handler;
			tail_handler = temp_tail_handler;
			break;
		case ']':
			throw std::runtime_error("regex_construct函数不应该处理]字符，应交给create_switch_tree处理");
			break;
		case '(':
			std::pair(temp_head_handler, temp_tail_handler) = regex_construct(in, tail_node_tag(-1, -1), false, true);
			if (temp_head_handler == -1 || temp_tail_handler == -1)
			{
				throw std::invalid_argument("非法正则");
			}
			get_node(tail_handler)->add_nocondition_transfer(temp_head_handler);
			pre_tail_handler = tail_handler;
			tail_handler = temp_tail_handler;
			break;
		case ')':
			in >> c_now;
			if (!in)
			{
				c_now = '\0';
				break;
			}
			switch (c_now)
			{
			case '*':
				temp_tail_handler = m_node_manager.emplace_node();
				get_node(tail_handler)->add_nocondition_transfer(temp_tail_handler);
				get_node(temp_tail_handler)->add_nocondition_transfer(head_handler);
				get_node(head_handler)->add_nocondition_transfer(temp_tail_handler);
				pre_tail_handler = tail_handler;
				tail_handler = temp_tail_handler;
				break;
			case '+':
				temp_tail_handler = m_node_manager.emplace_node();
				get_node(temp_tail_handler)->add_nocondition_transfer(head_handler);
				get_node(tail_handler)->add_nocondition_transfer(head_handler);
				pre_tail_handler = tail_handler;
				tail_handler = temp_tail_handler;
				break;
			case '?':
				temp_tail_handler = m_node_manager.emplace_node();
				get_node(tail_handler)->add_nocondition_transfer(temp_tail_handler);
				get_node(head_handler)->add_nocondition_transfer(temp_tail_handler);
				pre_tail_handler = tail_handler;
				tail_handler = temp_tail_handler;
				break;
			default:
				in.putback(c_now);
				break;
			}
			if (return_when_right_bracket)
			{
				c_now = '\0';
			}
			break;
		case '+':	//仅对单个字符生效
			temp_tail_handler = m_node_manager.emplace_node();
			get_node(tail_handler)->add_nocondition_transfer(temp_tail_handler);
			get_node(pre_tail_handler)->add_nocondition_transfer(temp_tail_handler);
			pre_tail_handler = tail_handler;
			tail_handler = temp_tail_handler;
			break;
		case '*':	//仅对单个字符生效
			temp_tail_handler = m_node_manager.emplace_node();
			get_node(tail_handler)->add_nocondition_transfer(temp_tail_handler);
			get_node(pre_tail_handler)->add_nocondition_transfer(temp_tail_handler);
			get_node(temp_tail_handler)->add_nocondition_transfer(pre_tail_handler);
			pre_tail_handler = tail_handler;
			tail_handler = temp_tail_handler;
			break;
		case '?':
			temp_tail_handler = m_node_manager.emplace_node();
			get_node(tail_handler)->add_nocondition_transfer(temp_tail_handler);
			get_node(pre_tail_handler)->add_nocondition_transfer(temp_tail_handler);
			pre_tail_handler = tail_handler;
			tail_handler = pre_tail_handler;
			break;
		case '\\':
			in >> c_now;
			if (!in || c_now == '\0')
			{
				throw std::invalid_argument("非法正则");
			}
			temp_tail_handler = m_node_manager.emplace_node();
			get_node(tail_handler)->add_condition_transfer(c_now, temp_tail_handler);
			pre_tail_handler = tail_handler;
			tail_handler = temp_tail_handler;
			break;
		default:
			temp_tail_handler = m_node_manager.emplace_node();
			get_node(tail_handler)->add_condition_transfer(c_now, temp_tail_handler);
			pre_tail_handler = tail_handler;
			tail_handler = temp_tail_handler;
			break;
		}
		if (c_now != '\0')
		{
			in >> c_now;
		}
	}
	if (head_handler != tail_handler)
	{
		if (add_to_NFA_head)
		{
			get_node(head_node_handler)->add_nocondition_transfer(head_handler);
			add_tail_node(tail_handler, tag);
		}
	}
	else
	{
		m_node_manager.remove_node(head_handler);
		head_handler = tail_handler = -1;
	}
	return std::pair<node_handler_t, node_handler_t>(head_handler, tail_handler);
}

std::pair<NFA_generator::node_handler_t, NFA_generator::node_handler_t>
NFA_generator::word_construct(const std::string& str, const tail_node_tag& tag)
{
	if (str.size() == 0)
	{
		return std::pair(-1, -1);
	}
	node_handler_t head_handler = m_node_manager.emplace_node();
	node_handler_t tail_handler = head_handler;
	for (auto c : str)
	{
		node_handler_t temp_handler = m_node_manager.emplace_node();
		get_node(tail_handler)->add_condition_transfer(c, temp_handler);
		tail_handler = temp_handler;
	}
	get_node(head_node_handler)->add_nocondition_transfer(head_handler);
	add_tail_node(tail_handler, tag);
	return std::pair(head_handler, tail_handler);
}

void NFA_generator::merge_optimization()
{
	m_node_manager.set_all_merge_allowed();
	std::queue<node_handler_t>q;
	for (auto x : get_node(head_node_handler)->m_uset_nocondition_transfer_nodes_handler)
	{
		q.push(x);
	}
	while (!q.empty())
	{
		node_handler_t handler_now = q.front();
		q.pop();
		bool merged = false;
		bool can_merge = m_node_manager.can_merge(handler_now);
		if (!can_merge)
		{
			continue;
		}
		for (auto x : get_node(handler_now)->m_uset_nocondition_transfer_nodes_handler)
		{
			merged |= m_node_manager.merge<NFA_generator>(handler_now, x, *this, merge);
			q.push(x);
		}
		if (merged)
		{
			q.push(handler_now);
		}
		else
		{
			m_node_manager.set_merge_refused(handler_now);
		}
	}
}

inline bool NFA_generator::remove_tail_node(NFA_node* pointer)
{
	if (pointer == nullptr)
	{
		return false;
	}
	m_umap_tail_nodes.erase(pointer);
	return true;
}

bool NFA_generator::add_tail_node(NFA_node* pointer, const tail_node_tag& tag)
{
	if (pointer == nullptr)
	{
		return false;
	}
	m_umap_tail_nodes.insert(std::pair(pointer, tag));
	return false;
}

std::pair<NFA_generator::node_handler_t, NFA_generator::node_handler_t>
NFA_generator::create_switch_tree(std::istream& in)
{
	node_handler_t head_handler = m_node_manager.emplace_node();
	node_handler_t tail_handler = m_node_manager.emplace_node();
	char c_now;
	in >> c_now;
	while (in && c_now != ']')
	{
		get_node(head_handler)->add_condition_transfer(c_now, tail_handler);
	}
	if (head_handler == tail_handler)
	{
		m_node_manager.remove_node(head_handler);
		return std::pair(-1, -1);
	}
	if (c_now != ']')
	{
		throw std::invalid_argument("非法正则");
	}
	in >> c_now;
	if (!in)
	{
		return std::pair(head_handler, tail_handler);
	}
	switch (c_now)
	{
	case '*':
		get_node(head_handler)->add_nocondition_transfer(tail_handler);
		get_node(tail_handler)->add_nocondition_transfer(head_handler);
		break;
	case '+':
		get_node(tail_handler)->add_nocondition_transfer(head_handler);
		break;
	case '?':
		get_node(head_handler)->add_nocondition_transfer(tail_handler);
		break;
	default:
		in.putback(c_now);
		break;
	}
	return std::pair(head_handler, tail_handler);
}

bool merge(NFA_generator::NFA_node& node_dst, NFA_generator::NFA_node& node_src, NFA_generator& generator)
{
	NFA_generator::tail_node_tag dst_tag = generator.get_tail_tag(&node_dst);
	NFA_generator::tail_node_tag src_tag = generator.get_tail_tag(&node_src);
	if (dst_tag != NFA_generator::tail_node_tag(-1, -1) && src_tag != NFA_generator::tail_node_tag(-1, -1))
	{//两个都为尾节点的节点不能合并
		return false;
	}
	bool result = node_dst.merge(node_src);
	if (result)
	{
		if (src_tag != NFA_generator::tail_node_tag(-1, -1))
		{
			generator.remove_tail_node(&node_src);
			generator.add_tail_node(&node_dst, src_tag);
		}
		return true;
	}
	else
	{
		return false;
	}
}