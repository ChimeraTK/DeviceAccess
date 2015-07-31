/**
 *      @file           exBase.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides base class for exception handling                
 */
#ifndef MTCA4U_EXBASE_H
#define	MTCA4U_EXBASE_H

#include <exception>
#include <string>

namespace mtca4u{

/**
 *      @brief  Provides base class for exception handling . 
 *      
 *      Stores exception ID and exception description in the form of text string.
 *
 */
class exBase : public std::exception {
protected:
    std::string         exMessage;      /**< exception description*/
    unsigned int        exID;           /**< exception ID*/
public:
    /**
     * @brief Class constructor 
     *  
     * @param _exMessage exception description string
     * @param _exID exception ID
     */
    exBase(const std::string &_exMessage, unsigned int _exID);
    /**
     * @brief Accessor. Returns exception description string
     * 
     * @return exception description string
     */
    virtual const char* what() const throw();
    /**
     * @brief Accessor. Returns exception ID
     * 
     * @return exception ID
     */
    virtual unsigned int getID() const;
    /**
     * Class destructor
     */
    virtual ~exBase() throw();
private:

};

}//namespace mtca4u

#endif	/* MTCA4U_EXBASE_H */

