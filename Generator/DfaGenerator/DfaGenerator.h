#pragma once

#include "NfaGenerator.h"
#include "common.h"

class DfaGenerator {
 public:
  DfaGenerator();
  DfaGenerator(const DfaGenerator&) = delete;
  DfaGenerator(DfaGenerator&&) = delete;

  int AddKeyword(const std::string& str);

 private:
  NfaGenerator nfa_generator_;
  std::vector<std::string> keywords_;
  //用vector存储是因为hash可能溢出，导致两个字符串对应同一个hash值
  std::unordered_map<string_hash_type, std::vector<size_t>> hashnum_to_index_;
};