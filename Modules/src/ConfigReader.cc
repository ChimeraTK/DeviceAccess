#include <libxml++/libxml++.h>

#include "ConfigReader.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  /** Functor to fill variableMap */
  struct FunctorFill {
    FunctorFill(ConfigReader *owner, const std::string &type, const std::string &name, const std::string &value, bool &processed)
    : _owner(owner), _type(type), _name(name), _value(value), _processed(processed)
    {}
    
    template<typename PAIR>
    void operator()(PAIR&) const {
      
      // extract the user type from the pair
      typedef typename PAIR::first_type T;
      
      // skip this type, if not matching the type string in the config file
      if(_type != boost::fusion::at_key<T>(_owner->typeMap)) return;
      
      _owner->createVar<T>(_name, _value);
      _processed = true;
    }
    
    ConfigReader *_owner;
    const std::string &_type, &_name, &_value;
    bool &_processed;   // must be a non-const reference, since we want to return this to the caller
    
    typedef boost::fusion::pair<std::string,ConfigReader::Var<std::string>> StringPair;
  };

  /*********************************************************************************************************************/

  template<typename T>
  void ConfigReader::createVar(const std::string &name, const std::string &value) {
    // convert value into user type
    /// @todo error handling!
    std::stringstream stream(value);
    T convertedValue;
    if(typeid(T) == typeid(int8_t) || typeid(T) == typeid(uint8_t)) {       // prevent interpreting int8-types as characters
      int16_t intermediate;
      stream >> intermediate;
      convertedValue = intermediate;
    }
    else {      // note: string is done in template specialisation
      stream >> convertedValue;
    }
    
    // place the variable onto the vector
    std::map<std::string, ConfigReader::Var<T>> &theMap = boost::fusion::at_key<T>(variableMap.table);
    theMap.emplace(std::make_pair(name, ConfigReader::Var<T>(this, name, convertedValue)));
  }

  /*********************************************************************************************************************/

  template<>
  void ConfigReader::createVar<std::string>(const std::string &name, const std::string &value) {
    // place the variable onto the vector
    std::map<std::string, ConfigReader::Var<std::string>> &theMap = boost::fusion::at_key<std::string>(variableMap.table);
    theMap.emplace(std::make_pair(name, ConfigReader::Var<std::string>(this, name, value)));
  }

  /*********************************************************************************************************************/

  ConfigReader::ConfigReader(EntityOwner *owner, const std::string &name, const std::string &fileName,
                             const std::unordered_set<std::string> &tags)
  : ApplicationModule(owner, name, "Configuration read from file '"+fileName+"'", false, tags),
    _fileName(fileName)
  {
    // parse the file into a DOM structure
    xmlpp::DomParser parser;
    try {
      parser.parse_file(fileName);
    }
    catch(xmlpp::exception &e) { /// @todo change exception!
      throw std::runtime_error("ConfigReader: Error opening the config file '"+fileName+"': "+e.what());
    }
    
    // get root element
    const auto root = parser.get_document()->get_root_node();
    if(root->get_name() != "configuration") {
      parsingError("Expected 'configuration' tag instead of: "+root->get_name());
    }

    // parsing loop
    for(const auto& child : root->get_children()) {
      // cast into element, ignore if not an element (e.g. comment)
      const xmlpp::Element *element = dynamic_cast<const xmlpp::Element*>(child);
      if(!element) continue;
      if(element->get_name() != "variable") {
        parsingError("Expected 'variable' tag instead of: "+root->get_name());
      }

      // obtain attributes from the element
      auto name = element->get_attribute("name");
      if(!name) parsingError("Missing attribute 'name' for the 'variable' tag.");
      auto type = element->get_attribute("type");
      if(!type) parsingError("Missing attribute 'type' for the 'variable' tag.");
      auto value = element->get_attribute("value");
      if(!value) parsingError("Missing attribute 'value' for the 'variable' tag.");
      
      // create accessor and store value in map using the functor
      bool processed{false};
      boost::fusion::for_each( variableMap.table,
                               FunctorFill(this, type->get_value(), name->get_value(), value->get_value(), processed) );
      if(!processed) parsingError("Incorrect value '"+type->get_value()+"' for attribute 'type' of the 'variable' tag.");
    }
  }

  /********************************************************************************************************************/

  void ConfigReader::parsingError(const std::string &message) {
    throw std::runtime_error("ConfigReader: Error parsing the config file '"+_fileName+"': "+message);
  }

  /*********************************************************************************************************************/

  /** Functor to set values to the accessors */
  struct FunctorSetValues {
    FunctorSetValues(ConfigReader *owner) : _owner(owner) {}
    
    template<typename PAIR>
    void operator()(PAIR&) const {
      
      // get user type and vector
      typedef typename PAIR::first_type T;
      std::map<std::string, ConfigReader::Var<T>> &theMap = boost::fusion::at_key<T>(_owner->variableMap.table);
      
      // iterate vector and set values
      for(auto &pair : theMap) {
        auto &var = pair.second;
        var._accessor = var._value;
        var._accessor.write();
      }
      
    }
    
    ConfigReader *_owner;
  };

  /*********************************************************************************************************************/

  void ConfigReader::mainLoop() {
    std::cout << "ConfigReader::mainLoop()" << std::endl;
    boost::fusion::for_each( variableMap.table, FunctorSetValues(this) );
  }

  /*********************************************************************************************************************/

} // namespace ChimeraTK

