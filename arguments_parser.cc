#include <cstddef>
#include <fstream>
#include <functional>

#include "arguments_parser.h"

using OptionalArgumentsMap = std::map<std::string, std::function<void()>>;
static void RemoveModeFlags(OptionalArgumentsMap& oam) {
  for (auto&& flag : {"-i", "-r", "-a"}) {
    oam.erase(flag);
  }
}
codes::Result parse::Arguments::Parse(int argc, char** argv) {
  if (argc < 2) {
    return codes::invalidArguments;
  }
  auto result = codes::success;
  std::size_t last_argument_index = argc - 1;
  std::size_t current_argument_index = 1;
  OptionalArgumentsMap optional_arguments;
  optional_arguments["-r"] = [&] {
    mode = SimulationMode::runToCompletion;
    RemoveModeFlags(optional_arguments);
  };
  optional_arguments["-i"] = [&] {
    mode = SimulationMode::interactive;
    RemoveModeFlags(optional_arguments);
  };
  optional_arguments["-a"] = [&] {
    mode = SimulationMode::automatic;
    RemoveModeFlags(optional_arguments);
  };
  optional_arguments["-o"] = [&] {
    need_output = true;
    std::size_t next_argument_index = current_argument_index + 1;
    if (next_argument_index <= last_argument_index) {
      auto next_argument = argv[next_argument_index];
      if (optional_arguments.count(next_argument) == 0) {
        reportFile = std::ofstream(next_argument);
        if (!reportFile) {
          result = codes::outputFileError;
        }
        current_argument_index = next_argument_index;
      }
    }
    optional_arguments.erase("-o");
  };
  while (result == codes::success && current_argument_index < argc) {
    auto handler = optional_arguments.find(argv[current_argument_index]);
    if (handler == optional_arguments.end()) {
      result = codes::invalidArguments;
    } else {
      handler->second();
    }
  }
  return result;
}
