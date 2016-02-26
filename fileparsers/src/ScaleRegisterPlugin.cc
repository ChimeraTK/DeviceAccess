/*
 * ScaleRegisterPlugin.cc
 *
 *  Created on: Feb 26, 2016
 *      Author: Martin Hierholzer
 */

#include "RegisterPluginFactory.h"
#include "ScaleRegisterPlugin.h"

namespace mtca4u {

  /********************************************************************************************************************/

  /** The register accessor used by the ScaleRegisterPlugin */
  template<typename T>
  class ScaleRegisterPluginAccessor : public BufferingRegisterAccessorImpl<T> {
    public:

      /** The constructor takes the original accessor and the scaling factor as arguments */
      ScaleRegisterPluginAccessor(boost::shared_ptr< BufferingRegisterAccessorImpl<T> > accessor,
          Value<double> scalingFactor)
      : _accessor(accessor), _scalingFactor(scalingFactor)
      {}

      virtual ~ScaleRegisterPluginAccessor() {};

      virtual void read() {
        // read from hardware
        _accessor->read();
        // apply scaling factor while copying buffer from underlying accessor to our buffer
        auto itTarget = BufferingRegisterAccessorImpl<T>::cookedBuffer.begin();
        for(auto itSource = _accessor->begin(); itSource != _accessor->end(); ++itSource) {
          *itTarget = (*itSource) * _scalingFactor;
          ++itTarget;
        }
      }

      virtual void write() {
        // apply scaling factor while copying buffer from our buffer to underlying accessor
        auto itSource = BufferingRegisterAccessorImpl<T>::cookedBuffer.begin();
        for(auto itTarget = _accessor->begin(); itTarget != _accessor->end(); ++itTarget) {
          *itTarget = (*itSource) / _scalingFactor;
          ++itSource;
        }
        // write to hardware
        _accessor->write();
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const ScaleRegisterPluginAccessor<T> >(other);
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
  void ScaleRegisterPluginAccessor<std::string>::read();

  template<>
  void ScaleRegisterPluginAccessor<std::string>::write();

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

  ScaleRegisterPlugin::ScaleRegisterPlugin(const std::map<std::string, Value<std::string> > &parameters) {
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
  boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >* ScaleRegisterPlugin::getBufferingRegisterAccessorImpl(
      void *accessor_ptr) {
    auto accessor = static_cast<boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >*>(accessor_ptr);
    boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > *newAccessor = new boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >();
    newAccessor->reset(new ScaleRegisterPluginAccessor<UserType>(*accessor, scalingFactor) );
    delete accessor; // just the shared pointer, which was already copied
    return newAccessor;
  }

  VIRTUAL_FUNCTION_TEMPLATE_IMPLEMENTER(ScaleRegisterPlugin, getBufferingRegisterAccessorImpl,
      getBufferingRegisterAccessorImpl, void*)

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginAccessor<std::string>::read() {
    // read from hardware
    _accessor->read();
    // apply scaling factor while copying buffer from underlying accessor to our buffer
    auto itTarget = BufferingRegisterAccessorImpl<std::string>::cookedBuffer.begin();
    for(auto itSource = _accessor->begin(); itSource != _accessor->end(); ++itSource) {
      *itTarget = std::to_string(std::stod(*itSource) * _scalingFactor);
      ++itTarget;
    }
  }

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginAccessor<std::string>::write() {
    // apply scaling factor while copying buffer from our buffer to underlying accessor
    auto itSource = BufferingRegisterAccessorImpl<std::string>::cookedBuffer.begin();
    for(auto itTarget = _accessor->begin(); itTarget != _accessor->end(); ++itTarget) {
      *itTarget = std::to_string(std::stod(*itSource) / _scalingFactor);
      ++itSource;
    }
    // write to hardware
    _accessor->write();
  }

} /* namespace mtca4u */
