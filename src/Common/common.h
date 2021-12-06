/// @file common.h
/// @brief 存储公开的定义和类型

#ifndef COMMON_COMMON_H_
#define COMMON_COMMON_H_

#include <climits>

namespace frontend::common {
/// @brief 语法分析机配置文件名
constexpr const char* kSyntaxConfigFileName = "syntax_config.conf";
/// @brief 词法分析机配置文件名
constexpr const char* kDfaConfigFileName = "dfa_config.conf";
/// @brief char可能取值的数目
constexpr size_t kCharNum = CHAR_MAX - CHAR_MIN + 1;

/// @brief 运算符结合类型：左结合，右结合
enum class OperatorAssociatityType { kLeftToRight, kRightToLeft };

}  // namespace frontend::common

#endif  /// !COMMON_COMMON_H_