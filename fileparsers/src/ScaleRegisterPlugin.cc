/*
 * ScaleRegisterPlugin.cc
 *
 *  Created on: Feb 26, 2016
 *      Author: Martin Hierholzer
 */

#include "RegisterPluginFactory.h"
#include "ScaleRegisterPlugin.h"
#include "RegisterAccessor.h"

namespace mtca4u {

  /********************************************************************************************************************/

  /** Register ScaleRegisterPlugin with the RegisterPluginFactory */
  class ScaleRegisterPluginRegisterer {
    public:
      ScaleRegisterPluginRegisterer() {
        RegisterPluginFactory::getInstance().registerPlugin("scale",&ScaleRegisterPlugin::createInstance);
      }
  };
  ScaleRegisterPluginRegisterer scaleRegisterPluginRegisterer;

  /********************************************************************************************************************/

  /** The register accessor used by the ScaleRegisterPlugin */
  template<typename T>
  class ScaleRegisterPluginBufferingAccessor : public BufferingRegisterAccessorImpl<T> {
    public:

      /** The constructor takes the original accessor and the scaling factor as arguments */
      ScaleRegisterPluginBufferingAccessor(boost::shared_ptr< BufferingRegisterAccessorImpl<T> > accessor,
          Value<double> scalingFactor)
      : _accessor(accessor), _scalingFactor(scalingFactor)
      {
        // reserve space for cooked buffer
        BufferingRegisterAccessorImpl<T>::cookedBuffer.resize(_accessor->getNumberOfElements());
      }

      virtual ~ScaleRegisterPluginBufferingAccessor() {};

      virtual void read() {
        // read from hardware
        _accessor->read();
        // apply scaling factor while copying buffer from underlying accessor to our buffer
        for(unsigned int i=0; i<BufferingRegisterAccessorImpl<T>::cookedBuffer.size(); i++) {
          BufferingRegisterAccessorImpl<T>::cookedBuffer[i] = (*_accessor)[i] * _scalingFactor;
        }
      }

      virtual void write() {
        // apply scaling factor while copying buffer from our buffer to underlying accessor
        for(unsigned int i=0; i<BufferingRegisterAccessorImpl<T>::cookedBuffer.size(); i++) {
          (*_accessor)[i] = BufferingRegisterAccessorImpl<T>::cookedBuffer[i] / _scalingFactor;
        }
        // write to hardware
        _accessor->write();
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const ScaleRegisterPluginBufferingAccessor<T> >(other);
        if(!rhsCasted) return false;
        if(_accessor != rhsCasted->_accessor) return false;
        if(_scalingFactor != rhsCasted->_scalingFactor) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return _accessor->isReadOnly();
      }

    protected:

      /** The underlying register accessor */
      boost::shared_ptr< BufferingRegisterAccessorImpl<T> > _accessor;

