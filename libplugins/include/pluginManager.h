/* 
 * File:   pluginManager.h
 * Author: apiotro
 *
 * Created on 1 sierpień 2012, 18:43
 */

#ifndef PLUGINMANAGER_H
#define	PLUGINMANAGER_H

#include "refCountPointer.h"
#include "pluginElem.h"
#include <string>

#include "pluginList.h"

#include "exPluginManager.h"
#include "dbg_print.h"
#include <dirent.h>

template<typename T, typename W>
class pluginManager {
private:
    pluginList<pluginElem<T, W>*, W> plist;
public:
    pluginManager();
    virtual ~pluginManager();
public:
    T* getPluginObject(W pluginID);
    void loadPlugins(const std::string &dir);
    void destroyPluginObject(W pluginID, T* obj);
};

template<typename T, typename W>
pluginManager<T, W>::pluginManager() {
}

template<typename T, typename W>
pluginManager<T, W>::~pluginManager() {
}

template<typename T, typename W>
void pluginManager<T, W>::loadPlugins(const std::string &dir) {
    DIR *dp;
    struct dirent *dirp;
    std::string dir_new = dir;
    std::string file_name;
    size_t found;
    pluginElem<T, W>* newPlugin;

    if (dir[dir.length() - 1] != '/')
        dir_new += "/";
    if ((dp = opendir(dir.c_str())) == NULL) {
        throw exPluginManager("Cannot open directory: \"" + dir + "\"", exPluginManager::EX_NO_PLUGIN_DIRECTORY);
    }
    while ((dirp = readdir(dp)) != NULL) {
        if (dirp->d_type != DT_REG && dirp->d_type != DT_LNK)
            continue;
        file_name = std::string(dirp->d_name);
        found = file_name.find_last_of(".");
        if (found == std::string::npos)
            continue;
        if (file_name.substr(found) == ".so") {
            try {
                newPlugin = new pluginElem<T, W>(dir_new + file_name);
                plist.insert(newPlugin);
            } catch (exPluginElem &ex) {
                dbg_print("PROBLEM WITH PLUGIN FILE: %s\n", ex.what());
            }
        }
    }
    closedir(dp);
}

template<typename T, typename W>
T* pluginManager<T, W>::getPluginObject(W pluginID) {
    pluginElem<T, W>* pelem = plist.getPluginElem(pluginID);
    return pelem->createPluginObject();
}

template<typename T, typename W>
void pluginManager<T, W>::destroyPluginObject(W pluginID, T* obj) {
    pluginElem<T, W>* pelem = plist.getPluginElem(pluginID);
    pelem->destroyPluginObject(obj);
}

#endif	/* PLUGINMANAGER_H */

