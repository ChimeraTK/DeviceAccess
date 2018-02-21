/*
 * ScaleRegisterPlugin.cc
 *
 *  Created on: Feb 26, 2016
 *      Author: Martin Hierholzer
 */

#include "RegisterPluginFactory.h"
#include "ScaleRegisterPlugin.h"
#include "NDRegisterAccessorDecorator.h"

namespace ChimeraTK {

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
  class ScaleRegisterPluginRegisterAccessor : public NDRegisterAccessorDecorator<UserType> {
    public:

      /** The constructor takes the original accessor and the scaling factor as arguments */
      ScaleRegisterPluginRegisterAccessor(boost::shared_ptr< NDRegisterAccessor<UserType> > accessor,
          DynamicValue<double> scalingFactor)
      : NDRegisterAccessorDecorator<UserType>(accessor), _scalingFactor(scalingFactor)
      {}

      void doPostRead() override {
        _target->postRead();
        // apply scaling factor while copying buffer from underlying accessor to our buffer
        for(unsigned int i=0; i<NDRegisterAccessor<UserType>::buffer_2D.size(); i++) {
          for(unsigned int k=0; k<NDRegisterAccessor<UserType>::buffer_2D[i].size(); k++) {
            NDRegisterAccessor<UserType>::buffer_2D[i][k] = _target->accessData(i,k) * _scalingFactor;
          }
        }
      }

      void doPreWrite() override {
        // apply scaling factor while copying buffer from our buffer to underlying accessor
        for(unsigned int i=0; i<NDRegisterAccessor<UserType>::buffer_2D.size(); i++) {
          for(unsigned int k=0; k<NDRegisterAccessor<UserType>::buffer_2D[i].size(); k++) {
            _target->accessData(i,k) = NDRegisterAccessor<UserType>::buffer_2D[i][k] / _scalingFactor;
          }
        }
        _target->preWrite();
      }

    protected:

      /** The scaling factor */
      DynamicValue<double> _scalingFactor;

      using NDRegisterAccessorDecorator<UserType>::_target;

  };

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::doPostRead();

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::doPreWrite();

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
  void ScaleRegisterPluginRegisterAccessor<std::string>::doPostRead() {
    // apply scaling factor while copying buffer from underlying accessor to our buffer
    for(unsigned int i=0; i<NDRegisterAccessor<std::string>::buffer_2D.size(); i++) {
      for(unsigned int k=0; k<NDRegisterAccessor<std::string>::buffer_2D[i].size(); k++) {
        NDRegisterAccessor<std::string>::buffer_2D[i][k] =
            std::to_string(std::stod(_target->accessData(i,k)) * _scalingFactor);
      }
    }
  }

  /********************************************************************************************************************/

  template<>
  void ScaleRegisterPluginRegisterAccessor<std::string>::doPreWrite() {
    // apply scaling factor while copying buffer from our buffer to underlying accessor
    for(unsigned int i=0; i<NDRegisterAccessor<std::string>::buffer_2D.size(); i++) {
      for(unsigned int k=0; k<NDRegisterAccessor<std::string>::buffer_2D[i].size(); k++) {
        _target->accessData(i,k) =
            std::to_string(std::stod(NDRegisterAccessor<std::string>::buffer_2D[i][k]) / _scalingFactor);
      }
    }
  }

} /* namespace ChimeraTK */
