#pragma once

#include "Visitor.h"

#include <memory>
#include <string>

// Forward declarations
namespace xmlpp {
  class Document;
  class Element;
} // namespace xmlpp

namespace ChimeraTK {
  // Forward declarations
  class Application;
  class VariableNetworkNode;

  /**
   * @brief The XMLGeneratorVisitor class
   *
   * This class is responsible for generating the XML representation of the
   * Variables in an Application
   */
  class XMLGeneratorVisitor : public Visitor<Application, VariableNetworkNode> {
   public:
    XMLGeneratorVisitor();
    virtual ~XMLGeneratorVisitor() {}
    void dispatch(const Application& app);
    void dispatch(const VariableNetworkNode& node);

    void save(const std::string& filename);

   private:
    std::shared_ptr<xmlpp::Document> _doc;
    xmlpp::Element* _rootElement;
  };

} // namespace ChimeraTK
