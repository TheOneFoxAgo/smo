#include <chrono>
#include <cstddef>
#include <functional>
#include <ios>
#include <istream>
#include <map>
#include <random>
#include <string>
#include <utility>

#include "simulator_config.h"

std::istream &smo::operator>>(std::istream &in, SimulatorConfig &config) {
  std::istream::sentry sentry(in);
  if (!sentry) {
    return in;
  }
  std::map<std::string, std::function<void()>> headers{
      {"Requests:", [&] { in >> config.target_amount_of_requests; }},
      {"Buffer:", [&] { in >> config.buffer_capacity; }},
      {"Sources:",
       [&] {
         while (in) {
           std::size_t result = 0;
           in >> result;
           config.source_periods.push_back(std::chrono::milliseconds(result));
         }
       }},
      {"Devices:",
       [&] {
         while (in) {
           double result = 0.0;
           in >> result;
           config.device_distributions.push_back(
               std::exponential_distribution<>(result));
         }
       }},
  };
  std::size_t remaining = 0;
  std::string header;
  while (in && headers.size() != 0) {
    in >> header;
    auto matched_header = headers.find(header);
    if (matched_header != headers.end()) {
      matched_header->second();
      headers.erase(matched_header);
    } else {
      in.setstate(std::ios_base::failbit);
    }
  }

  return in;
}
