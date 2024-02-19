// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Device.h"
#include "DeviceBackendImpl.h"

#include <boost/fusion/include/at_key.hpp>
#include <boost/test/unit_test.hpp>

#include <functional>
#include <list>
#include <numeric>
#include <string>
#include <thread>
#include <utility>

// disable shadow warning, boost::mpl::for_each is triggering this warning on Ubuntu 16.04
#pragma GCC diagnostic ignored "-Wshadow"

namespace ChimeraTK {

  /**
   * Used by the Capabilities descriptor
   */
  enum class TestCapability {
    unspecified, ///< Capability is not specified, hence it is disabled and a warning is printed. Usually default.
    enabled,     ///< Enable tests requiring this capability
    disabled     ///< Disable tests requiring this capability and do not warn.
  };

  /**
   *  Descriptor for the test capabilities for each register. This allows a schema evolution of the test. New tests
   *  which require a new backend-specific function in the test in the register descriptor will be enabled through
   *  a corresponding Capability flag.
   *
   *  Construct object like this:
   *
   *  static constexpr auto capabilities =
   * TestCapabilities<>().enableForceDataLossWrite().disableAsyncReadInconsistency();
   *
   *  Use any number of enable/disable functions.
   */
  template<TestCapability _syncRead = TestCapability::enabled,
      TestCapability _forceDataLossWrite = TestCapability::unspecified,
      TestCapability _asyncReadInconsistency = TestCapability::unspecified,
      TestCapability _switchReadOnly = TestCapability::unspecified,
      TestCapability _switchWriteOnly = TestCapability::unspecified,
      TestCapability _writeNeverLosesData = TestCapability::unspecified,
      TestCapability _testWriteOnly = TestCapability::disabled, TestCapability _testReadOnly = TestCapability::disabled,
      TestCapability _testRawTransfer = TestCapability::unspecified,
      TestCapability _testCatalogue = TestCapability::enabled,
      TestCapability _setRemoteValueIncrementsVersion = TestCapability::enabled>
  struct TestCapabilities {
    constexpr TestCapabilities() = default;

    /// Allows to prevent the test from executing any synchronous read tests.
    /// This should be used only when testing TransferElements which do not support reads without
    /// AccessMode::wait_for_new_data, like e.g. the ControlSystemAdapter BidirectionalProcessArray. TransferElements
    /// handed out by real backends must always support this, to the syncReadTests capability should be enable for all
    /// backend tests.
    constexpr TestCapabilities<TestCapability::disabled, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        disableSyncRead() const {
      return {};
    }

    /// See setForceDataLossWrite() function in the register descriptor.
    constexpr TestCapabilities<_syncRead, TestCapability::enabled, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, TestCapability::disabled, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        enableForceDataLossWrite() const {
      static_assert(_writeNeverLosesData != TestCapability::enabled,
          "enableTestWriteNeverLosesData() and enableForceDataLossWrite() are mutually exclusive.");
      return {};
    }
    constexpr TestCapabilities<_syncRead, TestCapability::disabled, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        disableForceDataLossWrite() const {
      return {};
    }

    /// See forceAsyncReadInconsistency() function in the register descriptor.
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, TestCapability::enabled, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        enableAsyncReadInconsistency() const {
      return {};
    }
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, TestCapability::disabled, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        disableAsyncReadInconsistency() const {
      return {};
    }

    /// See switchReadOnly() function in the register descriptor.
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, TestCapability::enabled,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        enableSwitchReadOnly() const {
      return {};
    }
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, TestCapability::disabled,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        disableSwitchReadOnly() const {
      return {};
    }

    /// See switchWriteOnly() function in the register descriptor.
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        TestCapability::enabled, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        enableSwitchWriteOnly() const {
      return {};
    }
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        TestCapability::disabled, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        disableSwitchWriteOnly() const {
      return {};
    }

    /// Enable/disable test whether write transfers never report lost data (part of B.7.2). A series of writeQueueLength
    /// writes is performed and no data loss must be reported.
    /// Mutually exclusive with enableForceDataLossWrite().
    constexpr TestCapabilities<_syncRead, TestCapability::disabled, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, TestCapability::enabled, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        enableTestWriteNeverLosesData() const {
      static_assert(_forceDataLossWrite != TestCapability::enabled,
          "enableTestWriteNeverLosesData() and enableForceDataLossWrite() are mutualy exclusive.");
      return {};
    }
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, TestCapability::disabled, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        disableTestWriteNeverLosesData() const {
      return {};
    }

    /// Enable/disable testing only write operations, even if the register is readable
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, TestCapability::enabled, _testReadOnly, _testRawTransfer,
        _testCatalogue, _setRemoteValueIncrementsVersion>
        enableTestWriteOnly() const {
      return {};
    }
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, TestCapability::disabled, _testReadOnly, _testRawTransfer,
        _testCatalogue, _setRemoteValueIncrementsVersion>
        disableTestWriteOnly() const {
      return {};
    }

    /// Enable/disable testing only read operations, even if the register is readable
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, TestCapability::enabled, _testRawTransfer,
        _testCatalogue, _setRemoteValueIncrementsVersion>
        enableTestReadOnly() const {
      return {};
    }
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, TestCapability::disabled, _testRawTransfer,
        _testCatalogue, _setRemoteValueIncrementsVersion>
        disableTestReadOnly() const {
      return {};
    }

    /// Enable/disable testing the raw accessors
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, TestCapability::disabled, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        disableTestRawTransfer() const {
      return {};
    }
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, TestCapability::enabled, _testCatalogue,
        _setRemoteValueIncrementsVersion>
        enableTestRawTransfer() const {
      return {};
    }

    /// Enable/disable testing of catalogue content
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer,
        TestCapability::disabled, _setRemoteValueIncrementsVersion>
        disableTestCatalogue() const {
      return {};
    }
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer,
        TestCapability::enabled, _setRemoteValueIncrementsVersion>
        enableTestCatalogue() const {
      return {};
    }

    /// Enable/disable testing of version number increment in read operations after setRemoteValue
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        TestCapability::disabled>
        disableSetRemoteValueIncrementsVersion() const {
      return {};
    }
    constexpr TestCapabilities<_syncRead, _forceDataLossWrite, _asyncReadInconsistency, _switchReadOnly,
        _switchWriteOnly, _writeNeverLosesData, _testWriteOnly, _testReadOnly, _testRawTransfer, _testCatalogue,
        TestCapability::enabled>
        enableSetRemoteValueIncrementsVersion() const {
      return {};
    }

    static constexpr TestCapability syncRead{_syncRead};
    static constexpr TestCapability forceDataLossWrite{_forceDataLossWrite};
    static constexpr TestCapability asyncReadInconsistency{_asyncReadInconsistency};
    static constexpr TestCapability switchReadOnly{_switchReadOnly};
    static constexpr TestCapability switchWriteOnly{_switchWriteOnly};
    static constexpr TestCapability writeNeverLosesData{_writeNeverLosesData};
    static constexpr TestCapability testWriteOnly{_testWriteOnly};
    static constexpr TestCapability testReadOnly{_testReadOnly};
    static constexpr TestCapability testRawTransfer{_testRawTransfer};
    static constexpr TestCapability testCatalogue{_testCatalogue};
    static constexpr TestCapability setRemoteValueIncrementsVersion{_setRemoteValueIncrementsVersion};
  };

  /**
   *  Class to test any backend for correct behaviour. Instantiate this class and call all (!) preparatory functions to
   *  provide the tests with the backend-specific test actions etc. Finally call runTests() to execute all tests.
   *  Internally the BOOST unit test framework is used, so this shall be called inside a normal unit test.
   *
   *  Failing to call all preparatory functions will result in an error. This allows a safe test schema evolution - if
   *  more backend specific actions for enabling and disabling test conditions are needed for the tests and the backend
   *  test has not yet been updated, tests will fail.
   *
   *  Actions are usually specified as list of pairs of functors. The pair's first element is always the action to
   * enable the the test condition, the second is the action to disable it. By providing multiple entries in the lists
   * it is possible to test several code paths the backend has to end up in the intended test condition. For example in
   * case of forceRuntimeErrorOnRead(): runtime_errors in a read can be caused by a timeout in the communication
   * channel, or by a bad reply of the device. Two list entries would be provided in this case, one to make read
   * operations run into timeouts, and one to make the (dummy) device reply with garbage. If only one singe code path
   * exists to get to the test condition, it is perfectly fine to have only a single entry in the list.
   *
   *  In the same way as for the actions, names of registers etc. are provided as lists, so all test can be repeated for
   *  different registers, if required for full coverage.
   *
   *  Instantiate with default template argument, then call addRegister() to add any number of registers, i.e.:
   *
   *    auto ubt = UnifiedBackendTest<>.addRegister<RegisterA>().addRegister<RegisterB>().addRegister<RegisterC>()
   *    ubt.runTest("myCDD");
   *
   *  See addRegister() for more details.
   *
   *  Note: This is work in progress. Tests are by far not yet complete. Interface changes of the test class are also
   *  likely.
   */
  template<typename VECTOR_OF_REGISTERS_T = boost::mpl::vector<>>
  class UnifiedBackendTest {
   public:
    /**
     *  Add a register to be used by the test. This function takes a register descriptor in form of a struct type as
     *  template argument and returns a new UnifiedBackendTest object.
     *
     *  The register descriptor must be of the following form:
     *
     *  struct MyRegisterDescriptor {
     *    std::string path() {return "/path/of/register";}
     *    bool isWriteable() {return true;}
     *    bool isReadable() {return true;}
     *    ChimeraTK::AccessModeFlags supportedFlags() {return {ChimeraTK::AccessMode::wait_for_new_data};}
     *    size_t nChannels() {return 1;}
     *    size_t nElementsPerChannel() {return 5;}
     *    size_t nRuntimeErrorCases() {return 1;}                                 // see setForceRuntimeError()
     *
     *    /// Number of values to test in read and write tests. Put in how many calls to generateValue() are necessary
     *    /// to cover all corner cases to be tested. Should be at least 2 even if no corner cases are to be tested.
     *    /// Optional, defaults to 2.
     *    size_t nValuesToTest() {return 2;}
     *
     *    typedef int32_t minimumUserType;
     *    typedef minimumUserType rawUserType;  // only used if AccessMode::raw is supprted, can be omitted otherwise
     *
     *    /// Generate value which can be represented by the register. Make sure it's different from the
     *    /// previous one and that it's not all zero to ensure that the test is sensitive.
     *    /// Template argument 'Type 'can be UserType or RawType if raw is supported. If the 'raw' flag  is
     *    /// false, data is converted to the UserType (e.g. using ChimeraTK::numericToUserType), otherwise
     *    /// unconverted (raw) data is returned.
     *    /// In case the accessor does not support AccessMode::raw, the 'raw' argument can be omitted.
     *    template<typename Type>
     *    std::vector<std::vector<Type>> generateValue(bool raw = false);
     *
     *    /// Obtain the current value of the register. The value can be raw or converted to
     *    /// UserType (see generateValue()).  In case the accessor does not support AccessMode::raw, the 'raw' argument
     * can be omitted. template<typename Type> std::vector<std::vector<Type>> getRemoteValue(bool raw = false);
     *
     *    /// Set remote value to a value generated in the same way as in generateValue().
     *    void setRemoteValue();
     *
     *    /// Force runtime errors when reading or writing (at least) this register. Whether other registers are also
     *    /// affected by this is not important for the test (i.e. blocking the entire communication is ok).
     *    /// It is guaranteed that this function is always called in pairs with first enable = true and then
     *    /// enable = false, without calling setForceRuntimeError() for any other register in between.
     *    /// The second case argument will be set to a number between 0 and nRuntimeErrorCases()-1. Each case will be
     *    /// enabled and disabled separately. It is guaranteed that never two cases are enabled at the same time. If
     *    /// nRuntimeErrorCases() == 0, this function will never be called.
     *    void setForceRuntimeError(bool enable, size_t case);
     *
     *    /// Describe which test capabilities are provided by the register descriptor. Enable capabilities for which
     *    /// the corresponding functions are provided by this register descriptor. Disable capabilities which are
     *    /// intentionally not provided to avoid a warning.
     *    static constexpr auto capabilities = TestCapabilities<>().enableForceDataLossWrite();
     *
     *    /// Used by setForceDataLossWrite(). Can be omitted if Capability forceDataLossWrite = disabled.
     *    size_t writeQueueLength() {return std::numeric_limits<size_t>::max();}
     *
     *    /// Force data loss during write operations. It is expected that data loss occurs exactly writeQueueLength
     *    /// write operations after calling this function with enable=true.
     *    /// Can be omitted if Capability forceDataLossWrite = disabled.
     *    /// It is guaranteed that this function is always called in pairs with first enable = true and then
     *    /// enable = false.
     *    void setForceDataLossWrite(bool enable);
     *
     *    /// Do whatever necessary that data last received via a push-type subscription is inconsistent with the actual
     *    /// value (as read by a synchronous read). This can e.g. be achieved by changing the value without publishing
     *    /// the update to the subscribers.
     *    /// Can be omitted if Capability asyncReadInconsistency = disabled. This should be done only if such
     *    /// inconsistencies are already prevented by the protocol.
     *    void forceAsyncReadInconsistency();
     *
     *    /// Do whatever necessary that the register changes into a read-only register.
     *    /// Can be omitted if TestCapability switchReadOnly = disabled.
     *    /// It is guaranteed that this function is always called in pairs with first enable = true and then
     *    /// enable = false.
     *    void switchReadOnly(bool enable);
     *
     *    /// Do whatever necessary that the register changes into a write-only register.
     *    /// Can be omitted if TestCapability switchWriteOnly = disabled.
     *    /// It is guaranteed that this function is always called in pairs with first enable = true and then
     *    /// enable = false.
     *    void switchWriteOnly(bool enable);
     *  };
     *
     *  Note: Instances of the register descriptors are created and discarded arbitrarily. If it is necessary to store
     *  any data (e.g. seeds for generating values), use static member variables.
     *
     *  Properties of the register are implemented as functions instead of data members to make it easier to override
     *  values when using a common base class for multiple descriptors to avoid code duplication (without triggering a
     *  shadowing warning).
     */
    template<typename REG_T>
    UnifiedBackendTest<typename boost::mpl::push_back<VECTOR_OF_REGISTERS_T, REG_T>::type> addRegister() {
      UnifiedBackendTest<typename boost::mpl::push_back<VECTOR_OF_REGISTERS_T, REG_T>::type> x;
      if(_testOnlyTransferElement) x.testOnlyTransferElement();
      return x;
    }

