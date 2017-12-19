/*
 * ReadAny.h
 *
 *  Created on: Dec 19, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_READ_ANY_H
#define CHIMERATK_READ_ANY_H

#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  /** Read data asynchronously from all given TransferElements and wait until one of the TransferElements has
    *  new data. The ID of the TransferElement which received new data is returned as a reference. In case multiple
    *  TransferElements receive new data simultaneously (or already have new data available before the call to
    *  readAny()), the ID of the TransferElement with the oldest VersionNumber (see getVersionNumber()) will be returned
    *  by this function. This ensures that data is received in the order of sending (unless data is "dated back"
    *  and sent with an older version number, which might be the case e.g. when using the ControlSystemAdapter).
    *
    *  Note that the behaviour is undefined when putting the same TransferElement into the list - a result might
    *  be e.g. that it blocks for ever. This is due to a limitation in the underlying boost::wait_for_any()
    *  function. */
  mtca4u::TransferElementID readAny(std::list<std::reference_wrapper<mtca4u::TransferElementAbstractor>> elementsToRead);
  mtca4u::TransferElementID readAny(std::list<std::reference_wrapper<mtca4u::TransferElement>> elementsToRead);

/*******************************************************************************************************************/

  namespace detail {

    // Note: the %iterator in the third line prevents doxygen from creating a link which it cannot resolve.
    // It reads: std::list<TransferFuture::PlainFutureType>::iterator
    /** Internal class needed for TransferElement::readAny(): Provide a wrapper around the list iterator to effectivly
    *  allow passing std::list<TransferFuture>::iterator to boost::wait_for_any() which otherwise expects e.g.
    *  std::list<TransferFuture::PlainFutureType>::%iterator. This class provides the same interface as an interator of
    *  the expected type but adds the function getTransferFuture(), so we can obtain the TransferElement from the
    *  return value of boost::wait_for_any(). */
    class TransferFutureIterator {
      public:
        TransferFutureIterator(const std::list<TransferFuture>::iterator &it) : _it(it) {}
        TransferFutureIterator operator++() { TransferFutureIterator ret(_it); ++_it; return ret; }
        TransferFutureIterator operator++(int) { ++_it; return *this; }
        bool operator!=(const TransferFutureIterator &rhs) {return _it != rhs._it;}
        bool operator==(const TransferFutureIterator &rhs) {return _it == rhs._it;}
        TransferFuture::PlainFutureType& operator*() {return _it->getBoostFuture();}
        TransferFuture::PlainFutureType& operator->() {return _it->getBoostFuture();}
        TransferFuture& getTransferFuture() const {return *_it;}
      private:
        std::list<TransferFuture>::iterator _it;
    };

  } // namespace detail

} /* namespace ChimeraTK */

/*******************************************************************************************************************/

namespace std {

  template<>
  struct iterator_traits<ChimeraTK::detail::TransferFutureIterator> {
      typedef ChimeraTK::TransferFuture::PlainFutureType value_type;
      typedef size_t difference_type;
      typedef std::forward_iterator_tag iterator_category;
  };

} /* namespace std */

namespace ChimeraTK {

  /*******************************************************************************************************************/

  namespace detail {

    template<class TransferElementType>
    mtca4u::TransferElementID readAny(
              std::list<std::reference_wrapper<TransferElementType>> elementsToRead) {

      // build list of TransferFutures for all elements. Since readAsync() is a virtual call and we need to visit all
      // elements at least twice (once in wait_for_any and a second time for sorting by version number), this is assumed
      // to be less expensive than calling readAsync() on the fly in the TransferFutureIterator instead.
      std::list<TransferFuture> futureList;
      for(auto &elem : elementsToRead) {
        futureList.push_back(elem.get().readAsync());
      }
      // wait until any future is ready
      auto iter = boost::wait_for_any(detail::TransferFutureIterator(futureList.begin()),
                                      detail::TransferFutureIterator(futureList.end()));

      // Find the variable which has the oldest version number (to guarantee the order of updates).
      // Start with assuming that the future returned by boost::wait_for_any() has the oldes version number.
      TransferFuture theUpdate = iter.getTransferFuture();
      for(auto future : futureList) {
        // skip if this is the future returned by boost::wait_for_any()
        if(future == theUpdate) continue;
        // also skip if the future is not yet ready
        if(!future.hasNewData()) continue;
        // compare  version number with the version number of the stored future
        if(future.getBoostFuture().get()->_versionNumber < theUpdate.getBoostFuture().get()->_versionNumber) {
          theUpdate = future;
        }
      }

      // complete the transfer (i.e. run postRead())
      theUpdate.wait();

      // return the transfer element as a shared pointer
      return theUpdate.getTransferElementID();

    }

  } // namespace detail

  /*******************************************************************************************************************/

  inline mtca4u::TransferElementID readAny(
          std::list<std::reference_wrapper<mtca4u::TransferElement>> elementsToRead) {
    return detail::readAny(elementsToRead);
  }

  /*******************************************************************************************************************/

  inline mtca4u::TransferElementID readAny(
          std::list<std::reference_wrapper<mtca4u::TransferElementAbstractor>> elementsToRead) {
    return detail::readAny(elementsToRead);
  }

} /* namespace ChimeraTK */

#endif // CHIMERATK_READ_ANY_H
