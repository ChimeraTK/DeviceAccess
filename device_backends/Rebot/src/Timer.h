#pragma once

#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

namespace ChimeraTK {
namespace Rebot {

  class Timer {
  public:
    using Callback = std::function<void()>;
    // https://stackoverflow.com/questions/34929152/class-that-stores-a-stdchronoduration-as-a-member
    using Period = boost::chrono::milliseconds;

    template <typename Rep, typename Period>
    Timer(Callback h, boost::chrono::duration<Rep, Period> i)
        : callback_(std::move(h)),
          i_(boost::chrono::duration_cast<decltype(i_)>(i)) {
      w_ = boost::thread([=]() { this->worker(); });
    }

    Timer();
    ~Timer();

    /**
     * start does not activate the timer if the timer period is 0.
     * */
    void start();

    void cancel();
    bool isActive();

    /*
     * Used to change the timer call back and period.
     *
     * Invoking this function cancels the timer implicitly; the
     * timer has to be restarted explicitly after a call to configure.
     *
     * */
    template <typename Rep, typename Period>
    void configure(const Callback& h, boost::chrono::duration<Rep, Period> i);

  private:
    Callback callback_{nullptr};
    Period i_{0};
    boost::thread w_{};
    boost::mutex m_;
    boost::condition_variable cv_{};
    bool cancelTimer_{ true };

    void worker();
  };

  template <typename R, typename P>
  void Timer::configure(const Callback& h, boost::chrono::duration<R, P> i) {
    Period i_ms = boost::chrono::duration_cast<decltype(i_)>(i);
    cancel();
    boost::unique_lock<boost::mutex> l(m_);
    callback_ = h;
    i_ = i_ms;
  }

} // namespace Rebot
} // namespace ChimeraTK
