#pragma once

#include <string>
#include <functional>
#include <list>

#include <boost/fusion/include/at_key.hpp>
#include <boost/test/unit_test.hpp>

#include "Device.h"

// disable shadow warning, boost::mpl::for_each is triggering this warning on Ubuntu 16.04
#pragma GCC diagnostic ignored "-Wshadow"

namespace ChimeraTK {

  /**
   *  Class to test any backend for correct behavior. Instantiate this class and call all (!) preparatory functions to
   *  provide the tests with the backend-specific test actions etc. Finally call runTests() to execute all tests.
   *  Internally the BOOST unit test framework is used, so this shall be called inside a normal unit test.
   * 
   *  Failing to call all preparatory functions will result in an error. This allows a safe test schema evolution - if
   *  more backend specific actions for enabling and disabling test conditions are needed for the tests and the backend
   *  test has not yet been updated, tests will fail.
   * 
   *  Actions are usually speficied as list of pairs of functors. The pair's first element is always the action to enable
   *  the the test condition, the second is the action to disable it. By providing multiple entries in the lists it is
   *  possible to test several code paths the backend has to end up in the intended test condition.
   *  For example in case of forceRuntimeErrorOnRead(): runtime_errors in a read can be caused by a timeout in the
   *  communication channel, or by a bad reply of the device. Two list entries would be provided in this case, one to make
   *  read operations run into timeouts, and one to make the (dummy) device reply with garbage.
   *  If only one singe code path exists to get to the test condition, it is perfectly fine to have only a single entry
   *  in the list.
   * 
   *  In the same way as for the actions, names of registers etc. are provided as lists, so all test can be repeated for
   *  different registers, if required for full coverage.
   * 
   *  Instatiate with default template argument, then call addRegister() to add any number of registers, i.e.:
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
     *    size_t writeQueueLength() {return std::numeric_limits<size_t>::max();}  // see setForceDataLossWrite()
     *    size_t nRuntimeErrorCases() {return 1;}                                 // see setForceRuntimeError()
     *    bool testAsyncReadInconsistency() {return true;}                        // see forceAsyncReadInconsistency()
     * 
     *    typedef int32_t minimumUserType;
     *    typedef minimumUserType rawUserType;  // only used if AccessMode::raw is supprted
     * 
     *    /// Generate value which can be represented by the register, convert it to the UserType (e.g. using
     *    /// ChimeraTK::numericToUserType) and return it.
     *    template<typename UserType>
     *    std::vector<std::vector<UserType>> generateValue();
     *
     *    /// Obtain the current value of the register, convert it to the UserType (e.g. using
     *    /// ChimeraTK::numericToUserType) and return it.
     *    template<typename UserType>
     *    std::vector<std::vector<UserType>> getRemoteValue();
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
     *    /// Force data loss during write operations. It is expected that data loss occurse exactly writeQueueLength
     *    /// write operations after calling this function with enable=true.
     *    /// If writeQueueLength == std::numeric_limits<size_t>::max() it is assumed that data loss can never happen and
     *    /// hence the corresponding test is not executed for this register (and this function is never called).
     *    /// It is guaranteed that this function is always called in pairs with first enable = true and then
     *    /// enable = false.
     *    void setForceDataLossWrite(bool enable);
     * 
     *    /// Do whatever necessary that data last received via a push-type subscription is inconsistent with the actual 
     *    /// value (as read by a synchronous read). This can e.g. be achieved by changing the value without publishng
     *    /// the update to the subscribers.
     *    /// This function will only be called if testAsyncReadInconsistency == true. testAsyncReadInconsistency should
     *    /// be set to false if and only if the underlying protocol prevents that such inconsistency could ever occur.
     *    void forceAsyncReadInconsistency();
     * 
     *    /// Optional: define the following data member to prevent the test from executing any synchronous read tests.
     *    /// This should be used only when testing TransferElements which do not support reads without 
     *    /// AccessMode::wait_for_new_data, like e.g. the ControlSystemAdapter ProcessArray. TransferElements handed
     *    /// out by real backends must always support this.
     *    /// bool disableSyncReadTests{true};
     *  };
     * 
     *  Note: Instances of the register descriptors are created and discarded arbitrarily. If it is necessary to store
     *  any data (e.g. seeds for generating values), use static member variables.
     * 
     *  Properties of the register are implemented as functions instead of data members to make it easier to override
     *  values when using a common base clase for multiple descriptors to avoid code duplication (without triggering a
     *  shadowing warning).
     */
    template<typename REG_T>
    UnifiedBackendTest<typename boost::mpl::push_back<VECTOR_OF_REGISTERS_T, REG_T>::type> addRegister() {
      UnifiedBackendTest<typename boost::mpl::push_back<VECTOR_OF_REGISTERS_T, REG_T>::type> x;
      if(_testOnlyTransferElement) x.testOnlyTransferElement();
      return x;
    }

