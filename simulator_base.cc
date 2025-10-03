#include <bits/chrono.h>

#include <chrono>
#include <cstddef>
#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <vector>

#include "simulator_base.h"
#include "smo_components.h"

// If simulation is completed, UB is triggered
void smo::SimulatorBase::UncheckedStep() {
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
smo::Report smo::SimulatorBase::GenerateReport() const {
  smo::Report report{std::vector<SourceReport>(sources_.size()),
                     std::vector<DeviceReport>(devices_.size())};
  for (size_t i = 0; i < sources_.size(); ++i) {
    auto& source = sources_[i];
    auto& source_report = report.source_reports[i];
    source_report.rejection_probability =
        static_cast<double>(source.rejected) / source.generated;
    source_report.average_buffer_time = source.AverageBufferTime();
    source_report.average_processing_time = source.AverageDeviceTime();
    source_report.average_full_time = source_report.average_buffer_time +
                                      source_report.average_processing_time;
    source_report.buffer_time_variance = source.BufferTimeVariance();
    source_report.processing_time_variance = source.DeviceTimeVariance();
  }
  for (size_t i = 0; i < devices_.size(); ++i) {
    auto& device = devices_[i];
    auto& device_report = report.device_reports[i];
    device_report.usage_coefficient =
        device.time_in_usage / current_simulation_time_;
  }
  return report;
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

void smo::SimulatorBase::HandleNewRequestCreation(std::size_t source_id) {
  auto& source = sources_[source_id];
  source.generated += 1;
  Request request{
      source_id,
      source.generated,
      current_simulation_time_,
  };
  current_amount_of_requests_ += 1;
  if (OccupyNextDevice(request) == Result::failure) {
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
  OnNewRequestCreation(request);
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
  OnDeviceRelease(device_id);
}
smo::Result smo::SimulatorBase::OccupyNextDevice(smo::Request request) {
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
    return Result::success;
  } else {
    return Result::failure;
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
  }
  special_events_.push(event);
}
void smo::SimulatorBase::OnNewRequestCreation(const Request& request) {}
void smo::SimulatorBase::OnDeviceRelease(std::size_t device_id) {}

static std::string format_date(smo::Time time) {
  if (time == smo::Time::max()) {
    return "Invalid";
  }
  std::string result;
  auto days = std::chrono::duration_cast<std::chrono::days>(time);
  time -= days;
  auto hours = std::chrono::duration_cast<std::chrono::hours>(time);
  time -= hours;
  auto minutes = std::chrono::duration_cast<std::chrono::minutes>(time);
  time -= minutes;
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time);
  time -= seconds;
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time);
  time -= millis;
  auto micros = std::chrono::duration_cast<std::chrono::microseconds>(time);
  time -= micros;
  auto AddToResult = [&](long long time, const char* postfix) {
    if (time != 0) {
      std::format_to(std::back_inserter(result), "{}{} ", time, postfix);
    }
  };
  AddToResult(days.count(), "d");
  AddToResult(hours.count(), "h");
  AddToResult(minutes.count(), "m");
  AddToResult(seconds.count(), "s");
  AddToResult(millis.count(), "ms");
  AddToResult(micros.count(), "us");
  if (result.empty()) {
    result += '0';
  } else {
    result.pop_back();
  }
  return result;
}
const std::ostream& smo::SimulatorBase::PrintCalendar(std::ostream& out) const {
  const auto indexWidth = 10;
  const auto nextWidth = 25;
  const auto signWidth = 6;
  using ItOut = std::ostream_iterator<char>;
  std::format_to(ItOut(out), "{:<{}} | {:<{}} | {:<{}} | {:<{}}\n",
                 "index:", indexWidth, "next:", nextWidth, "sign:", signWidth,
                 "request:", 2 * indexWidth + 1);
  auto PrintRow = [&](int i, Time next) {
    std::format_to(ItOut(out), "{:>{}} | {:>{}} | {:>{}}", i, indexWidth, next,
                   nextWidth, next == Time::max() ? 1 : 0, signWidth);
  };
  out << "Sources:\n";
  for (int i = 0; i < sources_.size(); ++i) {
    PrintRow(i, sources_[i].next_request);
    out << '\n';
  }
  auto PrintRequest = [&](const std::optional<Request>& req) {
    if (req.has_value()) {
      std::format_to(ItOut(out), " | {:>{}}:{:<{}}", req->source_id, indexWidth,
                     req->number, indexWidth);
    } else {
      std::format_to(ItOut(out), " | {:>{}}", "Free", 2 * indexWidth + 1);
    }
  };
  out << "Devices:\n";
  for (int i = 0; i < sources_.size(); ++i) {
    PrintRow(i, devices_[i].next_request);
    PrintRequest(devices_[i].current_request);
  }
  return out;
}
