// 该头文件存储当前解析到的源文件行数和列数
// 两个变量都是thread_loacl修饰的全局变量
#ifndef PARSER_LINE_AND_COLUMN_H_
#define PARSER_LINE_AND_COLUMN_H_

namespace frontend::parser {
// 线程全局变量，存储当前解析到的行数
// 从0开始计数
extern thread_local size_t line_;
// 线程全局变量，存储当前解析到的列数
// 从0开始计数
extern thread_local size_t column_;

extern inline size_t GetLine();
extern inline size_t GetColumn();
extern inline void SetLine(size_t line);
extern inline void SetColumn(size_t column);

}  // namespace frontend::parser

#endif  // !PARSER_LINE_AND_COLUMN_H_