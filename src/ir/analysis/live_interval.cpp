#include "ir/analysis/live_interval.h"
#include "ir/instruction.h"
#include <iostream>

namespace analysis {

void LiveInterval::AddRange(uint32_t start, uint32_t end) {
  if (start >= end) {
    return;
  }

  LiveRange new_range{start, end};

  auto it = std::lower_bound(ranges_.begin(), ranges_.end(), new_range);

  if (it != ranges_.begin()) {
    auto prev_it = std::prev(it);
    if (new_range.start <= prev_it->end) {
      prev_it->end = std::max(prev_it->end, new_range.end);
      new_range = *prev_it;
      it = ranges_.erase(prev_it);
    }
  }

  while (it != ranges_.end() && it->start <= new_range.end) {
    new_range.end = std::max(new_range.end, it->end);
    it = ranges_.erase(it);
  }

  ranges_.insert(it, new_range);
}

void LiveInterval::SetStart(uint32_t pos) {
  if (ranges_.empty()) {
    ranges_.push_back({pos, pos + 1});
  } else {
    if (pos > ranges_[0].start) {
      ranges_[0].start = pos;
    }
  }
}

bool LiveInterval::IsLiveAt(uint32_t point) const {
  for (const auto &range : ranges_) {
    if (range.Contains(point)) {
      return true;
    }
    if (range.start > point) {
      break;
    }
  }
  return false;
}

void LiveInterval::Dump(std::ostream &os) const {
  os << "i" << inst_->GetId() << ": ";
  for (const auto &range : ranges_) {
    os << "[" << range.start << ", " << range.end << ") ";
  }
}

} // namespace analysis
