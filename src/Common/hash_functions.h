#ifndef COMMON_HASH_FUNCTIONS_H_
#define COMMON_HASH_FUNCTIONS_H_

#include <boost/serialization/set.hpp>
#include <boost/serialization/unordered_set.hpp>
#include <boost/serialization/vector.hpp>
#include <iostream>

#include "Common/common.h"
#include "Common/id_wrapper.h"

namespace frontend::common {
enum class WrapperLabel { kStringHashType, kIntergalSetHashType };
using StringHashType = size_t;
using IntergalSetHashType = size_t;

constexpr unsigned long long hash_seed = 131;  // 31 131 1313 13131 131313 etc..

StringHashType HashString(const std::string& str);

template <class IdType>
inline IntergalSetHashType HashIntergalUnorderedSet(
    const std::unordered_set<IdType> intergal_set) {
  IntergalSetHashType result = 1;
  for (auto x : intergal_set) {
    result *= x;
  }
  return result;
}

template <class IdType>
inline IntergalSetHashType HashIntergalSet(
    const std::set<IdType> intergal_set) {
  IntergalSetHashType result = 1;
  for (auto x : intergal_set) {
    result *= x;
  }
  return result;
}

template <class IdType>
inline IntergalSetHashType HashIntergalVector(
    const std::vector<IdType> intergal_vec) {
  IntergalSetHashType result = 1;
  for (auto x : intergal_vec) {
    result *= x;
  }
  return result;
}

}  // namespace frontend::common
#endif  // !COMMON_COMMON_COMMON