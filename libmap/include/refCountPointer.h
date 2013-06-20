/**
 *      @file           refCountPointer.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides template-based implementation of inteligent pointer with reference counter                
 */
#ifndef IPOINTER_H
#define	IPOINTER_H
#include <iostream>

template <typename T>  class obj_wrapper;
/**
 *      @brief  Provides template-based implementation of inteligent pointer with reference counter                
 *      
 */
template<typename T>
class ref_count_pointer
{
    
    private:
        obj_wrapper<T> *pointed_elem_wrapper;                
        
    public:
        
        ref_count_pointer();
        ref_count_pointer(T* _pelem);
        ref_count_pointer(const ref_count_pointer<T> &_elem);
        ~ref_count_pointer(); 
                       
        T*        operator->();
        const T*  operator->() const;        
        ref_count_pointer& operator=(const ref_count_pointer<T> &_elem);
        operator bool();
        
        template <typename V> 
        friend std::ostream& operator<<(std::ostream &os, const ref_count_pointer<V>& me);
};

template <typename V>
std::ostream& operator<<(std::ostream &os, const ref_count_pointer<V>& me)
{
    os << *me.pointed_elem_wrapper;
    return os;
}

template<typename T> 
ref_count_pointer<T>::ref_count_pointer() 
        : pointed_elem_wrapper(0)
{    
}

template<typename T> 
ref_count_pointer<T>::ref_count_pointer(T* _pelem)        
{
    pointed_elem_wrapper = new obj_wrapper<T>(_pelem);
    pointed_elem_wrapper->incRefCounter();
}

template<typename T> 
ref_count_pointer<T>::ref_count_pointer(const ref_count_pointer<T> &_elem)
{
    pointed_elem_wrapper = _elem.pointed_elem_wrapper;
    pointed_elem_wrapper->incRefCounter();
}

template<typename T> 
ref_count_pointer<T>& ref_count_pointer<T>::operator=(const ref_count_pointer<T> &_elem)
{
    if (this == &_elem) return *this;
    
    _elem.pointed_elem_wrapper->incRefCounter();
    if (pointed_elem_wrapper && !pointed_elem_wrapper->decRefCounter()){
        delete pointed_elem_wrapper;
    }        
    pointed_elem_wrapper = _elem.pointed_elem_wrapper;
    return *this;
}

template<typename T>
T* ref_count_pointer<T>::operator->()
{
    return pointed_elem_wrapper->pointed_elem;
}

template<typename T>
const T* ref_count_pointer<T>::operator->() const
{
    return pointed_elem_wrapper->pointed_elem;
}

template<typename T> 
ref_count_pointer<T>::~ref_count_pointer()
{
    if (pointed_elem_wrapper && !pointed_elem_wrapper->decRefCounter()){
        delete pointed_elem_wrapper;
    }
}
template<typename T> 
ref_count_pointer<T>::operator bool()
{
    if (pointed_elem_wrapper && pointed_elem_wrapper->pointed_elem){
        return true;
    } else {
        return false;
    }
}


template <typename T>
class obj_wrapper
{
    friend class ref_count_pointer<T>;
    private:
        T* pointed_elem;
        int ref_counter;
    
        obj_wrapper(T* _elem);
        ~obj_wrapper();
        
        void incRefCounter();
        int  decRefCounter();
        template <typename V> 
        friend std::ostream& operator<<(std::ostream &os, const obj_wrapper<V>& me);
        
};

template <typename T>
obj_wrapper<T>::~obj_wrapper()        
{
    delete pointed_elem;
}

template <typename T>
obj_wrapper<T>::obj_wrapper(T* _elem)
        : pointed_elem(_elem), ref_counter(0)
{
    
}

template <typename T>
int obj_wrapper<T>::decRefCounter()
{
    return --ref_counter; 
    
}

template <typename T>
void obj_wrapper<T>::incRefCounter()
{
    ref_counter++; 
}

template <typename V>
std::ostream& operator<<(std::ostream &os, const obj_wrapper<V>& me)
{
    os << *me.pointed_elem;
    return os;
}






#endif	/* IPOINTER_H */

