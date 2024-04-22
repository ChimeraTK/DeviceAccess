// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "OneDRegisterAccessor.h"
#include <type_traits>

#include <cassert>
#include <cstring>
#include <iostream>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace ChimeraTK {

  /**
   *  generic header for opaque struct handling
   *  It has fields needed for communication in the same process, e.g. between different ApplicationCore modules.
   *  Fields needed for communication over the net belong not here but into the relevant ControlsystemAdapter, e.g. the
   *  OPCUA adapter could implement OPC UA binary mapping and use type ids from a dictionary, in the future.
   */
  struct OpaqueStructHeader {
    std::type_index dataTypeId;
    uint32_t totalLength = 0; // 0: unknown/not set. length includes header.
    explicit OpaqueStructHeader(std::type_index dataTypeId_) : dataTypeId(dataTypeId_) {}
  };

  /**
   * Provides interface to a struct that is mapped onto a 1D array of ValType
   * StructHeader must be derived from OpaqueStructHeader.
   * Variable-length structs are supported, as long as they do not grow beyond the size of the given 1D array.
   *
   * NOTE: MappedStruct concept is discouraged. Use DataConsistencyGroup of struct members instead if possible.
   * It is still required for MappedImage.
   */
  template<class StructHeader>
  class MappedStruct {
   public:
    enum class InitData { Yes, No };
    /// This keeps a reference to given OneDRegisterAccessor. If its underlying vector is swapped out,
    /// the MappedStruct stays valid only if the swapped-in vector was also setup as MappedStruct.
    explicit MappedStruct(
        ChimeraTK::OneDRegisterAccessor<unsigned char>& accToData, InitData doInitData = InitData::No);

    /// returns pointer to data for header and struct content. The returned pointer stays valid until
    /// write() or read() is called for the underlying accessor.
    unsigned char* data();
    /// capacity of used container
    size_t capacity() const;
    /// currently used size
    size_t size() const { return static_cast<OpaqueStructHeader*>(header())->totalLength; }
    /// returns header, e.g. for setting meta data
    StructHeader* header() { return reinterpret_cast<StructHeader*>(data()); }
    /// default initialize header and zero out data that follows
    void initData();

   protected:
    // We keep the accessor instead of the naked pointer to simplify usage, like this the object stays valid even after
    // memory used by accessor was swapped.
    ChimeraTK::OneDRegisterAccessor<unsigned char>& _accToData;
  };

  /*******************************  application to Image encoding *********************************/

  enum class ImgFormat : unsigned {
    Unset = 0,
    Gray8,
    Gray16,
    RGB24,
    RGBA32,
    // floating point formats for communication of intermediate results
    FLOAT1,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    DOUBLE1,
    DOUBLE2,
    DOUBLE3,
    DOUBLE4
  };
  // values are defined for a bit field
  enum class ImgOptions { RowMajor = 1, ColMajor = 0 };

  /// Image header
  struct ImgHeader : public OpaqueStructHeader {
    ImgHeader() : OpaqueStructHeader(typeid(ImgHeader)) {}

    uint32_t width = 0;
    uint32_t height = 0;
    /// start coordinates, in output
    int32_t x_start = 0;
    int32_t y_start = 0;
    /// can be used in output to provide scaled coordinates
    float scale_x = 1;
    float scale_y = 1;
    /// gray=1, rgb=3, rgba=4
    uint32_t channels = 0;
    uint32_t bytesPerPixel = 0;
    /// effective bits per pixel
    uint32_t effBitsPerPixel = 0;
    ImgFormat image_format{ImgFormat::Unset};
    ImgOptions options{ImgOptions::RowMajor};
    /// frame number/counter
    uint64_t frame = 0;
  };

  class MappedImage;
  /**
   * provides convenient matrix-like access for MappedImage
   */
  template<typename ValType, ImgOptions OPTIONS>
  class ImgView {
   public:
    ImgView(MappedImage* owner) : _mi(owner) {}
    /**
     * This allows to read/write image pixel values, for given coordinates.
     * dx, dy are relative to x_start, y_start, i.e. x = x_start+dx  on output side
     * channel is 0..2 for RGB
     * this method is for random access. for sequential access, iterators provide better performance
     */
    ValType& operator()(unsigned dx, unsigned dy, unsigned channel = 0);

    // simply define iterator access via pointers
    using iterator = ValType*;
    using value_type = ValType;
    // for iteration over whole image
    ValType* begin() { return beginRow(0); }
    ValType* end() { return beginRow(header()->height); }
    // these assume ROW-MAJOR ordering
    ValType* beginRow(unsigned row) { return vec() + row * header()->width * header()->channels; }
    ValType* endRow(unsigned row) { return beginRow(row + 1); }
    ImgHeader* header();

   protected:
    ValType* vec();
    MappedImage* _mi;
  };

  /**
   * interface to an image that is mapped onto a 1D array of ValType
   *
   * NOTE: MappedImage is based on MappedStruct concept, which is discouraged. Use DataConsistencyGroup of struct
   * members instead if possible.
   * MappedImage is currently in used by DoocsAdapter and DoocsBackend.
   */
  class MappedImage : public MappedStruct<ImgHeader> {
   public:
    using MappedStruct<ImgHeader>::MappedStruct;

    /// needs to be called after construction. corrupts all data.
    /// this throws logic_error if our buffer size is too small. Try lenghtForShape() to check that in advance
    void setShape(unsigned width, unsigned height, ImgFormat fmt);
    size_t lengthForShape(unsigned width, unsigned height, ImgFormat fmt);
    /// returns pointer to image payload data
    unsigned char* imgBody() { return data() + sizeof(ImgHeader); }

    /// returns an ImgView object which can be used like a matrix. The ImgView becomes invalid at next setShape call.
    /// It also becomes invalid when memory location of underlying MappedStruct changes.
    template<typename UserType, ImgOptions OPTIONS = ImgOptions::RowMajor>
    ImgView<UserType, OPTIONS> interpretedView() {
      [[maybe_unused]] auto* h = header();
      assert(h->channels > 0 && "call setShape() before interpretedView()!");
      assert(h->bytesPerPixel == h->channels * sizeof(UserType) &&
          "choose correct bytesPerPixel and channels value before conversion!");
      assert(((unsigned)h->options & (unsigned)ImgOptions::RowMajor) ==
              ((unsigned)OPTIONS & (unsigned)ImgOptions::RowMajor) &&
          "inconsistent data ordering col/row major");
      ImgView<UserType, OPTIONS> ret(this);
      return ret;
    }

   protected:
    void formatsDefinition(ImgFormat fmt, unsigned& channels, unsigned& bytesPerPixel);
  };

  /*************************** begin MappedStruct implementations  ************************************************/

  template<class StructHeader>
  MappedStruct<StructHeader>::MappedStruct(
      ChimeraTK::OneDRegisterAccessor<unsigned char>& accToData, InitData doInitData)
  : _accToData(accToData) {
    static_assert(std::is_base_of<OpaqueStructHeader, StructHeader>::value,
        "MappedStruct expects StructHeader to implement OpaqueStructHeader");
    if(doInitData == InitData::Yes) {
      initData();
    }
  }

  template<class StructHeader>
  unsigned char* MappedStruct<StructHeader>::data() {
    return _accToData.data();
  }

  template<class StructHeader>
  size_t MappedStruct<StructHeader>::capacity() const {
    // reason for cast: getNElements not declared const
    return const_cast<MappedStruct*>(this)->_accToData.getNElements();
  }

  template<class StructHeader>
  void MappedStruct<StructHeader>::initData() {
    size_t sh = sizeof(StructHeader);
    if(capacity() < sh) {
      throw logic_error("buffer provided to MappedStruct is too small for correct initialization");
    }
    auto* p = data();
    new(p) StructHeader;
    header()->totalLength = sh; // minimal length, could be larger
    memset(p + sh, 0, capacity() - sh);
  }

  /*************************** begin MappedImage implementations  ************************************************/

  inline void MappedImage::formatsDefinition(ImgFormat fmt, unsigned& channels, unsigned& bytesPerPixel) {
    switch(fmt) {
      case ImgFormat::Unset:
        assert(false && "ImgFormat::Unset not allowed");
        break;
      case ImgFormat::Gray8:
        channels = 1;
        bytesPerPixel = 1;
        break;
      case ImgFormat::Gray16:
        channels = 1;
        bytesPerPixel = 2;
        break;
      case ImgFormat::RGB24:
        channels = 3;
        bytesPerPixel = 3;
        break;
      case ImgFormat::RGBA32:
        channels = 4;
        bytesPerPixel = 4;
        break;
      case ImgFormat::FLOAT1:
      case ImgFormat::FLOAT2:
      case ImgFormat::FLOAT3:
      case ImgFormat::FLOAT4:
        channels = unsigned(fmt) - unsigned(ImgFormat::FLOAT1) + 1;
        bytesPerPixel = 4 * channels;
        break;
      case ImgFormat::DOUBLE1:
      case ImgFormat::DOUBLE2:
      case ImgFormat::DOUBLE3:
      case ImgFormat::DOUBLE4:
        channels = unsigned(fmt) - unsigned(ImgFormat::DOUBLE1) + 1;
        bytesPerPixel = 8 * channels;
        break;
    }
  }

  inline size_t MappedImage::lengthForShape(unsigned width, unsigned height, ImgFormat fmt) {
    unsigned channels;
    unsigned bytesPerPixel;
    formatsDefinition(fmt, channels, bytesPerPixel);
    return sizeof(ImgHeader) + (size_t)width * height * bytesPerPixel;
  }

  inline void MappedImage::setShape(unsigned width, unsigned height, ImgFormat fmt) {
    unsigned channels;
    unsigned bytesPerPixel;
    formatsDefinition(fmt, channels, bytesPerPixel);
    size_t totalLen = lengthForShape(width, height, fmt);
    if(totalLen > capacity()) {
      throw logic_error("MappedImage: provided buffer to small for requested image shape");
    }
    auto* h = header();
    // start with default values
    new(h) ImgHeader;
    h->image_format = fmt;
    h->totalLength = totalLen;
    h->width = width;
    h->height = height;
    h->channels = channels;
    h->bytesPerPixel = bytesPerPixel;
  }

  template<typename ValType, ImgOptions OPTIONS>
  ValType& ImgView<ValType, OPTIONS>::operator()(unsigned dx, unsigned dy, unsigned channel) {
    auto* h = header();
    assert(dy < h->height);
    assert(dx < h->width);
    assert(channel < h->channels);
    // this is the only place where row-major / column-major storage is decided
    // note, definition of row major/column major is confusing for images.
    // - for a matrix M(i,j) we say it is stored row-major if rows are stored without interleaving: M11, M12,...
    // - for the same Matrix, if we write M(x,y) for pixel value at coordite (x,y) of an image, this means
    //   that pixel _columns_ are stored without interleaving
    // So definition used here is opposite to matrix definition.
    if constexpr((unsigned)OPTIONS & (unsigned)ImgOptions::RowMajor) {
      return vec()[(dy * h->width + dx) * h->channels + channel];
    }
    else {
      return vec()[(dy + dx * h->height) * h->channels + channel];
    }
  }

  template<typename ValType, ImgOptions OPTIONS>
  ImgHeader* ImgView<ValType, OPTIONS>::header() {
    return _mi->header();
  }

  template<typename ValType, ImgOptions OPTIONS>
  ValType* ImgView<ValType, OPTIONS>::vec() {
    return reinterpret_cast<ValType*>(_mi->imgBody());
  }

} // namespace ChimeraTK
