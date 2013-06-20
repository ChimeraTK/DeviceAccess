#include "libplugins.h"
#include "exBase.h"
#include <iostream>
#include "plugins/include/pluginBase.h"

int main() {
    try {
        pluginManager<pluginBase, int> pmg;
        pmg.loadPlugins("./plugins");
        pluginBase* elem = pmg.getPluginObject(7);
        elem->doSomething();
        pmg.destroyPluginObject(7, elem);
        elem = pmg.getPluginObject(77);        
        elem->doSomething();
        pmg.destroyPluginObject(77, elem);
        pmg.destroyPluginObject(777, elem);
    } catch (exBase &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    return 0;
}