    /**
     *  "Strong typedef" for list of pairs of functors for enabling and disabling a test condition.
     */
    class EnableDisableActionList : public std::list<std::pair<std::function<void(void)>, std::function<void(void)>>> {
     public:
      using std::list<std::pair<std::function<void(void)>, std::function<void(void)>>>::list;
    };

    /**
     *  Execute all tests. Call this function within a BOOST_AUTO_TEST_CASE after calling all preparatory functions
     *  below. The tests are executed for the backend identified by the given CDD.
     *
     *  A second CDD should be specified, if it is possible to reach the same registers through a different backend.
     */
    void runTests(const std::string& cdd_, const std::string& cdd2_ = "");

    /**
     *  Call if not a real backend is tested but just a special TransferElement implementation. This will disable
     *  those tests which do not make sense in that context. Use this for testing the ControlSystemAdapter's
     *  ProcessArray, or the NDRegisterAccessorDecorator base class.
     */
    UnifiedBackendTest<VECTOR_OF_REGISTERS_T>& testOnlyTransferElement() {
      _testOnlyTransferElement = true;
      return *this;
    }

   protected:
    void test_B_3_1_2_1();
    void test_NOSPEC_write();
    void test_B_3_2_1_2();
    void test_B_3_2_2();
    void test_B_4_2_4();
    void test_B_6_4();
    void test_B_7_2();
    void test_B_8_2();
    void test_B_8_2_1();
    void test_B_8_3();
    void test_B_8_4();
    void test_B_8_5();
    void test_B_8_5_1();
    void test_B_8_5_2();
    void test_B_8_5_3();
    void test_B_8_6_6();
    void test_B_9_1();
    void test_B_9_2_2();
    void test_B_9_3_1();
    void test_B_9_3_2();
    void test_B_9_4_1();
    void test_B_9_5();
    void test_NOSPEC_newVersionAfterOpen();
    void test_B_11_2_1();
    void test_B_11_2_2();
    void test_B_11_6();
    void test_C_5_2_1_2();
    void test_C_5_2_2_2();
    void test_C_5_2_3_2();
    void test_C_5_2_5_2();
    void test_C_5_2_6_2();
    void test_C_5_2_7_2();
    void test_C_5_3();
    void test_C_5_3_2();
    void test_C_5_3_3();
    void test_NOSPEC_valueAfterConstruction();
    void test_NOSPEC_backendNotClosedAfterException();
    void test_NOSPEC_rawTransfer();
    void test_NOSPEC_catalogueRaw();
    void test_NOSPEC_catalogueReadWrite();

    /// Utility functions for recurring tasks
    void recoverDevice(ChimeraTK::Device& d);

    /// Utility functions for register traits
    template<typename REG_T>
    bool isRead(REG_T x = {}) {
      if(x.capabilities.syncRead == TestCapability::disabled) return false;
      if(x.capabilities.testWriteOnly == TestCapability::enabled) return false;
      return x.isReadable();
    }
    template<typename REG_T>
    bool isWrite(REG_T x = {}) {
      if(x.capabilities.testReadOnly == TestCapability::enabled) return false;
      return x.isWriteable();
    }
    template<typename REG_T>
    bool isSyncRead(REG_T x = {}) {
      if(x.capabilities.testWriteOnly == TestCapability::enabled) return false;
      return x.isReadable() && !x.supportedFlags().has(ChimeraTK::AccessMode::wait_for_new_data);
    }
    template<typename REG_T>
    bool isAsyncRead(REG_T x = {}) {
      if(x.capabilities.testWriteOnly == TestCapability::enabled) return false;
      return x.isReadable() && x.supportedFlags().has(ChimeraTK::AccessMode::wait_for_new_data);
    }
    template<typename REG_T>
    bool isRaw(REG_T x = {}) {
      return x.supportedFlags().has(ChimeraTK::AccessMode::raw);
    }
    template<typename REG_T>
    bool isReadOnly(REG_T x = {}) {
      return !x.isWriteable() && x.isReadable();
    }
    template<typename REG_T>
    bool isWriteOnly(REG_T x = {}) {
      return x.isWriteable() && !x.isReadable();
    }

    /// boost::mpl::vector with all register descriptors
    VECTOR_OF_REGISTERS_T registers;

    /// CDD for backend to test
    std::string cdd, cdd2;

    /// Flag whether to disable tests for the backend itself
    bool _testOnlyTransferElement{false};

    /// Special DeviceBackend used for testing the exception reporting to the backend
    struct ExceptionReportingBackend : DeviceBackendImpl {
      explicit ExceptionReportingBackend(boost::shared_ptr<DeviceBackend> target) : _target(std::move(target)) {}
      ~ExceptionReportingBackend() override = default;

      void setExceptionImpl() noexcept override {
        _hasSeenException = true;
        _target->setException(getActiveExceptionMessage());
        // do not keep the ExceptionReportingBackend in exception state, otherwise further exceptions do not cause this
        // function to be executed.
        setOpenedAndClearException();
      }

      /// Check whether setException() has been called since the last call to hasSeenException().
      bool hasSeenException() {
        bool ret = _hasSeenException;
        _hasSeenException = false;
        return ret;
      }

      void open() override {}
      void close() override {}
      std::string readDeviceInfo() override { return ""; }

      RegisterCatalogue getRegisterCatalogue() const override { throw; }

     private:
      boost::shared_ptr<DeviceBackend> _target;
      bool _hasSeenException{false};
    };

    // Proxy for calling setForceDataLossWrite() only if allowed by capabilities.
    template<typename T, bool condition = (T::capabilities.forceDataLossWrite == TestCapability::enabled)>
    struct setForceDataLossWrite_proxy_helper {
      setForceDataLossWrite_proxy_helper(T t, bool enable) { t.setForceDataLossWrite(enable); }
    };

    template<typename T>
    struct setForceDataLossWrite_proxy_helper<T, false> {
      setForceDataLossWrite_proxy_helper(T, bool) {
        std::cout << "Unexpected use of disabled capability." << std::endl;
        std::terminate();
      }
    };

    template<typename T>
    void setForceDataLossWrite(T t, bool enable) {
      setForceDataLossWrite_proxy_helper<T>{t, enable};
    }

    // Proxy for calling forceAsyncReadInconsistency() only if allowed by capabilities.
    template<typename T, bool condition = (T::capabilities.asyncReadInconsistency == TestCapability::enabled)>
    struct forceAsyncReadInconsistency_proxy_helper {
      explicit forceAsyncReadInconsistency_proxy_helper(T t) { t.forceAsyncReadInconsistency(); }
    };

    template<typename T>
    struct forceAsyncReadInconsistency_proxy_helper<T, false> {
      explicit forceAsyncReadInconsistency_proxy_helper(T) {
        std::cout << "Unexpected use of disabled capability." << std::endl;
        std::terminate();
      }
    };

    template<typename T>
    void forceAsyncReadInconsistency(T t) {
      forceAsyncReadInconsistency_proxy_helper<T>{t};
    }

    // Proxy for calling switchReadOnly() only if allowed by capabilities.
    template<typename T, bool condition = (T::capabilities.switchReadOnly == TestCapability::enabled)>
    struct switchReadOnly_proxy_helper {
      switchReadOnly_proxy_helper(T t, bool enable) { t.switchReadOnly(enable); }
    };

    template<typename T>
    struct switchReadOnly_proxy_helper<T, false> {
      switchReadOnly_proxy_helper(T, bool) {
        std::cout << "Unexpected use of disabled capability." << std::endl;
        std::terminate();
      }
    };

    template<typename T>
    void switchReadOnly(T t, bool enable) {
      switchReadOnly_proxy_helper<T>{t, enable};
    }

    // Proxy for calling switchWriteOnly() only if allowed by capabilities.
    template<typename T, bool condition = (T::capabilities.switchWriteOnly == TestCapability::enabled)>
    struct switchWriteOnly_proxy_helper {
      switchWriteOnly_proxy_helper(T t, bool enable) { t.switchWriteOnly(enable); }
    };

    template<typename T>
    struct switchWriteOnly_proxy_helper<T, false> {
      switchWriteOnly_proxy_helper(T, bool) {
        std::cout << "Unexpected use of disabled capability." << std::endl;
        std::terminate();
      }
    };

    template<typename T>
    void switchWriteOnly(T t, bool enable) {
      switchWriteOnly_proxy_helper<T>{t, enable};
    }

    // Proxy for getting the writeQueueLength only if allowed by capabilities.
    template<typename T,
        bool condition = (T::capabilities.forceDataLossWrite == TestCapability::enabled ||
            T::capabilities.writeNeverLosesData == TestCapability::enabled)>
    struct writeQueueLength_proxy_helper {
      explicit writeQueueLength_proxy_helper(T t) { result = t.writeQueueLength(); }
      size_t result;
    };

    template<typename T>
    struct writeQueueLength_proxy_helper<T, false> {
      explicit writeQueueLength_proxy_helper(T) {
        std::cout << "Unexpected use of disabled capability." << std::endl;
        std::terminate();
      }
      size_t result{0};
    };

    template<typename T>
    size_t writeQueueLength(T t) {
      return writeQueueLength_proxy_helper<T>{t}.result;
    }

    // Proxy for getting nValuesToTest() if defined, and otherwise use the default
    template<typename T>
    class has_nValuesToTest {
      using one = char;
      struct two {
        char x[2];
      };

      template<typename C>
      static one test(decltype(&C::nValuesToTest));
      template<typename C>
      static two test(...);

     public:
      enum { value = sizeof(test<T>(0)) == sizeof(char) };
    };

    template<typename T, bool hasFn = has_nValuesToTest<T>::value>
    struct nValuesToTest_proxy_helper {
      explicit nValuesToTest_proxy_helper(T t) { result = t.nValuesToTest(); }
      size_t result;
    };
    template<typename T>
    struct nValuesToTest_proxy_helper<T, false> {
      explicit nValuesToTest_proxy_helper(T) { result = 2; }
      size_t result;
    };

    template<typename T>
    size_t nValuesToTest(T t) {
      return nValuesToTest_proxy_helper<T>{t}.result;
    }
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  /*
   * Implementations below this point.
   *
   * This is made header-only to avoid the need to link against BOOST unit test libraries for the DeviceAccess
   * runtime library, or to introduce another library shipped with DeviceAccess just for this class.
   */
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  // Helper template function to compare values appropriately for the type
  template<typename UserType>
  bool compareHelper(UserType a, UserType b) {
    return a == b;
  }

  template<>
  inline bool compareHelper<double>(double a, double b) {
    return std::abs(a - b) <= std::numeric_limits<double>::epsilon() * 10. * std::max(std::abs(a), std::abs(b));
  }

  template<>
  inline bool compareHelper<float>(float a, float b) {
    return std::abs(a - b) <= std::numeric_limits<float>::epsilon() * 10.F * std::max(std::abs(a), std::abs(b));
  }

  // Turn off the linter warning. It shows for strings only and we can't change the template signature here.
  template<>
  inline bool compareHelper<std::string>(std::string a, std::string b) { // NOLINT(performance-unnecessary-value-param)
    return a == b;
  }
} // namespace ChimeraTK

namespace std {
  inline std::string to_string(const std::string& v) {
    return v;
  }
  inline std::string to_string(const char*& v) {
    return {v};
  }
} // namespace std

namespace ChimeraTK {

