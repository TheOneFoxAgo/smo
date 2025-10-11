#include <cstddef>
#include <cstdint>
#include <functional>
#include <ios>
#include <istream>
#include <map>
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
         std::uint64_t result = 0;
         in >> result;
         while (in) {
           config.source_periods.push_back(result);
           in >> result;
         }
         in.clear();
       }},
      {"Devices:",
       [&] {
         double result = 0.0;
         in >> result;
         while (in) {
           config.device_coefficients.push_back(result);
           in >> result;
         }
         in.clear();
       }},
  };
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
