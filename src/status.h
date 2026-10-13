#pragma once

#include "session.h"

#include <string>

namespace typist {

// Text shown below the progress bar during a finite exercise.
std::string FormatStatus(const Snapshot& snapshot);

}  // namespace typist
