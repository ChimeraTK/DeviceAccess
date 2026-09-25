// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

/**
 * This header collects some conversion routines which should be useful both for ApplicationCore Python bindings
 * and DeviceAccess Python bindings.
 * We put PyConvert.h into DeviceAccess, even though the latter does not have pybind11 as dependency.
 * The file is only there to be used from ApplicationCore and DeviceAccess-PythonBindings, which depend on pybind11.
 *
 * Our goal is to support differently packed user data as input, and without data loss convert to right ChimeraTK
 * UserType scalar or vector. We support:
 * - Numpy arrays of the usual types
 * - Python lists of native Python scalars (bool, int, float, string) or numpy scalars (e.g. np.float32)
 *
 * The naive approach of defining std::variant<list of C++ user types or vector or those> does not work since it
 * results in forceful type coercion, e.g. np.float32 is truncated to integers.
 * Pybind11 has a 2 pass algorithm for type conversion:
 * Pass 1 checks for a perfect match, of the included types in the std::variant.
 * If a perfect match is not found, it falls back to Pass 2, which looks at implicit type conversions.
 * When we defined std::variant<int, float, double> and pass in a np.float32 value, the latter is not deteced as perfect
 * match for C++-float in Pass 1, and then implicitly truncated to an integer value in Pass 2.
 *
 * Since we finally anyway make use of our custom type conversion function userTypeToUserType, we must avoid as far as
 * possible, automatic type conversions of the input.
 */

#include <ChimeraTK/Exception.h>
#include <ChimeraTK/SupportedUserTypes.h>

#include <pybind11/numpy.h>

namespace ChimeraTK {

  namespace py = pybind11;

  /********************************************************************************************************************/

  /// Convert a python scalar (bool, int, float, str; also numpy scalars) into the expected UserType.
  ///
  /// We deliberately avoid relying on a lossy cast of the python scalar into a single std::variant, since that can
  /// mangle values (e.g. python ints which do not fit into int64, or negative values converted into an unsigned
  /// register). Instead we examine the python type explicitly and preserve the full range of the value.
  template<typename expectedUserType>
  expectedUserType convertPyScalar(const py::object& input) {
    // check for numpy scalars (e.g., np.int32, np.float64)
    // NumPy scalars have an `.item()` method that returns standard Python primitives
    py::object clean_input = input;
    if(py::hasattr(input, "item")) {
      // This throws if it's not a numpy scalar or array with single element
      clean_input = input.attr("item")(); // Convert np.int32(5) -> Python int(5)
    }

    // Python strings - str or numpy str
    if(py::isinstance<py::str>(clean_input)) {
      return userTypeToUserType<expectedUserType>(clean_input.cast<std::string>());
    }

    // Python float is float64
    if(py::isinstance<py::float_>(clean_input)) {
      return userTypeToUserType<expectedUserType>(clean_input.cast<double>());
    }
    // input is Python bool or int, but bool is a subclass of int in Python
    if(py::isinstance<py::int_>(clean_input)) {
      // in order not to lose information, cast negative integers to int64 and positive to uint64,
      // before userTypeToUserType does any clamping
      // Determine the sign via an object-level comparison with 0 (portable; _PyLong_Sign would be faster but is a
      // private CPython API).
      if(clean_input.attr("__lt__")(py::int_(0)).cast<bool>()) {
        return userTypeToUserType<expectedUserType>(clean_input.cast<int64_t>());
      }
      return userTypeToUserType<expectedUserType>(clean_input.cast<uint64_t>());
    }
    // input is something unknown but might still define Python number protocol
    return userTypeToUserType<expectedUserType>(clean_input.cast<double>());
  }

  /********************************************************************************************************************/

  /// helper for 1D numpy array conversion when data types are know at compile time
  template<typename expectedUserType, typename FROM>
  std::vector<expectedUserType> convertTypedPyArray(const py::array& arr) {
    std::vector<expectedUserType> converted(arr.size());
    // Convert reference handle to typed wrapper (Zero-copy, O(1) operation)
    auto typed_arr = arr.cast<py::array_t<FROM>>();

    // Request an unchecked proxy view (NDIM=1 for 1D, 2 for 2D, etc.)
    // Disables GIL acquisition and runtime bounds-checking overhead.
    auto r = typed_arr.template unchecked<1>();

    if constexpr(std::is_same_v<expectedUserType, FROM>) {
      // memcpy shortcut when in/out types are same
      std::memcpy(converted.data(), r.data(0), r.shape(0) * r.itemsize());
    }
    else {
      // Directly access elements
      for(ssize_t i = 0; i < r.shape(0); ++i) {
        // Zero-copy direct read
        // std::cout << "received value: " << in << " of size " << sizeof(FROM) << " bytes.\n";
        converted[i] = userTypeToUserType<expectedUserType>(r(i));
      }
    }
    return converted;
  }

