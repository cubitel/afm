#include <stdio.h>
#include <stdarg.h>

int __cdecl __ms_vsnprintf(char * __restrict__ d,size_t n,const char * __restrict__ format,va_list arg)
{
    return vsnprintf(d, n, format, arg);
}
