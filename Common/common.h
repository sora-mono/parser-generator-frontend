#pragma once

#include <iostream>
#include <set>
#include <unordered_set>
#include <vector>

using StringHashType = unsigned long long;
using IntergalSetHashType = unsigned long long;

const unsigned long long hash_seed = 131;  // 31 131 1313 13131 131313 etc..
const size_t kchar_num = 256;
StringHashType HashString(const std::string& str);

template <class T>
requires std::is_integral_v<T> inline IntergalSetHashType HashIntergalSet(
    const std::unordered_set<T> intergal_set) {
  IntergalSetHashType result = 1;
  for (auto x : intergal_set) {
    result *= x;
  }
  return result;
}

template <class T>
requires std::is_integral_v<T> inline IntergalSetHashType HashIntergalSet(
    const std::set<T> intergal_set) {
  IntergalSetHashType result = 1;
  for (auto x : intergal_set) {
    result *= x;
  }
  return result;
}

template <class T>
requires std::is_integral_v<T> inline IntergalSetHashType HashIntergalSet(
    const std::vector<T> intergal_vec) {
  IntergalSetHashType result = 1;
  for (auto x : intergal_vec) {
    result *= x;
  }
  return result;
}
