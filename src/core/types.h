#pragma once

#include <cstdint>

namespace core
{
    template<class Tag, class Handle = uint32_t>
    struct HandleType
    {
        static constexpr Handle kInvalidId = Handle(-1);
        Handle id = kInvalidId;
    };

    // Memory size literal
    constexpr inline unsigned long long operator "" _Kb(unsigned long long x)
    {
        return x * 1024;
    }

    constexpr inline unsigned long long operator "" _Mb(unsigned long long x)
    {
        return x * 1024_Kb;
    }

    constexpr inline unsigned long long operator "" _Gb(unsigned long long x)
    {
        return x * 1024_Mb;
    }
}