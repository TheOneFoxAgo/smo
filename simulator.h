#ifndef SIMULATOR_H_
#define SIMULATOR_H_
#include <random>

#include "simulator_base.h"
#include "simulator_config.h"
#include "smo_components.h"

namespace smo {
class Simulator final : public smo::SimulatorBase {
 public:
  Simulator(std::vector<std::chrono::milliseconds> source_periods,
            std::vector<std::exponential_distribution<>> device_distributions,
            std::size_t buffer_capacity, std::size_t target_amount_of_requests);
  Simulator(SimulatorConfig config);

  void Reset() override;
  std::vector<smo::Request> FakeBuffer() const;

 protected:
  std::optional<smo::Request> PutInBuffer(smo::Request request) override;
  std::optional<smo::Request> TakeOutOfBuffer() override;
  std::optional<std::size_t> PickDevice() override;
  smo::Time DeviceProcessingTime(std::size_t device_id,
                                 const smo::Request& request) override;
  smo::Time SourcePeriod(std::size_t source_id) override;

 private:
  std::mt19937 random_gen_;
  std::vector<std::chrono::milliseconds> source_periods_;
  std::vector<std::exponential_distribution<>> device_distributions_;
  std::vector<std::deque<smo::Request>> storage_;
  std::size_t buffer_capacity_;
  std::size_t buffer_size_;
  std::size_t current_packet_;
  std::vector<smo::DeviceStatistics>::const_iterator next_device_pointer_;
};
}  // namespace smo
#endif
