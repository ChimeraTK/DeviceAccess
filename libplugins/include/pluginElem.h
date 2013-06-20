/* 
 * File:   pluginElem.h
 * Author: apiotro
 *
 * Created on 2 sierpień 2012, 08:47
 */

#ifndef PLUGINELEM_H
#define	PLUGINELEM_H
#include <string>


#include "pluginElem.h"
#include "dbg_print.h"
#include "exPluginElem.h"
#include <dlfcn.h>
#include <iostream>

template<typename T, typename W>
class pluginElem {
private:
    std::string pluginFile;
    W pluginID;
    void *lib_handle;
    void readPluginID();

public:
    pluginElem(const std::string& _pluginFile);
    virtual ~pluginElem();

    W getPluginID();
    std::string getPluginFileName();
    T* createPluginObject();
    void destroyPluginObject(T* obj);
private:

};

template<typename T, typename W>
pluginElem<T, W>::pluginElem(const std::string& _pluginFile)
: pluginFile(_pluginFile) { 
    lib_handle = dlopen(pluginFile.c_str(), RTLD_LAZY);
    if (!lib_handle) {
        throw exPluginElem(std::string("ERROR: ") + dlerror(), exPluginElem::EX_CANNOT_OPEN_PLUGIN);
    }
    dlerror();
    readPluginID();
}

template<typename T, typename W>
void pluginElem<T, W>::readPluginID() {
    
    W (*getPluginID)();    
    getPluginID = (W (*)())dlsym(lib_handle, "getPluginID");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        throw exPluginElem(std::string("ERROR: ") + dlsym_error, exPluginElem::EX_CANNOT_LOAD_FUNCTION);
    }
    pluginID = getPluginID();   
}

template<typename T, typename W>
T* pluginElem<T, W>::createPluginObject()
{   
    T* (*create)();
    T* obj;
    
    create = (T* (*)())dlsym(lib_handle, "create");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        throw exPluginElem(std::string("ERROR: ") + dlsym_error, exPluginElem::EX_CANNOT_LOAD_FUNCTION);
    }
    obj = create();
    return obj;
}

template<typename T, typename W>
void pluginElem<T, W>::destroyPluginObject(T* obj)
{   
    void (*destroy)(T*);

    destroy = (void (*)(T*))dlsym(lib_handle, "destroy");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        throw exPluginElem(std::string("ERROR: ") + dlsym_error, exPluginElem::EX_CANNOT_LOAD_FUNCTION);
    }
    destroy(obj);
}

template<typename T, typename W>
pluginElem<T, W>::~pluginElem() {
    dlclose(lib_handle);
}

template<typename T, typename W>
W pluginElem<T, W>::getPluginID() {
    return pluginID;
}

template<typename T, typename W>
std::string pluginElem<T, W>::getPluginFileName() {
    return pluginFile;
}

#endif	/* PLUGINELEM_H */

