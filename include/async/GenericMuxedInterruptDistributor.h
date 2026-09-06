// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "../NDRegisterAccessor.h"
#include "../RegisterPath.h"
#include "MuxedInterruptDistributor.h"

#include <atomic>
#include <bitset>
#include <chrono>
#include <map>
#include <memory>
#include <thread>

namespace ChimeraTK::async {

  enum GmidOptionCode {
    ISR = 0, // Interrupt Status Register
    IER,     // Interrupt Enable Register
    MER,     // Master Enable Register,
    MIE,     // Master Interrupt Enable  Functionally equivalent to MER
    GIE,     // Global Interrupt Enable, Functionally equivalent to MER
    ICR,     // Interrupt Clear Register
    IAR,     // Interrupt Acknowledge Register
    IPR,     // Interrupt Pending Register =ISR & IER, a convenience feature for software.
    SIE,     // Set Interrupt Enable
    CIE,     // Clear Interrupt Enable
    IMaskR,  // Interrupt Mask Register. IMR Acronym Collision. Temporarily not allowed. See
    IModeR,  // Interrupt Mode Register. IMR Acronym Collision, defined in the standard but not allowed; REMOVE?
    IVR,     // defined in the standard but not allowed; REMOVE?
    ILR,     // defined in the standard but not allowed; REMOVE?
    IVAR,    // defined in the standard but not allowed; REMOVE?
    IVEAR,   // defined in the standard but not allowed; REMOVE?
    OPTION_CODE_COUNT, // used for counting how many valid enums there are here & setting list lengths
    INVALID_OPTION_CODE,
  };

  /********************************************************************************************************************/

  class GenericMuxedInterruptDistributor : public MuxedInterruptDistributor {
   public:
    explicit GenericMuxedInterruptDistributor(const boost::shared_ptr<SubDomain<std::nullptr_t>>& parent,
        const std::string& registerPath, std::bitset<(ulong)GmidOptionCode::OPTION_CODE_COUNT> optionRegisterSettings);
    ~GenericMuxedInterruptDistributor() override;

    /**
     * Handle gets called when a trigger comes in. It implements the handshake with the interrupt controller
     */
    void handle(VersionNumber version) override;

    void activate(VersionNumber version) override;

    /**
     *  Create parses the json configuration snippet 'description', and calls the constructor.
     *  It returns an initalized GenericMuxedInterruptDistributor.
     *  'descriptor' is a JSON snippet containing configuration data. Elsewhere it's also called descriptionJsonStr
     */
    static std::unique_ptr<GenericMuxedInterruptDistributor> create(std::string const& description, // THE JSON SNIPPET
        const boost::shared_ptr<SubDomain<std::nullptr_t>>& parent);

   protected:
    bool _ierIsReallyImaskr;
    bool _haveSieAndCie;
    bool _hasMer;
    uint32_t _activeInterrupts{0}; // like a local copy of IER

    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _isr;
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _ier; // May point to IER or Imaskr
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _icr; // May point to ICR, IAR, or ISR, which act identically
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _mer; // May point to MER, MIE, or GIE, all act identically.
                                                          // We can have at most 1 of {MIE, GIE, MER}
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _sie; // We either have both SIE or CIE or neither.
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _cie;

    RegisterPath _path; // just a string path, with the added overwritten / operator etc.

    /** Counter incremented by handle() after each completed run. Used by the watchdog.*/
    std::atomic<uint64_t> _irqRunCount{0};

    /** Counter of distinct watchdog detections (i.e. how many times the interrupt handler was found
     *  wedged / starved and a device exception was raised). */
    std::atomic<uint64_t> _watchdogExceptionCount{0};

    /** Set true once the watchdog has raised a device exception*/
    std::atomic<bool> _watchdogAlerted{false};

    /** Interrupt bits that were pending/enabled in the ISR in the most recent handle() run but were not
     *  distributed to any sub-domain. */
    std::atomic<uint32_t> _undistributedInterrupts{0};

    /** Read-only snapshot mapping each interrupt bit index for diaganostics */
    std::atomic<std::shared_ptr<const std::map<size_t, std::string>>> _bitToSubDomainId;

    /** Watchdog thread and its stop flag. */
    std::thread _watchdogThread;
    std::atomic<bool> _stopWatchdog{false};
    /** Watchdog polling/decision interval (experimental) */
    static constexpr std::chrono::milliseconds _watchdogInterval{1000};

    /** watchdog thread. */
    void watchdogLoop();

    /** Format a bit mask into "bit N (treeID [..])" entries using the read-only snapshot map. */
    std::string formatInterruptMask(uint32_t mask);

    /**
     * In mask, 1 bits clear the corresponding registers, 0 bits do nothing.
     * So if bit i of mask is 1, then register i gets cleared
     * Multiple bits can be 1 at a time.
     */
    void clearInterruptsFromMask(uint32_t mask);

    inline void clearOneInterrupt(uint32_t ithInterrupt);

    inline void clearAllInterrupts();

    inline void clearAllEnabledInterrupts();

    /**
     * Disables each interrupt corresponding to the 1 bits in mask, and updates _activeInterrupts.
     * Depending on the options, may perform the disable using CIE, IER, or IMaskR.
     */
    void disableInterruptsFromMask(uint32_t mask);

    inline void disableOneInterrupt(uint32_t ithInterrupt);

    /**
     * For each bit in mask that is a 1, the corresponding interrupt gets enabled,
     * and the internal copy of enabled registers is updated.
     */
    void enableInterruptsFromMask(uint32_t mask);

    inline void enableOneInterrupt(uint32_t ithInterrupt);

    void activateSubDomain(SubDomain<std::nullptr_t>& subDomain, VersionNumber const& version) override;
  };

} // namespace ChimeraTK::async
