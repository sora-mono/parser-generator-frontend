#ifndef GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
#define GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
#include <any>
#include <boost/serialization/string.hpp>
#include <boost/serialization/variant.hpp>

#include "Common/common.h"
namespace frontend::generator::syntaxgenerator {
// 接口类，所有用户定义函数均从该类派生
class ProcessFunctionInterface {
 public:
  using UserData = std::any;
  // 终结单词的内容
  struct TerminalWordData {
    // 单词
    std::string word;
    // 单词所在行数
    size_t line;
  };
  // 非终结节点的内容
  struct NonTerminalWordData {
    NonTerminalWordData() = default;
    NonTerminalWordData(UserData&& user_data)
        : user_data_(std::move(user_data)) {}
    NonTerminalWordData(NonTerminalWordData&&) = default;
    NonTerminalWordData(const NonTerminalWordData&) = default;
    NonTerminalWordData& operator=(const NonTerminalWordData&) = default;
    UserData user_data_;
  };
  // 存储产生式中单个节点所携带的信息
  struct WordDataToUser {
    // 仅有非终结节点和终结节点两种可能
    frontend::common::ProductionNodeType node_type;
    // 该单词的数据
    boost::variant<TerminalWordData, NonTerminalWordData> word_data_to_user_;
    TerminalWordData& GetTerminalWordData() {
      return boost::get<TerminalWordData>(word_data_to_user_);
    }
    NonTerminalWordData& GetNonTerminalWordData() {
      return boost::get<NonTerminalWordData>(word_data_to_user_);
    }
    template <class T>
    void SetWordData(T&& word_data) {
      word_data_to_user_ = word_data;
    }
  };

  virtual ~ProcessFunctionInterface();

  // 参数标号越低入栈时间越晚
  // 返回的值作为移入该产生式规约得到的非终结符号下一次参与规约传递的参数
  // 空规约节点的node_type是ProductionNodeType::kEndNode
  virtual UserData Reduct(std::vector<WordDataToUser>&& user_data_) = 0;
};
}  // namespace frontend::generator::syntaxgenerator
#endif  // !GENERATOR_SYNTAXGENERATOR_PROCESS_FUNCTION_INTERFACE_H_
