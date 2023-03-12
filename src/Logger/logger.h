#ifndef LOGGER_LOGGER_H_
#define LOGGER_LOGGER_H_

#include <iostream>

template <class Arg>
void Log(const char prefix[], Arg&& arg) {
  std::cout << prefix << arg << '\n';
}

#ifdef ENABLE_LOG

#define LOG_INFO(module_str, describe) Log(module_str##" Info: ", describe);
#define LOG_WARNING(module_str, describe) Log(module_str##" Warning: ", describe);
#define LOG_ERROR(module_str, describe) Log(module_str##" Error: ", describe);

#else

#define LOG_INFO(...)
#define LOG_WARNING(...)
#define LOG_ERROR(...)

#endif  // ENABLE_LOG

#endif  // !LOGGER_LOGGER_H_