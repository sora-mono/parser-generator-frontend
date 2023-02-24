/// @file process_function_interface.h
/// @brief 该文件定义包装用户定义规约函数的类的基类
#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
#include <any>
#include <boost/serialization/base_object.hpp>
#include <cassert>
#include <memory>
#include <vector>

namespace frontend::generator::syntax_generator {
/// @class ProcessFunctionInterface process_function_interface.h
/// @brief 所有包装用户定义函数的类均从该类派生
/// @details
/// 所有派生类必须且仅允许重载Reduct函数，该函数返回规约后用户返回的数据
/// 该数据在下一次规约时使用
class ProcessFunctionInterface {
 public:
  virtual ~ProcessFunctionInterface() {}

  /// @brief 调用用户定义的规约函数
  /// @param[in] word_data ：规约的产生式体中每个产生式的数据
  /// @return 返回用户规约后返回的数据
  /// @details
  /// word_data中数据顺序为产生式定义顺序
  /// @note 空规约节点存储std::monostate
  virtual std::any Reduct(std::vector<std::any>&& word_data) const = 0;

 private:
  /// @brief 允许序列化类访问
  friend class boost::serialization::access;

  /// @brief 序列化该类的函数
  /// @param[in,out] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  /// 所有派生类均需重写该函数
  template <class Archive>
  void serialize(Archive&& ar, const unsigned int version) {}
};

/// @class RootReductClass process_function_interface.h
/// @brief 内部实现用根节点的规约函数
/// @details
/// 语法分析机配置生成时会自动创建一个内部根节点，该根节点为非终结产生式，
/// 其唯一的产生式体为用户定义的根产生式；
/// 该产生式仅支持ActionType::kAccept操作
/// RootReductClass用来包装该类的规约函数
class RootReductClass : public ProcessFunctionInterface {
  /// @brief 调用内部根节点的规约函数
  /// @param[in] word_data ：规约的产生式体中每个产生式的数据
  /// @return 返回UserData()
  /// @details
  /// word_data中数据顺序为产生式定义顺序
  /// @note 空规约节点存储std::monostate
  /// @attention 内部根节点仅允许ActionType::kAccept，不允许规约操作
  /// 调用该函数会导致触发assert(false)
  virtual std::any Reduct(std::vector<std::any>&& word_data) const override {
    assert(false);
    // 防止警告
    return std::any();
  }

 private:
  /// @brief 允许序列化类访问
  friend class boost::serialization::access;

  /// @brief 序列化该类的函数
  /// @param[in,out] ar ：序列化使用的档案
  /// @param[in] version ：序列化文件版本
  /// @attention 该函数应由boost库调用而非手动调用
  /// 所有派生类均需重写该函数
  template <class Archive>
  void serialize(Archive&& ar, const unsigned int version) {
    ar& boost::serialization::base_object<ProcessFunctionInterface>(*this);
  }
};
}  // namespace frontend::generator::syntax_generator

#endif  /// !GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
