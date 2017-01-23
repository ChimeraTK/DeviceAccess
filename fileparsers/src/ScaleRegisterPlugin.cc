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
  template<typename UserType>
  class ScaleRegisterPluginRegisterAccessor : public NDRegisterAccessor<UserType> {
    public:

      /** The constructor takes the original accessor and the scaling factor as arguments */
      ScaleRegisterPluginRegisterAccessor(boost::shared_ptr< NDRegisterAccessor<UserType> > accessor,
          DynamicValue<double> scalingFactor)
      : _accessor(accessor), _scalingFactor(scalingFactor)
      {
        // reserve space for cooked buffer
        NDRegisterAccessor<UserType>::buffer_2D.resize(_accessor->getNumberOfChannels());
        NDRegisterAccessor<UserType>::buffer_2D[0].resize(_accessor->getNumberOfSamples());
      }

      virtual ~ScaleRegisterPluginRegisterAccessor() {};

      void postRead() {
        // apply scaling factor while copying buffer from underlying accessor to our buffer
        for(unsigned int i=0; i<NDRegisterAccessor<UserType>::buffer_2D.size(); i++) {
          for(unsigned int k=0; k<NDRegisterAccessor<UserType>::buffer_2D[i].size(); k++) {
            NDRegisterAccessor<UserType>::buffer_2D[i][k] = _accessor->accessData(i,k) * _scalingFactor;
          }
        }
      }
    
      virtual void doReadTransfer() {
        _accessor->read();
      }

      virtual bool doReadTransferNonBlocking() {
	return _accessor->readNonBlocking();
      }

      virtual void preWrite() {
        // apply scaling factor while copying buffer from our buffer to underlying accessor
        for(unsigned int i=0; i<NDRegisterAccessor<UserType>::buffer_2D.size(); i++) {
          for(unsigned int k=0; k<NDRegisterAccessor<UserType>::buffer_2D[i].size(); k++) {
            _accessor->accessData(i,k) = NDRegisterAccessor<UserType>::buffer_2D[i][k] / _scalingFactor;
          }
        }
      }

      virtual void write() {
        preWrite();
        _accessor->write();
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const ScaleRegisterPluginRegisterAccessor<UserType> >(other);
        if(!rhsCasted) return false;
        if(_accessor != rhsCasted->_accessor) return false;
        if(_scalingFactor != rhsCasted->_scalingFactor) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return _accessor->isReadOnly();
      }

      virtual bool isReadable() const {
        return true;
      }

      virtual bool isWriteable() const {
        return true;
      }

     virtual FixedPointConverter getFixedPointConverter() const {
        return _accessor->getFixedPointConverter();
      }

    protected:

      /** The underlying register accessor */
      boost::shared_ptr< NDRegisterAccessor<UserType> > _accessor;

      /** The scaling factor */
      DynamicValue<double> _scalingFactor;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _accessor->getHardwareAccessingElements();
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        if(_accessor->isSameRegister(newElement)) {
          _accessor = boost::static_pointer_cast< NDRegisterAccessor<UserType> >(newElement);
        }
      }

  };

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::postRead();

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::preWrite();

  /********************************************************************************************************************/

  ScaleRegisterPlugin::ScaleRegisterPlugin(const std::map<std::string, DynamicValue<std::string> > &parameters) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(decorateRegisterAccessor_impl);
    try {
      scalingFactor = parameters.at("factor");
    }
    catch(std::out_of_range &e) {
      throw DeviceException("ScaleRegisterPlugin: Missing parameter 'factor'.", DeviceException::WRONG_PARAMETER);
    }
  }

  /********************************************************************************************************************/

  boost::shared_ptr<RegisterInfoPlugin> ScaleRegisterPlugin::createInstance(
      const std::map<std::string, DynamicValue<std::string> > &parameters) {
    return boost::shared_ptr<RegisterInfoPlugin>(new ScaleRegisterPlugin(parameters));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< NDRegisterAccessor<UserType> > ScaleRegisterPlugin::decorateRegisterAccessor_impl(
      boost::shared_ptr< NDRegisterAccessor<UserType> > accessor) const {
    return boost::shared_ptr< NDRegisterAccessor<UserType> >(
        new ScaleRegisterPluginRegisterAccessor<UserType>(accessor, scalingFactor));
  }

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::postRead() {
    // apply scaling factor while copying buffer from underlying accessor to our buffer
    for(unsigned int i=0; i<NDRegisterAccessor<std::string>::buffer_2D.size(); i++) {
      for(unsigned int k=0; k<NDRegisterAccessor<std::string>::buffer_2D[i].size(); k++) {
        NDRegisterAccessor<std::string>::buffer_2D[i][k] =
            std::to_string(std::stod(_accessor->accessData(i,k)) * _scalingFactor);
      }
    }
  }

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::preWrite() {
    // apply scaling factor while copying buffer from our buffer to underlying accessor
    for(unsigned int i=0; i<NDRegisterAccessor<std::string>::buffer_2D.size(); i++) {
      for(unsigned int k=0; k<NDRegisterAccessor<std::string>::buffer_2D[i].size(); k++) {
        _accessor->accessData(i,k) =
            std::to_string(std::stod(NDRegisterAccessor<std::string>::buffer_2D[i][k]) / _scalingFactor);
      }
    }
  }

} /* namespace mtca4u */
