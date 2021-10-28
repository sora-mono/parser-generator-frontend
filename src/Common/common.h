#ifndef COMMON_COMMON_H_
#define COMMON_COMMON_H_

#include <climits>

namespace frontend::common {
// 语法分析机配置文件名
constexpr const char* kSyntaxConfigFileName = "syntax_config.conf";
// 词法分析机配置文件名
constexpr const char* kDfaConfigFileName = "dfa_config.conf";
// char可能取值的数目
constexpr size_t kCharNum = CHAR_MAX - CHAR_MIN + 1;
// 产生式配置中功能区的个税
// 关键字定义区
// 基础终结产生式定义区
// 运算符定义区
// 普通产生式区
constexpr size_t kFunctionPartSize = 4;

// 运算符结合类型：左结合，右结合
enum class OperatorAssociatityType { kLeftToRight, kRightToLeft };

}  // namespace frontend::common

#endif  // !COMMON_COMMON_H_