
#ifndef LOGICNAMEMAPPER_H
#define	LOGICNAMEMAPPER_H

#include <string>
#include <map>
#include <vector>
#include "logicNameData.h"

class logicNameMapper {
public:  
   class iterator {
    private:
        std::map<std::string, logicNameData*>::iterator iter;
    public:
        iterator();
        iterator(std::map<std::string, logicNameData*>::iterator _iter);
        bool operator!=(const iterator& _iter);
        iterator& operator++();
        logicNameData* operator*();
    }; 
private:
    std::map<std::string, logicNameData*> logicNameMapping;
    std::string                           logicMapFileName;
    std::vector<std::string> parseRegularExpr(const std::string& val, const std::string& fileName, const std::string& line, size_t lineNr, size_t& posOpen, size_t& posClose);
public:
    logicNameMapper();
    virtual ~logicNameMapper();
    
    void parse(const std::string& fileName);
    logicNameData* get(const std::string& logName) const;
    
    
    iterator begin();
    iterator end();
    
    std::string getMapFileName() const;
    
#ifdef __DEBUG_MODE__    
    friend std::ostream& operator<<(std::ostream &os, const logicNameMapper& lnm);
#endif    
private:

};

#endif	/* LOGICNAMEMAPPER_H */

