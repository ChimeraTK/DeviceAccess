#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "SubdeviceBackend.h"

#include "BackendFactory.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "NDRegisterAccessorDecorator.h"

namespace ChimeraTK {

  boost::shared_ptr<DeviceBackend> SubdeviceBackend::createInstance(std::string /*host*/, std::string instance,
                                                          std::list<std::string> parameters, std::string mapFileName) {

    // there is only one possible parameter, a map file. It is optional
    if(parameters.size() == 1) {
      // in case the map file is coming from the URI the mapFileName string from the third dmap file column should be empty
      if(mapFileName.empty()) {
        // we use the parameter from the URI
        // \todo FIXME This can be a relative path. In case the URI is coming from a dmap file,
        // and no map file has been defined in the third column, this path is not interpreted relative to the dmap file.
        // Note: you cannot always interpret it relative to the dmap file because the URI can directly come from the Devic::open() function,
        // although a dmap file path has been set. We don't know this here.
        mapFileName = *(parameters.begin());
      }
      else {
        // We take the entry from the dmap file because it contains the correct path relative to the dmap file
        // (this is case we print a warning)
        std::cout << "Warning: map file name specified in the sdm URI and the third column of the dmap file. "
                  << "Taking the name from the dmap file ('" << mapFileName << "')" << std::endl;
      }
    }

    return boost::shared_ptr<DeviceBackend> (new SubdeviceBackend(instance, mapFileName));
  }

  /*******************************************************************************************************************/

