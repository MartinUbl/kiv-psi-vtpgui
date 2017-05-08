/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#pragma once

#include <memory>
#include <mutex>
#include <assert.h>

/*
 * Template class for implementing singleton design pattern
 */
template <class T>
class CSingleton
{
    private:
        // the only one singleton instance
        static std::unique_ptr<T> m_instance;
        // run once flag for implementing thread-safe singleton access
        static std::once_flag m_once;

    protected:
        CSingleton() {};

    public:
        // retrieve singleton instance
        static T* getInstance()
        {
            std::call_once
            (
                CSingleton<T>::m_once,
                []() {
                    CSingleton<T>::m_instance.reset(new T());
                }
            );

            return m_instance.get();
        }
};

template<class T>
std::unique_ptr<T> CSingleton<T>::m_instance = nullptr;

template<class T>
std::once_flag CSingleton<T>::m_once;
