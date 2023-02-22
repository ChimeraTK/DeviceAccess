// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <exception>
#include <string>

namespace ChimeraTK {

  /**
   *  Exception thrown when a runtime error has occured. Runtime errors are e.g.
   * communication errors or hardware failures which can occur at any time. Those
   * errors are therefore not detectable by other means. Typically it is possible
   * to recover from a runtime_error (after the root cause of the error has been
   * resolved) e.g. simply by retrying the operation (potentially after reopening
   * the device, if applicable).
   */
  class runtime_error : public std::exception {
   public:
    /**
     *  Constructor. The passed message is returned by a call to what() and should
     * describe what exactly went wront.
     */
    explicit runtime_error(std::string message) noexcept;

    /**
     *  Return the message describing what exactly went wrong. The returned
     * message is only descriptive and only meant for display. Program logic must
     * never be based on the content of this string.
     */
    [[nodiscard]] const char* what() const noexcept override;

   private:
    std::string _message;
  };

  /**
   *  Exception thrown when a logic error has occured. This usually means that the
   * program logic is flawed, which points to a programming or configuration
   * error. The exception is also thrown if a feature is used which is not
   *  implemented (e.g. an unsupported AccessModeFlag has been specified). After
   * the exception, the system might be in an unspecified condition which might
   * require shutting down the application.
   *
   *  Note, that it should be generally possible to avoid thrown logic_error
   * exceptions in the first place by checking the system status and only
   * perfoming only allowed operations. Therefore it is good practice to not catch
   * this type of exceptions in applications, or to catch the error only for
   * proper dispaly in a GUI and then terminate.
   */
  class logic_error : public std::exception {
   public:
    /**
     *  Constructor. The passed message is returned by a call to what() and should
     * describe what exactly went wront.
     */
    explicit logic_error(std::string message) noexcept;

    /**
     *  Return the message describing what exactly went wrong. The returned
     * message is only descriptive and only meant for display. Program logic must
     * never be based on the content of this string.
     */
    [[nodiscard]] const char* what() const noexcept override;

   private:
    std::string _message;
  };

} // namespace ChimeraTK