  SubdeviceBackend::SubdeviceBackend(std::string instance, std::string mapFileName) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);

    // decode target information from the instance
    std::vector<std::string> tokens;
    boost::split(tokens, instance, boost::is_any_of(","));

    // check if type is specified
    if(tokens.size() < 1) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Type must be specified in sdm URI.");
    }

    // check if target alias name is specified and open the target device
    if(tokens.size() < 2) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Target device name must be specified in sdm URI.");
    }
    targetAlias = tokens[1];

    // type "area":
    if(tokens[0] == "area") {
      type = Type::area;

      // check if target register name is specified
      if(tokens.size() < 3) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target register name must be specified in sdm URI for type 'area'.");
      }

      targetArea = tokens[2];

      // check for extra arguments
      if(tokens.size() > 3) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Too many tokens in instance specified in sdm URI for type 'area'.");
      }

    }
    // unknown type
    else {
      throw ChimeraTK::logic_error("SubdeviceBackend: Unknown type '+"+tokens[0]+"' specified.");
    }

    // parse map file
    if(mapFileName == "") {
      throw ChimeraTK::logic_error("SubdeviceBackend: Map file must be specified.");
    }
    MapFileParser parser;
    _registerMap = parser.parse(mapFileName);
    _catalogue = _registerMap->getRegisterCatalogue();

  }

  /*******************************************************************************************************************/

  void SubdeviceBackend::open() {
    // open target backend
    BackendFactory &factoryInstance = BackendFactory::getInstance();
    targetDevice =  factoryInstance.createBackend(targetAlias);
    if(!targetDevice->isOpen()) {      // createBackend may return an already opened instance for some backends
      targetDevice->open();
    }
    _opened = true;
  }

  /*******************************************************************************************************************/

  void SubdeviceBackend::close() {
    targetDevice->close();
    _opened = false;
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  class FixedPointConvertingDecorator : public NDRegisterAccessorDecorator<UserType, TargetUserType> {

    public:

      FixedPointConvertingDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>> &target,
                                    FixedPointConverter fixedPointConverter)
      : NDRegisterAccessorDecorator<UserType, TargetUserType>(target),
        _fixedPointConverter(fixedPointConverter)
      {}

      void doPreRead() override {
        _target->preRead();
      }

      void doPostRead() override {
        _target->postRead();
        for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
          for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
            buffer_2D[i][j] = _fixedPointConverter.toCooked<UserType>(_target->accessChannel(i)[j]);
          }
        }
      }

      void doPreWrite() override {
        for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
          for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
            _target->accessChannel(i)[j] = _fixedPointConverter.toRaw<UserType>(buffer_2D[i][j]);
          }
        }
        _target->preWrite();
      }

      void doPostWrite() override {
        _target->postWrite();
      }

      bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const> &other) const override {
        auto casted = boost::dynamic_pointer_cast<FixedPointConvertingDecorator<UserType,TargetUserType> const>(other);
        if(!casted) return false;
        if(_fixedPointConverter != casted->_fixedPointConverter) return false;
        return _target->mayReplaceOther(casted->_target);
      }

    protected:

      FixedPointConverter _fixedPointConverter;

      using NDRegisterAccessorDecorator<UserType, TargetUserType>::_target;
      using NDRegisterAccessor<UserType>::buffer_2D;

  };

  /********************************************************************************************************************/

  template<typename TargetUserType>
  class FixedPointConvertingRawDecorator : public NDRegisterAccessorDecorator<TargetUserType> {

    public:

      FixedPointConvertingRawDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>> &target,
                                       FixedPointConverter fixedPointConverter)
      : NDRegisterAccessorDecorator<TargetUserType>(target),
        _fixedPointConverter(fixedPointConverter)
      {
        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCoocked_impl);
        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCoocked_impl);
      }

      template<typename COOCKED_TYPE>
      COOCKED_TYPE getAsCoocked_impl(unsigned int channel, unsigned int sample) {
        return _fixedPointConverter.toCooked<COOCKED_TYPE>(buffer_2D[channel][sample]);
      }

      template<typename COOCKED_TYPE>
      void setAsCoocked_impl(unsigned int channel, unsigned int sample, COOCKED_TYPE value) {
        buffer_2D[channel][sample] = _fixedPointConverter.toRaw<COOCKED_TYPE>(value);
      }

      bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const> &other) const override {
        auto casted = boost::dynamic_pointer_cast<FixedPointConvertingRawDecorator<TargetUserType> const>(other);
        if(!casted) return false;
        if(_fixedPointConverter != casted->_fixedPointConverter) return false;
        return _target->mayReplaceOther(casted->_target);
      }

    protected:

      FixedPointConverter _fixedPointConverter;

      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( FixedPointConvertingRawDecorator<TargetUserType>, getAsCoocked_impl, 2 );
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( FixedPointConvertingRawDecorator<TargetUserType>, setAsCoocked_impl, 3 );

      using NDRegisterAccessorDecorator<TargetUserType>::_target;
      using NDRegisterAccessor<TargetUserType>::buffer_2D;

  };

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< NDRegisterAccessor<UserType> > SubdeviceBackend::getRegisterAccessor_impl(
      const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    assert(type == Type::area);

    // obtain register info
    auto info = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(_catalogue.getRegister(registerPathName));

    // check that the bar is 0
    if(info->bar != 0) {
      throw ChimeraTK::logic_error("SubdeviceBackend: BARs other then 0 are not supported. Register '"+registerPathName+
                            "' is in BAR "+std::to_string(info->bar)+".");
    }

    // check that the register is not a 2D multiplexed register, which is not yet supported
    if(info->is2DMultiplexed) {
      throw ChimeraTK::logic_error("SubdeviceBackend: 2D multiplexed registers are not yet supported.");
    }

    // compute full offset (from map file and function arguments)
    size_t byteOffset = info->address + sizeof(int32_t)*wordOffsetInRegister;
    if(byteOffset % 4 != 0) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Only addresses which are a multiple of 4 are supported.");
    }
    size_t wordOffset = byteOffset / 4;

    // compute effective length
    if(numberOfWords == 0) {
      numberOfWords = info->nElements;
    }
    else if(numberOfWords > info->nElements) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Requested "+std::to_string(numberOfWords)+" elements from register '"+
                            registerPathName+"', which only has a length of "+std::to_string(info->nElements)+
                            " elements.");
    }

    // check if raw transfer?
    bool isRaw = flags.has(AccessMode::raw);

    // obtain target accessor in raw mode
    flags.add(AccessMode::raw);
    auto rawAcc = targetDevice->getRegisterAccessor<int32_t>(targetArea, numberOfWords, wordOffset, flags);

    // decorate with appropriate FixedPointConvertingDecorator. This is done even when in raw mode so we can properly
    // implement getAsCoocked()/setAsCooked().
    if(!isRaw) {
      return boost::make_shared<FixedPointConvertingDecorator<UserType, int32_t>>( rawAcc,
                    FixedPointConverter(registerPathName, info->width, info->nFractionalBits, info->signedFlag) );
    }
    else {
      // this is handled by the template specialisation for int32_t
      throw ChimeraTK::logic_error("Given UserType when obtaining the SubdeviceBackend in raw mode does not "
          "match the expected type. Use an int32_t instead! (Register name: "+registerPathName+"')");
    }
  }

  /********************************************************************************************************************/

  template<>
  boost::shared_ptr< NDRegisterAccessor<int32_t> > SubdeviceBackend::getRegisterAccessor_impl<int32_t>(
      const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    assert(type == Type::area);

    // obtain register info
    auto info = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(_catalogue.getRegister(registerPathName));

    // check that the bar is 0
    if(info->bar != 0) {
      throw ChimeraTK::logic_error("SubdeviceBackend: BARs other then 0 are not supported. Register '"+registerPathName+
                            "' is in BAR "+std::to_string(info->bar)+".");
    }

    // check that the register is not a 2D multiplexed register, which is not yet supported
    if(info->is2DMultiplexed) {
      throw ChimeraTK::logic_error("SubdeviceBackend: 2D multiplexed registers are not yet supported.");
    }

    // compute full offset (from map file and function arguments)
    size_t byteOffset = info->address + sizeof(int32_t)*wordOffsetInRegister;
    if(byteOffset % 4 != 0) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Only addresses which are a multiple of 4 are supported.");
    }
    size_t wordOffset = byteOffset / 4;

    // compute effective length
    if(numberOfWords == 0) {
      numberOfWords = info->nElements;
    }
    else if(numberOfWords > info->nElements) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Requested "+std::to_string(numberOfWords)+" elements from register '"+
                            registerPathName+"', which only has a length of "+std::to_string(info->nElements)+
                            " elements.");
    }

    // check if raw transfer?
    bool isRaw = flags.has(AccessMode::raw);

    // obtain target accessor in raw mode
    flags.add(AccessMode::raw);
    auto rawAcc = targetDevice->getRegisterAccessor<int32_t>(targetArea, numberOfWords, wordOffset, flags);

    // decorate with appropriate FixedPointConvertingDecorator. This is done even when in raw mode so we can properly
    // implement getAsCoocked()/setAsCooked().
    if(!isRaw) {
      return boost::make_shared<FixedPointConvertingDecorator<int32_t, int32_t>>( rawAcc,
                    FixedPointConverter(registerPathName, info->width, info->nFractionalBits, info->signedFlag) );
    }
    else {
      return boost::make_shared<FixedPointConvertingRawDecorator<int32_t>>( rawAcc,
                    FixedPointConverter(registerPathName, info->width, info->nFractionalBits, info->signedFlag) );
    }
  }

} // namespace ChimeraTK
