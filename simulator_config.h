#ifndef SIMULATOR_CONFIG_H_
#define SIMULATOR_CONFIG_H_

#include <chrono>
#include <cstddef>
#include <iosfwd>
#include <random>
#include <vector>

namespace smo {
struct SimulatorConfig {
  std::size_t buffer_capacity;
  std::size_t target_amount_of_requests;
  std::vector<std::chrono::milliseconds> source_periods;
  std::vector<std::exponential_distribution<>> device_distributions;
};
std::istream& operator>>(std::istream& in, SimulatorConfig& config);
}  // namespace smo
#endif
