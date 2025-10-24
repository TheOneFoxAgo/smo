#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <optional>
#include <vector>

#include "simulator.h"

smo::Simulator::Simulator(std::vector<smo::Time> source_periods,
                          std::vector<double> device_coefficients,
                          std::size_t buffer_capacity,
                          std::size_t target_amount_of_requests,
                          SimulatorLaw law)
    : smo::SimulatorBase{source_periods.size(), device_coefficients.size(),
                         target_amount_of_requests},
      law_(law),
      random_gen_(std::mt19937(std::random_device{}())),
      source_periods_(std::move(source_periods)),
      device_coefficients_(std::move(device_coefficients)),
      storage_(std::vector<std::deque<smo::Request>>(source_periods_.size())),
      buffer_capacity_(buffer_capacity) {
  next_device_pointer_ = device_statistics().begin();
  Init();
}
smo::Simulator::Simulator(SimulatorConfig config, SimulatorLaw law)
    : smo::Simulator{std::move(config.source_periods),
                     std::move(config.device_coefficients),
                     config.buffer_capacity, config.target_amount_of_requests,
                     law} {}
void smo::Simulator::Reset() {
  smo::SimulatorBase::Reset();
  for (auto& subbuffer : storage_) {
    subbuffer.clear();
  }
  Init();
}

void smo::Simulator::Init() {
  for (std::size_t i = 0; i < source_periods_.size(); ++i) {
    AddSpecialEvent(SpecialEvent{
        SpecialEventKind::generateNewRequest,
        source_periods_[i],
        i,
    });
  }
}
std::vector<smo::Request> smo::Simulator::FakeBuffer() const {
  std::vector<Request> result;
  result.reserve(buffer_capacity_);
  for (const auto& subbuffer : storage_) {
    result.insert(result.end(), subbuffer.begin(), subbuffer.end());
  }
  std::sort(result.begin(), result.end(),
            [](const Request& lhs, const Request& rhs) {
              if (lhs.generation_time == rhs.generation_time) {
                return lhs.source_id < rhs.source_id;
              } else {
                return lhs.generation_time < rhs.generation_time;
              }
            });
  return result;
}

std::size_t smo::Simulator::current_packet() const { return current_packet_; }

const std::vector<std::deque<smo::Request>>& smo::Simulator::RealBuffer()
    const {
  return storage_;
}

std::optional<smo::Request> smo::Simulator::PutInBuffer(Request request) {
  std::optional<Request> rejected;
  if (buffer_size_ == buffer_capacity_) {
    auto not_empty_subbuffer =
        std::find_if_not(storage_.rbegin(), storage_.rend(),
                         std::mem_fn(&std::deque<Request>::empty));
    rejected = not_empty_subbuffer->front();
    not_empty_subbuffer->pop_front();
    buffer_size_ -= 1;
  }
  storage_[request.source_id].push_back(request);
  buffer_size_ += 1;
  return rejected;
}
std::optional<smo::Request> smo::Simulator::TakeOutOfBuffer() {
  if (buffer_size_ == 0) {
    return std::nullopt;
  } else {
    std::deque<Request>* subbuffer = &storage_[current_packet_];
    if (subbuffer->empty()) {
      auto not_empty_subbuffer =
          std::find_if_not(storage_.begin(), storage_.end(),
                           std::mem_fn(&std::deque<Request>::empty));
      current_packet_ = not_empty_subbuffer - storage_.begin();
      subbuffer = &*not_empty_subbuffer;
    }
    auto result = subbuffer->front();
    subbuffer->pop_front();
    buffer_size_ -= 1;
    return result;
  }
}
std::optional<std::size_t> smo::Simulator::PickDevice() {
  std::optional<std::size_t> result;
  auto DeviceIsOccupied = [](const DeviceStatistics& device) {
    return device.current_request.has_value();
  };
  const auto& devices = device_statistics();
  auto candidate =
      std::find_if_not(next_device_pointer_, devices.end(), DeviceIsOccupied);
  if (candidate != devices.end()) {
    result = candidate - devices.begin();
  }
  if (!result.has_value()) {
    candidate = std::find_if_not(devices.begin(), next_device_pointer_,
                                 DeviceIsOccupied);
  }
  if (candidate != next_device_pointer_) {
    result = candidate - devices.begin();
  }
  if (result.has_value()) {
    if (++candidate == devices.end()) {
      next_device_pointer_ = devices.begin();
    } else {
      next_device_pointer_ = candidate;
    }
  }
  return result;
}
smo::Time smo::Simulator::DeviceProcessingTime(std::size_t device_id,
                                               const Request& request) {
  if (law_ == SimulatorLaw::deterministic) {
    return smo::Time(device_coefficients_[device_id]);
  } else {
    return smo::Time(device_coefficients_[device_id] *
                     distribution_(random_gen_));
  }
}
smo::Time smo::Simulator::SourcePeriod(std::size_t source_id) {
  return source_periods_[source_id];
}
