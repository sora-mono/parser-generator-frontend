#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
#include <any>
#include <boost/serialization/string.hpp>
#include <boost/serialization/variant.hpp>

namespace frontend::generator::syntax_generator {
// 接口类，所有用户定义函数均从该类派生
class ProcessFunctionInterface {
 public:
  using UserData = std::any;
  // 终结单词的内容
  struct TerminalWordData {
    // 单词
    std::string word;
  };
  // 非终结节点的内容
  struct NonTerminalWordData {
    NonTerminalWordData() = default;
    NonTerminalWordData(UserData&& user_data)
        : user_returned_data(std::move(user_data)) {}
    UserData user_returned_data;
  };
  // 存储产生式中单个节点所携带的信息
  class WordDataToUser {
   public:
    const TerminalWordData& GetTerminalWordData() const {
      return boost::get<TerminalWordData>(word_data_to_user);
    }
    TerminalWordData& GetTerminalWordData() {
      return const_cast<TerminalWordData&>(
          const_cast<const WordDataToUser&>(*this).GetTerminalWordData());
    }
    const NonTerminalWordData& GetNonTerminalWordData() const {
      return boost::get<NonTerminalWordData>(word_data_to_user);
    }
    NonTerminalWordData& GetNonTerminalWordData() {
      return const_cast<NonTerminalWordData&>(
          const_cast<const WordDataToUser&>(*this).GetNonTerminalWordData());
    }

    template <class BasicObjectType>
    void SetWordDataToUser(BasicObjectType&& word_data) {
      word_data_to_user = std::forward<BasicObjectType>(word_data);
    }

   private:
    // 该单词的数据
    // 空规约节点存储NonTerminalWordData
    // 其中NonTerminalWordData::user_data_为空
    boost::variant<TerminalWordData, NonTerminalWordData> word_data_to_user;
  };

  virtual ~ProcessFunctionInterface() {}

  // 参数为产生式定义顺序
  // 返回的值作为移入该产生式规约得到的非终结符号下一次参与规约传递的参数
  // 空规约节点的node_type是ProductionNodeType::kEndNode
  virtual UserData Reduct(std::vector<WordDataToUser>&& word_data) const = 0;

 private:
  // 允许序列化类访问
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive&& ar, const unsigned int version) {}
};

// 内部实现用根节点的规约函数
class RootReductClass : public ProcessFunctionInterface {
  virtual UserData Reduct(
      std::vector<WordDataToUser>&& word_data) const override {
    return UserData();
  }

 private:
  // 允许序列化类访问
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive&& ar, const unsigned int version) {
    ar& boost::serialization::base_object<ProcessFunctionInterface>(*this);
  }
};
}  // namespace frontend::generator::syntax_generator

#endif  // !GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
