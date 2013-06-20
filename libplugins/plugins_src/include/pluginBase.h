/* 
 * File:   pluginBase.h
 * Author: apiotro
 *
 * Created on 2 sierpień 2012, 08:57
 */

#ifndef PLUGINBASE_H
#define	PLUGINBASE_H

class pluginBase {
public:
    pluginBase();
    virtual ~pluginBase();
    virtual void doSomething() = 0;
private:

};

#endif	/* PLUGINBASE_H */

