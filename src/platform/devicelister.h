#pragma once

#include "device.h"

#include <string>
#include <vector>

namespace bmap {

// Enumerate removable/external storage devices that are safe flash targets.
// Returns false and sets *err on failure.
bool listDevices(std::vector<Device>* out, std::string* err);

}  // namespace bmap
