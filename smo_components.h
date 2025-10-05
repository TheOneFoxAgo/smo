#ifndef SMO_COMPONENTS_H_
#define SMO_COMPONENTS_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <queue>
#include <string>
#include <vector>

namespace smo {
using Time = std::chrono::duration<double>;
struct SourceStatistics {
  Time AverageBufferTime() const;
  Time AverageDeviceTime() const;
  Time::rep BufferTimeVariance() const;
  Time::rep DeviceTimeVariance() const;
  void AddTimeInBuffer(Time time);
  void AddTimeInDevice(Time time);

  std::size_t generated;
  std::size_t rejected;
  Time next_request;
  Time time_in_buffer;
  Time time_in_device;
  Time::rep time_squared_in_buffer;
  Time::rep time_squared_in_device;
};
struct Request {
  std::size_t source_id;
  std::size_t number;
  Time generation_time;
};
struct DeviceStatistics {
  Time next_request;
  Time time_in_usage;
  std::optional<Request> current_request;
};
enum class SpecialEventKind {
  generateNewRequest,
  deviceRelease,
  endOfSimulation
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
