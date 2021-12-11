/// @file line_and_column.h
/// @brief 存储当前解析到的源文件行数和列数
/// @note 表示行数与列数的变量都是thread_loacl修饰的全局变量
#ifndef PARSER_LINE_AND_COLUMN_H_
#define PARSER_LINE_AND_COLUMN_H_

namespace frontend::parser {

/// @brief 获取当前行数
/// @return 返回当前行数
/// @note 从0开始计算
extern inline size_t GetLine();
/// @brief 获取当前列数
/// @return 返回当前行数
/// @note 从0开始计算
extern inline size_t GetColumn();
/// @brief 设置当前行数
/// @param[in] line ：待设置的行数
/// @note 从0开始计算
extern inline void SetLine(size_t line);
/// @brief 设置当前列数
/// @param[in] column ：待设置的列数
/// @note 从0开始计算
extern inline void SetColumn(size_t column);

}  // namespace frontend::parser

#endif  /// !PARSER_LINE_AND_COLUMN_H_