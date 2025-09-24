#ifndef SMO_COMPONENTS_H_
#define SMO_COMPONENTS_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <queue>
#include <ratio>
#include <vector>

namespace smo {
using Time = std::chrono::duration<double, std::milli>;
enum class Result { success = 0, failure };
struct Source {
  Time period;
  std::size_t generated;
  std::vector<Time> times_in_buffer;
  std::vector<Time> times_in_device;
};
struct Request {
  std::size_t source_id;
  Time last_interaction;
};
struct Device {
  Time processing_time;
  Time time_in_usage;
  std::optional<Request> request;
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
enum class SpecialEventKind { generateNewRequest, deviceRelease };
struct SpecialEvent {
  SpecialEventKind kind;
  Time planned_time;
  std::size_t id;
};
struct SpecialEventComparator {
  bool operator()(const SpecialEvent& lhs, const SpecialEvent& rhs) const;
};
class SpecialEventQueue
    : public std::priority_queue<SpecialEvent, std::vector<SpecialEvent>,
                                 SpecialEventComparator> {
 public:
  void Clear();
  void RemoveExcessGenerations();
};
}  // namespace smo

#endif
