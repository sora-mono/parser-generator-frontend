#include "line_and_column.h"

namespace frontend::parser {
/// @brief 线程全局变量，存储当前解析到的行数
/// @note 从0开始计数
static thread_local size_t line_ = 0;
/// @brief 线程全局变量，存储当前解析到的列数
/// @note 从0开始计数
static thread_local size_t column_ = 0;

size_t GetLine() { return line_; }
size_t GetColumn() { return column_; }
void SetLine(size_t line) { line_ = line; }
void SetColumn(size_t column) { column_ = column; }
}  // namespace frontend::parser