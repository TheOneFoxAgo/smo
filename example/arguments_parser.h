#ifndef ARGUMENTS_PARSER_H_
#define ARGUMENTS_PARSER_H_

#include <fstream>
#include <functional>
#include <istream>
#include <map>
#include <optional>

#include "return_codes.h"
namespace parse {
enum class SimulationMode {
  runToCompletion,
  interactive,
  automatic,
};
struct Arguments {
  codes::Result Parse(int argc, char** argv);
  SimulationMode mode = SimulationMode::runToCompletion;
  bool need_output = false;
  std::optional<std::ofstream> reportFile;
};
}  // namespace parse

#endif
