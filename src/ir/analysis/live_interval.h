#pragma once

#include <algorithm>
#include <cstdint>
#include <iosfwd>
#include <vector>

#include "ir/instruction.h"

namespace analysis {

struct LiveRange {
  uint32_t start;
  uint32_t end;

  bool Contains(uint32_t point) const { return point >= start && point < end; }

  bool Intersects(const LiveRange &other) const { return std::max(start, other.start) < std::min(end, other.end); }

  // Needed for sorting
  bool operator<(const LiveRange &other) const { return start < other.start; }
};

class LiveInterval {
public:
  explicit LiveInterval(const Instruction *inst) : inst_(inst) {}

  void AddRange(uint32_t start, uint32_t end);
  void SetStart(uint32_t pos);

  bool IsLiveAt(uint32_t point) const;

  const Instruction *GetInstruction() const { return inst_; }
  const std::vector<LiveRange> &GetRanges() const { return ranges_; }

  void Dump(std::ostream &os) const;

private:
  const Instruction *inst_;
  std::vector<LiveRange> ranges_;
};

} // namespace analysis
