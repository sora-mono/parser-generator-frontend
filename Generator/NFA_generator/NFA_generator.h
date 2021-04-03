#pragma once

#include"multimap_node_manager.h"

#include<iostream>
#include<vector>
#include<unordered_map>
#include<unordered_set>

class NFA_generator
{
public:
	struct NFA_node;
	using node_handler_t = multimap_node_manager<NFA_node>::node_handler_t;
	using node_gather_t = size_t;

	struct NFA_node
	{
		NFA_node(){}
		NFA_node(const NFA_node& node) :
			m_umap_nodes_forward(node.m_umap_nodes_forward),
			m_vec_nocondition_transfer_nodes_index(node.m_vec_nocondition_transfer_nodes_index) {}
		NFA_node(NFA_node&& node) :
			m_umap_nodes_forward(std::move(node.m_umap_nodes_forward)),
			m_vec_nocondition_transfer_nodes_index(std::move(node.m_vec_nocondition_transfer_nodes_index)) {}

		node_handler_t get_forward_nodes_index(char c_transfer);
		const std::vector<node_handler_t>& get_uncondition_transfer_nodes_index();
		bool add_transfer(char c_transfer, node_handler_t node_index);	//添加转移条件下节点，遇到已存在节点会返回false
		bool add_nocondition_transfer(node_handler_t node_index);			//添加无条件转移节点
		bool remove_transfer(char c_treasfer);							//移除转移条件，函数保证执行后不存在节点（无论原来是否存在)
		bool remove_nocondition_transfer(node_handler_t node_index);		//同上，移除无条件节点，输入-1代表清除所有

		std::unordered_map<char, node_gather_t>m_umap_nodes_forward;	//记录转移条件与前向节点，一个条件仅允许对应一个节点
		std::vector<node_handler_t>m_vec_nocondition_transfer_nodes_index;	//存储无条件转移节点
	};

	NFA_generator();
	NFA_generator(const NFA_generator&) = delete;
	NFA_generator(NFA_generator&&) = delete;

	//生成tail无条件转移到head的记录，如设置了allow_merge则可能将head合并到tail并修改tail指向合并后节点，仅在不会向head节点添加条件转移时设置该位
	bool add_nocondition_transfer(node_handler_t node_tail_index, node_handler_t& node_head_index, bool allow_merge);
	std::pair<node_handler_t, node_handler_t> regex_construct(std::istream& in, bool add_to_NFA_head, bool return_when_right_bracket);	//解析正则并添加到已有NFA中，返回生成的自动机的头结点和尾节点
	std::pair<node_handler_t, node_handler_t> word_construct(const std::string& str);	//添加一个由字符串构成的NFA

private:
	node_handler_t head_node_index;		//所有NFA的头结点

	auto add_tail_node(node_handler_t index) { m_uset_tail_nodes.insert(index); }
	std::pair<node_handler_t, node_handler_t> create_switch_tree(std::istream& in);	//生成可选字符序列，会读取]后的*,+,?等限定符

	std::unordered_set<node_handler_t> m_uset_tail_nodes;	//该set用来存储所有尾节点
	multimap_node_manager<NFA_node>m_node_manager;
};