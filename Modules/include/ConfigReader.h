/*
 *  Generic module to read an XML config file and provide the defined values as constant variables
 */

#ifndef CHIMERATK_APPLICATION_CORE_CONFIG_READER_H
#define CHIMERATK_APPLICATION_CORE_CONFIG_READER_H

#include <map>

#include <mtca4u/SupportedUserTypes.h>

#include "ApplicationCore.h"

namespace ChimeraTK {

  struct FunctorFill;
  struct FunctorSetValues;

  struct ConfigReader : ApplicationModule {
        
      ConfigReader(EntityOwner *owner, const std::string &name, const std::string &fileName,
                  const std::unordered_set<std::string> &tags={});
      
      void mainLoop() override;

    protected:
      
      /** File name */
      std::string _fileName;

      /** throw a parsing error with more information */
      void parsingError(const std::string &message);
      
      /** Class holding the value and the accessor for one configuration variable */
      template<typename T>
      struct Var {
          Var(Module *owner, const std::string &name, const T &value)
          : _accessor(owner, name, "unknown", "Configuration variable"),
          _value(value)
          {}

          ScalarOutput<T> _accessor;
          T _value;
      };
      
      /** Create an instance of Var<T> and place it on the variableMap */
      template<typename T>
      void createVar(const std::string &name, const std::string &value);
      
      /** Define type for vector of Var, so we can put it into the TemplateUserTypeMap */
      template<typename T>
      using VectorOfVar = std::vector<Var<T>>;
      
      /** Type-depending map of vectors of variables */
      mtca4u::TemplateUserTypeMap<VectorOfVar> variableMap;
      
      /** Map assigning string type identifyers to C++ types */
      mtca4u::SingleTypeUserTypeMap<const char*> typeMap{"int8","uint8","int16","uint16","int32","uint32",
                                                         "int64","uint64","float","double","string"};
                                                         
      friend struct FunctorFill;
      friend struct FunctorSetValues;
                                                         
  };

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_CONFIG_READER_H */
