#ifndef CPARSERFRONTEND_C_PARSER_FRONTEND_H_
#define CPARSERFRONTEND_C_PARSER_FRONTEND_H_

#include <unordered_map>

#include "Generator/SyntaxGenerator/process_function_interface.h"
#include "action_scope_system.h"
#include "flow_control.h"
#include "operator_node.h"
#include "type_system.h"

namespace c_parser_frontend {
// 引用一些类型的定义
using c_parser_frontend::action_scope_system::VarietyScopeSystem;
using AddVarietyResult = c_parser_frontend::action_scope_system::
    VarietyScopeSystem::AddVarietyResult;
using c_parser_frontend::flow_control::FunctionDefine;
using c_parser_frontend::operator_node::VarietyOperatorNode;
using c_parser_frontend::type_system::StructOrBasicType;
using c_parser_frontend::type_system::TypeInterface;
using c_parser_frontend::type_system::TypeSystem;

class CParserFrontend {
  using AddTypeResult = TypeSystem::AddTypeResult;
  using GetTypeResult = TypeSystem::GetTypeResult;

 public:
  enum class AddFunctionDefineResult {
    // 可以添加的情况
    kSuccess,  // 成功添加
    // 不可以添加的情况
    kReDefine,  // 重定义函数
    kOverLoad   // 重载函数
  };
  // 添加类型
  // 返回添加结果
  AddTypeResult AddType(std::string&& type_name,
                        std::shared_ptr<const TypeInterface>&& type_pointer) {
    return type_system_.AddType(std::move(type_name), std::move(type_pointer));
  }
  std::pair<std::shared_ptr<const TypeInterface>, GetTypeResult> GetType(
      const std::string& type_name, StructOrBasicType type_prefer) {
    return type_system_.GetType(type_name, type_prefer);
  }
  // 增加一级作用域等级
  void AddActionScopeLevel() { variety_system.AddActionScopeLevel(); }
  // 弹出一级作用域，会弹出该作用域内所有的变量
  void PopActionScope() { variety_system.PopActionScope(); }
  size_t GetActionScopeLevel() const {
    return variety_system.GetActionScopeLevel();
  }
  // 添加变量
  // 返回添加结果
  AddVarietyResult AddVariety(
      std::string&& variety_name,
      std::shared_ptr<VarietyOperatorNode>&& operator_node) {
    return variety_system.AddVariety(std::move(variety_name),
                                     std::move(operator_node));
  }
  std::pair<std::shared_ptr<VarietyOperatorNode>, bool> GetVariety(
      const std::string& variety_name) {
    return variety_system.GetVariety(variety_name);
  }
  // 添加函数声明，允许多次声明相同函数
  // 返回是否添加成功
  // C语言不支持函数重载，重名函数会添加失败
  // 如果添加失败则返回函数数据的控制权
  template <class FunctionName>
  std::pair<std::unique_ptr<FunctionDefine>, bool> AddFunctionAnnounce(
      FunctionName&& function_name,
      std::unique_ptr<FunctionDefine>&& function_announce_data);
  // 添加函数定义，只能添加一次定义
  // 返回添加结果
  // 如果添加失败则返回函数数据的控制权
  template <class FunctionName>
  std::pair<std::unique_ptr<FunctionDefine>, AddFunctionDefineResult>
  AddFunctionDefine(FunctionName&& function_name,
                    std::unique_ptr<FunctionDefine>&& function_define_data);
  // 获取函数信息
  // 如果不存在给定名称的函数则返回空指针
  FunctionDefine* GetFunction(const std::string& function_name);

 private:
  // 类型系统
  TypeSystem type_system_;
  // 变量作用域系统
  VarietyScopeSystem variety_system;
  // 函数定义，键值为函数名
  std::unordered_map<std::string, std::unique_ptr<FunctionDefine>> functions_;
};

// 添加函数声明，允许多次声明相同函数
// 返回是否添加成功
// C语言不支持函数重载，重名函数会添加失败
// 如果添加失败则返回函数数据的控制权

template <class FunctionName>
inline std::pair<std::unique_ptr<FunctionDefine>, bool>
CParserFrontend::AddFunctionAnnounce(
    FunctionName&& function_name,
    std::unique_ptr<FunctionDefine>&& function_announce_data) {
  auto [iter, inserted] =
      functions_.emplace(std::forward<FunctionName>(function_name),
                         std::move(function_announce_data));
  if (!inserted) [[unlikely]] {
    // 已存在同名函数，检验是否为同一函数
    if (function_announce_data->GetFunctionTypeReference().GetArgumentTypes() !=
        iter->second->GetFunctionTypeReference().GetArgumentTypes())
        [[unlikely]] {
      // 同名非相同函数
      // C语言不支持函数重载，不能添加
      return std::make_pair(std::move(function_announce_data), false);
    }
  }
  return std::make_pair(std::unique_ptr<FunctionDefine>(), true);
}

// 添加函数定义，只能添加一次定义
// 返回添加结果
// 如果添加失败则返回函数数据的控制权

template <class FunctionName>
inline std::pair<std::unique_ptr<FunctionDefine>,
                 CParserFrontend::AddFunctionDefineResult>
CParserFrontend::AddFunctionDefine(
    FunctionName&& function_name,
    std::unique_ptr<FunctionDefine>&& function_define_data) {
  auto [iter, inserted] =
      functions_.emplace(std::forward<FunctionName>(function_name),
                         std::move(function_define_data));
  if (!inserted) {
    // 已经存在函数定义或者重名函数
    // 判断待添加的函数和已有的函数是不是同一个函数
    // 已知函数名相同，只需判断参数类型
    if (function_define_data->GetFunctionTypeReference().GetArgumentTypes() !=
        iter->second->GetFunctionTypeReference().GetArgumentTypes())
        [[unlikely]] {
      // 同名非相同函数
      // C语言不支持函数重载，不能添加
      return std::make_pair(std::move(function_define_data),
                            AddFunctionDefineResult::kOverLoad);
    }
    // 判断已有部分是声明还是定义
    if (iter->second->GetSentences().size() != 0) [[unlikely]] {
      // 重定义
      return std::make_pair(std::move(function_define_data),
                            AddFunctionDefineResult::kReDefine);
    }
  }
  return std::make_pair(std::unique_ptr<FunctionDefine>(),
                        AddFunctionDefineResult::kSuccess);
}

}  // namespace c_parser_frontend

#endif  // !CPARSERFRONTEND_C_PARSER_FRONTEND_H_
