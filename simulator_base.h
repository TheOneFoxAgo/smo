#ifndef SIMULATOR_BASE_H_
#define SIMULATOR_BASE_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <queue>
#include <vector>

#include "smo_components.h"

namespace smo {
class SimulatorBase {
 public:
  SimulatorBase(std::size_t sources_amount, std::size_t devices_amount,
                std::size_t target_amount_of_requests);
  virtual ~SimulatorBase() = default;

  SpecialEvent Step();
  void RunToCompletion();
  virtual void Reset();
  virtual void ResetWithNewAmountOfRequests(
      std::size_t target_amount_of_requests);
  bool is_completed() const;
  std::size_t current_amount_of_requests() const;
  std::size_t target_amount_of_requests() const;
  std::size_t rejected_amount() const;
  Time current_simulation_time() const;
  const std::vector<SourceStatistics>& source_statistics() const;
  const std::vector<DeviceStatistics>& device_statistics() const;

 protected:
  void AddSpecialEvent(SpecialEvent event);

  virtual std::optional<Request> PutInBuffer(Request request) = 0;
  virtual std::optional<Request> TakeOutOfBuffer() = 0;
  virtual std::optional<std::size_t> PickDevice() = 0;
  virtual Time DeviceProcessingTime(std::size_t device_id,
                                    const Request& request) = 0;
  virtual Time SourcePeriod(std::size_t source_id) = 0;

 private:
  void HandleBufferOverflow(const Request& request);
  void HandleNewRequestCreation(std::size_t source_id);
  void HandleDeviceRelease(std::size_t device_id);
  bool OccupyNextDevice(Request request);
  SpecialEvent UncheckedStep();

  std::vector<SourceStatistics> sources_;
  std::vector<DeviceStatistics> devices_;
  special_event_queue special_events_;
  std::size_t current_amount_of_requests_{0};
  std::size_t target_amount_of_requests_{0};
  std::size_t rejected_amount_{0};
  Time current_simulation_time_{0};
};
}  // namespace smo
#endif
