#pragma once
#ifndef _SHARPEN_GENERICMAIL_HPP
#define _SHARPEN_GENERICMAIL_HPP

#include "Mail.hpp"
#include "GenericMailHeader.hpp"
#include "TypeTraits.hpp"

namespace sharpen
{
    class GenericMail:private sharpen::Mail
    {
    private:
        using Self = sharpen::GenericMail;
        using Base = sharpen::Mail;
    
        sharpen::GenericMailHeader &GenericHeader() noexcept;

        const sharpen::GenericMailHeader &GenericHeader() const noexcept;
    public:

        GenericMail() noexcept;
    
        explicit GenericMail(std::uint32_t magic) noexcept;

        explicit GenericMail(sharpen::Mail mail) noexcept;
    
        GenericMail(const Self &other) = default;
    
        GenericMail(Self &&other) noexcept = default;
    
        Self &operator=(const Self &other) = default;
    
        Self &operator=(Self &&other) noexcept = default;
    
        ~GenericMail() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        template<typename _T,typename _Check = sharpen::EnableIf<std::is_standard_layout<_T>::value && sizeof(_T) == sharpen::GenericMailHeader::formSize>>
        inline _T &Form() noexcept
        {
            return this->GenericHeader().Form<_T>();
        }

        template<typename _T,typename _Check = sharpen::EnableIf<std::is_standard_layout<_T>::value && sizeof(_T) == sharpen::GenericMailHeader::formSize>>
        inline const _T &Form() const noexcept
        {
            return this->GenericHeader().Form<_T>();
        }

        std::uint32_t GetMagic() const noexcept;

        void SetMagic(std::uint32_t magic) noexcept;

        inline const sharpen::ByteBuffer &ConstContent() const noexcept
        {
            return Base::Content();
        }

        void SetContent(sharpen::ByteBuffer content);
    };
}

#endif