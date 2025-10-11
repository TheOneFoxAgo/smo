#ifndef SIMULATOR_CONFIG_H_
#define SIMULATOR_CONFIG_H_

#include <chrono>
#include <cstddef>
#include <iosfwd>
#include <random>
#include <vector>

#include "../smo_components.h"

namespace smo {
struct SimulatorConfig {
  std::size_t buffer_capacity;
  std::size_t target_amount_of_requests;
  std::vector<smo::Time> source_periods;
  std::vector<double> device_coefficients;
};
std::istream& operator>>(std::istream& in, SimulatorConfig& config);
}  // namespace smo
#endif
