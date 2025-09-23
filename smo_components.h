#ifndef SMO_COMPONENTS_H_
#define SMO_COMPONENTS_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <queue>
#include <ratio>
#include <vector>

namespace smo {
using Time = std::chrono::duration<double, std::milli>;
enum Result { success = 0, simulationEnded };
struct Source {
  std::size_t id;
  Time period;
  std::size_t generated;
  std::vector<Time> times_in_buffer;
  std::vector<Time> times_in_device;
};
struct Device {
  std::size_t id;
  Time processing_time;
  Time time_in_usage;
  bool occupied;
};
struct SourceReport {
  std::size_t generated_requests;
  Time reject_probability;
  Time average_buffer_time;
  Time average_processing_time;
  Time average_full_time;
  Time buffer_time_variance;
  Time processing_time_variance;
};
struct DeviceReport {
  double usage_coefficient;
};
struct Report {
  std::vector<SourceReport> source_reports;
  std::vector<DeviceReport> device_reports;
};
struct Request {
  std::size_t source_id;
  Time last_interaction;
};
enum SpecialEventKind { generateNewRequest, freeDevice };
struct SpecialEvent {
  SpecialEventKind kind;
  Time planned_time;
  std::size_t id;
};
struct SpecialEventComparator {
  bool operator()(SpecialEvent lhs, SpecialEvent rhs) const;
};
using SpecialEventQueue =
    std::priority_queue<SpecialEvent, std::vector<SpecialEvent>,
                        SpecialEventComparator>;
}  // namespace smo

#endif
