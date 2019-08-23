#ifndef CHIMERATK_REBOT_PROTOCOL_1
#define CHIMERATK_REBOT_PROTOCOL_1

#include "RebotProtocol0.h"
#include <chrono>

namespace ChimeraTK {

  struct RebotProtocol1 : public RebotProtocol0 {
    explicit RebotProtocol1(boost::shared_ptr<Rebot::Connection>& tcpCommunicator);
    virtual ~RebotProtocol1(){};

    virtual void read(uint32_t addressInBytes, int32_t* data, size_t sizeInBytes) override;
    virtual void write(uint32_t addressInBytes, int32_t const* data, size_t sizeInBytes) override;
    virtual void sendHeartbeat() override;

    /** No need to make it atomic (time_points cannot be because they are not
     * trivially copyable). It is protected by the hardware accessing mutex in the
     * Rebot backend. Make sure you get it every time you read or write this time
     * stamp.
     */
    std::chrono::time_point<std::chrono::steady_clock> _lastSendTime;
  };

} // namespace ChimeraTK

#endif // CHIMERATK_REBOT_PROTOCOL_1
