#ifndef ARGUMENTS_PARSER_H_
#define ARGUMENTS_PARSER_H_

#include <cstddef>
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
  std::size_t max_requests = 1'000'000;
  SimulationMode mode = SimulationMode::runToCompletion;
  bool need_output = false;
  std::optional<std::ofstream> report_file;
  std::ifstream input_file;
};
}  // namespace parse

#endif
