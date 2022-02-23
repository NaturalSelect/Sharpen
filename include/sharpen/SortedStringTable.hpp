#pragma once
#ifndef _SHARPEN_SORTEDSTRINGTABLE_HPP
#define _SHARPEN_SORTEDSTRINGTABLE_HPP

#include "SstRoot.hpp"
#include "SstDataBlock.hpp"
#include "SegmentedCircleCache.hpp"
#include "BloomFilter.hpp"
#include "ExistStatus.hpp"
#include "SortedStringTableBuilder.hpp"
#include "SstOption.hpp"

namespace sharpen
{
    class SortedStringTable:public sharpen::Noncopyable
    {
    private:
        using Self = sharpen::SortedStringTable;

        static constexpr sharpen::Size defaultFilterBits{10};

        sharpen::FileChannelPtr channel_;
        sharpen::SstRoot root_;
        sharpen::Size filterBits_;
        mutable sharpen::SegmentedCircleCache<sharpen::SstDataBlock> dataCache_;
        mutable sharpen::SegmentedCircleCache<sharpen::BloomFilter<sharpen::ByteBuffer>> filterCache_;

        void LoadRoot();

        std::shared_ptr<sharpen::SstDataBlock> LoadDataBlockCache(const sharpen::ByteBuffer &cacheKey,sharpen::Uint64 offset,sharpen::Uint64 size) const;

        sharpen::BloomFilter<sharpen::ByteBuffer> LoadFilter(const sharpen::ByteBuffer &key) const;

        std::shared_ptr<sharpen::BloomFilter<sharpen::ByteBuffer>> LoadFilterCache(const sharpen::ByteBuffer &cacheKey,sharpen::Uint64 offset,sharpen::Uint64 size) const;

    public:
        //read
        explicit SortedStringTable(sharpen::FileChannelPtr channel);

        SortedStringTable(sharpen::FileChannelPtr channel,sharpen::SstOption opt)
            :SortedStringTable(std::move(channel),Self::defaultFilterBits,opt)
        {}

        SortedStringTable(sharpen::FileChannelPtr channel,sharpen::Size bitsOfElements,sharpen::SstOption opt);

        template<typename _Iterator,typename _Check = sharpen::EnableIf<sharpen::IsWalKeyValuePairIterator<_Iterator>::Value>>
        SortedStringTable(sharpen::FileChannelPtr channel,_Iterator begin,_Iterator end,bool eraseDeleted)
            :SortedStringTable(std::move(channel),begin,end,Self::defaultFilterBits,eraseDeleted)
        {}

        template<typename _Iterator,typename _Check = sharpen::EnableIf<sharpen::IsWalKeyValuePairIterator<_Iterator>::Value>>
        SortedStringTable(sharpen::FileChannelPtr channel,_Iterator begin,_Iterator end,sharpen::Size filterBits,bool eraseDeleted)
            :SortedStringTable(std::move(channel),begin,end,filterBits,eraseDeleted,sharpen::SstOption{})
        {}

        template<typename _Iterator,typename _Check = sharpen::EnableIf<sharpen::IsWalKeyValuePairIterator<_Iterator>::Value>>
        SortedStringTable(sharpen::FileChannelPtr channel,_Iterator begin,_Iterator end,sharpen::Size filterBits,bool eraseDeleted,sharpen::SstOption opt)
            :channel_(std::move(channel))
            ,root_()
            ,filterBits_(filterBits)
            ,dataCache_(opt.GetDataCacheSize())
            ,filterCache_(filterBits != 0 ? opt.GetFilterCacheSize():0)
        {
            sharpen::Uint64 size = this->channel_->GetFileSize();
            if(size)
            {
                this->channel_->Truncate();
            }
            this->root_ = sharpen::SortedStringTableBuilder::DumpWalToTable<sharpen::SstDataBlock>(this->channel_,opt.GetBlockSize(),begin,end,filterBits,eraseDeleted);
        }

        template<typename _Iterator,typename _Check = decltype(std::declval<Self*&>() = &(*std::declval<_Iterator>()))>
        SortedStringTable(sharpen::FileChannelPtr channel,_Iterator begin,_Iterator end,bool eraseDeleted,bool ordered)
            :SortedStringTable(std::move(channel),begin,end,Self::defaultFilterBits,eraseDeleted,ordered)
        {}

        template<typename _Iterator,typename _Check = decltype(std::declval<Self*&>() = &(*std::declval<_Iterator>()))>
        SortedStringTable(sharpen::FileChannelPtr channel,_Iterator begin,_Iterator end,sharpen::Size filtersBits,bool eraseDeleted,bool ordered)
            :SortedStringTable(std::move(channel),begin,end,filtersBits,eraseDeleted,ordered,sharpen::SstOption{})
        {}

        template<typename _Iterator,typename _Check = decltype(std::declval<Self*&>() = &(*std::declval<_Iterator>()))>
        SortedStringTable(sharpen::FileChannelPtr channel,_Iterator begin,_Iterator end,sharpen::Size filtersBits,bool eraseDeleted,bool ordered,sharpen::SstOption opt)
            :channel_(std::move(channel))
            ,root_()
            ,filterBits_(filtersBits)
            ,dataCache_(opt.GetDataCacheSize())
            ,filterCache_(filtersBits != 0 ? opt.GetFilterCacheSize():0)
        {
            sharpen::Uint64 size = this->channel_->GetFileSize();
            if(size)
            {
                this->channel_->Truncate();
            }
            std::vector<sharpen::SstVector> vec;
            vec.reserve(sharpen::GetRangeSize(begin,end));
            for (sharpen::Size i = 0;begin != end; ++begin,++i)
            {
                vec.emplace_back(&begin->Root(),begin->channel_);
            }
            if (ordered)
            {
                this->root_ = sharpen::SortedStringTableBuilder::CombineTables<sharpen::SstDataBlock>(this->channel_,vec.begin(),vec.end(),this->filterBits_,eraseDeleted);
            }
            else
            {
                this->root_ = sharpen::SortedStringTableBuilder::MergeTables<sharpen::SstDataBlock>(this->channel_,opt.GetBlockSize(),vec.begin(),vec.end(),filterBits_,eraseDeleted);
            }
        }

        SortedStringTable(Self &&other) noexcept = default;
    
        ~SortedStringTable() noexcept = default;

        Self &operator=(Self &&other) noexcept;

        sharpen::ExistStatus Exist(const sharpen::ByteBuffer &key) const;

        sharpen::SstDataBlock LoadDataBlock(sharpen::Uint64 offset,sharpen::Uint64 size) const;

        sharpen::SstDataBlock LoadDataBlock(const sharpen::ByteBuffer &key) const;

        std::shared_ptr<const sharpen::SstDataBlock> GetDataBlockFromCache(const sharpen::ByteBuffer &key) const;

        std::shared_ptr<const sharpen::SstDataBlock> GetDataBlock(const sharpen::ByteBuffer &key,bool doCache) const;

        inline std::shared_ptr<const sharpen::SstDataBlock> GetDataBlock(const sharpen::ByteBuffer &key) const
        {
            return this->GetDataBlock(key,true);
        }

        sharpen::ByteBuffer Get(const sharpen::ByteBuffer &key) const;

        sharpen::Optional<sharpen::ByteBuffer> TryGet(const sharpen::ByteBuffer &key) const;

        bool Empty() const;

        const sharpen::SstRoot &Root() noexcept
        {
            return this->root_;
        }
    };
}

#endif