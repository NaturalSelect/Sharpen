#pragma once
#ifndef _SHARPEN_BUFFEROPS_HPP
#define _SHARPEN_BUFFEROPS_HPP

#include <cstdint>
#include <cstddef>
#include "TypeTraits.hpp"

#include <cstring>

namespace sharpen
{
    extern unsigned char Crc16TableHeight[256];

    extern unsigned char Crc16TableLow[256];

    //CRC16-MODBUS
    std::uint16_t Crc16(const char *data, std::size_t size) noexcept;

    //Adler32
    std::uint32_t Adler32(const char *data, std::size_t size) noexcept;

    //Base64 Encode Size
    std::size_t ComputeBase64EncodeSize(std::size_t size) noexcept;

    //Base64 Decode Size
    std::size_t ComputeBase64DecodeSize(std::size_t size) noexcept;

    //Base64 Encode
    bool Base64Encode(char *dst, std::size_t dstSize, const char *src, std::size_t srcSize) noexcept;

    //Base64 Decode Mapping
    char Base64DecodeMapping(unsigned char c) noexcept;

    //Base64 Decode
    bool Base64Decode(char *dst, std::size_t dstSize, const char *src, std::size_t srcSize) noexcept;

    //FNV-1a hash 32bits
    template<typename _Iterator,typename _Check = decltype(static_cast<char>(0) == *std::declval<_Iterator>())>
    inline std::size_t BufferHash32(_Iterator begin,_Iterator end) noexcept
    {
        constexpr std::size_t offsetBasis = 0x811c9dc5;
        constexpr std::size_t prime = 0x01000193;
        std::size_t hash = offsetBasis;
        while(begin != end)
        {
            hash ^= *begin;
            hash *= prime;
            ++begin;
        }
        return hash;
    }

    //FNV-1a hash 32bits
    std::size_t BufferHash32(const char *data, std::size_t size) noexcept;

    //FNV-1a hash 64bits
    template<typename _Iterator,typename _Check = decltype(static_cast<char>(0) == *std::declval<_Iterator>())>
    inline std::uint64_t BufferHash64(_Iterator begin,_Iterator end) noexcept
    {
        constexpr std::uint64_t offsetBasis = 0xcbf29ce484222325;
        constexpr std::uint64_t prime = 0x00000100000001B3;
        std::size_t hash = offsetBasis;
        while(begin != end)
        {
            hash ^= *begin;
            hash *= prime;
            ++begin;
        }
        return hash;
    }

    //FNV-1a hash 64bits
    std::uint64_t BufferHash64(const char *data, std::size_t size) noexcept;

    template <typename _U, typename _Check = sharpen::EnableIf<std::is_same<std::size_t, std::uint64_t>::value>>
    inline _U InternalBetterBufferHash(const char *data,std::size_t size,int) noexcept
    {
        return BufferHash64(data,size);
    }

    template <typename _U>
    inline _U InternalBetterBufferHash(const char *data,std::size_t size,...) noexcept
    {
        return BufferHash32(data,size);
    }

    inline std::size_t BufferHash(const char *data,std::size_t size) noexcept
    {
#ifndef SHARPEN_FORCE_HASH32
        return sharpen::InternalBetterBufferHash<std::size_t>(data,size,0);
#else
        return sharpen::BufferHash32(data,size);
#endif
    }

    //1     if leftBuf >  rightBuf
    //0     if leftBuf == rightBuf
    //-1    if leftBuf <  rigthBuf
    std::int32_t BufferCompare(const char *leftBuf,std::size_t leftSize,const char *rightBuf,std::size_t rightSize) noexcept;
}

#endif