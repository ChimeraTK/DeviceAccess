/*
 * Profiler.h
 *
 *  Created on: May 15, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_PROFILER_H
#define CHIMERATK_PROFILER_H

#include <string>
#include <atomic>
#include <chrono>
#include <list>
#include <mutex>
#include <assert.h>

namespace ChimeraTK {

  class Profiler {

    public:

      class ThreadData {

        public:

          /** Return the name of the thread */
          const std::string& getName() const { return name; }

          /** Return the integrated active time of the thread in microseconds. */
          uint64_t getIntegratedTime() const { return integratedTime; }

          /** Return the integrated active time of the thread in microseconds and atomically reset the counter to 0. */
          uint64_t getAndResetIntegratedTime() {
            uint64_t time = integratedTime;
            integratedTime.fetch_sub(time);
            return time;
          }

        private:

          friend class Profiler;

          /** Copy of Application::threadName(), stored here to make it accessible outside the thread */
          std::string name;

          /** Reference point for the time measurement */
          std::chrono::high_resolution_clock::time_point lastActiated;

          /** Flag whether this thread is currently active */
          bool isActive{false};

          /** Integrated time this thread was active in microseconds */
          std::atomic<uint64_t> integratedTime;

      };

      /** Register a thread in the profiler. This function must be called in each thread before calling
       *  startMeasurement() and stopMeasurement() in the same thread. The function must not be called twice in the
       *  same thread. The call to this function implicitly triggers starting the time measurement
       *  (see startMeasurement()) */
      static void registerThread(const std::string &name) {
        getThreadData().name = name;
        std::lock_guard<std::mutex> lock(threadDataList_mutex);
        threadDataList.emplace_back(&getThreadData());
        startMeasurement();
      }

      /** Obtain a list of ThreadData references for all threads registered with the profiler. */
      static const std::list<ThreadData*>& getDataList() {
        return threadDataList;
      }

      /** Start the time measurement for the current thread. Call this immediately after the thread woke up e.g. from
       *  blocking read. */
      static void startMeasurement() {
        if(getThreadData().isActive) return;
        getThreadData().isActive = true;
        getThreadData().lastActiated = std::chrono::high_resolution_clock::now();
      }

      /** Stop the time measurement for the current thread. Call this right before putting the thread to sleep e.g.
       *  before a blocking read. */
      static void stopMeasurement() {
        if(!getThreadData().isActive) return;
        getThreadData().isActive = false;
        auto duration = std::chrono::high_resolution_clock::now() - getThreadData().lastActiated;
        getThreadData().integratedTime += std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
      }

    private:

      /** Return the ThreadData object associated with the current thread. */
      static ThreadData& getThreadData() {
        thread_local static ThreadData data;
        return data;
      }

      /** List of ThreadData references registered with the profiler.  */
      static std::list<ThreadData*> threadDataList;

      /** Mutex for write access to the threadDataList member. Access to existing list entries through the public
       *  member functions of ThreadData is allowed without holding this mutex. */
      static std::mutex threadDataList_mutex;

  };

}

#endif /* CHIMERATK_PROFILER_H */
