#pragma once

#include<iostream>
#include<vector>
#include<set>

using string_hash_type = unsigned long long;

const unsigned long long hash_seed = 131;	// 31 131 1313 13131 131313 etc..

string_hash_type hash_string(const std::string& str);