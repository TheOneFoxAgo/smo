#ifndef SIMULATOR_H_
#define SIMULATOR_H_
#include <cstddef>
#include <deque>
#include <random>
#include <vector>

#include "../simulator_base.h"
#include "../smo_components.h"
#include "simulator_config.h"

namespace smo {
class Simulator final : public smo::SimulatorBase {
 public:
  Simulator(std::vector<smo::Time> source_periods,
            std::vector<double> device_coefficients,
            std::size_t buffer_capacity, std::size_t target_amount_of_requests);
  Simulator(SimulatorConfig config);

  void Reset() override;
  std::vector<smo::Request> FakeBuffer() const;
  const std::vector<std::deque<Request>>& RealBuffer() const;

 protected:
  std::optional<Request> PutInBuffer(Request request) override;
  std::optional<Request> TakeOutOfBuffer() override;
  std::optional<std::size_t> PickDevice() override;
  Time DeviceProcessingTime(std::size_t device_id,
                            const Request& request) override;
  smo::Time SourcePeriod(std::size_t source_id) override;

 private:
  void Init();

  std::mt19937 random_gen_;
  std::exponential_distribution<> distribution_{1.0};
  std::vector<smo::Time> source_periods_;
  std::vector<double> device_coefficients_;
  std::vector<std::deque<Request>> storage_;
  std::size_t buffer_capacity_ = 0;
  std::size_t buffer_size_ = 0;
  std::size_t current_packet_ = 0;
  std::vector<smo::DeviceStatistics>::const_iterator next_device_pointer_;
};
}  // namespace smo
#endif
