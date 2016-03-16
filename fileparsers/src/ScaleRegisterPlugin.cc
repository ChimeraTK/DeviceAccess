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
          DynamicValue<double> scalingFactor)
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

      virtual FixedPointConverter getFixedPointConverter() const {
        return _accessor->getFixedPointConverter();
      }

    protected:

      /** The underlying register accessor */
      boost::shared_ptr< BufferingRegisterAccessorImpl<T> > _accessor;

      /** The scaling factor */
      DynamicValue<double> _scalingFactor;

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

  ScaleRegisterPlugin::ScaleRegisterPlugin(const std::map<std::string, DynamicValue<std::string> > &parameters) {
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
      const std::map<std::string, DynamicValue<std::string> > &parameters) {
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

} /* namespace mtca4u */
