#pragma once
#include <stdarg.h>
#include <stdexcept>

namespace error {

inline void write_msg(g_state_t const * const state, char const * fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    vprintf(fmt, vl);      //see newlib-supp/newlib-supp.c, write function
    va_end(vl);
}
 
template<typename TException> void throw_err(g_state_t const * const state, char const * fmt, ...)
{
    char buf[2048];
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);      //see newlib-supp/newlib-supp.c, write function
    va_end(vl);    
    throw TException(buf);
}


struct interpret_error : public std::runtime_error      //raised from parser
{
     interpret_error(char const * err) : std::runtime_error(err){}
};

struct gcpu_error : public std::runtime_error           //raised from gcpu module
{
    gcpu_error(char const * err) : std::runtime_error(err){}
};


}
