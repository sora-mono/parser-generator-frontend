#include <iostream>
#include <set>
#include <unordered_set>
#include <vector>

#ifndef COMMON_COMMON_H_
#define COMMON_COMMON_H_

namespace frontend::common {
using StringHashType = unsigned long long;
using IntergalSetHashType = unsigned long long;

const unsigned long long hash_seed = 131;  // 31 131 1313 13131 131313 etc..
const size_t kCharNum = CHAR_MAX - CHAR_MIN + 1;
StringHashType HashString(const std::string& str);

template <class T>
requires std::is_integral_v<T> inline IntergalSetHashType
HashIntergalUnorderedSet(const std::unordered_set<T> intergal_set) {
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
requires std::is_integral_v<T> inline IntergalSetHashType HashIntergalVector(
    const std::vector<T> intergal_vec) {
  IntergalSetHashType result = 1;
  for (auto x : intergal_vec) {
    result *= x;
  }
  return result;
}

}  // namespace frontend::common
#endif  // !COMMON_COMMON_COMMON