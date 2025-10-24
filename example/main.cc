#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "arguments_parser.h"
#include "print.h"
#include "return_codes.h"
#include "simulator.h"
#include "simulator_config.h"

double CalculateNextTargetAmountOfRequests(double rejection_probability);
int main(int argc, char** argv) {
  parse::Arguments args;
  auto parse_result = args.Parse(argc, argv);
  if (parse_result) {
    smo::PrintUsage(std::cerr);
    return parse_result;
  }
  smo::SimulatorConfig config;
  args.input_file >> config;
  if (!std::cin || config.buffer_capacity < 0 ||
      config.device_coefficients.size() == 0 ||
      config.source_periods.size() == 0 ||
      config.target_amount_of_requests <= 0) {
    return codes::configError;
  }
  smo::Simulator simulator(std::move(config), args.law);
  switch (args.mode) {
    case parse::SimulationMode::runToCompletion:
      simulator.RunToCompletion();
      break;
    case parse::SimulationMode::interactive: {
      std::cout << "Interactive mode. Input h to get help\n\n";
      smo::PrintSimulationState(std::cout, simulator);
      std::string command;
      std::map<std::string, std::function<void()>> comand_handlers{
          {"",
           [&] {
             auto event = simulator.Step();
             smo::PrintEvent(std::cout, event);
             smo::PrintSimulationState(std::cout, simulator);
           }},
          {"q", [&] { simulator.RunToCompletion(); }},
          {"p", [&] { smo::PrintRealBuffer(std::cout, simulator); }},
          {"h", [&] { smo::PrintHelp(std::cout); }},
      };
      while (!(command == "q" || simulator.is_completed())) {
        std::getline(std::cin, command);
        auto handler = comand_handlers.find(command);
        if (handler == comand_handlers.end()) {
          std::cout << "Incorrect command. Try \"h\" for a full list of"
                       "available commands\n";
        } else {
          handler->second();
        }
      }
      break;
    }
    case parse::SimulationMode::automatic: {
      std::size_t next_requests = 0;
      double prev_rejection = 0.0;
      double current_rejection = -1.0;
      while (true) {
        prev_rejection = current_rejection;

        simulator.RunToCompletion();
        std::size_t current_requests = simulator.target_amount_of_requests();
        current_rejection =
            static_cast<double>(simulator.rejected_amount()) / current_requests;
        if (current_rejection < 1.0 / args.max_requests) {
          std::cout << "Can't estimate requests amount, because rejection "
                       "probability is too small: "
                    << current_rejection << '\n';
          break;
        }
        next_requests = CalculateNextTargetAmountOfRequests(current_rejection);
        std::cout << "Rejection probability: " << current_rejection << '\n';
        if (next_requests > args.max_requests) {
          std::cerr << "Incorrect guess!" << '\n';
          return codes::incorrectGuess;
        }
        if (std::abs((current_rejection - prev_rejection) / prev_rejection) <
            0.1) {
          std::cout << "Calculated amount of requests: "
                    << static_cast<std::size_t>(next_requests) << '\n';
          break;
        }
        simulator.ResetWithNewAmountOfRequests(next_requests);
      }
      break;
    }
  }
  if (args.need_output) {
    if (args.report_file.has_value()) {
      smo::PrintReport(*args.report_file, simulator);
    } else {
      smo::PrintReport(std::cout, simulator);
    }
  }
}

double CalculateNextTargetAmountOfRequests(double rejection_probability) {
  const double t_a = 1.643;
  const double delta = 0.1;
  const double p = rejection_probability;
  return (t_a * t_a * (1 - p)) / (p * delta * delta);
}