      /** The scaling factor */
      Value<double> _scalingFactor;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _accessor->getHardwareAccessingElements();
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        if(_accessor->isSameRegister(newElement)) {
          _accessor = boost::static_pointer_cast< BufferingRegisterAccessorImpl<T> >(newElement);
        }
      }

  };

  template<>
  void ScaleRegisterPluginBufferingAccessor<std::string>::read();

  template<>
  void ScaleRegisterPluginBufferingAccessor<std::string>::write();

  /********************************************************************************************************************/

  /** The non-buffering register accessor used by the ScaleRegisterPlugin */
  class ScaleRegisterPluginAccessor : public RegisterAccessor {
    public:

      /** The constructor takes the original accessor and the scaling factor as arguments */
      ScaleRegisterPluginAccessor(boost::shared_ptr< RegisterAccessor > accessor,
          Value<double> scalingFactor)
      : RegisterAccessor(boost::shared_ptr<DeviceBackend>()), _accessor(accessor), _scalingFactor(scalingFactor)
      {
        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(read_impl);
        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(write_impl);
      }

      void readRaw(int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0) const {
        _accessor->readRaw(data,dataSize,addRegOffset);
      }

      void writeRaw(int32_t const *data, size_t dataSize = 0, uint32_t addRegOffset = 0) {
        _accessor->writeRaw(data,dataSize,addRegOffset);
      }

      RegisterInfoMap::RegisterInfo const &getRegisterInfo() const {
        return _accessor->getRegisterInfo();
      }

      FixedPointConverter const &getFixedPointConverter() const {
        return _accessor->getFixedPointConverter();
      }

      virtual unsigned int getNumberOfElements() const {
        return _accessor->getNumberOfElements();
      }

    private:

      template <typename ConvertedDataType>
      void read_impl(ConvertedDataType *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const {
        // read from hardware into temporary buffer
        std::vector<ConvertedDataType> buffer(nWords);
        _accessor->read(buffer.data(), nWords, wordOffsetInRegister);
        // apply scaling factor while copying buffer from underlying accessor to the target buffer
        for(unsigned int i=0; i<nWords; i++) {
          convertedData[i] = buffer[i] * _scalingFactor;
        }
      }
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( ScaleRegisterPluginAccessor, read_impl, 3);

      template <typename ConvertedDataType>
      void write_impl(const ConvertedDataType *convertedData, size_t nWords, uint32_t wordOffsetInRegister) {
        // create temporary buffer
        std::vector<ConvertedDataType> buffer(nWords);
        // apply scaling factor while copying buffer from source buffer to temporary buffer
        for(unsigned int i=0; i<nWords; i++) {
          buffer[i] = convertedData[i] / _scalingFactor;
        }
        // write from temporary buffer to hardware
        _accessor->write(buffer.data(), nWords, wordOffsetInRegister);
      }
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( ScaleRegisterPluginAccessor, write_impl, 3);

      /** The underlying register accessor */
      boost::shared_ptr< RegisterAccessor > _accessor;

      /** The scaling factor */
      Value<double> _scalingFactor;

  };

  template<>
  void ScaleRegisterPluginAccessor::read_impl<std::string>(std::string *convertedData, size_t nWords,
      uint32_t wordOffsetInRegister) const;

  template<>
  void ScaleRegisterPluginAccessor::write_impl<std::string>(const std::string *convertedData, size_t nWords,
      uint32_t wordOffsetInRegister);

  /********************************************************************************************************************/

  ScaleRegisterPlugin::ScaleRegisterPlugin(const std::map<std::string, Value<std::string> > &parameters) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(decorateBufferingRegisterAccessor_impl);
    try {
      scalingFactor = parameters.at("factor");
    }
    catch(std::out_of_range &e) {
      throw DeviceException("ScaleRegisterPlugin: Missing parameter 'factor'.", DeviceException::EX_WRONG_PARAMETER);
    }
  }

  /********************************************************************************************************************/

  boost::shared_ptr<RegisterPlugin> ScaleRegisterPlugin::createInstance(
      const std::map<std::string, Value<std::string> > &parameters) {
    return boost::shared_ptr<RegisterPlugin>(new ScaleRegisterPlugin(parameters));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > ScaleRegisterPlugin::decorateBufferingRegisterAccessor_impl(
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor) const {
    return boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >(
        new ScaleRegisterPluginBufferingAccessor<UserType>(accessor, scalingFactor));
  }

  /********************************************************************************************************************/

  boost::shared_ptr<RegisterAccessor> ScaleRegisterPlugin::decorateRegisterAccessor(boost::shared_ptr<RegisterAccessor> accessor) {
    return boost::shared_ptr<RegisterAccessor>(new ScaleRegisterPluginAccessor(accessor, scalingFactor));
  }

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginBufferingAccessor<std::string>::read() {
    // read from hardware
    _accessor->read();
    // apply scaling factor while copying buffer from underlying accessor to our buffer
    for(unsigned int i=0; i<BufferingRegisterAccessorImpl<std::string>::cookedBuffer.size(); i++) {
      BufferingRegisterAccessorImpl<std::string>::cookedBuffer[i] = std::to_string(std::stod((*_accessor)[i]) * _scalingFactor);
    }
  }

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginBufferingAccessor<std::string>::write() {
    // apply scaling factor while copying buffer from our buffer to underlying accessor
    for(unsigned int i=0; i<BufferingRegisterAccessorImpl<std::string>::cookedBuffer.size(); i++) {
      (*_accessor)[i] = std::to_string(std::stod(BufferingRegisterAccessorImpl<std::string>::cookedBuffer[i]) / _scalingFactor);
    }
    // write to hardware
    _accessor->write();
  }

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginAccessor::read_impl<std::string>(std::string *convertedData, size_t nWords,
      uint32_t wordOffsetInRegister) const {
    // read from hardware into temporary buffer
    std::vector<std::string> buffer(nWords);
    _accessor->read(buffer.data(), nWords, wordOffsetInRegister);
    // apply scaling factor while copying buffer from underlying accessor to the target buffer
    for(unsigned int i=0; i<nWords; i++) {
      convertedData[i] = std::to_string(std::stod(buffer[i]) * _scalingFactor);
    }
  }

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginAccessor::write_impl<std::string>(const std::string *convertedData, size_t nWords,
      uint32_t wordOffsetInRegister) {
    // create temporary buffer
    std::vector<std::string> buffer(nWords);
    // apply scaling factor while copying buffer from source buffer to temporary buffer
    for(unsigned int i=0; i<nWords; i++) {
      buffer[i] = std::to_string(std::stod(convertedData[i]) / _scalingFactor);
    }
    // write from temporary buffer to hardware
    _accessor->write(buffer.data(), nWords, wordOffsetInRegister);
  }

} /* namespace mtca4u */
