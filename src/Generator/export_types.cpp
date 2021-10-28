#include "export_types.h"

namespace frontend::generator::dfa_generator::nfa_generator {

// 根据上一个操作是否为规约判断使用左侧单目运算符优先级还是双目运算符优先级
// 返回获取到的结合类型和优先级

std::pair<frontend::generator::syntax_generator::OperatorAssociatityType,
          size_t>
WordAttachedData::GetAssociatityTypeAndPriority(
    bool is_last_operate_reduct) const {
  assert(
      node_type ==
      frontend::generator::syntax_generator::ProductionNodeType::kOperatorNode);
  if (binary_operator_priority != -1) {
    if (unary_operator_priority != -1) {
      // 两种语义均存在
      if (is_last_operate_reduct) {
        // 上次操作为规约，应使用左侧单目运算符语义
        return std::make_pair(unary_operator_associate_type,
                              unary_operator_priority);
      } else {
        // 上次操作为移入，应使用双目运算符语义
        return std::make_pair(binary_operator_associate_type,
                              binary_operator_priority);
      }
    } else {
      // 仅存在双目运算符语义，直接返回
      return std::make_pair(binary_operator_associate_type,
                            binary_operator_priority);
    }
  } else {
    // 仅存在单目运算符语义，直接返回
    return std::make_pair(unary_operator_associate_type,
                          unary_operator_priority);
  }
}
bool nfa_generator::WordAttachedData::operator==(
    const WordAttachedData& saved_data) const {
  return (production_node_id == saved_data.production_node_id &&
          node_type == saved_data.node_type &&
          binary_operator_associate_type ==
              saved_data.binary_operator_associate_type &&
          binary_operator_priority == saved_data.binary_operator_priority &&
          unary_operator_associate_type ==
              saved_data.unary_operator_associate_type &&
          unary_operator_priority == saved_data.unary_operator_priority);
}
}  // namespace frontend::generator::dfa_generator::nfa_generator