  /********************************************************************************************************************/

  // Helper macro to compare the value on an accessor and the expected 2D value
  // Note: we use a macro and not a function, so BOOST_ERROR prints us the line number of the actual test!

  // As the function only works with the correct objects and it is unlikely that expressions are used as input
  // parameters, we  turn off the linter warning about parentheses around the macro arguments. The parentheses would
  // make the code harder to read.

  // NOLINTBEGIN(bugprone-macro-parentheses)
#define CHECK_EQUALITY(accessor, expectedValue)                                                                        \
  {                                                                                                                    \
    typedef typename decltype(expectedValue)::value_type::value_type CHECK_EQUALITY_UserType;                          \
    std::string fail;                                                                                                  \
    BOOST_CHECK_EQUAL(accessor.getNChannels(), expectedValue.size());                                                  \
    BOOST_CHECK_EQUAL(accessor.getNElementsPerChannel(), expectedValue[0].size());                                     \
    bool CHECK_EQUALITY_warnExpectedZero = true;                                                                       \
    for(size_t CHECK_EQUALITY_i = 0; CHECK_EQUALITY_i < expectedValue.size(); ++CHECK_EQUALITY_i) {                    \
      for(size_t CHECK_EQUALITY_k = 0; CHECK_EQUALITY_k < expectedValue[0].size(); ++CHECK_EQUALITY_k) {               \
        if(CHECK_EQUALITY_warnExpectedZero &&                                                                          \
            !compareHelper(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k], CHECK_EQUALITY_UserType())) {            \
          /* non-zero value found in expectedValue, no need to warn about all-zero expected value */                   \
          CHECK_EQUALITY_warnExpectedZero = false;                                                                     \
        }                                                                                                              \
        if(!compareHelper(                                                                                             \
               accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k], expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k])) {     \
          if(fail.empty()) {                                                                                           \
            fail = "Accessor content differs from expected value. First difference at index [" +                       \
                std::to_string(CHECK_EQUALITY_i) + "][" + std::to_string(CHECK_EQUALITY_k) +                           \
                "]: " + std::to_string(accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k]) +                                 \
                " != " + std::to_string(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k]);                            \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
    }                                                                                                                  \
    if(!fail.empty()) {                                                                                                \
      BOOST_ERROR(fail);                                                                                               \
    }                                                                                                                  \
    if(CHECK_EQUALITY_warnExpectedZero && !std::is_same<CHECK_EQUALITY_UserType, ChimeraTK::Boolean>::value) {         \
      BOOST_ERROR("Comparison with all-zero expectedValue! Test may be insensitive! Check the "                        \
                  "generateValue() implementations!");                                                                 \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)(0)

// Similar to CHECK_EQUALITY, but compares two 2D vectors
#define CHECK_EQUALITY_VECTOR(foundValue, expectedValue)                                                               \
  {                                                                                                                    \
    typedef typename decltype(expectedValue)::value_type::value_type CHECK_EQUALITY_UserType;                          \
    std::string fail;                                                                                                  \
    BOOST_CHECK_EQUAL(foundValue.size(), expectedValue.size());                                                        \
    BOOST_CHECK_EQUAL(foundValue[0].size(), expectedValue[0].size());                                                  \
    bool CHECK_EQUALITY_warnExpectedZero = true;                                                                       \
    for(size_t CHECK_EQUALITY_i = 0; CHECK_EQUALITY_i < expectedValue.size(); ++CHECK_EQUALITY_i) {                    \
      for(size_t CHECK_EQUALITY_k = 0; CHECK_EQUALITY_k < expectedValue[0].size(); ++CHECK_EQUALITY_k) {               \
        if(CHECK_EQUALITY_warnExpectedZero &&                                                                          \
            !compareHelper(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k], CHECK_EQUALITY_UserType())) {            \
          /* non-zero value found in expectedValue, no need to warn about all-zero expected value */                   \
          CHECK_EQUALITY_warnExpectedZero = false;                                                                     \
        }                                                                                                              \
        if(!compareHelper(                                                                                             \
               foundValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k], expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k])) {   \
          if(fail.empty()) {                                                                                           \
            fail = "Data content differs from expected value. First difference at index [" +                           \
                std::to_string(CHECK_EQUALITY_i) + "][" + std::to_string(CHECK_EQUALITY_k) +                           \
                "]: " + std::to_string(foundValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k]) +                               \
                " != " + std::to_string(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k]);                            \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
    }                                                                                                                  \
    if(!fail.empty()) {                                                                                                \
      BOOST_ERROR(fail);                                                                                               \
    }                                                                                                                  \
    if(CHECK_EQUALITY_warnExpectedZero && !std::is_same<CHECK_EQUALITY_UserType, ChimeraTK::Boolean>::value) {         \
      BOOST_ERROR("Comparison with all-zero expectedValue! Test may be insensitive! Check the "                        \
                  "generateValue() implementations!");                                                                 \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)(0)

// Similar to CHECK_EQUALITY, but runs readLatest() on the accessor in a loop until the expected value has arrived, for
// at most maxMilliseconds. If the expected value is not seen within that time, an error is risen.
#define CHECK_EQUALITY_TIMEOUT(accessor, expectedValue, maxMilliseconds)                                               \
  {                                                                                                                    \
    typedef typename decltype(expectedValue)::value_type::value_type CHECK_EQUALITY_UserType;                          \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    bool CHECK_EQUALITY_warnExpectedZero = true;                                                                       \
    while(true) {                                                                                                      \
      accessor.readLatest();                                                                                           \
      std::string fail;                                                                                                \
      BOOST_CHECK_EQUAL(accessor.getNChannels(), expectedValue.size());                                                \
      BOOST_CHECK_EQUAL(accessor.getNElementsPerChannel(), expectedValue[0].size());                                   \
      for(size_t CHECK_EQUALITY_i = 0; CHECK_EQUALITY_i < expectedValue.size(); ++CHECK_EQUALITY_i) {                  \
        for(size_t CHECK_EQUALITY_k = 0; CHECK_EQUALITY_k < expectedValue[0].size(); ++CHECK_EQUALITY_k) {             \
          if(CHECK_EQUALITY_warnExpectedZero &&                                                                        \
              !compareHelper(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k], CHECK_EQUALITY_UserType())) {          \
            /* non-zero value found in expectedValue, no need to warn about all-zero expected value */                 \
            CHECK_EQUALITY_warnExpectedZero = false;                                                                   \
          }                                                                                                            \
          if(!compareHelper(                                                                                           \
                 accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k], expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k])) {   \
            if(fail.empty()) {                                                                                         \
              fail = "Accessor content differs from expected value. First difference at index [" +                     \
                  std::to_string(CHECK_EQUALITY_i) + "][" + std::to_string(CHECK_EQUALITY_k) +                         \
                  "]: " + std::to_string(accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k]) +                               \
                  " != " + std::to_string(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k]);                          \
            }                                                                                                          \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
      if(fail.empty()) break;                                                                                          \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK_MESSAGE(!timeout_reached, fail);                                                                     \
      if(timeout_reached) break;                                                                                       \
      usleep(10000);                                                                                                   \
    }                                                                                                                  \
    if(CHECK_EQUALITY_warnExpectedZero && !std::is_same<CHECK_EQUALITY_UserType, ChimeraTK::Boolean>::value) {         \
      BOOST_ERROR("Comparison with all-zero expectedValue! Test may be insensitive! Check the "                        \
                  "generateValue() implementations!");                                                                 \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)(0)

