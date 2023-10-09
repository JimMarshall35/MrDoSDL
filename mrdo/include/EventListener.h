#pragma once

// Based heavily on https://cratonica.wordpress.com/implementing-c-events-in-c/


template <typename T>
class Delegate
{
public:
	virtual void operator()(T param) = 0;
};

#define LISTENER(thisType, handler, type)\
    class __L##handler##__ : public Delegate< type >\
    {\
        public:\
            __L##handler##__ ( thisType * obj )\
            : _obj(obj) {}\
            inline void operator()( type param )\
            {\
                _obj-> handler (param);\
            }\
            thisType * _obj;\
    };\
    __L##handler##__ L##handler;


