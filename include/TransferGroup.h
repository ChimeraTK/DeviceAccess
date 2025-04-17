// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElementAbstractor.h"

#include <set>

namespace ChimeraTK {

  /**
   * Group multiple data accessors to efficiently trigger data transfers on the whole group. In case of some backends
   * like the LogicalNameMappingBackend, grouping data accessors can avoid unnecessary transfers of the same data. This
   * happens in particular, if accessing the data of one accessor requires transfer of a bigger block of data containing
   * also data of another accessor (e.g. channel accessors for multiplexed 2D registers).
   *
   * Note that read() and write() of the accessors put into the group can no longer be used. Instead, read() and write()
   * of the TransferGroup should be called.
   *
   * Important note: If accessors pointing to the same values are added to the TransferGroup, the behaviour will be
   * undefined when writing. Depending on the backend and on the exact scenario, the accessors might appear like a copy
   * sharing the internal buffers, thus writing to one accessor may (or may not) change the content of the other. Also
   * calling write() has then undefined behaviour, since it is not defined from which accessor the values will be
   * written to the device (maybe both in an undefined order).
   */
  class TransferGroup {
   public:
    /**
     * Add a register accessor to the group. The register accessor might internally be altered so that accessors
     * accessing the same hardware register will share their buffers. Register accessors must not be placed into
     * multiple TransferGroups.
     */
    void addAccessor(TransferElementAbstractor& accessor);

    /** See the other signature of addAccessor() */
    void addAccessor(const boost::shared_ptr<TransferElement>& accessor);

    /** Trigger read transfer for all accessors in the group */
    void read();

    /** Trigger write transfer for all accessors in the group */
    void write(VersionNumber versionNumber = {});

    /**
     * Check if transfer group is read-only. A transfer group is read-only, if at least one of its transfer elements is
     * read-only.
     */
    [[nodiscard]] bool isReadOnly();

    /**
     * Check if transfer group is readable. A transfer group is readable, if all its transfer elements are readable.
     */
    [[nodiscard]] bool isReadable();

    /**
     * Check if transfer group is writeable. A transfer group is writeable, if all its transfer elements are writeable.
     */
    [[nodiscard]] bool isWriteable();

    /**
     * Print information about the accessors in this group to screen, which might help to understand which transfers
     * were merged and which were not.
     */
    void dump();

   protected:
    /**
     * List of low-level TransferElements in this group, which are directly responsible for the hardware access, and a
     * flag whether there has been an exception in pre-read
     */
    std::map<boost::shared_ptr<TransferElement>, bool /*hasSeenException*/> _lowLevelElementsAndExceptionFlags;

    /**
     * List of all CopyRegisterDecorators in the group. On these elements, postRead() has to be executed before all
     * other elements.
     */
    std::set<boost::shared_ptr<TransferElement>> _copyDecorators;

    /** List of high-level TransferElements in this group which are directly used by the user */
    std::set<boost::shared_ptr<TransferElement>> _highLevelElements;

    /** Cached value whether all elements are readable. */
    bool _isReadable{false};

    /** Cached value whether all elements are writeable. */
    bool _isWriteable{false};

    /** Flag whether to update the cached information */
    bool _cachedReadableWriteableIsValid{false};

    /** Helper function to update the cached state variables */
    void updateIsReadableWriteable();

    // Helper function to avoid code duplication. Needs to be run for two lists.
    void runPostReads(const std::set<boost::shared_ptr<TransferElement>>& elements,
        const std::exception_ptr& firstDetectedRuntimeError);

    // Counter how many runtime errors have been thrown.
    size_t _nRuntimeErrors;

   private:
    void addAccessorImpl(TransferElementAbstractor& accessor, bool isTemporaryAbstractor);
  };

} /* namespace ChimeraTK */