    /**
     *  "Strong typedef" for list of pairs of functors for enabling and disbaling a test condition.
     */
    class EnableDisableActionList : public std::list<std::pair<std::function<void(void)>, std::function<void(void)>>> {
     public:
      using std::list<std::pair<std::function<void(void)>, std::function<void(void)>>>::list;
    };

    /**
     *  Execute all tests. Call this function within a BOOST_AUTO_TEST_CASE after calling all preparatory functions below.
     *  The tests are executed for the backend identified by the given CDD.
     */
    void runTests(const std::string& cdd);

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
    void test_B_11_2_1();
    void test_B_11_2_2();
    void test_B_11_6();
    void test_C_5_2_1_2();
    void test_C_5_2_2_2();
    void test_C_5_2_3_2();
    void test_C_5_2_5_2();
    void test_C_5_2_6_2();
    void test_C_5_2_7_2();
    void test_NOSPEC_valueAfterConstruction();

    /// Utility functions for recurring tasks
    void recoverDevice(ChimeraTK::Device& d);

    /// Helper class for isRead(). This allows us to check whether REG_T::disableSyncReadTests exists. If not, a
    /// default value of false is assumed.
    template<class REG_T, class Enable = void>
    struct RegT_disableSyncReadTests_getter {
      static bool get(REG_T) { return false; }
    };

    template<class REG_T>
    struct RegT_disableSyncReadTests_getter<REG_T,
        typename std::enable_if<std::is_integral<decltype(REG_T::disableSyncReadTests)>::value>::type> {
      static bool get(REG_T x) { return x.disableSyncReadTests; }
    };

    /// Utility functions for register traits
    template<typename REG_T>
    bool isRead(REG_T x = {}) {
      if(RegT_disableSyncReadTests_getter<REG_T>::get(x)) return false;
      return x.isReadable();
    }
    template<typename REG_T>
    bool isWrite(REG_T x = {}) {
      return x.isWriteable();
    }
    template<typename REG_T>
    bool isSyncRead(REG_T x = {}) {
      return x.isReadable() && !x.supportedFlags().has(ChimeraTK::AccessMode::wait_for_new_data);
    }
    template<typename REG_T>
    bool isAsyncRead(REG_T x = {}) {
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
    std::string cdd;

    /// Flag wheter to disable tests for the backend itself
    bool _testOnlyTransferElement{false};

    /// Special DeviceBackend used for testing the exception reporting to the backend
    struct ExceptionReportingBackend : DeviceBackendImpl {
      ExceptionReportingBackend(const boost::shared_ptr<DeviceBackend>& target) : _target(target) {}
      ~ExceptionReportingBackend() override {}

      void setException() override {
        _hasSeenException = true;
        _target->setException();
      }

      /// Check whether setException() has been called since the last call to hasSeenException().
      bool hasSeenException() {
        bool ret = _hasSeenException;
        _hasSeenException = false;
        return ret;
      }

      void open() override {}
      void close() override {}
      bool isFunctional() const override { return false; }
      std::string readDeviceInfo() override { return ""; }

     private:
      boost::shared_ptr<DeviceBackend> _target;
      bool _hasSeenException{false};
    };
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
  bool compareHelper<double>(double a, double b) {
    return std::abs(a - b) < std::abs(std::max(a, b) / 1e6);
  }

  template<>
  bool compareHelper<float>(float a, float b) {
    return std::abs(a - b) < std::abs(std::max(a, b) / 1e6F);
  }

  template<>
  bool compareHelper<std::string>(std::string a, std::string b) {
    return a == b;
  }
} // namespace ChimeraTK

namespace std {
  std::string to_string(const std::string& v) { return v; }
  std::string to_string(const char*& v) { return {v}; }
} // namespace std

namespace ChimeraTK {

  /********************************************************************************************************************/

