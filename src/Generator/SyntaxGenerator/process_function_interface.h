/// @file process_function_interface.h
/// @brief 该文件定义包装用户定义规约函数的类的基类
#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
#include <any>
#include <boost/serialization/string.hpp>
#include <boost/serialization/variant.hpp>
#include <variant>

namespace frontend::generator::syntax_generator {
/// @class ProcessFunctionInterface process_function_interface.h
/// @brief 所有包装用户定义函数的类均从该类派生
/// @details
/// 所有派生类必须且仅允许重载Reduct函数，该函数返回规约后用户返回的数据
/// 该数据在下一次规约时使用
class ProcessFunctionInterface {
 public:
  /// @brief 定义用户返回的数据类型
  using UserData = std::any;
  /// @brief 终结产生式单词的数据
  struct TerminalWordData {
    /// 单词
    std::string word;
  };
  /// @brief 非终结产生式的数据
  struct NonTerminalWordData {
    NonTerminalWordData() = default;
    NonTerminalWordData(UserData&& user_data)
        : user_returned_data(std::move(user_data)) {}
    UserData user_returned_data;
  };
  /// @class ProcessFunctionInterface::WordDataToUser
  /// process_function_interface.h
  /// @brief 提供给用户的规约用数据中单个产生式的数据
  /// @note 该类不存储独立的类型信息，用户在使用时通过产生式可以知道不同位置的
  /// 数据是终结节点获取到的单词还是非终结节点规约得到的数据
  class WordDataToUser {
   public:
    /// @brief 获取终结产生式数据const引用
    /// @return 将存储的数据转换为终结产生式数据类型的const引用
    /// @note 不检查是否可以正确转换
    const TerminalWordData& GetTerminalWordData() const {
      return std::get<TerminalWordData>(word_data_to_user_);
    }
    /// @brief 获取终结产生式数据引用
    /// @return 将存储的数据转换为终结产生式数据类型的引用
    /// @note 不检查是否可以正确转换
    TerminalWordData& GetTerminalWordData() {
      return const_cast<TerminalWordData&>(
          const_cast<const WordDataToUser&>(*this).GetTerminalWordData());
    }
    /// @brief 获取非终结产生式数据const引用
    /// @return 将存储的数据转换为非终结产生式数据类型的const引用
    /// @note 不检查是否可以正确转换
    const NonTerminalWordData& GetNonTerminalWordData() const {
      return std::get<NonTerminalWordData>(word_data_to_user_);
    }
    /// @brief 获取非终结产生式数据引用
    /// @return 将存储的数据转换为非终结产生式数据类型的引用
    /// @note 不检查是否可以正确转换
    NonTerminalWordData& GetNonTerminalWordData() {
      return const_cast<NonTerminalWordData&>(
          const_cast<const WordDataToUser&>(*this).GetNonTerminalWordData());
    }
    /// @brief 获取是否有产生式数据
    /// @return 返回是否携带产生式数据
    /// @retval true ：携带产生式数据
    /// @retval false ：不携带产生式数据（对应产生式空规约）
    bool Empty() const {
      return std::get_if<std::monostate>(&word_data_to_user_) != nullptr;
    }

    /// @brief 设置存储的数据
    /// @param[in] word_data ：新数据
    /// @note 仅支持std::monostate、TerminalWordData和NonTerminalWordData的
    /// const左值引用和右值引用
    template <class BasicObjectType>
    void SetWordDataToUser(BasicObjectType&& word_data) {
      word_data_to_user_ = std::forward<BasicObjectType>(word_data);
    }

   private:
    /// @brief 该单词的数据
    /// @note 空规约节点存储std::monostate
    std::variant<std::monostate, TerminalWordData, NonTerminalWordData>
        word_data_to_user_;
  };

  virtual ~ProcessFunctionInterface() {}

  /// @brief 调用用户定义的规约函数
  /// @param[in] word_data ：规约的产生式体中每个产生式的数据
  /// @return 返回用户规约后返回的数据
  /// @details
  /// word_data中数据顺序为产生式定义顺序
  /// @note 空规约节点存储std::monostate
  virtual UserData Reduct(std::vector<WordDataToUser>&& word_data) const = 0;

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
  virtual UserData Reduct(
      std::vector<WordDataToUser>&& word_data) const override {
    assert(false);
    // 防止警告
    return UserData();
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
