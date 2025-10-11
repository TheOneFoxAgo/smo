#include <algorithm>

#include "smo_components.h"

static double CalculateVariance(double squares, double average,
                                std::size_t amount) {
  return squares / amount - average * average;
}
double smo::SourceStatistics::AverageBufferTime() const {
  return static_cast<double>(time_in_buffer) / generated;
}
double smo::SourceStatistics::AverageDeviceTime() const {
  return static_cast<double>(time_in_device) / (generated - rejected);
}
double smo::SourceStatistics::BufferTimeVariance() const {
  return CalculateVariance(time_squared_in_buffer, AverageBufferTime(),
                           generated);
}
double smo::SourceStatistics::DeviceTimeVariance() const {
  return CalculateVariance(time_squared_in_device, AverageDeviceTime(),
                           generated - rejected);
}
void smo::SourceStatistics::AddTimeInBuffer(smo::Time time) {
  time_in_buffer += time;
  time_squared_in_buffer += time * time;
}
void smo::SourceStatistics::AddTimeInDevice(smo::Time time) {
  time_in_device += time;
  time_squared_in_device += time * time;
}
bool smo::SpecialEventComparator::operator()(const SpecialEvent& lhs,
                                             const SpecialEvent& rhs) const {
  if (lhs.kind == SpecialEventKind::endOfSimulation) {
    return true;
  }
  if (rhs.kind == SpecialEventKind::endOfSimulation) {
    return false;
  }
  if (lhs.kind == SpecialEventKind::generateNewRequest &&
      rhs.kind == SpecialEventKind::deviceRelease) {
    return lhs.planned_time >= rhs.planned_time;
  } else {
    return lhs.planned_time > rhs.planned_time;
  }
}

void smo::special_event_queue::clear() { this->c.clear(); }

void smo::special_event_queue::remove_excess_generations() {
  auto new_end =
      std::remove_if(this->c.begin(), this->c.end(), [](SpecialEvent event) {
        return event.kind == SpecialEventKind::generateNewRequest;
      });
  this->c.erase(new_end, c.end());
  std::make_heap(this->c.begin(), this->c.end(), this->comp);
}
