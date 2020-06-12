/*
 * TransferGroup.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_TRANSFER_GROUP_H
#define CHIMERA_TK_TRANSFER_GROUP_H

#include <set>

#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  /** Group multiple data accessors to efficiently trigger data transfers on the
   * whole group. In case of some backends like the LogicalNameMappingBackend,
   * grouping data accessors can avoid unnecessary transfers of the same data.
   *  This happens in particular, if accessing the data of one accessor requires
   * transfer of a bigger block of data containing also data of another accessor
   * (e.g. channel accessors for multiplexed 2D registers).
   *
   *  Note that read() and write() of the accessors put into the group can no
   * longer be used. Instead, read() and write() of the TransferGroup should be
   * called.
   *
   *  Grouping accessors can only work with accessors that internally buffer the
   * transferred data. Therefore the deprecated RegisterAccessor is not supported,
   * as its read() and write() functions always directly read from/write to the
   * hardware.
   *
   *  Important note: If accessors pointing to the same values are added to the
   * TransferGroup, the behaviour will be when writing. Depending on the backend
   * and on the exact scenario, the accessors might appear like a copy sharing the
   * internal buffers, thus writing to one accessor may (or may not) change the
   * content of the other. Also calling write() has then undefined behaviour,
   * since it is not defined from which accessor the values will be written to the
   * device (maybe both in an undefined order).
   */
  class TransferGroup {
   public:
    TransferGroup() : readOnly(false) {}
    ~TransferGroup() {}

    /** Add a register accessor to the group. The register accessor might
     * internally be altered so that accessors accessing the same hardware
     * register will share their buffers. Register accessors must not be placed
     * into multiple TransferGroups. */
    void addAccessor(TransferElementAbstractor& accessor);
    void addAccessor(const boost::shared_ptr<TransferElement>& accessor);

    /** Trigger read transfer for all accessors in the group */
    void read();

    /** Trigger write transfer for all accessors in the group */
    void write(VersionNumber versionNumber = {});

    /** Check if transfer group is read-only. A transfer group is read-only, if at
     * least one of its transfer elements is read-only. */
    bool isReadOnly();

    /** Print information about the accessors in this group to screen, which might
     * help to understand which transfers were merged and which were not. */
    void dump();

   protected:
    /** List of low-level TransferElements in this group, which are directly
     * responsible for the hardware access, and a flag whether there has been an exception in pre-pread */
    std::map<boost::shared_ptr<TransferElement>, bool /*hasSeenException*/> lowLevelElementsAndExceptionFlags;

    /** List of all CopyRegisterDecorators in the group. On these elements,
     * postRead() has to be executed before all other elements. */
    std::set<boost::shared_ptr<TransferElement>> copyDecorators;

    /** List of high-level TransferElements in this group which are directly used
     * by the user */
    std::set<boost::shared_ptr<TransferElement>> highLevelElements;

    /** Flag if group is read-only */
    bool readOnly;

    // Helper struct to merge several exceptions.
    // All messages are merged into one. Separate book-keeping for boost::thread_interrupted. It is given priority when throwing.
    // All other exceptions are merged into a ChimeraTK::runtime_error.
    struct ExceptionHandlingResult {
      bool hasSeenException;
      std::string message;
      bool hasSeenThreadInterrupted;

      ExceptionHandlingResult(bool hasX = false, std::string m = {}, bool hasThreadIntr = false)
      : hasSeenException(hasX), message(m), hasSeenThreadInterrupted(hasThreadIntr) {}

      inline ExceptionHandlingResult operator+=(ExceptionHandlingResult const& other) {
        hasSeenException |= other.hasSeenException;
        // the if-statements avoid empty lines each time += is called with a result without exceptions
        if(message.empty()) {
          // no line to break, just copy the other message
          message = other.message;
        }
        else {
          if(!other.message.empty()) {
            message += "\n" + other.message;
          } // nothing to add if other message is empty
        }
        hasSeenThreadInterrupted |= other.hasSeenThreadInterrupted;
        return *this;
      }

      // re-throw an exception if there was one.
      inline void reThrow() {
        if(hasSeenThreadInterrupted) throw boost::thread_interrupted();
        if(hasSeenException) throw runtime_error(message);
      }
    };

    // helper function to avoid code duplication. Needs to be run for two lists
    ExceptionHandlingResult runPostReads(std::set<boost::shared_ptr<TransferElement>>& elements);

    // return whether there has been an exception
    template<typename Callable>
    ExceptionHandlingResult handlePostExceptions(Callable function);
  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_TRANSFER_GROUP_H */
