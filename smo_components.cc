#include <algorithm>

#include "smo_components.h"

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