  /********************************************************************************************************************/

  /**
   * This takes 1D numpy arrays of any type.
   * Advantage of py::array over std::vector is, it directly maps memory of numpy arrays, eliminating the copy and
   * convert altogether where possible. When conversion must happen, the stricter numpy rules for conversion apply,
   * which forbid information loss.
   */
  template<typename expectedUserType>
  std::vector<expectedUserType> convertPyArray(const py::array& arr) {
    py::dtype dt = arr.dtype();

    if(dt.is(py::dtype::of<int8_t>())) {
      return convertTypedPyArray<expectedUserType, int8_t>(arr);
    }
    if(dt.is(py::dtype::of<int16_t>())) {
      return convertTypedPyArray<expectedUserType, int16_t>(arr);
    }
    if(dt.is(py::dtype::of<int32_t>())) {
      return convertTypedPyArray<expectedUserType, int32_t>(arr);
    }
    if(dt.is(py::dtype::of<int64_t>())) {
      return convertTypedPyArray<expectedUserType, int64_t>(arr);
    }
    if(dt.is(py::dtype::of<uint8_t>())) {
      return convertTypedPyArray<expectedUserType, uint8_t>(arr);
    }
    if(dt.is(py::dtype::of<uint16_t>())) {
      return convertTypedPyArray<expectedUserType, uint16_t>(arr);
    }
    if(dt.is(py::dtype::of<uint32_t>())) {
      return convertTypedPyArray<expectedUserType, uint32_t>(arr);
    }
    if(dt.is(py::dtype::of<uint64_t>())) {
      return convertTypedPyArray<expectedUserType, uint64_t>(arr);
    }
    if(dt.is(py::dtype::of<float>())) {
      return convertTypedPyArray<expectedUserType, float>(arr);
    }
    if(dt.is(py::dtype::of<double>())) {
      return convertTypedPyArray<expectedUserType, double>(arr);
    }
    if(dt.is(py::dtype::of<bool>())) {
      return convertTypedPyArray<expectedUserType, bool>(arr);
    }

    // note, we exclude numpy arrays of strings since they are fixed-width of unknown width
    throw ChimeraTK::logic_error("Unsupported NumPy array dtype!");
  }

  /********************************************************************************************************************/

  /// this takes native python lists, content possibly of mixed types
  /// convertToPyArr should be preferred when input is a numpy-array
  template<typename expectedUserType>
  std::vector<expectedUserType> convertPyList(const py::object& input) {
    // Note, the following does not work because the cast often chooses a wrong type when doing auto conversion:
    // auto vec = input.cast<std::vector<std::variant<int, double>>();
    // Instead, we must manually go over the sequence and convert every item

    auto seq = input.cast<py::sequence>();
    std::vector<expectedUserType> converted(seq.size());

    std::transform(seq.begin(), seq.end(), converted.begin(), [](const auto& itemHandle) {
      auto v = py::reinterpret_borrow<py::object>(itemHandle);
      return convertPyScalar<expectedUserType>(v);
    });
    return converted;
  }

  /********************************************************************************************************************/

  /// input may be numpy arrays, numpy scalars, python primitives, and lists of numpy scalars or python primitives
  template<typename expectedUserType>
  std::vector<expectedUserType> convertPyObject(const py::object& input, bool allowScalar, bool allowArray) {
    assert(allowScalar || allowArray);
    if(allowArray) {
      // check numpy arrays first (Zero-copy, preserves multi-dim metadata)
      if(py::isinstance<py::array>(input)) {
        auto arr = input.cast<py::array>();
        return convertPyArray<expectedUserType>(arr);
      }

      // Python lists/tuples must go to convertPyList, NOT to convertPyScalar,
      // even when a scalar is also allowed (e.g. write1D with allowScalar=true).
      if(py::isinstance<py::sequence>(input) && !py::isinstance<py::str>(input)) {
        return convertPyList<expectedUserType>(input);
      }
    }

    if(allowScalar) {
      return {convertPyScalar<expectedUserType>(input)};
    }

    if(allowArray) {
      // when input is a scalar string, and allowScalar=false so it's clear it should be treated as array of chars

      // try casting to vector (Handles Python lists/tuples)
      return convertPyList<expectedUserType>(input);
    }

    throw ChimeraTK::logic_error("unsupported parameters for convertPyObject");
  }

  /********************************************************************************************************************/

