#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

#include "simulator_base.h"
#include "smo_components.h"

// If simulation is completed, UB is triggered
void smo::SimulatorBase::UncheckedStep() {
  SpecialEvent current_event = special_events_.top();
  full_simulation_time_ = current_event.planned_time;
  special_events_.pop();
  switch (current_event.kind) {
    case SpecialEventKind::generateNewRequest:
      HandleNewRequestCreation(current_event.id);
      break;
    case SpecialEventKind::deviceRelease:
      HandleDeviceRelease(current_event.id);
      break;
  }
}

smo::Result smo::SimulatorBase::Step() {
  if (is_completed()) {
    return Result::failure;
  }
  UncheckedStep();
  return Result::success;
}

smo::Result smo::SimulatorBase::RunToCompletion(
    size_t target_amount_of_requests) {
  if (is_completed()) {
    return Result::failure;
  }
  while (!is_completed()) {
    UncheckedStep();
  }
  return Result::success;
}

static smo::Time calculate_average(const std::vector<smo::Time>& times,
                                   smo::Time::rep amount) {
  smo::Time result{0};
  for (auto& t : times) {
    result += t;
  }
  return result / amount;
}
static smo::Time calculate_variance(const std::vector<smo::Time>& times,
                                    smo::Time average) {
  smo::Time::rep result = 0;
  for (auto& t : times) {
    auto diff = (t - average).count();
    result += diff * diff;
  }
  return smo::Time(result);
}
smo::Report smo::SimulatorBase::GenerateReport() const {
  smo::Report report{std::vector<SourceReport>(sources_.size()),
                     std::vector<DeviceReport>(devices_.size())};
  for (size_t i = 0; i < sources_.size(); ++i) {
    auto& source = sources_[i];
    auto& source_report = report.source_reports[i];
    source_report.average_buffer_time =
        calculate_average(source.times_in_buffer, source.generated);
    source_report.average_processing_time =
        calculate_average(source.times_in_device, source.generated);
    source_report.average_full_time = source_report.average_buffer_time +
                                      source_report.average_processing_time;
    source_report.buffer_time_variance = calculate_variance(
        source.times_in_buffer, source_report.average_buffer_time);
    source_report.processing_time_variance = calculate_variance(
        source.times_in_device, source_report.average_processing_time);
  }
  for (size_t i = 0; i < devices_.size(); ++i) {
    auto& device = devices_[i];
    auto& device_report = report.device_reports[i];
    device_report.usage_coefficient =
        device.time_in_usage / full_simulation_time_;
  }
  return report;
}

void smo::SimulatorBase::Reset() {
  for (auto& s : sources_) {
    s.generated = 0;
    s.times_in_buffer.clear();
    s.times_in_device.clear();
  }
  for (auto& d : devices_) {
    d.request = std::nullopt;
    d.time_in_usage = Time(0);
  }
  buffer_.clear();
  special_events_.Clear();
  current_amount_of_requests_ = 0;
  full_simulation_time_ = Time(0);
  Init();
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
smo::Time smo::SimulatorBase::full_simulation_time() const {
  return full_simulation_time_;
}

void smo::SimulatorBase::HandleNewRequestCreation(std::size_t source_id) {
  auto& source = sources_[source_id];
  Request request{
      source_id,
      full_simulation_time_,
  };
  current_amount_of_requests_ += 1;
  if (OccupyNextDevice(request) == Result::failure) {
    if (PutInBuffer(request) == Result::failure) {
      HandleBufferOverflow(request);
    }
  }

  if (current_amount_of_requests_ >= target_amount_of_requests_) {
    special_events_.RemoveExcessGenerations();
  } else {
    special_events_.push(SpecialEvent{
        SpecialEventKind::generateNewRequest,
        full_simulation_time_ + source.period,
        source_id,
    });
  }
}
void smo::SimulatorBase::HandleDeviceRelease(std::size_t device_id) {
  auto& device = devices_[device_id];
  sources_[device.request->source_id].times_in_device.push_back(
      full_simulation_time_ - device.request->last_interaction);
  device.request = std::nullopt;
  auto request = TakeOutOfBuffer();
  if (request.has_value()) {
    sources_[request->source_id].times_in_buffer.push_back(
        full_simulation_time_ - request->last_interaction);
    request->last_interaction = full_simulation_time_;
    OccupyNextDevice(*request);
  }
}
smo::Result smo::SimulatorBase::OccupyNextDevice(smo::Request request) {
  auto device_id = PickDevice();
  if (device_id.has_value()) {
    auto& device = devices_[*device_id];
    device.request = request;
    special_events_.push(SpecialEvent{
        SpecialEventKind::deviceRelease,
        full_simulation_time_ + device.processing_time,
        *device_id,
    });
    return Result::success;
  } else {
    return Result::failure;
  }
}
void smo::SimulatorBase::Init() {
  for (size_t i = 0; i < sources_.size(); ++i) {
    auto& source = sources_[i];
    special_events_.push(
        SpecialEvent{SpecialEventKind::generateNewRequest, source.period, i});
  }
}
