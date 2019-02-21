#include "ExperimentalFeatures.h"

namespace ChimeraTK {
std::atomic<bool> ExperimentalFeatures::isEnabled{false};
ExperimentalFeatures::Reminder ExperimentalFeatures::reminder;
} // namespace ChimeraTK
