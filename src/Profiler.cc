#include "Profiler.h"

namespace ChimeraTK {

std::list<Profiler::ThreadData *> Profiler::threadDataList;

std::mutex Profiler::threadDataList_mutex;

} /* namespace ChimeraTK */
