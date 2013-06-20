/**
 *      @file           iterator_T.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides template-based implementation of iterator                
 */

#ifndef ITERATOR_H
#define	ITERATOR_H
/**
 *      @brief  Provides template-based implementation of iterator                
 */
template<typename I, typename T>
class iterator_T {
private:
    I iter;
public:
    iterator_T();
    iterator_T(I _iter);
    bool operator!=(const iterator_T& _iter);
    iterator_T& operator++();
    T& operator*();
};

template<typename I, typename T>
iterator_T<I, T>::iterator_T() {
}

template<typename I, typename T>
iterator_T<I, T>::iterator_T(I _iter)
: iter(_iter) {
}

template<typename I, typename T>
bool iterator_T<I, T>::operator!=(const iterator_T& _iter) {
    return iter != _iter.iter;
}

template<typename I, typename T>
iterator_T<I, T>& iterator_T<I, T>::operator++() {
    iter++;
    return *this;
}
template<typename I, typename T>
T& iterator_T<I, T>::operator*() {
    return (*iter);
}

#endif	/* ITERATOR_H */

