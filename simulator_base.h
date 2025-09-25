#ifndef SIMULATOR_BASE_H_
#define SIMULATOR_BASE_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <queue>
#include <vector>

#include "smo_components.h"

namespace smo {
class SimulatorBase {
 public:
  SimulatorBase(std::size_t sources_amount, std::size_t devices_amount,
                std::size_t target_amount_of_requests)
      : sources_(std::vector<smo::Source>(sources_amount)),
        devices_(std::vector<smo::Device>(devices_amount)),
        current_amount_of_requests_(0),
        target_amount_of_requests_(target_amount_of_requests) {
    Init();
  }
  smo::Result Step();
  smo::Result RunToCompletion(std::size_t target_amount_of_requests);
  smo::Report GenerateReport() const;
  void Reset();
  void Reset(std::size_t target_amount_of_requests);

  bool is_completed() const;
  std::size_t current_amount_of_requests() const;
  std::size_t target_amount_of_requests() const;
  smo::Time full_simulation_time() const;

 protected:
  virtual smo::Result PutInBuffer(Request request);
  virtual std::optional<Request> TakeOutOfBuffer();
  virtual std::optional<std::size_t> PickDevice();
  virtual smo::Time DeviceProcessingTime(std::size_t device_id);
  virtual smo::Time SourcePeriod(std::size_t source_id);

  std::vector<smo::Source> sources_;
  std::vector<smo::Device> devices_;
  std::size_t current_amount_of_requests_;
  std::size_t target_amount_of_requests_;
  smo::Time current_simulation_time_;

 private:
  void HandleBufferOverflow(Request request);
  void HandleNewRequestCreation(std::size_t source_id);
  void HandleDeviceRelease(std::size_t device_id);
  smo::Result OccupyNextDevice(Request request);
  void Init();
  void UncheckedStep();

  smo::SpecialEventQueue special_events_;
};
}  // namespace smo
#endif