  // Helper macro to compare the value on an accessor and the expected 2D value
  // Note: we use a macro and not a function, so BOOST_ERROR prints us the line number of the actual test!
#define CHECK_EQUALITY(accessor, expectedValue)                                                                        \
  {                                                                                                                    \
    std::string fail;                                                                                                  \
    BOOST_CHECK_EQUAL(accessor.getNChannels(), expectedValue.size());                                                  \
    BOOST_CHECK_EQUAL(accessor.getNElementsPerChannel(), expectedValue[0].size());                                     \
    for(size_t CHECK_EQUALITY_i = 0; CHECK_EQUALITY_i < expectedValue.size(); ++CHECK_EQUALITY_i) {                    \
      for(size_t CHECK_EQUALITY_k = 0; CHECK_EQUALITY_k < expectedValue[0].size(); ++CHECK_EQUALITY_k) {               \
        if(!compareHelper(                                                                                             \
               accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k], expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k])) {     \
          if(fail.size() == 0) {                                                                                       \
            fail = "Accessor content differs from expected value. First difference at index [" +                       \
                std::to_string(CHECK_EQUALITY_i) + "][" + std::to_string(CHECK_EQUALITY_k) +                           \
                "]: " + std::to_string(accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k]) +                                 \
                " != " + std::to_string(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k]);                            \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
    }                                                                                                                  \
    if(fail != "") {                                                                                                   \
      BOOST_ERROR(fail);                                                                                               \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)(0)

// Similar to CHECK_EQUALITY, but compares two 2D vectors
#define CHECK_EQUALITY_VECTOR(value, expectedValue)                                                                    \
  {                                                                                                                    \
    std::string fail;                                                                                                  \
    BOOST_CHECK_EQUAL(value.size(), expectedValue.size());                                                             \
    BOOST_CHECK_EQUAL(value[0].size(), expectedValue[0].size());                                                       \
    for(size_t CHECK_EQUALITY_i = 0; CHECK_EQUALITY_i < expectedValue.size(); ++CHECK_EQUALITY_i) {                    \
      for(size_t CHECK_EQUALITY_k = 0; CHECK_EQUALITY_k < expectedValue[0].size(); ++CHECK_EQUALITY_k) {               \
        if(!compareHelper(                                                                                             \
               value[CHECK_EQUALITY_i][CHECK_EQUALITY_k], expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k])) {        \
          if(fail.size() == 0) {                                                                                       \
            fail = "Data content differs from expected value. First difference at index [" +                           \
                std::to_string(CHECK_EQUALITY_i) + "][" + std::to_string(CHECK_EQUALITY_k) +                           \
                "]: " + std::to_string(value[CHECK_EQUALITY_i][CHECK_EQUALITY_k]) +                                    \
                " != " + std::to_string(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k]);                            \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
    }                                                                                                                  \
    if(fail != "") {                                                                                                   \
      BOOST_ERROR(fail);                                                                                               \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)(0)

// Simiar to CHECK_EQUALITY, but runs readLatest() on the accessor in a loop until the expected value has arrived, for
// at most maxMillisecods. If the expected value is not seen within that time, an error is risen.
#define CHECK_EQUALITY_TIMEOUT(accessor, expectedValue, maxMilliseconds)                                               \
  {                                                                                                                    \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    while(true) {                                                                                                      \
      accessor.readLatest();                                                                                           \
      std::string fail;                                                                                                \
      BOOST_CHECK_EQUAL(accessor.getNChannels(), expectedValue.size());                                                \
      BOOST_CHECK_EQUAL(accessor.getNElementsPerChannel(), expectedValue[0].size());                                   \
      for(size_t CHECK_EQUALITY_i = 0; CHECK_EQUALITY_i < expectedValue.size(); ++CHECK_EQUALITY_i) {                  \
        for(size_t CHECK_EQUALITY_k = 0; CHECK_EQUALITY_k < expectedValue[0].size(); ++CHECK_EQUALITY_k) {             \
          if(!compareHelper(                                                                                           \
                 accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k], expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k])) {   \
            if(fail.size() == 0) {                                                                                     \
              fail = "Accessor content differs from expected value. First difference at index [" +                     \
                  std::to_string(CHECK_EQUALITY_i) + "][" + std::to_string(CHECK_EQUALITY_k) +                         \
                  "]: " + std::to_string(accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k]) +                               \
                  " != " + std::to_string(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k]);                          \
            }                                                                                                          \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
      if(fail.size() == 0) break;                                                                                      \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK_MESSAGE(!timeout_reached, fail);                                                                     \
      if(timeout_reached) break;                                                                                       \
      usleep(10000);                                                                                                   \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)(0)

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
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::runTests(const std::string& backend) {
    cdd = backend;
    std::cout << "=== UnifiedBackendTest for " << cdd << std::endl;

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
    test_B_11_2_1();
    test_B_11_2_2();
    test_B_11_6();
    test_C_5_2_1_2();
    test_C_5_2_2_2();
    test_C_5_2_3_2();
    test_C_5_2_5_2();
    test_C_5_2_6_2();
    test_C_5_2_7_2();
    test_NOSPEC_valueAfterConstruction();
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
          BOOST_ERROR("Device did not recover within 60 seconds after forced ChimeraTK::runtime_error.");
        }
      }
      continue;
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

      // Set another remote value to be read.
      x.setRemoteValue();
      auto v2 = x.template getRemoteValue<UserType>();

      // Read second value
      reg.read();

      // Check application buffer
      CHECK_EQUALITY(reg, v2);
      BOOST_CHECK(reg.dataValidity() == DataValidity::ok);

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
      auto theValue = x.template generateValue<UserType>();

      reg = theValue;
      reg.write();

      // check remote value
      auto v1 = x.template getRemoteValue<UserType>();
      CHECK_EQUALITY_VECTOR(v1, theValue);
      
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
      auto v1 = x.template getRemoteValue<UserType>();
      CHECK_EQUALITY_VECTOR(v1, theValue);
    });

