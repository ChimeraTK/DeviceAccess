#pragma once

#include "Visitor.h"

#include <functional> // for std::reference_wrapper
#include <list>
#include <ostream>

namespace ChimeraTK {

// Forward declarations
class VariableNetworkNode;

/**
 * @brief A helper class to replace the output stream temporarily
 *
 * This is a helper class that is used in the Graphviz dumper to be able to dump
 * the nodes to a stringstream instead of directly to the file.
 *
 * Ideally, the pushStream()/popStream() functions should be called in pairs but
 * popStream() will do nothing if the stack is empty.
 */
class PushableStream {
public:
  PushableStream(std::ostream &stream) : _streamStack{stream} {}
  virtual ~PushableStream() {}

  void pushStream(std::ostream &stream) { _streamStack.push_back(stream); }

  std::ostream &stream() { return _streamStack.back().get(); }

  void popStream() {
    if (_streamStack.size() == 1)
      return;

    _streamStack.pop_back();
  }

private:
  std::list<std::reference_wrapper<std::ostream>> _streamStack;
};

/**
 * @brief The VariableNetworkNodeDumpingVisitor class
 *
 * This class is serving as one of the base classes for the Graphviz dumper as
 * well as the textual dumper providing detailed information about a node.
 */
class VariableNetworkNodeDumpingVisitor : public Visitor<VariableNetworkNode>,
                                          public PushableStream {
public:
  /**
   * @brief VariableNetworkNodeDumpingVisitor::VariableNetworkNodeDumpingVisitor
   * @param stream instance of std::ostream to write to
   * @param separator the separator to use
   *
   * Separator is used to be able to use the function in the Graphviz and
   * textual connection dumper. We are using newlines for Graphviz, and space
   * for textual
   */
  VariableNetworkNodeDumpingVisitor(std::ostream &stream,
                                    const std::string &separator);
  virtual ~VariableNetworkNodeDumpingVisitor() {}

  /**
   * @brief dispatch
   * @param t Node to visit
   *
   * Visitor function for VariableNetworkNode. Will dump a verbose description
   * of the node
   */
  void dispatch(const VariableNetworkNode &t);

private:
  std::string _separator;
};

} // namespace ChimeraTK
