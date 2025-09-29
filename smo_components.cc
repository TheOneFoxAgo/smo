#include <algorithm>

#include "smo_components.h"

static smo::Time::rep CalculateVariance(smo::Time::rep squares,
                                        smo::Time::rep average,
                                        std::size_t amount) {
  return squares / amount - average * average;
}
smo::Time smo::SourceStatistics::AverageBufferTime() const {
  return time_in_buffer / generated;
}
smo::Time smo::SourceStatistics::AverageDeviceTime() const {
  return time_in_device / (generated - rejected);
}
smo::Time::rep smo::SourceStatistics::BufferTimeVariance() const {
  return CalculateVariance(time_squared_in_buffer, AverageBufferTime().count(),
                           generated);
}
smo::Time::rep smo::SourceStatistics::DeviceTimeVariance() const {
  return CalculateVariance(time_squared_in_device, AverageDeviceTime().count(),
                           generated - rejected);
}
void smo::SourceStatistics::AddTimeInBuffer(smo::Time time) {
  auto time_tics = time.count();
  time_in_buffer += time;
  time_squared_in_buffer += time_tics * time_tics;
}
void smo::SourceStatistics::AddTimeInDevice(smo::Time time) {
  auto time_tics = time.count();
  time_in_device += time;
  time_squared_in_device += time_tics * time_tics;
}
bool smo::SpecialEventComparator::operator()(const SpecialEvent& lhs,
                                             const SpecialEvent& rhs) const {
  return lhs.planned_time > rhs.planned_time;
}

void smo::SpecialEventQueue::Clear() { this->c.clear(); }

void smo::SpecialEventQueue::RemoveExcessGenerations() {
  auto new_end =
      std::remove_if(this->c.begin(), this->c.end(), [](SpecialEvent event) {
        return event.kind == SpecialEventKind::generateNewRequest;
      });
  this->c.erase(new_end, c.end());
  std::make_heap(this->c.begin(), this->c.end(), this->comp);
}
