#include <algorithm>
#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <random>
#include <vector>

#include "simulator_base.h"
#include "smo_components.h"

class Simulator final : public smo::SimulatorBase {
 public:
  Simulator(std::vector<std::chrono::milliseconds> source_periods,
            std::vector<std::exponential_distribution<>> device_distributions,
            std::size_t buffer_capacity, std::size_t target_amount_of_requests)
      : smo::SimulatorBase{source_periods.size(), device_distributions.size(),
                           target_amount_of_requests},
        random_gen_(std::mt19937(std::random_device{}())),
        source_periods_(source_periods),
        device_distributions_(device_distributions),
        buffer_capacity_(buffer_capacity),
        storage_(std::vector<std::deque<smo::Request>>(source_periods.size())) {
  }

  void Reset() override {
    smo::SimulatorBase::Reset();
    for (auto& subbuffer : storage_) {
      subbuffer.clear();
    }
  }
  std::vector<smo::Request> FakeBuffer() const {
    std::vector<smo::Request> result(buffer_capacity_);
    auto begin = result.begin();
    for (const auto& subbuffer : storage_) {
      begin = std::copy(subbuffer.begin(), subbuffer.end(), begin);
    }
    std::sort(result.begin(), result.end(),
              [](const smo::Request& lhs, const smo::Request& rhs) {
                if (lhs.generation_time == rhs.generation_time) {
                  return lhs.source_id < rhs.source_id;
                } else {
                  return lhs.generation_time < rhs.generation_time;
                }
              });
    return result;
  }

 protected:
  std::optional<smo::Request> PutInBuffer(smo::Request request) override {
    std::optional<smo::Request> rejected;
    if (buffer_size_ == buffer_capacity_) {
      auto not_empty_subbuffer =
          std::find_if_not(storage_.rbegin(), storage_.rend(),
                           std::mem_fn(&std::deque<smo::Request>::empty));
      rejected = not_empty_subbuffer->front();
      not_empty_subbuffer->pop_front();
      buffer_size_ -= 1;
    }
    storage_[request.source_id].push_back(request);
    buffer_size_ += 1;
    return rejected;
  }
  std::optional<smo::Request> TakeOutOfBuffer() override {
    if (buffer_size_ == 0) {
      return std::nullopt;
    } else {
      std::deque<smo::Request>& subbuffer = storage_[current_packet_];
      if (subbuffer.empty()) {
        auto not_empty_subbuffer =
            std::find_if_not(storage_.begin(), storage_.end(),
                             std::mem_fn(&std::deque<smo::Request>::empty));
        current_packet_ = not_empty_subbuffer - storage_.begin();
        subbuffer = *not_empty_subbuffer;
      }
      auto result = subbuffer.front();
      subbuffer.pop_front();
      buffer_size_ -= 1;
      return result;
    }
  }
  std::optional<std::size_t> PickDevice() override {
    std::optional<std::size_t> result;
    auto TryToPick = [&](std::size_t i) {
      if (!device_statistics(i).current_request.has_value()) {
        result = i;
        if (i == device_distributions_.size() - 1) {
          next_device_index_ = 0;
        } else {
          next_device_index_ = i + 1;
        }
      }
    };
    for (auto i = next_device_index_; i < device_distributions_.size(); ++i) {
      TryToPick(i);
    }
    if (result.has_value()) {
      return result;
    }
    for (auto i = 0; i < next_device_index_; ++i) {
      TryToPick(i);
    }
    return result;
  }
  smo::Time DeviceProcessingTime(std::size_t device_id,
                                 const smo::Request& request) override {
    return smo::Time(device_distributions_[device_id](random_gen_));
  }
  smo::Time SourcePeriod(std::size_t source_id) override {
    return source_periods_[source_id];
  }

 private:
  std::mt19937 random_gen_;
  std::vector<std::chrono::milliseconds> source_periods_;
  std::vector<std::exponential_distribution<>> device_distributions_;
  std::vector<std::deque<smo::Request>> storage_;
  std::size_t buffer_capacity_;
  std::size_t buffer_size_;
  std::size_t current_packet_;
  std::size_t next_device_index_;
};
