#ifndef GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT
#define GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT

#include "process_functions_classes.h"
#include "syntax_generator.h"

namespace frontend::generator::syntax_generator {
void SyntaxGenerator::ConfigConstruct() {
  // 下面的宏将包含的文件中用户定义的产生式转化为产生式构建配置
  // 如AddTerminalNode、AddNonTerminalNode等
#define GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_
#include "Config/ProductionConfig/production_config-inc.h"
#undef GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_
  // 定义结束标志
#define GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT_END
}
}  // namespace frontend::generator::syntax_generator

#endif  // !GENERATOR_SYNTAXGENERATOR_CONFIG_CONSTRUCT