  /// helper for 2D numpy array conversion when the data type is known at compile time
  template<typename expectedUserType, typename FROM>
  std::vector<std::vector<expectedUserType>> convertTypedPyArray2D(const py::array& arr) {
    auto typed = arr.cast<py::array_t<FROM>>();
    auto r = typed.template unchecked<2>();
    ssize_t rows = arr.shape(0);
    ssize_t cols = arr.shape(1);
    std::vector<std::vector<expectedUserType>> converted(rows, std::vector<expectedUserType>(cols));

    if constexpr(std::is_same_v<expectedUserType, FROM>) {
      if(arr.flags() & py::array::c_style) {
        // memcpy shortcut when in/out types are same
        for(ssize_t i = 0; i < rows; ++i) {
          std::memcpy(converted[i].data(), r.data(i, 0), cols * r.itemsize());
        }
        return converted;
      }
    }
    for(ssize_t i = 0; i < rows; ++i) {
      for(ssize_t j = 0; j < cols; ++j) {
        converted[i][j] = userTypeToUserType<expectedUserType>(r(i, j));
      }
    }
    return converted;
  }

  /********************************************************************************************************************/

  /// Take a 2D numpy array and convert it into a vector of vectors of expectedUserType (one inner vector per channel).
  template<typename expectedUserType>
  std::vector<std::vector<expectedUserType>> convertPyArray2D(const py::array& arr) {
    py::dtype dt = arr.dtype();
    if(dt.is(py::dtype::of<int8_t>())) {
      return convertTypedPyArray2D<expectedUserType, int8_t>(arr);
    }
    if(dt.is(py::dtype::of<int16_t>())) {
      return convertTypedPyArray2D<expectedUserType, int16_t>(arr);
    }
    if(dt.is(py::dtype::of<int32_t>())) {
      return convertTypedPyArray2D<expectedUserType, int32_t>(arr);
    }
    if(dt.is(py::dtype::of<int64_t>())) {
      return convertTypedPyArray2D<expectedUserType, int64_t>(arr);
    }
    if(dt.is(py::dtype::of<uint8_t>())) {
      return convertTypedPyArray2D<expectedUserType, uint8_t>(arr);
    }
    if(dt.is(py::dtype::of<uint16_t>())) {
      return convertTypedPyArray2D<expectedUserType, uint16_t>(arr);
    }
    if(dt.is(py::dtype::of<uint32_t>())) {
      return convertTypedPyArray2D<expectedUserType, uint32_t>(arr);
    }
    if(dt.is(py::dtype::of<uint64_t>())) {
      return convertTypedPyArray2D<expectedUserType, uint64_t>(arr);
    }
    if(dt.is(py::dtype::of<float>())) {
      return convertTypedPyArray2D<expectedUserType, float>(arr);
    }
    if(dt.is(py::dtype::of<double>())) {
      return convertTypedPyArray2D<expectedUserType, double>(arr);
    }
    if(dt.is(py::dtype::of<bool>())) {
      return convertTypedPyArray2D<expectedUserType, bool>(arr);
    }
    throw ChimeraTK::logic_error("Unsupported NumPy array dtype!");
  }

  /********************************************************************************************************************/

  /// input may be a nested python list (list of lists) or a 2D numpy array; result is one inner vector per channel.
  template<typename expectedUserType>
  std::vector<std::vector<expectedUserType>> convertPyObject2D(const py::object& input) {
    if(py::isinstance<py::array>(input)) {
      auto arr = input.cast<py::array>();
      if(arr.ndim() != 2) {
        throw ChimeraTK::logic_error("convertPyObject2D: expected a 2D numpy array");
      }
      return convertPyArray2D<expectedUserType>(arr);
    }

    // otherwise a sequence of sequences (e.g. numpy array wrapped as list, or list of lists)
    auto outer = input.cast<py::sequence>();
    std::vector<std::vector<expectedUserType>> result;
    result.reserve(outer.size());
    for(const auto& rowHandle : outer) {
      auto row = py::reinterpret_borrow<py::object>(rowHandle);
      result.push_back(convertPyObject<expectedUserType>(row, false, true));
    }
    return result;
  }

  /********************************************************************************************************************/

  /// Convert a python object (scalar, 1-element list, or 1-element array) into a UserType scalar value.
  /// This is used by scalar accessors and scalar write paths, where the user may pass a plain scalar or a
  /// list/array with a single element.
  template<typename expectedUserType>
  expectedUserType convertScalarValue(const py::object& input) {
    std::vector<expectedUserType> ret = convertPyObject<expectedUserType>(input, true, true);
    if(ret.size() != 1) {
      throw ChimeraTK::logic_error("array of size!=1 was supplied where a scalar value was expected");
    }
    return ret[0];
  }

  /// Just like convertScalarValue, but input may be empty
  template<typename expectedUserType>
  std::optional<expectedUserType> convertScalarOptionalValue(const py::object& input) {
    std::vector<expectedUserType> ret = convertPyObject<expectedUserType>(input, true, true);
    if(ret.size() > 1) {
      throw ChimeraTK::logic_error("array of size>1 was supplied where a scalar value was expected");
    }
    if(ret.empty()) {
      return {};
    }
    return ret[0];
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
