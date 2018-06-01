#pragma once

#include <string>
#include <iostream>
#include <list>
#include <map>
#include <memory>

namespace xmlpp {
    class Document;
    class Element;
}


namespace ChimeraTK {
class Application;
class VariableNetwork;
class VariableNetworkNode;
class EntityOwner;
class Module;

/* Losely based on https://stackoverflow.com/questions/11796121/implementing-the-visitor-pattern-using-c-templates#11802080 */

template <typename... Types>
class Visitor;

template <typename T>
class Visitor<T> {
public:
    virtual void dispatch(const T& t) = 0;
};

template <typename T, typename... Types>
class Visitor<T, Types...> : public Visitor<T>, public Visitor<Types...> {
public:
    using Visitor<Types...>::dispatch;
    using Visitor<T>::dispatch;
};


/**
 * @brief A helper class to replace the output stream temporarily
 *
 * This is a helper class that is used in the Graphviz dumper to be able to dump the nodes to a stringstream
 * instead of directly to the file.
 *
 * Ideally, the pushStream()/popStream() functions should be called in pairs but popStream() will do nothing
 * if the stack is empty.
 */
class PushableStream {
public:
    PushableStream(std::ostream &stream) : _streamStack(), _stream(stream) {}
    virtual ~PushableStream() {}

    void pushStream(std::ostream& stream) {
        _streamStack.push_back(_stream);
        _stream = stream;
    }

    std::ostream& stream() { return _stream.get(); }

    void popStream() {
        if (_streamStack.empty())
            return;

        _stream = _streamStack.back();
        _streamStack.pop_back();
    }
private:
    std::list<std::reference_wrapper<std::ostream>> _streamStack;
    std::reference_wrapper<std::ostream> _stream;
};

/**
 * @brief The VariableNetworkNodeDumpingVisitor class
 *
 * This class is serving as one of the base classes for the Graphviz dumper as well as the textual dumper
 * providing detailed information about a node.
 */
class VariableNetworkNodeDumpingVisitor : public Visitor<VariableNetworkNode>, public PushableStream {
public:
    /**
     * @brief VariableNetworkNodeDumpingVisitor::VariableNetworkNodeDumpingVisitor
     * @param stream instance of std::ostream to write to
     * @param separator the separator to use
     *
     * Separator is used to be able to use the function in the Graphviz and textual connection dumper.
     * We are using newlines for Graphviz, and space for textual
     */
    VariableNetworkNodeDumpingVisitor(std::ostream &stream, const std::string& separator);
    virtual ~VariableNetworkNodeDumpingVisitor() {}

    /**
     * @brief dispatch
     * @param t Node to visit
     *
     * Visitor function for VariableNetworkNode. Will dump a verbose description of the node
     */
    void dispatch(const VariableNetworkNode& t);

private:
    std::string _separator;
};

/**
 * @brief The VariableNetworkDumpingVisitor class
 *
 * This class provides a textual dump of the VariableNetwork
 */
class VariableNetworkDumpingVisitor : public Visitor<VariableNetwork>, public VariableNetworkNodeDumpingVisitor {
public:
    VariableNetworkDumpingVisitor(const std::string &prefix, std::ostream &stream);
    virtual ~VariableNetworkDumpingVisitor() {}
    void dispatch(const VariableNetwork& t);
    using Visitor<VariableNetworkNode>::dispatch;

private:
    std::string _prefix;
};

/**
 * @brief The VariableNetworkGraphDumpingVisitor class
 *
 * This class provides a Graphiviz dump of the VariableNetwork.
 * Due to the potential size of the resulting graph, it is recommended to use SVG for
 * rendering the resulting graph.
 */
class VariableNetworkGraphDumpingVisitor : public Visitor<Application, VariableNetwork>, VariableNetworkNodeDumpingVisitor {
public:
    VariableNetworkGraphDumpingVisitor(std::ostream& stream);
    virtual ~VariableNetworkGraphDumpingVisitor() {}
    void dispatch(const Application& t);
    void dispatch(const VariableNetwork& t);
    void dispatch(const VariableNetworkNode& t);
private:
    std::map<std::string, std::string> _triggerMap;
    std::list<std::string> _triggerConnections;
    std::list<std::string> _prefix;
    unsigned _networkCount;
    unsigned _triggerCount;

    std::string prefix() { return _prefix.back(); }
    void pushPrefix(const std::string& prefix) { _prefix.push_back(prefix); }
    void popPrefix() { _prefix.pop_back(); }
};

/**
 * @brief The XMLGeneratorVisitor class
 *
 * This class is responsible for generating the XML representation of the Variables in an Application
 */
class XMLGeneratorVisitor : public Visitor<Application, VariableNetworkNode> {
public:
    XMLGeneratorVisitor();
    virtual ~XMLGeneratorVisitor() {}
    void dispatch(const Application& app);
    void dispatch(const VariableNetworkNode &node);

    void save(const std::string &filename);
private:
    std::shared_ptr<xmlpp::Document> _doc;
    xmlpp::Element *_rootElement;
};

/**
 * @brief The ModuleGraphVisitor class
 *
 * This class is responsible for generating the Graphiviz representation of the module hierarchy.
 */
class ModuleGraphVisitor : public Visitor<EntityOwner, Module, VariableNetworkNode> {
public:
    ModuleGraphVisitor(std::ostream& stream, bool showVariables = true);
    virtual ~ModuleGraphVisitor() {}

    void dispatch(const EntityOwner &owner);
    void dispatch(const Module &module);
    void dispatch(const VariableNetworkNode &node);
private:
    std::ostream& _stream;
    bool _showVariables;

    void dumpEntityOwner(const EntityOwner &owner);
};
}
