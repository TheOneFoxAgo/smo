#include <bits/chrono.h>

#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

#include "simulator_base.h"
#include "smo_components.h"

smo::SimulatorBase::SimulatorBase(std::size_t sources_amount,
                                  std::size_t devices_amount,
                                  std::size_t target_amount_of_requests)
    : sources_(std::vector<SourceStatistics>(sources_amount)),
      devices_(std::vector<DeviceStatistics>(devices_amount)),
      current_amount_of_requests_(0),
      target_amount_of_requests_(target_amount_of_requests) {}
// If simulation is completed, UB is triggered
smo::SpecialEvent smo::SimulatorBase::UncheckedStep() {
  SpecialEvent current_event = special_events_.top();
  current_simulation_time_ = current_event.planned_time;
  special_events_.pop();
  switch (current_event.kind) {
    case SpecialEventKind::generateNewRequest:
      HandleNewRequestCreation(current_event.id);
      break;
    case SpecialEventKind::deviceRelease:
      HandleDeviceRelease(current_event.id);
      break;
    case SpecialEventKind::endOfSimulation:
      break;
  }
  return current_event;
}

smo::SpecialEvent smo::SimulatorBase::Step() {
  if (is_completed()) {
    return SpecialEvent{SpecialEventKind::endOfSimulation};
  }
  return UncheckedStep();
}

void smo::SimulatorBase::RunToCompletion(size_t target_amount_of_requests) {
  if (is_completed()) {
    return;
  }
  while (!is_completed()) {
    UncheckedStep();
  }
}

static smo::Time CalculateAverage(const std::vector<smo::Time>& times,
                                  smo::Time::rep amount) {
  smo::Time result{0};
  for (auto& t : times) {
    result += t;
  }
  return result / amount;
}
static smo::Time CalculateVariance(const std::vector<smo::Time>& times,
                                   smo::Time average) {
  smo::Time::rep result = 0;
  for (auto& t : times) {
    auto diff = (t - average).count();
    result += diff * diff;
  }
  return smo::Time(result);
}

void smo::SimulatorBase::Reset() {
  for (auto& s : sources_) {
    s.generated = 0;
    s.rejected = 0;
    s.time_in_device = Time(0);
    s.time_squared_in_device = 0;
    s.time_in_buffer = Time(0);
    s.time_squared_in_buffer = 0;
  }
  for (auto& d : devices_) {
    d.current_request = std::nullopt;
    d.time_in_usage = Time(0);
  }
  special_events_.clear();
  current_amount_of_requests_ = 0;
  current_simulation_time_ = Time(0);
}

void smo::SimulatorBase::Reset(std::size_t target_amount_of_requests) {
  Reset();
  target_amount_of_requests_ = target_amount_of_requests;
}

bool smo::SimulatorBase::is_completed() const {
  return special_events_.empty();
}

std::size_t smo::SimulatorBase::current_amount_of_requests() const {
  return current_amount_of_requests_;
}
std::size_t smo::SimulatorBase::target_amount_of_requests() const {
  return target_amount_of_requests_;
}
smo::Time smo::SimulatorBase::current_simulation_time() const {
  return current_simulation_time_;
}
const std::vector<smo::SourceStatistics>&
smo::SimulatorBase::source_statistics() const {
  return sources_;
}
const std::vector<smo::DeviceStatistics>&
smo::SimulatorBase::device_statistics() const {
  return devices_;
}

void smo::SimulatorBase::HandleNewRequestCreation(std::size_t source_id) {
  auto& source = sources_[source_id];
  source.generated += 1;
  Request request{
      source_id,
      source.generated,
      current_simulation_time_,
  };
  current_amount_of_requests_ += 1;
  if (!OccupyNextDevice(request)) {
    auto rejected_request = PutInBuffer(request);
    if (rejected_request.has_value()) {
      HandleBufferOverflow(*rejected_request);
    }
  }

  if (current_amount_of_requests_ >= target_amount_of_requests_) {
    special_events_.remove_excess_generations();
    for (auto& source : sources_) {
      source.next_request = Time::max();
    }
  } else {
    AddSpecialEvent(SpecialEvent{
        SpecialEventKind::generateNewRequest,
        current_simulation_time_ + SourcePeriod(source_id),
        source_id,
    });
  }
}

void smo::SimulatorBase::HandleBufferOverflow(const smo::Request& request) {
  auto time = current_simulation_time_ - request.generation_time;
  auto& source = sources_[request.source_id];
  source.AddTimeInBuffer(time);
  source.rejected += 1;
}

void smo::SimulatorBase::HandleDeviceRelease(std::size_t device_id) {
  auto& device = devices_[device_id];
  device.current_request = std::nullopt;
  auto request = TakeOutOfBuffer();
  if (request.has_value()) {
    auto time = current_simulation_time_ - request->generation_time;
    sources_[request->source_id].AddTimeInBuffer(time);
    OccupyNextDevice(*request);
  } else {
    device.next_request = Time::max();
  }
}
bool smo::SimulatorBase::OccupyNextDevice(smo::Request request) {
  auto device_id = PickDevice();
  if (device_id.has_value()) {
    auto& device = devices_[*device_id];
    auto processing_time = DeviceProcessingTime(*device_id, request);
    sources_[request.source_id].AddTimeInDevice(processing_time);
    device.current_request = request;
    AddSpecialEvent(SpecialEvent{
        SpecialEventKind::deviceRelease,
        current_simulation_time_ + processing_time,
        *device_id,
    });
    return true;
  } else {
    return false;
  }
}
void smo::SimulatorBase::AddSpecialEvent(smo::SpecialEvent event) {
  switch (event.kind) {
    case SpecialEventKind::generateNewRequest:
      sources_[event.id].next_request = event.planned_time;
      break;
    case SpecialEventKind::deviceRelease:
      devices_[event.id].next_request = event.planned_time;
      break;
    case SpecialEventKind::endOfSimulation:
      break;
  }
  special_events_.push(event);
}
