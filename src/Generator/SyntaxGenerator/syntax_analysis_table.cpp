#include "syntax_analysis_table.h"

namespace frontend::generator::syntax_generator {

SyntaxAnalysisTableEntry& SyntaxAnalysisTableEntry::operator=(
    SyntaxAnalysisTableEntry&& syntax_analysis_table_entry) {
  action_and_attached_data_ =
      std::move(syntax_analysis_table_entry.action_and_attached_data_);
  nonterminal_node_transform_table_ =
      std::move(syntax_analysis_table_entry.nonterminal_node_transform_table_);
  return *this;
}

void SyntaxAnalysisTableEntry::ResetEntryId(
    const std::unordered_map<SyntaxAnalysisTableEntryId,
                             SyntaxAnalysisTableEntryId>&
        old_entry_id_to_new_entry_id) {
  //处理终结节点的动作
  for (auto& action_and_attached_data : GetAllActionAndAttachedData()) {
    switch (action_and_attached_data.second->GetActionType()) {
      case ActionType::kShift:
      case ActionType::kShiftReduct: {
        // 获取原条目ID的引用
        ShiftAttachedData& shift_attached_data =
            action_and_attached_data.second->GetShiftAttachedData();
        auto iter = old_entry_id_to_new_entry_id.find(
            shift_attached_data.GetNextSyntaxAnalysisTableEntryId());
        if (iter != old_entry_id_to_new_entry_id.end()) [[unlikely]] {
          // 更新为新的条目ID
          shift_attached_data.SetNextSyntaxAnalysisTableEntryId(iter->second);
        }
      } break;
      case ActionType::kReduct:
      case ActionType::kAccept:
        // 无需做任何更改
        break;
      default:
        assert(false);
        break;
    }
  }
  //处理非终结节点的转移
  for (auto& target : GetAllNonTerminalNodeTransformTarget()) {
    SyntaxAnalysisTableEntryId old_entry_id = target.second;
    SyntaxAnalysisTableEntryId new_entry_id =
        old_entry_id_to_new_entry_id.find(old_entry_id)->second;
    if (old_entry_id != new_entry_id) {
      SetNonTerminalNodeTransformId(target.first, new_entry_id);
    }
  }
}

bool SyntaxAnalysisTableEntry::ShiftAttachedData::operator==(
    const ActionAndAttachedDataInterface& shift_attached_data) const {
  return ActionAndAttachedDataInterface::operator==(shift_attached_data) &&
         next_entry_id_ ==
             static_cast<const ShiftAttachedData&>(shift_attached_data)
                 .next_entry_id_;
}

bool SyntaxAnalysisTableEntry::ReductAttachedData::operator==(
    const ActionAndAttachedDataInterface& reduct_attached_data) const {
  if (ActionAndAttachedDataInterface::operator==(reduct_attached_data))
      [[likely]] {
    const ReductAttachedData& real_type_reduct_attached_data =
        static_cast<const ReductAttachedData&>(reduct_attached_data);
    return reducted_nonterminal_node_id_ ==
               real_type_reduct_attached_data.reducted_nonterminal_node_id_ &&
           process_function_class_id_ ==
               real_type_reduct_attached_data.process_function_class_id_ &&
           production_body_ == real_type_reduct_attached_data.production_body_;
  } else {
    return false;
  }
}

bool SyntaxAnalysisTableEntry::ShiftReductAttachedData::operator==(
    const ActionAndAttachedDataInterface& attached_data) const {
  if (ActionAndAttachedDataInterface::operator==(attached_data)) [[likely]] {
    return ActionAndAttachedDataInterface::operator==(attached_data) &&
           shift_attached_data_ ==
               static_cast<const ShiftReductAttachedData&>(attached_data)
                   .shift_attached_data_ &&
           reduct_attached_data_ ==
               static_cast<const ShiftReductAttachedData&>(attached_data)
                   .reduct_attached_data_;
  } else {
    return false;
  }
}

const SyntaxAnalysisTableEntry::ShiftAttachedData&
SyntaxAnalysisTableEntry::ActionAndAttachedDataInterface::GetShiftAttachedData()
    const {
  assert(false);
  // 防止警告
  return reinterpret_cast<const ShiftAttachedData&>(*this);
}

const SyntaxAnalysisTableEntry::ReductAttachedData& SyntaxAnalysisTableEntry::
    ActionAndAttachedDataInterface::GetReductAttachedData() const {
  assert(false);
  // 防止警告
  return reinterpret_cast<const ReductAttachedData&>(*this);
}

const SyntaxAnalysisTableEntry::ShiftReductAttachedData&
SyntaxAnalysisTableEntry::ActionAndAttachedDataInterface::
    GetShiftReductAttachedData() const {
  assert(false);
  // 防止警告
  return reinterpret_cast<const ShiftReductAttachedData&>(*this);
}
bool SyntaxAnalysisTableEntry::ShiftReductAttachedData::IsSameOrPart(
    const ActionAndAttachedDataInterface& attached_data) const {
  switch (attached_data.GetActionType()) {
    case ActionType::kShift:
      return shift_attached_data_ ==
             static_cast<const ShiftAttachedData&>(attached_data);
      break;
    case ActionType::kReduct:
      return reduct_attached_data_ ==
             static_cast<const ReductAttachedData&>(attached_data);
      break;
    case ActionType::kShiftReduct:
      return operator==(attached_data);
      break;
    default:
      assert(false);
      // 防止警告
      return false;
      break;
  }
}
}  // namespace frontend::generator::syntax_generator