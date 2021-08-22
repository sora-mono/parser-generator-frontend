#include "c_parser_frontend.h"

// 获取函数信息
// 如果不存在给定名称的函数则返回空指针
namespace c_parser_frontend {
inline FunctionDefine* CParserFrontend::GetFunction(
    const std::string& function_name) {
  auto iter = functions_.find(function_name);
  if (iter == functions_.end()) [[unlikely]] {
    return nullptr;
  } else {
    return &*iter->second;
  }
}
}  // namespace c_parser_frontend