    // close device again
    d.close();
  }

/********************************************************************************************************************/

/// Helper macros for test_B_4_2_4
#define STORE_APPLICATION_BUFFER(UserType, accessor)                                                                   \
  std::vector<std::vector<UserType>> STORE_APPLICATION_BUFFER_data;                                                    \
  VersionNumber STORE_APPLICATION_BUFFER_version;                                                                      \
  DataValidity STORE_APPLICATION_BUFFER_validity;                                                                      \
  for(size_t i = 0; i < accessor.getNChannels(); ++i) {                                                                \
    STORE_APPLICATION_BUFFER_data.push_back(accessor[i]);                                                              \
  }                                                                                                                    \
  STORE_APPLICATION_BUFFER_version = accessor.getVersionNumber();                                                      \
  STORE_APPLICATION_BUFFER_validity = accessor.dataValidity()

#define CHECK_APPLICATION_BUFFER(UserType, accessor)                                                                   \
  CHECK_EQUALITY(accessor, STORE_APPLICATION_BUFFER_data);                                                             \
  BOOST_CHECK(STORE_APPLICATION_BUFFER_version == accessor.getVersionNumber());                                        \
  BOOST_CHECK(STORE_APPLICATION_BUFFER_validity == accessor.dataValidity())

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
      STORE_APPLICATION_BUFFER(UserType, reg);
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
      STORE_APPLICATION_BUFFER(UserType, reg);
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

      // write some value, calling the stages manually
      auto theValue = x.template generateValue<UserType>();
      reg = theValue;
      VersionNumber ver;
      STORE_APPLICATION_BUFFER(UserType, reg);
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
        reg.read(); //initial value
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
      if(!this->isWrite(x) || x.writeQueueLength() == std::numeric_limits<size_t>::max()) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;

      // enable test condition
      size_t attempts = x.writeQueueLength();
      x.setForceDataLossWrite(true);

      // open the device
      d.open();

      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

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
      x.setForceDataLossWrite(false);

      // check remote value, must be the last written value
      auto v1 = x.template getRemoteValue<UserType>();
      CHECK_EQUALITY(reg, v1);

      // close device again
      d.close();
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

    // open the device
    d.open();

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
   *  other values are surviving. For this the test needs to know the length of the implementation specific data transport
   *  queue, which is currently not known to it (the length of the _readQueue continuation is always 1).
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

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;

      // obtain accessor for the test
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
        // read initial value
        reg.read();

        // execute preRead without exception state
        reg.getHighLevelImplElement()->preRead(TransferType::read);

        // enable exceptions on read
        x.setForceRuntimeError(true, i);

        // Check for runtime_error as it is popped of the queue
        BOOST_CHECK_THROW(reg.getHighLevelImplElement()->readTransfer(), ChimeraTK::runtime_error);

        // complete the operation
        reg.getHighLevelImplElement()->postRead(TransferType::read, false);

        // disable exceptions on read
        x.setForceRuntimeError(false, i);

        // recover
        this->recoverDevice(d);
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
      if(!this->isAsyncRead(x) || !x.testAsyncReadInconsistency()) return;
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
      x.forceAsyncReadInconsistency();

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
   *  Test no async transfers until activateAsyncRead() for TEs created before open.
   *  * \anchor UnifiedTest_TransferElement_B_8_5 \ref transferElement_B_8_5 "B.8.5"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_5() {
    if(_testOnlyTransferElement) return;
    std::cout << "--- test_B_8_5 - no async transfers until activateAsyncRead() for TEs created before open"
              << std::endl;
    Device d(cdd);

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;

      // First step: measure time until initial value arrives, so we know how long to wait to exclude that an initial
      // value arrives wrongly.
      std::chrono::duration<double> timeToInitialValue;
      {
        // start time measurement
        auto t0 = std::chrono::steady_clock::now();

        // open the device
        d.open();

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
        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

        // open the device
        d.open();

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

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

      // Set remote value to be read.
      x.setRemoteValue();
      auto v1 = x.template getRemoteValue<UserType>();

      // open the device
      d.open();

      // Activate async read
      d.activateAsyncRead();

      // Read initial value
      reg.read();

      // Check application buffer
      CHECK_EQUALITY(reg, v1);

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

        // open the device
        d.open();

        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

        // Read initial value
        reg.read();

        // Check application buffer
        CHECK_EQUALITY(reg, v1);

        // close device again
        d.close();
      }

      // Second check: Concurrent updates do not cause inconsistency. Note: This test cannot possilby cover all
      // potential scenarios for race conditions, hence only one simple scenario is tested.
      {
        // Set initial remote value, to make sure it is different from the next remote value set below
        x.setRemoteValue();

        // open the device
        d.open();

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
   *  Test no activation required for accessors created after open
   *  * \anchor UnifiedTest_TransferElement_B_8_5_3 \ref transferElement_B_8_5_3 "B.8.5.3"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_8_5_3() {
    std::cout << "--- test_B_8_5_3 - no activation required for accessors created after open" << std::endl;
    Device d(cdd);

    // obtain deactivated accessors
    std::list<TransferElementAbstractor> deactivatedAccessors;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << " (deactivated async read)" << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});
      deactivatedAccessors.push_back(reg);
    });

    // open the device
    d.open();

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      VersionNumber someVersion{nullptr};

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
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
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
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});
      reg.read(); // initial value

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
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

        // make a successful readNonBlocking (no data) to make sure the exception state is gone. no reporting must take
        // place here.
        BOOST_CHECK_NO_THROW(reg.readNonBlocking());
        BOOST_CHECK(!erb->hasSeenException());
      }
    });

    std::cout << "... write" << std::endl;
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isWrite(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      for(size_t i = 0; i < x.nRuntimeErrorCases(); ++i) {
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
    d.setException();

    // each accessor has now an exception in the queue -> remove from queue
    for(auto& accessor : accessors) {
      BOOST_CHECK_THROW(accessor.read(), runtime_error); // (no test intended, just catch)
    }

    // call setException repeatedly
    d.setException();
    d.setException();

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
      d.setException();

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
    d.setException();

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
   *  Test doReadTransferSynchonously throws runtime_error after setException() until recovery
   *  * \anchor UnifiedTest_TransferElement_B_9_4_1 \ref transferElement_B_9_4_1 "B.9.4.1"
   */
  template<typename VECTOR_OF_REGISTERS_T>
  void UnifiedBackendTest<VECTOR_OF_REGISTERS_T>::test_B_9_4_1() {
    if(_testOnlyTransferElement) return;
    std::cout
        << "--- test_B_9_4_1 - doReadTransferSynchonously throws runtime_error after setException() until recovery"
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
      d.setException();

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
      d.setException();

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
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(!this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << " (async)" << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data});

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

    // open the device
    d.open();

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
      BOOST_CHECK(reg2.getVersionNumber() == reg.getVersionNumber());

      // Change value, must be seen by both accessors, again same version expected
      x.setRemoteValue();

      reg.read();
      reg2.read();
      BOOST_CHECK(reg.getVersionNumber() == reg2.getVersionNumber());
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
        std::cout << "    NElementsPerChannel = " << sizeMap[registerName] << std::endl;
      });
    }

    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
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
      if(this->isAsyncRead(x)) return;
      typedef typename decltype(x)::minimumUserType UserType;
      auto registerName = x.path();
      std::cout << "... registerName = " << registerName << " (wait_for_new_data throws)" << std::endl;
      BOOST_CHECK_THROW(
          d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {AccessMode::wait_for_new_data}), logic_error);
    });
    boost::mpl::for_each<VECTOR_OF_REGISTERS_T>([&](auto x) {
      if(this->isRaw(x)) return;
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
   *  Test small getter functions of accessors
   * 
   *  This tests if small getter functions of accessors work as expected, and it verifies that the implementation complies
   *  for to the following TransferElement specifications:
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
