#include "reduct_functions.h"

#include <iostream>
namespace brainfuck {
std::nullptr_t SingleOperateRightMovePointer(std::string&& str_operator) {
  std::cout << "++ptr;\n";
  return nullptr;
}
std::nullptr_t SingleOperateLeftMovePointer(std::string&& str_operator) {
  std::cout << "--ptr;\n";
  return nullptr;
}
std::nullptr_t SingleOperateValuePlusOne(std::string&& str_operator) {
  std::cout << "++*ptr;\n";
  return nullptr;
}
std::nullptr_t SingleOperateValueMinusOne(std::string&& str_operator) {
  std::cout << "--*ptr;\n";
  return nullptr;
}
std::nullptr_t SingleOperateOutputValue(std::string&& str_operator) {
  std::cout << "std::cout << *ptr;\n";
  return nullptr;
}
std::nullptr_t SingleOperateInputValue(std::string&& str_operator) {
  std::cout << "std::cin >> *ptr;\n";
  return nullptr;
}
std::nullptr_t WhileBegin(std::string&& str_operator) {
  std::cout << "while(*ptr) {\n";
  return nullptr;
}
std::nullptr_t WhileSentence(std::nullptr_t, std::nullptr_t,
                             std::string&& str_while_end) {
  std::cout << "}\n";
  return nullptr;
}
std::nullptr_t SingleOperateWhileSentence(std::nullptr_t) { return nullptr; }
std::nullptr_t Operates(std::nullptr_t) { return nullptr; }
std::nullptr_t OperatesExtend(std::nullptr_t, std::nullptr_t) {
  return nullptr;
}
std::nullptr_t Root(std::nullptr_t) { return nullptr; }
}  // namespace brainfuck