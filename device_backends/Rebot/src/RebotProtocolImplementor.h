#ifndef CHIMERATK_REBOT_PROTOCOL_IMPLEMENTOR
#define CHIMERATK_REBOT_PROTOCOL_IMPLEMENTOR

#include <cstdint>
#include <cstddef>

namespace ChimeraTK{
  
  /** This is the base class of the code which implements the ReboT protocol.
   *  Starting from version 0, the implementors can derrive from each other to
   *  reuse code from previous versions, or replace the implementation.
   */
  struct RebotProtocolImplementor{
    virtual void read(uint32_t addressInBytes, int32_t* data,
                      size_t sizeInBytes) = 0;
    virtual void write(uint32_t addressInBytes, int32_t const* data,
                       size_t sizeInBytes) = 0;
    virtual void sendHeartbeat() = 0;
    virtual ~RebotProtocolImplementor(){};
  };
  
}// namespace ChimeraTK

#endif // CHIMERATK_REBOT_PROTOCOL_IMPLEMENTOR
