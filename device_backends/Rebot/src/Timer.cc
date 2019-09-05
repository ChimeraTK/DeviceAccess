#include "Timer.h"
#include "Exception.h"
#include "testableRebotSleep.h"

namespace ChimeraTK {
namespace Rebot {
  Timer::Timer() {
    w_ = boost::thread([=]() { this->worker(); });
  }

  Timer::~Timer() {
    w_.interrupt();
    w_.join();
  }

  void Timer::start() {
    if (not i_.count()) {
      return;
    }
    boost::lock_guard<boost::mutex> l(m_);
    cancelTimer_ = false;
    cv_.notify_all();
  }

  void Timer::cancel() {
    boost::lock_guard<boost::mutex> l(m_);
    cancelTimer_ = true;
    cv_.notify_all();
  }

  bool Timer::isActive() {
    boost::lock_guard<boost::mutex> l(m_);
    return !cancelTimer_;
  }

  void Timer::worker() {
    // auto baseTime = boost::chrono::system_clock::now();
    auto baseTime = testable_rebot_sleep::now();

    boost::unique_lock<boost::mutex> l(m_);
    while (true) {
      if (cv_.wait_until(l, baseTime + i_,
                         [=]() { return cancelTimer_ == true; })) {
        // timer cancelled.
        cv_.wait(l, [this]() { return cancelTimer_ == false; });
        baseTime = testable_rebot_sleep::now();
      } else {
        // timer not cancelled
        // mutex is locked at this point; hence cancelTimer_cannot change during
        // callback.
        baseTime = testable_rebot_sleep::now();
        if (callback_) {
          callback_();
        }
      }
    }
  }

} // namespace Rebot
} // namespace ChimeraTK