// Similar to CHECK_EQUALITY_TIMEOUT, but compares two 2D vectors
#define CHECK_EQUALITY_VECTOR_TIMEOUT(foundValue, expectedValue, maxMilliseconds)                                      \
  {                                                                                                                    \
    typedef typename decltype(expectedValue)::value_type::value_type CHECK_EQUALITY_UserType;                          \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    bool CHECK_EQUALITY_warnExpectedZero = true;                                                                       \
    while(true) {                                                                                                      \
      std::string fail;                                                                                                \
      auto CHECK_EQUALITY_value = foundValue; /* Copy value for consistency and performance in the following code */   \
      BOOST_CHECK_EQUAL(theValue.size(), expectedValue.size());                                                        \
      BOOST_CHECK_EQUAL(theValue[0].size(), expectedValue[0].size());                                                  \
      for(size_t CHECK_EQUALITY_i = 0; CHECK_EQUALITY_i < expectedValue.size(); ++CHECK_EQUALITY_i) {                  \
        for(size_t CHECK_EQUALITY_k = 0; CHECK_EQUALITY_k < expectedValue[0].size(); ++CHECK_EQUALITY_k) {             \
          if(CHECK_EQUALITY_warnExpectedZero &&                                                                        \
              !compareHelper(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k], CHECK_EQUALITY_UserType())) {          \
            /* non-zero value found in expectedValue, no need to warn about all-zero expected value */                 \
            CHECK_EQUALITY_warnExpectedZero = false;                                                                   \
          }                                                                                                            \
          if(!compareHelper(CHECK_EQUALITY_value[CHECK_EQUALITY_i][CHECK_EQUALITY_k],                                  \
                 expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k])) {                                                 \
            if(fail.empty()) {                                                                                         \
              fail = "Data content differs from expected value. First difference at index [" +                         \
                  std::to_string(CHECK_EQUALITY_i) + "][" + std::to_string(CHECK_EQUALITY_k) +                         \
                  "]: " + std::to_string(CHECK_EQUALITY_value[CHECK_EQUALITY_i][CHECK_EQUALITY_k]) +                   \
                  " != " + std::to_string(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k]);                          \
            }                                                                                                          \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
      if(fail.empty()) break;                                                                                          \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK_MESSAGE(!timeout_reached, fail);                                                                     \
      if(timeout_reached) break;                                                                                       \
      usleep(10000);                                                                                                   \
    }                                                                                                                  \
    if(CHECK_EQUALITY_warnExpectedZero && !std::is_same<CHECK_EQUALITY_UserType, ChimeraTK::Boolean>::value) {         \
      BOOST_ERROR("Comparison with all-zero expectedValue! Test may be insensitive! Check the "                        \
                  "generateValue() implementations!");                                                                 \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)(0)
  // NOLINTEND(bugprone-macro-parentheses)

#define CHECK_TIMEOUT(condition, maxMilliseconds)                                                                      \
  {                                                                                                                    \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    while(!(condition)) {                                                                                              \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK(!timeout_reached);                                                                                   \
      if(timeout_reached) break;                                                                                       \
      usleep(1000);                                                                                                    \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)0

  /********************************************************************************************************************/

  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::runTests(const std::string& cdd_, const std::string& cdd2_) {
    cdd = cdd_;
    cdd2 = cdd2_;
    std::cout << "=== UnifiedBackendTest for " << cdd;
    if(!cdd2.empty()) std::cout << " and " << cdd2;
    std::cout << std::endl;

    size_t nSyncReadRegisters = 0;
    size_t nAsyncReadRegisters = 0;
    size_t nWriteRegisters = 0;
    size_t nRawRegisters = 0;
    size_t nReadOnlyRegisters = 0;
    size_t nWriteOnlyRegisters = 0;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(this->isAsyncRead(x)) ++nAsyncReadRegisters;
      if(this->isSyncRead(x)) ++nSyncReadRegisters;
      if(this->isWrite(x)) ++nWriteRegisters;
      if(this->isWrite(x) && !this->isRead(x)) ++nWriteOnlyRegisters;
      if(!this->isWrite(x) && this->isRead(x)) ++nReadOnlyRegisters;
      if(this->isRaw(x)) ++nRawRegisters;
      if(x.capabilities.forceDataLossWrite == TestCapability::unspecified) {
        std::cout << "WARNING: Register " << x.path() << " has unspecified capability forceDataLossWrite!" << std::endl;
      }
      if(x.capabilities.asyncReadInconsistency == TestCapability::unspecified) {
        std::cout << "WARNING: Register " << x.path() << " has unspecified capability asyncReadInconsistency!"
                  << std::endl;
      }
      if(x.capabilities.switchReadOnly == TestCapability::unspecified) {
        std::cout << "WARNING: Register " << x.path() << " has unspecified capability switchReadOnly!" << std::endl;
      }
      if(x.capabilities.switchWriteOnly == TestCapability::unspecified) {
        std::cout << "WARNING: Register " << x.path() << " has unspecified capability switchWriteOnly!" << std::endl;
      }
      if(x.capabilities.writeNeverLosesData == TestCapability::unspecified) {
        std::cout << "WARNING: Register " << x.path() << " has unspecified capability writeNeverLosesData!"
                  << std::endl;
      }
    });

    std::cout << "Using " << nSyncReadRegisters << " synchronous and " << nAsyncReadRegisters
              << " asynchronous read and " << nWriteRegisters << " write test registers." << std::endl;
    std::cout << "Of those are " << nRawRegisters << " supporting raw mode, " << nReadOnlyRegisters
              << " are read-only and " << nWriteOnlyRegisters << " write-only." << std::endl;

    if(nSyncReadRegisters + nAsyncReadRegisters + nWriteRegisters == 0) {
      std::cout << "ERROR: No test registers specified. Cannot perform tests." << std::endl;
      std::exit(1);
    }

    if(nSyncReadRegisters + nAsyncReadRegisters == 0) {
      std::cout << "WARNING: No read test registers specified. This is acceptable only if the backend does not "
                << "support reading at all." << std::endl;
    }
    else if(nSyncReadRegisters == 0) {
      std::cout
          << "WARNING: No synchronous read test registers specified. This is acceptable only if the backend has only "
          << "registers which support AccessMode::wait_for_new_data." << std::endl;
    }
    else if(nAsyncReadRegisters == 0) {
      std::cout
          << "WARNING: No asynchronous read test registers specified. This is acceptable only if the backend does not "
          << "support AccessMode::wait_for_new_data at all." << std::endl;
    }
    if(nWriteRegisters == 0) {
      std::cout << "WARNING: No write test registers specified. This is acceptable only if the backend does not "
                << "support writing at all." << std::endl;
    }

    if(nRawRegisters == 0) {
      std::cout << "WARNING: No raw registers specified. This is acceptable only if the backend does not "
                << "support raw access mode at all." << std::endl;
    }
    if(nReadOnlyRegisters == 0) {
      std::cout << "WARNING: No read-only registers specified." << std::endl;
    }
    if(nWriteOnlyRegisters == 0) {
      std::cout << "WARNING: No write-only registers specified." << std::endl;
    }

    // run the tests
    test_B_3_1_2_1();
    test_NOSPEC_write();
    test_B_3_2_1_2();
    test_B_3_2_2();
    test_B_4_2_4();
    test_B_6_4();
    test_B_7_2();
    test_B_8_2();
    test_B_8_2_1();
    test_B_8_3();
    test_B_8_4();
    test_B_8_5();
    test_B_8_5_1();
    test_B_8_5_2();
    test_B_8_5_3();
    test_B_8_6_6();
    test_B_9_1();
    test_B_9_2_2();
    test_B_9_3_1();
    test_B_9_3_2();
    test_B_9_4_1();
    test_B_9_5();
    test_NOSPEC_newVersionAfterOpen();
    test_B_11_2_1();
    test_B_11_2_2();
    test_B_11_6();
    test_C_5_2_1_2();
    test_C_5_2_2_2();
    test_C_5_2_3_2();
    test_C_5_2_5_2();
    test_C_5_2_6_2();
    test_C_5_2_7_2();
    test_C_5_3();
    test_C_5_3_2();
    test_C_5_3_3();
    test_NOSPEC_valueAfterConstruction();
    test_NOSPEC_backendNotClosedAfterException();
    test_NOSPEC_rawTransfer();
    test_NOSPEC_catalogueRaw();
    test_NOSPEC_catalogueReadWrite();
  }

  /********************************************************************************************************************/

  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::recoverDevice(ChimeraTK::Device& d) {
    for(size_t i = 0;; ++i) {
      try {
        d.open();
        break;
      }
      catch(ChimeraTK::runtime_error&) {
        usleep(10000); // 10ms
        if(i > 6000) {
          BOOST_FAIL("Device did not recover within 60 seconds after forced ChimeraTK::runtime_error.");
        }
      }
    }
  }

  /********************************************************************************************************************/

  /**
   *  Test synchronous read.
   *  * \anchor UnifiedTest_TransferElement_B_3_1_2_1 \ref transferElement_B_3_1_2_1 "B.3.1.2.1"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_3_1_2_1() {
    std::cout << "--- test_B_3_1_2_1 - synchronous read" << std::endl;
    Device d(cdd);

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // Set remote value to be read.
      x.setRemoteValue();
      std::vector<std::vector<UserType>> v1 = x.template getRemoteValue<UserType>();

      // open the device
      d.open();

      // Read value
      reg.read();

      // Check application buffer
      CHECK_EQUALITY(reg, v1);
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);

      // Set an intermediate remote value to be overwritten next
      x.setRemoteValue();
      usleep(100000); // give potential race conditions a chance to pop up more easily...

      std::vector<std::vector<UserType>> v2;
      for(size_t iter = 0; iter < this->nValuesToTest(x); ++iter) {
        // Set another remote value to be read.
        x.setRemoteValue();
        v2 = x.template getRemoteValue<UserType>();

        // Read second value
        reg.read();

        // Check application buffer
        CHECK_EQUALITY(reg, v2);
        BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      }

      // Reading again without changing remote value does not block and gives the same value
      reg.read();

      // Check application buffer
      CHECK_EQUALITY(reg, v2);
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);

      // close device again
      d.close();
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test write.
   *  * MISSING SPEC
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_NOSPEC_write() {
    std::cout << "--- test_NOSPEC_write - write" << std::endl;
    Device d(cdd);

    // open the device
    d.open();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // write some value
      for(size_t iter = 0; iter < this->nValuesToTest(x); ++iter) {
        auto theValue = x.template generateValue<UserType>();

        reg = theValue;
        reg.write();

        // check remote value (with timeout, because the write might complete asynchronously)
        CHECK_EQUALITY_VECTOR_TIMEOUT(x.template getRemoteValue<UserType>(), theValue, 10000);
      }
    });

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test write() does not destroy application buffer
   *  * \anchor UnifiedTest_TransferElement_B_3_2_1_2 \ref transferElement_B_3_2_1_2 "B.3.2.1.2"
   *
   * (Exception case is covered by B.6.4)
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_3_2_1_2() {
    std::cout << "--- test_B_3_2_1_2 - write() does not destroy application buffer" << std::endl;
    Device d(cdd);

    // open the device
    d.open();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // write some value
      auto theValue = x.template generateValue<UserType>();
      reg = theValue;
      VersionNumber ver;
      reg.write(ver);

      // check that application data buffer is not changed (non-destructive write, B.3.2.1.2)
      BOOST_CHECK(reg.getNChannels() == theValue.size());
      BOOST_CHECK(reg.getNElementsPerChannel() == theValue[0].size());
      CHECK_EQUALITY(reg, theValue);

      // check the version number
      BOOST_CHECK(reg.getVersionNumber() == ver);
    });

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test destructive write.
   *  * \anchor UnifiedTest_TransferElement_B_3_2_2 \ref transferElement_B_3_2_2 "B.3.2.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_3_2_2() {
    std::cout << "--- test_B_3_2_2 - destructive write" << std::endl;
    Device d(cdd);

    // open the device
    d.open();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // write some value destructively
      auto theValue = x.template generateValue<UserType>();
      reg = theValue;
      VersionNumber ver;
      reg.writeDestructively(ver);

      // check that application data buffer shape is not changed (content may be lost)
      BOOST_CHECK(reg.getNChannels() == theValue.size());
      BOOST_CHECK(reg.getNElementsPerChannel() == theValue[0].size());

      // check the version number
      BOOST_CHECK(reg.getVersionNumber() == ver);

      // check remote value
      CHECK_EQUALITY_VECTOR_TIMEOUT(x.template getRemoteValue<UserType>(), theValue, 10000);
    });

    // close device again
    d.close();
  }

/********************************************************************************************************************/

// Turn off the warning about parameter parentheses. They probably are even wrong for template arguments,
// and clutter the readability for variable names.
// NOLINTBEGIN(bugprone-macro-parentheses)

/// Helper macros for test_B_4_2_4
#define ALTER_AND_STORE_APPLICATION_BUFFER(UserType, accessor)                                                         \
  std::vector<std::vector<UserType>> STORE_APPLICATION_BUFFER_data;                                                    \
  VersionNumber STORE_APPLICATION_BUFFER_version;                                                                      \
  DataValidity STORE_APPLICATION_BUFFER_validity;                                                                      \
  for(size_t i = 0; i < accessor.getNChannels(); ++i) {                                                                \
    if constexpr(std::is_arithmetic_v<UserType>)                                                                       \
      std::iota(accessor[i].begin(), accessor[i].end(), std::numeric_limits<UserType>::min() + 1);                     \
    if constexpr(std::is_same_v<std::string, UserType>) std::fill(accessor[i].begin(), accessor[i].end(), "FACECAFE"); \
    STORE_APPLICATION_BUFFER_data.push_back(accessor[i]);                                                              \
  }                                                                                                                    \
  STORE_APPLICATION_BUFFER_version = accessor.getVersionNumber();                                                      \
  STORE_APPLICATION_BUFFER_validity = accessor.dataValidity()

