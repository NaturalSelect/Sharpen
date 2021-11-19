#pragma once
#ifndef _SHARPEN_INTOPS_HPP
#define _SHARPEN_INTOPS_HPP

#include "TypeTraits.hpp"
#include "ByteOrder.hpp"

namespace sharpen
{
    template <typename _T, typename _Check = sharpen::EnableIf<std::is_integral<_T>::value>>
    sharpen::Size MinSizeof(_T val)
    {
        const char *data = reinterpret_cast<const char *>(&val);
#ifdef SHARPEN_IS_BIG_ENDIAN
        for (sharpen::Size i = 0; i < sizeof(val); ++i)
        {
            if (data[sizeof(val) - 1 - i] == 0)
            {
                return i;
            }
        }
#else
        for (sharpen::Size i = 0; i < sizeof(val); ++i)
        {
            if (data[i] == 0)
            {
                return i;
            }
        }
#endif
        return sizeof(_T);
    }
}

#endif