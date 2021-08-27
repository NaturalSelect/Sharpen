#pragma once
#ifndef _SHARPEN_TYPETRAITS_HPP
#define _SHARPEN_TYPETRAITS_HPP

#include <type_traits>

namespace sharpen
{
    template<typename _Fn,typename ..._Args>
    struct IsCallable
    {
    private:
        struct FasleType;

        template<typename _U>
        static auto Test(int) -> decltype(std::declval<_Fn>()(std::declval<_Args>()...));

        static FasleType Test(...);
    public:
        static constexpr bool Value = std::is_same<FasleType,decltype(Test(0))>::value;
    };

    template<typename ..._T>
    using TypeChecker = void;

    template<bool _Value>
    struct BoolType
    {
        constexpr static bool Value = _Value;
    };

    using TrueType = sharpen::BoolType<true>;

    using FalseType = sharpen::BoolType<false>;

    template<typename _Checker>
    struct ValidContainer
    {
    private:
        template<typename _Arg,typename _Check = decltype(std::declval<_Checker>()(std::declval<_Arg>()))>
        constexpr sharpen::TrueType Test(int) noexcept
        {
            return sharpen::TrueType{};
        }

        template<typename _Arg>
        constexpr sharpen::FalseType Test(...) noexcept
        {
            return sharpen::FalseType{};
        }
    public:

        template<typename _Arg>
        constexpr auto operator()(_Arg &&arg) noexcept -> decltype(Test<_Arg>(0))
        {
            return Test<_Arg>(0);
        }
    };

    template<typename _Check>
    constexpr sharpen::ValidContainer<_Check> InternalIsValid() noexcept
    {
        return sharpen::ValidContainer<_Check>();
    }

    template<typename _Check,typename _T>
    using IsValid = decltype(sharpen::InternalIsValid<_Check>()(std::declval<_T>()));

    struct MatchesContainer
    {
        template<template<class ...> class _Tmp,typename ..._T,typename _Check = _Tmp<_T...>>
        constexpr static sharpen::TrueType Matches(int) noexcept
        {
            return sharpen::TrueType();
        }

        template<template<class ...> class _Tmp,typename ..._T>
        constexpr static sharpen::FalseType Matches(...) noexcept
        {
            return sharpen::FalseType();
        }
    };
    
    template<template<class ...> class _Tmp,typename ..._T>
    using IsMatches = decltype(sharpen::MatchesContainer::Matches<_Tmp,_T...>(0));
}

#endif