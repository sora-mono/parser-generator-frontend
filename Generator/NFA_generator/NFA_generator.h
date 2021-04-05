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
	using tail_node_tag = std::pair<size_t, size_t>;	//前半部分为tag序号，后半部分为优先级，数字越大优先级越高
	friend bool merge(NFA_node& node_dst, NFA_node& node_src, NFA_generator& generator);

	struct NFA_node
	{
		NFA_node() {}
		NFA_node(const NFA_node& node) :
			m_umap_nodes_forward(node.m_umap_nodes_forward),
			m_uset_nocondition_transfer_nodes_handler(node.m_uset_nocondition_transfer_nodes_handler) {}
		NFA_node(NFA_node&& node) :
			m_umap_nodes_forward(std::move(node.m_umap_nodes_forward)),
			m_uset_nocondition_transfer_nodes_handler(std::move(node.m_uset_nocondition_transfer_nodes_handler)) {}

		node_handler_t get_forward_nodes_handler(char c_transfer);
		const std::unordered_set<node_handler_t>& get_uncondition_transfer_nodes_handler();
		bool add_condition_transfer(char c_transfer, node_handler_t node_handler);		//添加条件转移节点，遇到已存在节点会返回false
		bool add_nocondition_transfer(node_handler_t node_handler);			//添加无条件转移节点
		bool remove_condition_transfer(char c_treasfer);								//移除一个转移条件节点，函数保证执行后不存在节点（无论原来是否存在)
		bool remove_nocondition_transfer(node_handler_t node_handler);		//同上，移除一个无条件节点，输入-1代表清除所有

		bool merge(NFA_node& node_src);

		std::unordered_map<char, node_gather_t>m_umap_nodes_forward;	//记录转移条件与前向节点，一个条件仅允许对应一个节点
		std::unordered_set<node_handler_t>m_uset_nocondition_transfer_nodes_handler;	//存储无条件转移节点
	};

	NFA_generator();
	NFA_generator(const NFA_generator&) = delete;
	NFA_generator(NFA_generator&&) = delete;

	const tail_node_tag get_tail_tag(NFA_node* pointer);
	const tail_node_tag get_tail_tag(node_handler_t handler);
	NFA_node* get_node(node_handler_t handler);
	//解析正则并添加到已有NFA中，返回生成的自动机的头结点和尾节点，自动处理结尾的范围限制符号
	std::pair<node_handler_t, node_handler_t> regex_construct(std::istream& in, const tail_node_tag& tag, bool add_to_NFA_head, bool return_when_right_bracket);
	std::pair<node_handler_t, node_handler_t> word_construct(const std::string& str, const tail_node_tag& tag);	//添加一个由字符串构成的NFA，自动处理结尾的范围限制符号
	void merge_optimization();		//合并优化，降低节点数以降低子集构造法集合大小，直接使用NFA也可以降低成本

	std::pair<std::unordered_set<node_handler_t>, tail_node_tag>
		closure(node_handler_t handler);
	std::pair<std::unordered_set<node_handler_t>, tail_node_tag>
		GOTO(node_handler_t handler_src, char c_transform);	//返回goto后的节点的闭包

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version = 0);		//序列化容器用

private:
	bool remove_tail_node(NFA_node* pointer);
	bool add_tail_node(NFA_node* pointer, const tail_node_tag& tag);
	bool add_tail_node(node_handler_t handler, const tail_node_tag& tag) { return add_tail_node(get_node(handler), tag); }

	std::pair<node_handler_t, node_handler_t> create_switch_tree(std::istream& in);	//生成可选字符序列，会读取]后的*,+,?等限定符

	node_handler_t head_node_handler;		//所有NFA的头结点
	std::unordered_map<NFA_node*, tail_node_tag> m_umap_tail_nodes;	//该set用来存储所有尾节点和对应单词的tag
	multimap_node_manager<NFA_node>m_node_manager;
};

bool merge(NFA_generator::NFA_node& node_dst, NFA_generator::NFA_node& node_src, NFA_generator& generator);