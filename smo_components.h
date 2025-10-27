#ifndef SMO_COMPONENTS_H_
#define SMO_COMPONENTS_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <string>
#include <vector>

namespace smo {
using Time = std::uint64_t;
constexpr Time maxTime = std::numeric_limits<Time>::max();
struct SourceStatistics {
  double AverageBufferTime() const;
  double AverageDeviceTime() const;
  double BufferTimeVariance() const;
  double DeviceTimeVariance() const;
  void AddTimeInBuffer(Time time);
  void AddTimeInDevice(Time time);

  std::size_t generated = 0;
  std::size_t rejected = 0;
  Time next_request = 0;
  Time time_in_buffer = 0;
  Time time_in_device = 0;
  double time_squared_in_buffer = 0;
  double time_squared_in_device = 0;
};
struct Request {
  std::size_t source_id;
  std::size_t number;
  Time generation_time;
};
struct DeviceStatistics {
  Time next_request = maxTime;
  Time time_in_usage = 0;
  std::optional<Request> current_request;
};
enum class SpecialEventKind {
  endOfSimulation = 0,
  generateNewRequest = 1,
  deviceRelease = 2,
};
struct SpecialEvent {
  SpecialEventKind kind;
  Time planned_time;
  std::size_t id;
};
struct SpecialEventComparator {
  bool operator()(const SpecialEvent& lhs, const SpecialEvent& rhs) const;
};
// Breaking the Google naming scheme,
// because it's an extension of std collection.
class special_event_queue
    : public std::priority_queue<SpecialEvent, std::vector<SpecialEvent>,
                                 SpecialEventComparator> {
 public:
  void clear();
  void remove_excess_generations();
};
}  // namespace smo

#endif
