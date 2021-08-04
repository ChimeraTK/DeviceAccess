#pragma once

#include "AsyncNDRegisterAccessor.h"
#include <mutex>

namespace ChimeraTK {

  /** There will be one manager per variable.
 */
  template<typename UserType>
  class AsyncAccessorManager {
   public:
    void subscribe(boost::shared_ptr<AsyncNDRegisterAccessor<UserType, AsyncAccessorManager<UserType>>>& accessor) {
      std::lock_guard<std::mutex> accessorListLock(_accesorListMutex);
      _accessors.push_back(accessor);
    }

    // FIXME: The parameter is not used at all.....
    void unsubscribe(boost::shared_ptr<AsyncNDRegisterAccessor<UserType, AsyncAccessorManager<UserType>>>& accessor) {
      std::lock_guard<std::mutex> accessorListLock(_accesorListMutex);
      for(auto it = _accessors.begin(); it != _accessors.end(); ++it) {
        // This code is called from the destructor of an AsyncNDRegisterAccessor inside a boost::shared_ptr.
        // When this code is called the weak_ptr is already not lockable any more. We just use this to identify
        // which element is to be removed. If we get the wrong one it does not matter because then the other destructor will get it.
        if(it->lock().get() == nullptr) {
          _accessors.erase(it);
          return;
        }
      }
      std::cout << "AsyncAccessorManager: Could not unlist instance!" << std::endl;
      assert(false);
    }

    void sendDestrictively(typename NDRegisterAccessor<UserType>::Buffer& data) {
      std::lock_guard<std::mutex> accessorListLock(_accesorListMutex);

      executeWithCopy([](boost::shared_ptr<AsyncNDRegisterAccessor<UserType, AsyncAccessorManager<UserType>>>& accessor,
                          typename NDRegisterAccessor<UserType>::Buffer& d) { accessor->sendDestructively(d); },
          data);
    }

    void sendException(std::exception_ptr& e) {
      std::lock_guard<std::mutex> accessorListLock(_accesorListMutex);
      for(auto it : _accessors) {
        it->lock()->send(e);
      }
    }

    void activate(typename NDRegisterAccessor<UserType>::Buffer& initialVale) {
      std::lock_guard<std::mutex> accessorListLock(_accesorListMutex);

      executeWithCopy([](boost::shared_ptr<AsyncNDRegisterAccessor<UserType, AsyncAccessorManager<UserType>>>& accessor,
                          typename NDRegisterAccessor<UserType>::Buffer& d) { accessor->activate(d); },
          initialVale);
    }

   private:
    std::mutex _accesorListMutex;
    std::list<boost::weak_ptr<AsyncNDRegisterAccessor<UserType, AsyncAccessorManager<UserType>>>> _accessors;
    typename NDRegisterAccessor<UserType>::Buffer _sendBuffer;

    template<typename Function>
    void executeWithCopy(Function& function, typename NDRegisterAccessor<UserType>::Buffer& data) {
      // FIXME: should this raise an exception?
      if(_accessors.empty()) return;

      // Do a copy for all but the last element
      for(auto it = _accessors.begin(); it != --(_accessors.end()); ++it) {
        // make a copy and send it desctrucively
        _sendBuffer = data;
        function(it->lock(), _sendBuffer);
      }
      // for the last element we can give away the data without copy.
      function(_accessors.back().lock(), data);
    }
  };

} // namespace ChimeraTK
