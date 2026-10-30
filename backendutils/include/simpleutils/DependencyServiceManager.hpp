#pragma once

#include <vector>

class DependencyServiceManager
{
public:
    template <typename T>
    static constexpr T* get()
    {
        auto it = services<T>.begin();
        for (auto it = services<T>.begin(); it != services<T>.end(); it++)
        {
            if (dynamic_cast<T*>(*it) != nullptr)
                return *it;
        }
        return nullptr;
    }

    template <typename T>
    static void registerService(T* service) { services<T>.push_back(service); }

    template <typename T>
    static std::vector<T*> services;
};

template <typename T>
std::vector<T*> DependencyServiceManager::services;