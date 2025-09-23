#ifndef SIMULATOR_BASE_H_
#define SIMULATOR_BASE_H_

#include <cstdint>
#include <functional>
#include <queue>
#include <vector>

#include "smo_components.h"

namespace smo {
class SimulatorBase {
 public:
  SimulatorBase(std::vector<smo::Source> &&sources,
                std::vector<smo::Device> &&devices,
                std::vector<smo::Request> &&buffer)
      : sources_(sources),
        devices_(devices),
        buffer_(buffer),
        current_amount_of_requests_(0),
        completed_(false) {}
  smo::Result Step();
  smo::Result RunToCompletion(std::uint64_t target_amount_of_requests);
  smo::Report GenerateReport();
  void Reset();

  bool is_completed();
  std::size_t current_amount_of_requests();

 protected:
  virtual void HandleBufferOverflow();
  virtual void PutInBuffer();
  virtual void TakeOutOfBuffer();

  std::vector<smo::Source> sources_;
  std::vector<smo::Device> devices_;
  std::vector<smo::Request> buffer_;

 private:
  void HandleNewRequestCreation();
  void HandleDeviceRelease();

  smo::SpecialEventQueue special_events_;
  bool completed_;
  std::size_t current_amount_of_requests_;
};
}  // namespace smo
#endif
