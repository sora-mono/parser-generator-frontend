#pragma once

#include "common.h"

#include <string>

StringHashType HashString(const std::string& str) {
  StringHashType hash_result = 0;
  for (auto x : str) {
    hash_result = hash_result * hash_seed + x;
  }
  return hash_result;
}