#define CHECK_APPLICATION_BUFFER(UserType, accessor)                                                                   \
  CHECK_EQUALITY(accessor, STORE_APPLICATION_BUFFER_data);                                                             \
  BOOST_CHECK(STORE_APPLICATION_BUFFER_version == accessor.getVersionNumber());                                        \
  BOOST_CHECK(STORE_APPLICATION_BUFFER_validity == accessor.dataValidity())

  // NOLINTEND(bugprone-macro-parentheses)

  /**
   *  Test transfer implementations do not change the application buffer
   *  * \anchor UnifiedTest_TransferElement_B_4_2_4 \ref transferElement_B_4_2_4 "B.4.2.4"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_4_2_4() {
    std::cout << "--- test_B_4_2_4 - transfer implementations do not change the application buffer" << std::endl;
    Device d(cdd);

    // open the device
    d.open();

    std::cout << "... writeTransfer()" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      auto te = reg.getHighLevelImplElement();

      // write some value, calling the stages manually
      auto theValue = x.template generateValue<UserType>();
      reg = theValue;
      VersionNumber ver;
      te->preWrite(TransferType::write, ver);
      ALTER_AND_STORE_APPLICATION_BUFFER(UserType, reg);
      te->writeTransfer(ver);
      CHECK_APPLICATION_BUFFER(UserType, reg);
      te->postWrite(TransferType::write, ver);
    });

    std::cout << "... writeTransferDestructively()" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      auto te = reg.getHighLevelImplElement();

      // write some value, calling the stages manually
      auto theValue = x.template generateValue<UserType>();
      reg = theValue;
      VersionNumber ver;
      te->preWrite(TransferType::writeDestructively, ver);
      ALTER_AND_STORE_APPLICATION_BUFFER(UserType, reg);
      te->writeTransferDestructively(ver);
      CHECK_APPLICATION_BUFFER(UserType, reg);
      te->postWrite(TransferType::writeDestructively, ver);
    });

    std::cout << "... readTransferSynchronously()" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      auto te = reg.getHighLevelImplElement();

      // read some value, calling the stages manually
      auto theValue = x.template generateValue<UserType>();
      reg = theValue;
      VersionNumber ver;
      ALTER_AND_STORE_APPLICATION_BUFFER(UserType, reg);
      te->preRead(TransferType::read);
      CHECK_APPLICATION_BUFFER(UserType, reg);
      te->readTransfer();
      CHECK_APPLICATION_BUFFER(UserType, reg);
      te->postRead(TransferType::read, true);
    });

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test application buffer unchanged after exception
   *  * \anchor UnifiedTest_TransferElement_B_6_4 \ref transferElement_B_6_4 "B.6.4"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_6_4() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_6_4 - application buffer unchanged after exception" << std::endl;
    Device d(cdd);

    std::cout << "... synchronous read " << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      int someNumber = 42;

      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // alter the application buffer to make sure it is not changed under exception
      reg[0][0] = numericToUserType<UserType>(someNumber);
      reg.setDataValidity(DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == VersionNumber(nullptr)); // (quasi-assertion for the test)

      // trigger logic error
      BOOST_CHECK_THROW(reg.read(), logic_error); // (no check intended, just catch)

      // check that the application buffer has not changed after exception
      BOOST_CHECK(reg[0][0] == numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == VersionNumber(nullptr));

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // open the device, then let it throw runtime_error exceptions
        d.open();

        // enable runtime errors
        x.setForceRuntimeError(true, i);

        // trigger runtime_error
        BOOST_CHECK_THROW(reg.read(), runtime_error); // (no check intended, just catch)

        // check that the application buffer has not changed after exception
        BOOST_CHECK(reg[0][0] == numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == VersionNumber(nullptr));

        // disable runtime errors
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);

        // close device again
        d.close();
      }
    });

    std::cout << "... asynchronous read " << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      int someNumber = 42;

      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      // alter the application buffer to make sure it is not changed under exception
      reg[0][0] = numericToUserType<UserType>(someNumber);
      reg.setDataValidity(DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == VersionNumber(nullptr)); // (quasi-assertion for the test)

      // trigger logic error via read()
      BOOST_CHECK_THROW(reg.read(), logic_error); // (no check intended, just catch)

      // check that the application buffer has not changed after exception
      BOOST_CHECK(reg[0][0] == numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == VersionNumber(nullptr));

      // trigger logic error via readNonBlocking()
      BOOST_CHECK_THROW(reg.readNonBlocking(), logic_error); // (no check intended, just catch)

      // check that the application buffer has not changed after exception
      BOOST_CHECK(reg[0][0] == numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == VersionNumber(nullptr));

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // open the device, then let it throw runtime_error exceptions
        d.open();
        d.activateAsyncRead();
        reg.read(); // initial value

        // enable runtime errors
        x.setForceRuntimeError(true, i);

        // alter the application buffer to make sure it is not changed under exception
        reg[0][0] = numericToUserType<UserType>(someNumber);
        reg.setDataValidity(DataValidity::ok);
        auto ver = reg.getVersionNumber();

        // trigger runtime_error via read
        BOOST_CHECK_THROW(reg.read(), runtime_error); // (no check intended, just catch)

        // check that the application buffer has not changed after exception
        BOOST_CHECK(reg[0][0] == numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == ver);

        // recover to get another exception
        x.setForceRuntimeError(false, i);
        this->recoverDevice(d);
        d.activateAsyncRead();
        reg.read(); // initial value
        x.setForceRuntimeError(true, i);

        // alter the application buffer to make sure it is not changed under exception
        reg[0][0] = numericToUserType<UserType>(someNumber);
        reg.setDataValidity(DataValidity::ok);
        ver = reg.getVersionNumber();

        // trigger runtime_error via readNonBlocking
        try {
          while(!reg.readNonBlocking()) usleep(10000);
        }
        catch(runtime_error&) {
        }

        // check that the application buffer has not changed after exception
        BOOST_CHECK(reg[0][0] == numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == ver);

        // disable exceptions on read
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);

        // close device again
        d.close();
      }
    });

    std::cout << "... write " << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      int someNumber = 42;

      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // alter the application buffer to make sure it is not changed under exception
      reg[0][0] = numericToUserType<UserType>(someNumber);
      reg.setDataValidity(DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == VersionNumber(nullptr)); // (quasi-assertion for the test)

      // trigger logic error
      BOOST_CHECK_THROW(reg.write(), logic_error); // (no check intended, just catch)

      // check that the application buffer has not changed after exception
      BOOST_CHECK(reg[0][0] == numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == VersionNumber(nullptr));

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // open the device, then let it throw runtime_error exceptions
        d.open();

        // enable exceptions on read
        x.setForceRuntimeError(true, i);

        // trigger runtime_error
        BOOST_CHECK_THROW(reg.write(), runtime_error); // (no check intended, just catch)

        // check that the application buffer has not changed after exception
        BOOST_CHECK(reg[0][0] == numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == VersionNumber(nullptr));

        // disable exceptions on read
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);

        // close device again
        d.close();
      }
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test data loss in write
   *  * \anchor UnifiedTest_TransferElement_B_7_2 \ref transferElement_B_7_2 "B.7.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_7_2() {
    std::cout << "--- test_B_7_2 - data loss in write" << std::endl;
    Device d(cdd);

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      if(x.capabilities.forceDataLossWrite == TestCapability::enabled) {
        typedef typename decltype(x)::minimumUserType UserType;
        auto registerName = x.path();
        std::cout << "... registerName = " << registerName << " (data loss expected)" << std::endl;

        // enable test condition
        size_t attempts = this->writeQueueLength(x) + 1;
        this->setForceDataLossWrite(x, true);

        // open the device
        d.open();

        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

        // write some value the requested number of attempts
        for(size_t i = 0; i < attempts; ++i) {
          auto theValue = x.template generateValue<UserType>();
          reg = theValue;
          VersionNumber someVersion;
          bool dataLost = reg.write(someVersion);
          if(i < attempts - 1) {
            BOOST_CHECK(dataLost == false);
          }
          else {
            BOOST_CHECK(dataLost == true);
          }
          // User buffer must be intact even when value was lost somewhere
          CHECK_EQUALITY(reg, theValue);
          BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
          BOOST_CHECK(reg.getVersionNumber() == someVersion);
        }

        // disable test condition
        this->setForceDataLossWrite(x, false);

        // check remote value, must be the last written value
        auto v1 = x.template getRemoteValue<UserType>();
        CHECK_EQUALITY(reg, v1);

        // close device again
        d.close();
      }
      else if(x.capabilities.writeNeverLosesData == TestCapability::enabled) {
        typedef typename decltype(x)::minimumUserType UserType;
        auto registerName = x.path();
        std::cout << "... registerName = " << registerName << " (data loss never expected)" << std::endl;

        // obtain number of attempts to make
        size_t attempts = this->writeQueueLength(x) + 1;

        // open the device
        d.open();

        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

        // write some value the requested number of attempts
        for(size_t i = 0; i < attempts; ++i) {
          auto theValue = x.template generateValue<UserType>();
          reg = theValue;
          VersionNumber someVersion;
          bool dataLost = reg.write(someVersion);
          BOOST_CHECK(dataLost == false);
        }

        // check remote value, must be the last written value
        auto v1 = x.template getRemoteValue<UserType>();
        CHECK_EQUALITY(reg, v1);

        // close device again
        d.close();
      }
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test async read fills _readQueue
   *  * \anchor UnifiedTest_TransferElement_B_8_2 \ref transferElement_B_8_2 "B.8.2"
   *
   *  Note: we do not care about read() vs. readNonBlocking() in this test, since that part of the implementation is in
   *  the base class and hence tested in testTransferElement.
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_2() {
    std::cout << "--- test_B_8_2 - async read fills _readQueue" << std::endl;
    Device d(cdd);

    // open the device and activate asynchronous reads
    d.open();
    d.activateAsyncRead();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      // Initial value (stays unchecked here)
      reg.read();
      usleep(100000); // give potential race conditions a chance to pop up more easily...
      BOOST_CHECK(reg.readNonBlocking() == false);

      // Set remote value to be read.
      x.setRemoteValue();
      auto v1 = x.template getRemoteValue<UserType>();

      // Read the value
      reg.read();
      usleep(100000); // give potential race conditions a chance to pop up more easily...
      BOOST_CHECK(reg.readNonBlocking() == false);

      // Check application buffer
      CHECK_EQUALITY(reg, v1);
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // Set multiple remote values in a row - they will be queued
      x.setRemoteValue();
      auto v2 = x.template getRemoteValue<UserType>();
      x.setRemoteValue();
      auto v3 = x.template getRemoteValue<UserType>();
      x.setRemoteValue();
      auto v4 = x.template getRemoteValue<UserType>();

      // Read and check second value
      reg.read();
      CHECK_EQUALITY(reg, v2);
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // Read and check third value
      reg.read();
      CHECK_EQUALITY(reg, v3);
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // Read and check fourth value
      reg.read();
      CHECK_EQUALITY(reg, v4);
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // No more data available
      BOOST_CHECK(reg.readNonBlocking() == false);
      CHECK_EQUALITY(reg, v4); // application buffer is unchanged (SPEC???)
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == someVersion);
    });

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test _readQueue overrun
   *  * \anchor UnifiedTest_TransferElement_B_8_2_1 \ref transferElement_B_8_2_1 "B.8.2.1"
   *
   *  FIXME: This test is not really complete. It tests whether the latest value survives. It does not test though which
   *  other values are surviving. For this the test needs to know the length of the implementation specific data
   * transport queue, which is currently not known to it (the length of the _readQueue continuation is always 1).
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_2_1() {
    std::cout << "--- test_B_8_2_1 - _readQueue overrun" << std::endl;
    Device d(cdd);

    // open the device
    d.open();

    // Activate async read
    d.activateAsyncRead();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      // Initial value (stays unchecked here)
      reg.read();
      usleep(100000); // give potential race conditions a chance to pop up more easily...
      BOOST_CHECK(reg.readNonBlocking() == false);

      // Provoke queue overflow by filling many values. We are only interested in the last one.
      for(size_t i = 0; i < 10; ++i) {
        x.setRemoteValue();
      }
      auto v5 = x.template getRemoteValue<UserType>();

      // Read last written value (B.8.2.1)
      BOOST_CHECK(reg.readLatest() == true);
      CHECK_EQUALITY(reg, v5);
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();
    });

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test new runtime errors are put to _readQueue in async reads
   *  * \anchor UnifiedTest_TransferElement_B_8_3 \ref transferElement_B_8_3 "B.8.3"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_3() {
    std::cout << "--- test_B_8_3 - new runtime errors are put to _readQueue in async reads" << std::endl;
    Device d(cdd);
    d.open();

    // Activate async read
    d.activateAsyncRead();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;

      // obtain accessor for the test
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // read initial value
        reg.read();

        // execute preRead without exception state
        reg.getHighLevelImplElement()->preRead(TransferType::read);

        // enable exceptions on read
        x.setForceRuntimeError(true, i);

        // Check for runtime_error as it is popped of the queue
        BOOST_CHECK_THROW(reg.getHighLevelImplElement()->readTransfer(), ChimeraTK::runtime_error);

        // Need to report the exception to the exception backend, because this is normally done in
        // TransferElement::read() etc.
        d.setException("Some message");

        // complete the operation
        reg.getHighLevelImplElement()->postRead(TransferType::read, false);

        // disable exceptions on read
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
        // Activate async read again
        d.activateAsyncRead();
      }
    });

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test async read consistency heartbeat
   *  * \anchor UnifiedTest_TransferElement_B_8_4 \ref transferElement_B_8_4 "B.8.4"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_4() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_8_4 - async read consistency heartbeat" << std::endl;

    Device d(cdd);

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x) || x.capabilities.asyncReadInconsistency != TestCapability::enabled) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;

      // open the device
      d.open();

      // Activate async read
      d.activateAsyncRead();

      // Set remote value to be read.
      x.setRemoteValue();
      auto v1 = x.template getRemoteValue<UserType>();

      // Obtain accessor
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      // Read and check initial value
      reg.read();
      CHECK_EQUALITY(reg, v1);
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // Provoke inconsistency
      this->forceAsyncReadInconsistency(x);

      // Wait for the exception which informs about the problem
      BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);

      // Recover the device
      this->recoverDevice(d);
      auto v2 = x.template getRemoteValue<UserType>();

      // Activate async read again
      d.activateAsyncRead();

      // Read and check value
      reg.read();
      CHECK_EQUALITY(reg, v2);
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // close device again
      d.close();
    });
  }
  /********************************************************************************************************************/

  /**
   *  Test no async transfers until activateAsyncRead().
   *  FIXME: This test is broken (see redmine ticket #11311)
   *  * \anchor UnifiedTest_TransferElement_B_8_5 \ref transferElement_B_8_5 "B.8.5"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_5() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_8_5 - no async transfers until activateAsyncRead()" << std::endl;
    Device d(cdd);

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;

      // First step: measure time until initial value arrives, so we know how long to wait to exclude that an initial
      // value arrives wrongly.
      std::chrono::duration<double> timeToInitialValue{};
      {
        // start time measurement
        auto t0 = std::chrono::steady_clock::now();

        // open the device
        d.open();
        d.activateAsyncRead();

        // obtain accessor
        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

        // make a successful read (initial value) to make sure the exception state is gone
        reg.read();

        // finish time measurement
        auto t1 = std::chrono::steady_clock::now();
        timeToInitialValue = t1 - t0;

        // close device again
        d.close();
      }

      // Second step: Check if no data arrives without activateAsyncRead()
      {
        // open the device, but don't call activateAsyncRead() yet
        d.open();
        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

        // wait 2 times longer than the time until initial value was received before
        std::this_thread::sleep_for(timeToInitialValue * 2);

        // no value must have arrived
        BOOST_CHECK(reg.readNonBlocking() == false);

        // Check again for possible side effects (automatic subscription on read) of reg.readNonBlocking()
        // wait 2 times longer than the time until initial value was received before
        std::this_thread::sleep_for(timeToInitialValue * 2);

        // no value must have arrived
        BOOST_CHECK(reg.readNonBlocking() == false);

        // close device again
        d.close();
      }
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test activateAsynchronousRead
   *  * \anchor UnifiedTest_TransferElement_B_8_5_1 \ref transferElement_B_8_5_1 "B.8.5.1"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_5_1() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_8_5_1 - activateAsynchronousRead" << std::endl;
    Device d(cdd);
    Device d2;
    if(!cdd2.empty()) {
      d2.open(cdd2);
      d2.close();
    }

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      TwoDRegisterAccessor<UserType> reg2;
      if(!cdd2.empty()) {
        d2.open();
        reg2.replace(d2.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data}));
        if(!cdd2.empty()) BOOST_CHECK(reg2.readNonBlocking() == false); /// REMOVE
      }

      // Set remote value to be read.
      x.setRemoteValue();
      auto v1 = x.template getRemoteValue<UserType>();

      // open the device
      d.open();
      if(!cdd2.empty()) BOOST_CHECK(reg2.readNonBlocking() == false); /// REMOVE

      // Activate async read
      d.activateAsyncRead();

      // Read initial value
      reg.read();

      // Check application buffer
      CHECK_EQUALITY(reg, v1);

      if(!cdd2.empty()) {
        // wait a bit, check that accessor of second device does not receive data
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        BOOST_CHECK(reg2.readNonBlocking() == false);

        // activate async read on second device and check again
        d2.activateAsyncRead();
        reg2.read();
        CHECK_EQUALITY(reg2, v1);
      }

      // close device again
      d.close();
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test initial value
   *  * \anchor UnifiedTest_TransferElement_B_8_5_2 \ref transferElement_B_8_5_2 "B.8.5.2",
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_5_2() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_8_5_2 - initial value" << std::endl;
    Device d(cdd);
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;

      // First check: initial value is correctly arriving
      {
        // Set remote value to be read.
        x.setRemoteValue();
        auto v1 = x.template getRemoteValue<UserType>();

        // open the device and activate async read
        d.open();
        d.activateAsyncRead();

        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

        // Read initial value
        reg.read();

        // Check application buffer
        CHECK_EQUALITY(reg, v1);

        // close device again
        d.close();
      }

      // Second check: Concurrent updates do not cause inconsistency. Note: This test cannot possibly cover all
      // potential scenarios for race conditions, hence only one simple scenario is tested.
      {
        // Set initial remote value, to make sure it is different from the next remote value set below
        x.setRemoteValue();

        // open the device and activate async read
        d.open();
        d.activateAsyncRead();

        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

        // Concurrently set another remote value to be read (while presumably the subscription is still being made).
        x.setRemoteValue();
        auto v2 = x.template getRemoteValue<UserType>();

        // Check that the second value arrives at some point (with timeout)
        CHECK_EQUALITY_TIMEOUT(reg, v2, 30000);

        // close device again
        d.close();
      }
    });
  }

  /********************************************************************************************************************/

  /**
   *  Accessors created after activateAsyncRead() are immediately active
   *  * \anchor UnifiedTest_TransferElement_B_8_5_3 \ref transferElement_B_8_5_3 "B.8.5.3"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_5_3() {
    std::cout << "--- test_B_8_5_3 - accessors created after activateAsyncRead() are immediately active" << std::endl;
    Device d(cdd);

    // open the device and activate async read
    d.open();
    d.activateAsyncRead();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();

      std::cout << "... registerName = " << registerName << " (activated async read)" << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      // Initial value should arrive
      CHECK_TIMEOUT(reg.readNonBlocking() == true, 30000);
    });

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test interrupt()
   *  * \anchor UnifiedTest_TransferElement_B_8_6_6 \ref transferElement_B_8_6_6 "B.8.6.6" (via high-level test)
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_6_6() {
    std::cout << "--- test_B_8_6_6 - interrupt()" << std::endl;

    Device d(cdd);
    auto backend = d.getBackend();
    d.open();

    // Activate async read
    d.activateAsyncRead();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});
      reg.read(); // initial value

      for(size_t i = 0; i < 2; ++i) {
        // execute blocking read in another thread
        boost::thread anotherThread([&] {
          reg.read();
          BOOST_ERROR("boost::thread_interrupt exception expected but not thrown.");
        });

        // interrupt the blocking operation
        reg.getHighLevelImplElement()->interrupt();

        // make sure the other thread can terminate
        anotherThread.join();

        // check accessor is still working
        x.setRemoteValue();
        auto v1 = x.template getRemoteValue<UserType>();
        reg.read();
        CHECK_EQUALITY(reg, v1);
        BOOST_CHECK(reg.dataValidity() == DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() > someVersion);
        someVersion = reg.getVersionNumber();
      }
    });

    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test reporting exceptions to exception backend
   *  * \anchor UnifiedTest_TransferElement_B_9_1 \ref transferElement_B_9_1 "B.9.1"
   *  FIXME: This test is testing base class functionality, so it should be moved to testTransferElement!
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_9_1() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_9_1 - reporting exceptions to exception backend" << std::endl;
    Device d(cdd);

    // open the device, then let it throw runtime_error exceptions
    d.open();

    std::cout << "... synchronous read" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x)) return;
      if(x.nRuntimeErrorCases() == 0) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on read
        x.setForceRuntimeError(true, i);

        // Runtime error should be reported via setException()
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.read(), runtime_error);
        BOOST_CHECK(erb->hasSeenException());

        // disable exceptions on read
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);

        // make a successful read to make sure the exception state is gone. no reporting must take place here.
        BOOST_CHECK_NO_THROW(reg.read());
        BOOST_CHECK(!erb->hasSeenException());
      }
    });

    std::cout << "... asynchronous read" << std::endl;
    d.activateAsyncRead();
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      if(x.nRuntimeErrorCases() == 0) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});
      reg.read(); // initial value

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on read
        x.setForceRuntimeError(true, i);

        // Runtime error should be reported via setException()
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.read(), runtime_error);
        BOOST_CHECK(erb->hasSeenException());

        // disable exceptions on read
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
        d.activateAsyncRead(); // turn async read back on

        // make a successful readNonBlocking (no data) to make sure the exception state is gone. no reporting must take
        // place here.
        BOOST_CHECK_NO_THROW(reg.readNonBlocking());
        BOOST_CHECK(!erb->hasSeenException());
      }
    });

    std::cout << "... write" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      if(x.nRuntimeErrorCases() == 0) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on write
        x.setForceRuntimeError(true, i);

        // Runtime error should be reported via setException()
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.write(), runtime_error);
        BOOST_CHECK(erb->hasSeenException());

        // disable exceptions on write
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);

        // make a successful write to make sure the exception state is gone. no reporting must take place here.
        BOOST_CHECK_NO_THROW(reg.write());
        BOOST_CHECK(!erb->hasSeenException());
      }
    });

    std::cout << "... isReadable" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(x.nRuntimeErrorCases() == 0) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      bool didThrow = false;
      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on write
        x.setForceRuntimeError(true, i);

        // Runtime error should be reported via setException()
        BOOST_CHECK(!erb->hasSeenException());
        try {
          [[maybe_unused]] auto result = reg.isReadable();
        }
        catch(...) {
          didThrow = true;
          BOOST_CHECK(erb->hasSeenException());
        }

        // disable exceptions on write
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
      }

      if(!didThrow) {
        std::cout << " (doesn't throw)" << std::endl;
      }
      else {
        std::cout << " (throws)" << std::endl;
      }
    });

    std::cout << "... isWriteable" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(x.nRuntimeErrorCases() == 0) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      bool didThrow = false;
      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on write
        x.setForceRuntimeError(true, i);

        // Runtime error should be reported via setException()
        BOOST_CHECK(!erb->hasSeenException());
        try {
          [[maybe_unused]] auto result = reg.isWriteable();
        }
        catch(...) {
          didThrow = true;
          BOOST_CHECK(erb->hasSeenException());
        }

        // disable exceptions on write
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
      }

      if(!didThrow) {
        std::cout << " (doesn't throw)" << std::endl;
      }
      else {
        std::cout << " (throws)" << std::endl;
      }
    });

    std::cout << "... isReadOnly" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      bool didThrow = false;
      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on write
        x.setForceRuntimeError(true, i);

        // Runtime error should be reported via setException()
        BOOST_CHECK(!erb->hasSeenException());
        try {
          [[maybe_unused]] auto result = reg.isReadOnly();
        }
        catch(...) {
          didThrow = true;
          BOOST_CHECK(erb->hasSeenException());
        }

        // disable exceptions on write
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
      }

      if(!didThrow) {
        std::cout << " (doesn't throw)" << std::endl;
      }
      else {
        std::cout << " (throws)" << std::endl;
      }
    });
    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test repeated setException() has no effect (in particular, no additional exceptions in async transfers)
   *  * \anchor UnifiedTest_TransferElement_B_9_2_2 \ref transferElement_B_9_2_2 "B.9.2.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_9_2_2() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_9_2_2 - repeated setException() has no effect" << std::endl;
    Device d(cdd);
    d.open();
    d.activateAsyncRead();

    // obtain accessors and read initial value
    std::list<TransferElementAbstractor> accessors;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;

      // obtain accessor for the test
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});
      accessors.push_back(reg);

      // read initial value
      reg.read();
    });

    // enter exception state
    d.setException("Some message");

    // each accessor has now an exception in the queue -> remove from queue
    for(auto& accessor : accessors) {
      BOOST_CHECK_THROW(accessor.read(), runtime_error); // (no test intended, just catch)
    }

    // call setException repeatedly
    d.setException("Some message");
    d.setException("Some message");

    // give potential race conditions a chance...
    usleep(10000);

    // each accessor still must not have any more exceptions in the queue
    for(auto& accessor : accessors) {
      BOOST_CHECK(accessor.readNonBlocking() == false);
    }

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test setException() disables asynchronous read transfers
   *  * \anchor UnifiedTest_TransferElement_B_9_3_1 \ref transferElement_B_9_3_1 "B.9.3.1"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_9_3_1() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_9_3_1 - setException() disables asynchronous read transfers" << std::endl;
    Device d(cdd);
    d.open();
    d.activateAsyncRead();

    // obtain accessors and read initial value
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;

      // obtain accessor for the test
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      // read initial value
      reg.read();

      // enter exception state
      d.setException("Some message");

      // send value, must not be received
      x.setRemoteValue();

      // get the exception off the queue
      BOOST_CHECK_THROW(reg.read(), runtime_error); // (no test intended, just catch)

      // give potential race conditions a chance...
      usleep(100000);

      // no value expected
      BOOST_CHECK(reg.readNonBlocking() == false);

      // recover device
      this->recoverDevice(d);
      d.activateAsyncRead(); // re-activate async read after recovery
    });

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test exactly one runtime_error in the _readQueue per async read accessor
   *  * \anchor UnifiedTest_TransferElement_B_9_3_2 \ref transferElement_B_9_3_2 "B.9.3.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_9_3_2() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_9_3_2 - exactly one runtime_error in the _readQueue per async read accessor" << std::endl;
    Device d(cdd);
    d.open();
    d.activateAsyncRead();

    // obtain accessors and read initial value
    std::list<TransferElementAbstractor> accessors;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;

      // obtain accessor for the test
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});
      accessors.push_back(reg);

      // read initial value
      reg.read();
    });

    // enter exception state
    d.setException("Some message");

    usleep(10000); // give potential race conditions a chance...

    // each accessor must have exactly one exception in the queue
    for(auto& accessor : accessors) {
      // call the read stages explicitly to make sure the exception is thrown in the right place
      accessor.getHighLevelImplElement()->preRead(TransferType::read);
      BOOST_CHECK_THROW(accessor.getHighLevelImplElement()->readTransfer(), runtime_error);
      accessor.getHighLevelImplElement()->postRead(TransferType::read, false);
      // no more exceptions in the queue are allowed
      BOOST_CHECK(accessor.readNonBlocking() == false);
    }

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test doReadTransferSynchronously throws runtime_error after setException() until recovery
   *  * \anchor UnifiedTest_TransferElement_B_9_4_1 \ref transferElement_B_9_4_1 "B.9.4.1"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_9_4_1() {
    if(_testOnlyTransferElement) return;
    std::cout
        << "--- test_B_9_4_1 - doReadTransferSynchronously throws runtime_error after setException() until recovery"
        << std::endl;
    Device d(cdd);
    d.open();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // put backend into exception state
      d.setException("Some message");

      // Check for runtime_error where it is now expected
      BOOST_CHECK_THROW(reg.read(), runtime_error);

      // recover
      this->recoverDevice(d);

      // make a successful read to make sure the exception state is gone
      BOOST_CHECK_NO_THROW(reg.read());
    });

    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test write operations throw after setException()
   *  * \anchor UnifiedTest_TransferElement_B_9_5 \ref transferElement_B_9_5 "B.9.5"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_9_5() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_9_5 - write operations throw after setException()" << std::endl;
    Device d(cdd);
    d.open();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // put backend into exception state
      d.setException("Some message");

      // Check for runtime_error where it is now expected
      BOOST_CHECK_THROW(reg.write(), runtime_error);

      // recover
      this->recoverDevice(d);

      // make a successful read to make sure the exception state is gone
      BOOST_CHECK_NO_THROW(reg.write());
    });

    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test version number bigger for newer values
   *  * \anchor UnifiedTest_TransferElement_B_11_2_1 \ref transferElement_B_11_2_1 "B.11.2.1",
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_11_2_1() {
    std::cout << "--- test_B_11_2_1 - version number bigger for newer values" << std::endl;
    Device d(cdd);

    // open the device
    d.open();

    // synchronous read
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x)) return;
      if(x.capabilities.setRemoteValueIncrementsVersion == TestCapability::disabled) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      for(size_t i = 0; i < 2; ++i) {
        // Set remote value to be read.
        x.setRemoteValue();

        // Read value
        reg.read();

        // Check application buffer
        BOOST_CHECK(reg.getVersionNumber() > someVersion);
        someVersion = reg.getVersionNumber();
      }
    });

    // asynchronous read
    d.activateAsyncRead();
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();

      std::cout << "... registerName = " << registerName << " (async)" << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      reg.read(); // initial value

      VersionNumber someVersion = reg.getVersionNumber();

      for(size_t i = 0; i < 2; ++i) {
        // Set remote value to be read.
        x.setRemoteValue();

        // Read value
        reg.read();

        // Check application buffer
        BOOST_CHECK(reg.getVersionNumber() > someVersion);
        someVersion = reg.getVersionNumber();
      }
    });

    // close device
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test versions after calling open() are newer than any version before.
   *  FIXME: missing in spec
   *
   *  This test is checking that initial values have version numbers larger than VersionNumbers created
   *  by the application before calling open().
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_NOSPEC_newVersionAfterOpen() {
    std::cout << "--- test_NOSPEC_newVersionsAfterOpen - version numbers after open() are newer" << std::endl;
    Device d(cdd);

    // Application can create version numbers any time.
    VersionNumber someVersion{};

    // Open the device. All versions from the backend must be newer than someVersion from now on.
    d.open();

    // synchronous read
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // Set remote value to be read.
      x.setRemoteValue();

      // Read value
      reg.read();

      // Check application buffer
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
    });

    // asynchronous read 1: activate before creating accessor
    d.activateAsyncRead();
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();

      std::cout << "... registerName = " << registerName << " (async1)" << std::endl;

      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      reg.read();

      // Check application buffer
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
    });

    // close device
    d.close();

    // asynchronous read 2: activate after creating accessor
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();

      someVersion = {};
      d.open();

      std::cout << "... registerName = " << registerName << " (async2)" << std::endl;

      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      d.activateAsyncRead();

      reg.read();

      // Check application buffer
      BOOST_CHECK(reg.getVersionNumber() > someVersion);

      d.close();
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test consistent data gets same VersionNumber
   *  * \anchor UnifiedTest_TransferElement_B_11_2_2 \ref transferElement_B_11_2_2 "B.11.2.2"
   *
   * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   * !!!!  TODO FIXME This test is not complete yet  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_11_2_2() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_11_2_2 - consistent data gets same VersionNumber" << std::endl;
    Device d(cdd);

    // open the device and activate async read
    d.open();
    d.activateAsyncRead();

    // CASE 1: consistency with the same register in async read
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;

      // Set remote value to be read.
      x.setRemoteValue();

      // Obtain accessor
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      // Read the initial value
      reg.read();

      // Read through second accessor
      auto reg2 = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});
      reg2.read();

      // Version must be identical to the version of the first accessor
      // Initial values are not necessarily consistent, so this check is skipped.
      // BOOST_CHECK_EQUAL(reg2.getVersionNumber(), reg.getVersionNumber());

      // Change value, must be seen by both accessors, again same version expected
      x.setRemoteValue();

      reg.read();
      reg2.read();
      BOOST_CHECK_EQUAL(reg.getVersionNumber(), reg2.getVersionNumber());
    });

    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test the value after construction for the version number in the application buffer
   *  * \anchor UnifiedTest_TransferElement_B_11_6 \ref transferElement_B_11_6 "B.11.6"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_11_6() {
    std::cout << "--- B.11.6 - value after construction for the version number in the application buffer" << std::endl;
    Device d(cdd);

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // check "value after construction" for VersionNumber
      BOOST_CHECK(reg.getVersionNumber() == VersionNumber(nullptr));
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test logic_error for non-existing register
   *  * \anchor UnifiedTest_TransferElement_C_5_2_1_2 \ref transferElement_C_5_2_1_2 "C.5.2.1.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_C_5_2_1_2() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_C_5_2_1_2 - logic_error for non-existing register" << std::endl;

    // Constructor must throw when device is closed
    {
      Device d(cdd);
      BOOST_CHECK_THROW(
          auto reg = d.getTwoDRegisterAccessor<int>("This_register_name_does_not_exist_for_sure/whileClosed"),
          logic_error);
    }

    // Constructor must throw when device is open
    {
      Device d(cdd);
      d.open();
      BOOST_CHECK_THROW(
          auto reg = d.getTwoDRegisterAccessor<int>("This_register_name_does_not_exist_for_sure/whileOpened"),
          logic_error);
      d.close();
    }
  }

  /********************************************************************************************************************/

  /**
   *  Test logic_error for exceeding register size
   *  * \anchor UnifiedTest_TransferElement_C_5_2_2_2 \ref transferElement_C_5_2_2_2 "C.5.2.2.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_C_5_2_2_2() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_C_5_2_2_2 - logic_error for exceeding register size" << std::endl;

    // Collect register sizes
    std::map<std::string, size_t> sizeMap;
    {
      Device d(cdd);
      boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
        typedef typename decltype(x)::minimumUserType UserType;
        auto registerName = x.path();
        std::cout << "... registerName = " << registerName << std::endl;
        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
        sizeMap[registerName] = reg.getNElementsPerChannel();
      });
    }

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      // number of elements too big
      {
        Device d(cdd);
        BOOST_CHECK_THROW(
            auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, sizeMap[registerName] + 1, 0), logic_error);
      }
      // one element, but behind the end
      {
        Device d(cdd);
        BOOST_CHECK_THROW(
            auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 1, sizeMap[registerName]), logic_error);
      }
      // full length but offset by 1 element (so 1 element too long)
      {
        Device d(cdd);
        BOOST_CHECK_THROW(
            auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, sizeMap[registerName], 1), logic_error);
      }
      // full length by default(=0) but offset by 1 element (so 1 element too long)
      {
        Device d(cdd);
        BOOST_CHECK_THROW(auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 1), logic_error);
      }
      // does not throw when full length and no offset specified
      {
        Device d(cdd);
        BOOST_CHECK_NO_THROW(auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, sizeMap[registerName], 0));
      }
      // does not throw when one element shorter and offset of 1 specified (only if register is long enough)
      if(sizeMap[registerName] > 1) {
        Device d(cdd);
        BOOST_CHECK_NO_THROW(
            auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, sizeMap[registerName] - 1, 1));
      }
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test logic_error for wrong access mode flags
   *  * \anchor UnifiedTest_TransferElement_C_5_2_3_2 \ref transferElement_C_5_2_3_2 "C.5.2.3.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_C_5_2_3_2() {
    std::cout << "--- test_C_5_2_3_2 - logic_error for wrong access mode flags" << std::endl;

    Device d(cdd);
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(x.supportedFlags().has(ChimeraTK::AccessMode::wait_for_new_data)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << " (wait_for_new_data throws)" << std::endl;
      BOOST_CHECK_THROW(
          d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data}), logic_error);
    });
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(x.supportedFlags().has(ChimeraTK::AccessMode::raw)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << " (raw throws)" << std::endl;
      BOOST_CHECK_THROW(d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::raw}), logic_error);
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test logic_error on operation while backend closed
   *  * \anchor UnifiedTest_TransferElement_C_5_2_5_2 \ref transferElement_C_5_2_5_2 "C.5.2.5.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_C_5_2_5_2() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_C_5_2_5_2 - logic_error on operation while backend closed" << std::endl;
    Device d(cdd);

    std::cout << "... synchronous read" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      BOOST_CHECK_THROW(reg.read(), logic_error);
    });

    std::cout << "... asynchronous read" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});
      BOOST_CHECK_THROW(reg.read(), logic_error);
      BOOST_CHECK_THROW(reg.readNonBlocking(), logic_error);
    });

    std::cout << "... write" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      BOOST_CHECK_THROW(reg.write(), logic_error);
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test logic_error on read operation on write-only register
   *  * \anchor UnifiedTest_TransferElement_C_5_2_6_2 \ref transferElement_C_5_2_6_2 "C.5.2.6.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_C_5_2_6_2() {
    std::cout << "--- test_C_5_2_6_2 - logic_error on read operation on write-only register" << std::endl;
    Device d(cdd);

    std::cout << "... synchronous read" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWriteOnly(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      BOOST_CHECK_THROW(reg.read(), logic_error);
    });

    std::cout << "... asynchronous read" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWriteOnly(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      BOOST_CHECK_THROW(
          d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data}), logic_error);
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test logic_error on write operation on read-only register
   *  * \anchor UnifiedTest_TransferElement_C_5_2_7_2 \ref transferElement_C_5_2_7_2 "C.5.2.7.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_C_5_2_7_2() {
    std::cout << "--- test_C_5_2_7_2 - logic_error on write operation on read-only register" << std::endl;
    Device d(cdd);

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isReadOnly(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      BOOST_CHECK_THROW(reg.write(), logic_error);
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test read-only/write-only information changes after runtime_error
   *  * \anchor UnifiedTest_TransferElement_C_5_3 \ref transferElement_C_5_3 "C.5.3"
   *  * \anchor UnifiedTest_TransferElement_C_5_3_1 \ref transferElement_C_5_3_1 "C.5.3.1"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_C_5_3() {
    std::cout << "--- test_C_5_3 - read-only/write-only information changes after runtime_error" << std::endl;
    Device d(cdd);
    d.open();

    // switch to read-only
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x) || !this->isWrite(x) || x.capabilities.switchReadOnly != TestCapability::enabled) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      this->switchReadOnly(x, true);
      BOOST_CHECK(reg.isWriteable() == true);        // C.5.3
      BOOST_CHECK_THROW(reg.write(), runtime_error); // C.5.3.1
      BOOST_CHECK(reg.isWriteable() == false);       // C.5.3
      this->switchReadOnly(x, false);
    });

    // switch to write-only (note: this test is untested, no backend supports this!)
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x) || !this->isWrite(x) || x.capabilities.switchWriteOnly != TestCapability::enabled) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      this->switchWriteOnly(x, true);
      BOOST_CHECK(reg.isReadable() == true);        // C.5.3
      BOOST_CHECK_THROW(reg.read(), runtime_error); // C.5.3.1
      BOOST_CHECK(reg.isReadable() == false);       // C.5.3
      this->switchWriteOnly(x, false);
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test read-only/write-only information cached per accessor
   *  * \anchor UnifiedTest_TransferElement_C_5_3_2 \ref transferElement_C_5_3_2 "C.5.3.2"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_C_5_3_2() {
    std::cout << "--- test_C_5_3_2 - read-only/write-only information cached per accessor" << std::endl;
    Device d(cdd);
    d.open();

    // switch to read-only
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x) || !this->isWrite(x) || x.capabilities.switchReadOnly != TestCapability::enabled) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg1 = d.getTwoDRegisterAccessor<UserType>(registerName);
      auto reg2 = d.getTwoDRegisterAccessor<UserType>(registerName);
      this->switchReadOnly(x, true);
      BOOST_CHECK_THROW(reg1.write(), runtime_error); // no check intended, just catch
      BOOST_CHECK(reg2.isWriteable() == true);
      this->switchReadOnly(x, false);
    });

    // switch to write-only (note: this test is untested, no backend supports this!)
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x) || !this->isWrite(x) || x.capabilities.switchWriteOnly != TestCapability::enabled) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg1 = d.getTwoDRegisterAccessor<UserType>(registerName);
      auto reg2 = d.getTwoDRegisterAccessor<UserType>(registerName);
      this->switchWriteOnly(x, true);
      BOOST_CHECK_THROW(reg1.read(), runtime_error); // no check intended, just catch
      BOOST_CHECK(reg2.isReadable() == true);
      this->switchWriteOnly(x, false);
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test read-only/write-only information always returned from cache if available
   *  * \anchor UnifiedTest_TransferElement_C_5_3_3 \ref transferElement_C_5_3_3 "C.5.3.3"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_C_5_3_3() {
    std::cout << "--- test_C_5_3_3 - read-only/write-only information always returned from cache if available"
              << std::endl;
    Device d(cdd);
    d.open();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // Obtain information. This also makes sure the TE caches it.
      auto isReadable = reg.isReadable();
      auto isWriteable = reg.isWriteable();

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on read
        x.setForceRuntimeError(true, i);

        // Now isReadable and isWriteable are not able to communicate with the device but still should give the same
        // result.
        BOOST_CHECK(reg.isReadable() == isReadable);
        BOOST_CHECK(reg.isWriteable() == isWriteable);

        // disable exceptions on read
        x.setForceRuntimeError(false, i);

        // recover shouldn't even be necessary, since no communication happened
        BOOST_CHECK(d.isFunctional() == true);
      }
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test the content of the application data buffer after construction.
   *  * MISSING SPEC
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_NOSPEC_valueAfterConstruction() {
    std::cout << "--- test_NOSPEC_valueAfterConstruction - content of the application data buffer after construction."
              << std::endl;
    Device d(cdd);

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // check "value after construction" for value buffer
      std::vector<UserType> v(reg.getNElementsPerChannel(), UserType());
      for(size_t i = 0; i < reg.getNChannels(); ++i) BOOST_CHECK(reg[i] == v);
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test that the backend does not close itself after seeing an exception
   *  * MISSING SPEC
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_NOSPEC_backendNotClosedAfterException() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_NOSPEC_backendNotClosedAfterException - backend not closed after exception" << std::endl;
    Device d(cdd);

    // open the device, then let it throw runtime_error exceptions
    d.open();

    std::cout << "... synchronous read" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isRead(x)) return;
      if(x.nRuntimeErrorCases() == 0) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on read
        x.setForceRuntimeError(true, i);

        // trigger runtime error
        BOOST_CHECK_THROW(reg.read(), runtime_error); // no test intended, just catch

        // check device is still open but in error state
        BOOST_CHECK(d.isOpened());
        BOOST_CHECK(!d.isFunctional());

        // check a failed attempt to recover does not change this
        BOOST_CHECK_THROW(d.open(), runtime_error); // no test intended, just catch
        BOOST_CHECK(d.isOpened());
        BOOST_CHECK(!d.isFunctional());

        // disable exceptions on read
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
      }
    });

    std::cout << "... asynchronous read" << std::endl;
    d.activateAsyncRead();
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      if(x.nRuntimeErrorCases() == 0) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        reg.read(); // initial value

        // enable exceptions on read
        x.setForceRuntimeError(true, i);

        // trigger runtime error
        BOOST_CHECK_THROW(reg.read(), runtime_error); // no test intended, just catch

        // check device is still open but in error state
        BOOST_CHECK(d.isOpened());
        BOOST_CHECK(!d.isFunctional());

        // check a failed attempt to recover does not change this
        BOOST_CHECK_THROW(d.open(), runtime_error); // no test intended, just catch
        BOOST_CHECK(d.isOpened());
        BOOST_CHECK(!d.isFunctional());

        // disable exceptions on read
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
        d.activateAsyncRead(); // turn async read back on
      }
    });

    std::cout << "... write" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      if(x.nRuntimeErrorCases() == 0) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on write
        x.setForceRuntimeError(true, i);

        // trigger runtime error
        BOOST_CHECK_THROW(reg.write(), runtime_error); // no test intended, just catch

        // check device is still open but in error state
        BOOST_CHECK(d.isOpened());
        BOOST_CHECK(!d.isFunctional());

        // check a failed attempt to recover does not change this
        BOOST_CHECK_THROW(d.open(), runtime_error); // no test intended, just catch
        BOOST_CHECK(d.isOpened());
        BOOST_CHECK(!d.isFunctional());

        // disable exceptions on write
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
      }
    });

    std::cout << "... isReadable" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(x.nRuntimeErrorCases() == 0) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      bool didThrow = false;
      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on write
        x.setForceRuntimeError(true, i);

        // attempt to trigger runtime error (no obligation to throw...)
        try {
          [[maybe_unused]] auto result = reg.isReadable();
        }
        catch(...) {
          didThrow = true;
          // check device is still open but in error state
          BOOST_CHECK(d.isOpened());
          BOOST_CHECK(!d.isFunctional());

          // check a failed attempt to recover does not change this
          BOOST_CHECK_THROW(d.open(), runtime_error); // no test intended, just catch
          BOOST_CHECK(d.isOpened());
          BOOST_CHECK(!d.isFunctional());
        }

        // disable exceptions on write
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
      }

      if(!didThrow) {
        std::cout << " (doesn't throw)" << std::endl;
      }
      else {
        std::cout << " (throws)" << std::endl;
      }
    });

    std::cout << "... isWriteable" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(x.nRuntimeErrorCases() == 0) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      bool didThrow = false;
      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on write
        x.setForceRuntimeError(true, i);

        // attempt to trigger runtime error (no obligation to throw...)
        try {
          [[maybe_unused]] auto result = reg.isWriteable();
        }
        catch(...) {
          didThrow = true;
          // check device is still open but in error state
          BOOST_CHECK(d.isOpened());
          BOOST_CHECK(!d.isFunctional());

          // check a failed attempt to recover does not change this
          BOOST_CHECK_THROW(d.open(), runtime_error); // no test intended, just catch
          BOOST_CHECK(d.isOpened());
          BOOST_CHECK(!d.isFunctional());
        }

        // disable exceptions on write
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
      }

      if(!didThrow) {
        std::cout << " (doesn't throw)" << std::endl;
      }
      else {
        std::cout << " (throws)" << std::endl;
      }
    });

    std::cout << "... isReadOnly" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      bool didThrow = false;
      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        std::cout << "    -> runtime_error case: " << i << std::endl;
        // enable exceptions on write
        x.setForceRuntimeError(true, i);

        // attempt to trigger runtime error (no obligation to throw...)
        try {
          [[maybe_unused]] auto result = reg.isReadOnly();
        }
        catch(...) {
          didThrow = true;
          // check device is still open but in error state
          BOOST_CHECK(d.isOpened());
          BOOST_CHECK(!d.isFunctional());

          // check a failed attempt to recover does not change this
          BOOST_CHECK_THROW(d.open(), runtime_error); // no test intended, just catch
          BOOST_CHECK(d.isOpened());
          BOOST_CHECK(!d.isFunctional());
        }

        // disable exceptions on write
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
      }

      if(!didThrow) {
        std::cout << " (doesn't throw)" << std::endl;
      }
      else {
        std::cout << " (throws)" << std::endl;
      }
    });
    // close device again
    d.close();
  }

  /********************************************************************************************************************/

  /**
   *  Test that the backend does not close itself after seeing an exception
   *  * MISSING SPEC
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_NOSPEC_rawTransfer() {
    std::cout << "--- test_NOSPEC_rawTransfer - test creation and reading/writing with access mode raw." << std::endl;
    Device d(cdd);
    d.open();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      // the test itself requires an extended interface, so the test can be disabled
      if constexpr(x.capabilities.testRawTransfer == TestCapability::enabled) {
        auto registerName = x.path();
        std::cout << "... registerName = " << registerName << std::endl;

        BOOST_REQUIRE_MESSAGE(this->isRaw(x),
            "Test configuration error: testRawTransfer is enabled for register without AccessMode::raw!");

        typedef typename decltype(x)::minimumUserType UserType;
        typedef typename decltype(x)::rawUserType RawType;
        // Use double as example for a not working user type
        BOOST_CHECK_THROW(
            d.getTwoDRegisterAccessor<double>(registerName, 0, 0, {AccessMode::raw}), ChimeraTK::logic_error);
        try {
          // test creation
          auto reg = d.getTwoDRegisterAccessor<RawType>(registerName, 0, 0, {AccessMode::raw});
          // the test itself requires an extended interface, so the test can be disabled
          if(x.isReadable()) {
            x.setRemoteValue();
            reg.read();
            auto expectedRawValue = x.template getRemoteValue<RawType>(/* getRaw = */ true);
            CHECK_EQUALITY(reg, expectedRawValue);

            auto expectedCookedValue = x.template getRemoteValue<UserType>();
            // fill into a vector<vector> and use CHECK_EQUALITY_VECTOR. This stops at the first mismatch, prints a good
            // error message, checks for all elements 0 etc.
            std::vector<std::vector<UserType>> readCookedValue;
            for(size_t channel = 0; channel < reg.getNChannels(); ++channel) {
              std::vector<UserType> readCookedChannel;
              for(size_t element = 0; element < reg.getNElementsPerChannel(); ++element) {
                readCookedChannel.push_back(reg.template getAsCooked<UserType>(channel, element));
              }
              readCookedValue.push_back(readCookedChannel);
            }
            CHECK_EQUALITY_VECTOR(readCookedValue, expectedCookedValue);
          }
          if(x.isWriteable()) {
            auto newValue = x.template generateValue<RawType>(/* getRaw = */ true);
            reg = newValue;
            reg.write();
            auto readbackValue = x.template getRemoteValue<RawType>(/* getRaw = */ true);
            CHECK_EQUALITY_VECTOR(readbackValue, newValue);

            // test setting as cooked
            auto newCookedValue = x.template generateValue<UserType>();
            for(size_t channel = 0; channel < reg.getNChannels(); ++channel) {
              for(size_t element = 0; element < reg.getNElementsPerChannel(); ++element) {
                reg.template setAsCooked<UserType>(channel, element, newCookedValue[channel][element]);
              }
            }
            reg.write();

            auto readbackCookedValue = x.template getRemoteValue<UserType>();
            CHECK_EQUALITY_VECTOR(readbackCookedValue, newCookedValue);
          }
        }
        catch(std::exception& e) {
          BOOST_CHECK_MESSAGE(false, std::string("Unexpected expeption: ") + e.what());
        }
      } // end of constexpr if
      if(this->isRaw(x)) {
        if(x.capabilities.testRawTransfer == TestCapability::disabled) {
          BOOST_REQUIRE_MESSAGE(false,
              "Test configuration error: testRawTransfer is disabled for register '" + std::string(x.path()) +
                  "' with AccessMode::raw!");
        }
        else if(x.capabilities.testRawTransfer == TestCapability::unspecified) {
          std::cout << "WARNING: testRawTransfer capability unspecified for register '" + std::string(x.path()) +
                  "' with AccessMode::raw. This will turn into a test configuration error in a future release!"
                    << std::endl;
        }
      }
      else {
        if(x.capabilities.testRawTransfer == TestCapability::unspecified) {
          std::cout << "Warning: testRawTransfer capability unspecified for register '" + std::string(x.path()) +
                  "' without AccessMode::raw. Please explicitly disable this test."
                    << std::endl;
        }
      }
    });
  }

  /********************************************************************************************************************/

  /**
   *  Test that the catalogue information for the raw accessor is correct
   *  * MISSING SPEC
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_NOSPEC_catalogueRaw() {
    std::cout << "--- test_NOSPEC_catalogueRaw - test catalogue entries for access mode raw." << std::endl;
    Device d(cdd);

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(x.capabilities.testCatalogue == TestCapability::disabled) {
        return;
      }

      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;

      // workaround for DUMMY_WRITABLE not having information in the catalogue yet
      if(std::string(registerName).find("DUMMY_WRITEABLE") != std::string::npos) {
        return;
      }

      if(std::string(registerName).find("DUMMY_INTERRUPT_") != std::string::npos) {
        return;
      }

      auto registerInfo = d.getRegisterCatalogue().getRegister(registerName);

      if(this->isRaw(x)) {
        BOOST_CHECK(registerInfo.getSupportedAccessModes().has(AccessMode::raw));
        BOOST_TEST(registerInfo.getDataDescriptor().rawDataType() != DataType::none);
      }
      else {
        BOOST_CHECK(not registerInfo.getSupportedAccessModes().has(AccessMode::raw));
        BOOST_CHECK((registerInfo.getDataDescriptor().rawDataType() == DataType::none) ||
            (registerInfo.getDataDescriptor().rawDataType() == DataType::Void));
      }
    });
  }

  /**
   *  Test that the catalogue and accessor information for read and write are correct.
   *  * MISSING SPEC
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_NOSPEC_catalogueReadWrite() {
    std::cout << "--- test_NOSPEC_catalogueReadWrite- test catalogue and accessor entries for read/write." << std::endl;
    Device d(cdd);

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(x.capabilities.testCatalogue == TestCapability::disabled) {
        return;
      }

      typedef typename decltype(x)::minimumUserType UserType;

      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;

      // workaround for DUMMY_WRITABLE not having information in the catalogue yet
      if(std::string(registerName).find("DUMMY_WRITEABLE") != std::string::npos) {
        return;
      }

      if(std::string(registerName).find("DUMMY_INTERRUPT_") != std::string::npos) {
        return;
      }

      auto registerInfo = d.getRegisterCatalogue().getRegister(registerName);
      auto accessor = d.getScalarRegisterAccessor<UserType>(registerName);

      BOOST_CHECK_EQUAL(this->isRead(x), registerInfo.isReadable());
      BOOST_CHECK_EQUAL(this->isRead(x), accessor.isReadable());
      BOOST_CHECK_EQUAL(this->isWrite(x), registerInfo.isWriteable());
      BOOST_CHECK_EQUAL(this->isWrite(x), accessor.isWriteable());
    });
  }

  /********************************************************************************************************************/
  /**
   *  Test small getter functions of accessors
   *
   *  This tests if small getter functions of accessors work as expected, and it verifies that the implementation
   * complies for to the following TransferElement specifications:
   *  * \anchor UnifiedTest_TransferElement_B_15_2 \ref transferElement_B_15_2 "B.15.2"
   */

  /********************************************************************************************************************/

  /**
   *  Test catalogue-accessor consistency
   *
   *  This tests if the catalogue information matches the actual accessor.
   */

  /********************************************************************************************************************/

  /**
   *  Test logic_errors
   *
   *  This tests if logic_errors are thrown in the right place, and it verifies that the implementation complies
   *  for to the following TransferElement specifications:
   *  * \anchor UnifiedTest_TransferElement_C_5_2 \ref transferElement_C_5_2 "C.5.2"
   */

  /********************************************************************************************************************/

} // namespace ChimeraTK
