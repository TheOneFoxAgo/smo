#include <cstddef>
#include <fstream>
#include <functional>
#include <string>

#include "arguments_parser.h"
#include "return_codes.h"

using OptionalArgumentsMap = std::map<std::string, std::function<void()>>;
static void RemoveModeFlags(OptionalArgumentsMap& oam) {
  for (auto&& flag : {"-i", "-r", "-a"}) {
    oam.erase(flag);
  }
}
codes::Result parse::Arguments::Parse(int argc, char** argv) {
  auto result = codes::success;
  std::size_t last_argument_index = argc - 1;
  int current_argument_index = 1;
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
        report_file = std::ofstream(next_argument);
        if (!report_file) {
          result = codes::outputFileError;
        }
        current_argument_index = next_argument_index;
      }
    }
    optional_arguments.erase("-o");
  };
  optional_arguments["-m"] = [&] {
    std::size_t next_argument_index = current_argument_index + 1;
    if (next_argument_index <= last_argument_index) {
      auto next_argument = argv[next_argument_index];
      try {
        max_requests = std::stol(next_argument);
        current_argument_index = next_argument_index;
      } catch (...) {
        result = codes::invalidArguments;
      }
    } else {
      result = codes::invalidArguments;
    }
    optional_arguments.erase("-m");
  };
  bool has_parsed_input_file = false;
  while (result == codes::success && current_argument_index < argc) {
    auto current_argument = argv[current_argument_index];
    auto handler = optional_arguments.find(current_argument);
    if (handler == optional_arguments.end()) {
      if (has_parsed_input_file) {
        result = codes::invalidArguments;
      } else {
        input_file = std::ifstream(current_argument);
        has_parsed_input_file = true;
      }
    } else {
      handler->second();
    }
    current_argument_index += 1;
  }
  if (!has_parsed_input_file) {
    result = codes::invalidArguments;
  }
  return result;
}
