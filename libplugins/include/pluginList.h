/* 
 * File:   pluginList.h
 * Author: apiotro
 *
 * Created on 2 sierpień 2012, 08:46
 */

#ifndef PLUGINLIST_H
#define	PLUGINLIST_H

#include "pluginElem.h"
#include "exPluginList.h"
#include <sstream>
#include <map>


template<typename T, typename W>
class pluginList {
private:
    std::map<W, T> pluginMap;
public:
    pluginList();
    virtual ~pluginList();
    
    void insert(T elem);
    T getPluginElem(W pluginID);
private:

};

template<typename T, typename W>
pluginList<T, W>::pluginList() {
}

template<typename T, typename W>
pluginList<T, W>::~pluginList() {
    typename std::map<W, T>::const_iterator iter;    
    for (iter = pluginMap.begin(); iter != pluginMap.end(); ++iter){
        delete (*iter).second;
    }
}

template<typename T, typename W>
void pluginList<T, W>::insert(T elem) {
    typename std::map<W, T>::const_iterator iter;
    
    iter = pluginMap.find(elem->getPluginID());
    if (iter == pluginMap.end()){
        pluginMap.insert(std::pair<W, T>(elem->getPluginID(), elem));
    } else {
        throw exPluginList("ERROR: plugins from files " +  elem->getPluginFileName() + " and " + (*iter).second->getPluginFileName() + " have the same ID.", exPluginList::EX_DUPLICATED_PLUGIN);
    }
}

template<typename T, typename W>
T pluginList<T, W>::getPluginElem(W pluginID) {
    typename std::map<W, T>::const_iterator iter;
    std::ostringstream os;
    iter = pluginMap.find(pluginID);
    if (iter == pluginMap.end()){
        os << "ERROR: cannot find plugin with ID equal to " << pluginID;
        throw exPluginList(os.str(), exPluginList::EX_NO_PLUGIN_AVAILABLE);
    } else {
        return pluginMap[pluginID];
    }
}

#endif	/* PLUGINLIST_H */

