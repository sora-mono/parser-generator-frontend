#ifndef COMMON_COMMON_H_
#define COMMON_COMMON_H_

namespace frontend::common {

// char可能取值的数目
constexpr size_t kCharNum = CHAR_MAX - CHAR_MIN + 1;
// 产生式配置中功能区的个税
// 关键字定义区
// 基础终结产生式定义区
// 运算符定义区
// 普通产生式区
constexpr size_t kFunctionPartSize = 4;
// 运算符结合类型：左结合，右结合
enum class OperatorAssociatityType { kLeftToRight, kRightToRight };
// 节点类型：终结符号，运算符，非终结符号，文件尾节点
// 为了支持ClassfiyProductionNodes，允许自定义项的值
// 如果自定义项的值则所有项的值都必须小于sizeof(ProductionNodeType)
enum class ProductionNodeType {
  kTerminalNode,
  kOperatorNode,
  kNonTerminalNode,
  kEndNode
};

}  // namespace frontend::common

#ifdef _DEBUG

#define USE_USER_DEFINED_FILE
#define USE_AMBIGUOUS_GRAMMAR

#endif  // _DEBUG

#endif  // !COMMON_COMMON_H_