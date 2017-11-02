#ifndef CHIMERATK_APPLICATION_CORE_CONFIG_READER_H
#define CHIMERATK_APPLICATION_CORE_CONFIG_READER_H

#include <map>

#include <mtca4u/SupportedUserTypes.h>

#include "ApplicationCore.h"

namespace ChimeraTK {

  struct FunctorFill;
  struct FunctorSetValues;

  /**
   *  Generic module to read an XML config file and provide the defined values as constant variables. The config file
   *  should look like this:
   *  \code{.xml}
<configuration>
  <variable name="variableName" type="int32" value="42"/>
  <variable name="anotherVariable" type="string" value="Hello world!"/>
</configuration>
      \endcode
   *
   *  Outputs are created for each variable, so they can be connected to other modules. All values will be provided to
   *  the receivers already in the preparation phase, so no read() must be called. Updates will never be sent, so any
   *  blocking read operation on the receivers will block forever.
   *
   *  Configuration values can already be accessed during the Application::defineConnection() function by using the
   *  ConfigReader::get() function.
   */
  struct ConfigReader : ApplicationModule {
        
      ConfigReader(EntityOwner *owner, const std::string &name, const std::string &fileName,
                  const std::unordered_set<std::string> &tags={});
      
      void mainLoop() override {}
      void prepare() override;
      
      /** Get value for given configuration variable. This is already accessible right after construction of this
       *  object. Throws std::out_of_range if variable doesn't exist. */
      template<typename T>
      const T& get(const std::string &variableName) const;

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
      
      /** Define type for map of std::string to Var, so we can put it into the TemplateUserTypeMap */
      template<typename T>
      using MapOfVar = std::map<std::string, Var<T>>;
      
      /** Type-depending map of vectors of variables */
      mtca4u::TemplateUserTypeMap<MapOfVar> variableMap;
      
      /** Map assigning string type identifyers to C++ types */
      mtca4u::SingleTypeUserTypeMap<const char*> typeMap{"int8","uint8","int16","uint16","int32","uint32",
                                                         "int64","uint64","float","double","string"};
                                                         
      friend struct FunctorFill;
      friend struct FunctorSetValues;
                                                         
  };

  /*********************************************************************************************************************/
  /*********************************************************************************************************************/

  template<typename T>
  const T& ConfigReader::get(const std::string &variableName) const {
    try {
      return boost::fusion::at_key<T>(variableMap.table).at(variableName)._value;
    }
    catch(std::out_of_range &e) {
      throw(std::out_of_range("ConfigReader: Cannot find a configuration variable of the name '"+
                              variableName+"' in the config file '"+_fileName+"'."));
    }
  }

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_CONFIG_READER_H */
