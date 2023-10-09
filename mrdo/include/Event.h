#pragma once
#include <vector>
#include <iostream>
#include "EventListener.h"

// Based heavily on https://cratonica.wordpress.com/implementing-c-events-in-c/


template <typename T>
class Event
{
public:
    void operator+=(Delegate<T>* del)
    {
        if (std::find(Delegates.begin(), Delegates.end(), del) != Delegates.end())
        {
            std::cerr << "WARNING: double subscription\n";
        }
        else
        {
            Delegates.push_back(del);
        }
    }
    
    void operator-=(Delegate<T>* del)
    {
        auto iter = std::find(Delegates.begin(), Delegates.end(), del);
        if (iter == Delegates.end())
        {
            std::cerr << "WARNING: trying to remove a delegate that doesn't exist\n";
        }
        else
        {
            Delegates.erase(iter);
        }
    }

    void operator()(T param)
    {
        for (Delegate<T>* del : Delegates)
        {
            (*del)(param);
        }
    }

private:
    std::vector<Delegate<T>*> Delegates;
};

