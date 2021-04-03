#pragma once

#include"common.h"
#include<string>

string_hash_type hash_string(const std::string& str)
{
	string_hash_type hash_result = 0;
	for (auto x : str)
	{
		hash_result = hash_result * hash_seed + x;
	}
	return hash_result;
}