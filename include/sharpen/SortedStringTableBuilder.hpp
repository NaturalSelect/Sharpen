#pragma once
#ifndef _SHARPEN_SORTEDSTRINGTABLEBUILDER_HPP
#define _SHARPEN_SORTEDSTRINGTABLEBUILDER_HPP

#include "SstDataBlock.hpp"
#include "BloomFilter.hpp"
#include "SortedStringTable.hpp"
#include "SortedStringTableBuilderConcept.hpp"

namespace sharpen
{
    struct SortedStringTableBuilder
    {
    private:
        template<typename _Iterator>
        struct Helper
        {
            template<typename _Block>
            static void PutWalKv(_Block &block,_Iterator ite)
            {
                if(!ite->second.IsDeleted())
                {
                    block.Put(ite->first,ite->second.Value());
                }
                else
                {
                    block.Delete(ite->first);
                }
            }
        };

        template<typename _Iterator>
        struct Helper<std::move_iterator<_Iterator>>
        {
            template<typename _Block>
            static void PutWalKv(_Block &block,std::move_iterator<_Iterator> ite)
            {
                if(!ite->second.IsDeleted())
                {
                    block.Put(std::move(ite->first),std::move(ite->second.Value()));
                }
                else
                {
                    block.Delete(std::move(ite->first));
                }
            }
        };

        template<typename _Block,typename _Check = sharpen::EnableIf<sharpen::IsSstDataBlock<_Block>::Value>>
        bool GetKv(_Block &block,sharpen::Size &groupIndex,sharpen::Size &keyIndex,sharpen::ByteBuffer &key,sharpen::ByteBuffer &value)
        {
            while (groupIndex != block.GetSize())
            {
                auto &group = *sharpen::IteratorForward(block.Begin(),groupIndex);
                while (keyIndex != group.GetSize())
                {
                    auto &kv = *sharpen::IteratorForward(group.Begin(),keyIndex);
                    key = kv->GetKey();
                    value = kv->Value();
                    ++keyIndex;
                    return true;
                }
                ++groupIndex;
            }
            return false;
        }

        using Self = sharpen::SortedStringTableBuilder;
    public:
        template<typename _Block,typename _Check = sharpen::EnableIf<sharpen::IsSstDataBlock<_Block>::Value>>
        static sharpen::BloomFilter<sharpen::ByteBuffer> BuildFilter(const _Block &block,sharpen::Size bits)
        {
            sharpen::BloomFilter<sharpen::ByteBuffer> filter{block.ComputeSize()*bits,bits};
            for (auto groupBegin = block.Begin(),groupEnd = block.End(); groupBegin != groupEnd; ++groupBegin)
            {
                for (auto keyBegin = groupBegin->Begin(),keyEnd = groupBegin->End(); keyBegin != keyEnd; ++keyBegin)
                {
                    filter.Add(keyBegin->GetKey());   
                }
            }
            return filter;
        }

        template<typename _Block,typename _Check = sharpen::EnableIf<sharpen::IsSstDataBlock<_Block>::Value>>
        static _Block BuildDataBlock(sharpen::FileChannelPtr channel,sharpen::Uint64 offset,sharpen::Uint64 size)
        {
            _Block block;
            sharpen::ByteBuffer buf{sharpen::IntCast<sharpen::Size>(size)};
            channel->ReadAsync(buf,offset);
            block.LoadFrom(buf);
            return block;
        }

        static sharpen::BloomFilter<sharpen::ByteBuffer> BuildFilter(sharpen::FileChannelPtr channel,sharpen::Uint64 offset,sharpen::Uint64 size,sharpen::Size bitsOfElements);

        template<typename _Block,typename _Iterator,typename _Check = sharpen::EnableIf<sharpen::IsWalKeyValuePairIterator<_Iterator>::Value && sharpen::IsSstDataBlock<_Block>::Value>>
        static sharpen::SortedStringTable BuildTable(sharpen::FileChannelPtr channel,sharpen::Size blockBytes,_Iterator begin,_Iterator end,sharpen::Size filterBits,bool eraseDeleted)
        {
            //build data block
            sharpen::SortedStringTable root;
            std::vector<_Block> blocks;
            std::vector<sharpen::BloomFilter<sharpen::ByteBuffer>> filters;
            blocks.reserve(8);
            sharpen::Size blockSize{0};
            while (begin != end)
            {
                if(blocks.empty() || blockSize > blockBytes)
                {
                    blockSize = 0;
                    blocks.emplace_back();
                }
                blockSize += begin->first.GetSize();
                blockSize += begin->second.Value().GetSize();
                Self::Helper<_Iterator>::PutWalKv(blocks.back(),begin);
                ++begin;
            }
            if(filterBits)
            {
                filters.reserve(blocks.size());
                root.MetaIndexBlock().Reserve(blocks.size());
            }
            root.IndexBlock().Reserve(blocks.size());
            sharpen::ByteBuffer buf;
            sharpen::Uint64 offset{0};
            //foreach blocks
            for (auto blockBegin = blocks.begin(),blockEnd = blocks.end(); blockBegin != blockEnd; ++blockBegin)
            {
                if(eraseDeleted)
                {
                    blockBegin->EraseDeleted();
                }
                //build filter
                if(filterBits)
                {
                    sharpen::ByteBuffer lastKey{blockBegin->LastKey()};
                    filters.emplace_back(Self::BuildFilter(*blockBegin,filterBits));
                    sharpen::SstBlock block{};
                    root.MetaIndexBlock().Put(std::move(lastKey),block);
                }
                sharpen::ByteBuffer lastKey{blockBegin->LastKey()};
                //write data blocks
                sharpen::Size size{blockBegin->StoreTo(buf)};
                try
                {
                    channel->WriteAsync(buf.Data(),size,offset);
                }
                catch(const std::exception&)
                {
                    channel->Truncate();
                    throw;
                }
                //build index block
                sharpen::SstBlock block;
                block.offset_ = offset;
                block.size_ = size;
                offset += size;
                root.IndexBlock().Put(std::move(lastKey),block);
            }
            sharpen::Size index{0};
            if(filterBits)
            {
                for (auto blockBegin = blocks.begin(),blockEnd = blocks.end(); blockBegin != blockEnd; ++blockBegin)
                {
                    sharpen::ByteBuffer lastKey{blockBegin->LastKey()};
                    //write filter
                    sharpen::Size size{filters[index].GetSize()};
                    buf.ExtendTo(size);
                    filters[index].CopyTo(buf.Data(),buf.GetSize());
                    try
                    {
                        channel->WriteAsync(buf.Data(),size,offset);
                    }
                    catch(const std::exception&)
                    {
                        channel->Truncate();
                        throw;
                    }
                    //build meta index
                    root.MetaIndexBlock()[index].Block().offset_ = offset;
                    root.MetaIndexBlock()[index].Block().size_ = size;
                    offset += size;
                    index++;
                }
                filters.clear();
                filters.shrink_to_fit();
            }
            blocks.clear();
            blocks.shrink_to_fit();
            //write footer
            root.StoreTo(channel,offset);
            return root;
        }

        //template<typename _Block,typename _Iterator>
        template<typename _Iterator>
        static sharpen::SortedStringTable BuildTable(sharpen::FileChannelPtr channel,sharpen::FileChannelPtr leftChannel,_Iterator leftBegin,_Iterator leftEnd,sharpen::FileChannelPtr rightChannel,_Iterator rightBegin,_Iterator rightEnd,sharpen::Size filterBits,bool eraseDeleted)
        {
            using _Block = sharpen::SstDataBlock;
            sharpen::SortedStringTable root;
            _Block block;
            sharpen::Size blockSize{0};
            sharpen::Optional<_Block> left;
            sharpen::Optional<_Block> right;
            //load first data block
            sharpen::Size leftGroupIndex{0};
            sharpen::Size leftKeyIndex{0};
            sharpen::ByteBuffer leftKey;
            sharpen::ByteBuffer leftVaue;
            sharpen::Size rightGroupIndex{0};
            sharpen::Size rightKeyIndex{0};
            sharpen::ByteBuffer rightKey;
            sharpen::ByteBuffer rightValue;
            do
            {
                //init blocks
                if(!left.Exist() && leftBegin != leftEnd)
                {
                    leftGroupIndex = 0;
                    leftKeyIndex = 0;
                    left.Construct(Self::BuildDataBlock<_Block>(leftChannel,leftBegin->Block().offset_,leftBegin->Block().offset_));
                    ++leftBegin;
                }
                if(!right.Exist() && rightBegin != rightEnd)
                {
                    rightGroupIndex = 0;
                    rightKeyIndex = 0;
                    right.Construct(Self::BuildDataBlock<_Block>(rightChannel,rightBegin->Block().offset_,rightBegin->Block().offset_));
                    ++rightBegin;
                }
                //for each every keys
                leftKey.Clear();
                leftVaue.Clear();
                rightKey.Clear();
                rightValue.Clear();
                if(left.Exist())
                {
                    auto &group = left.Get()[leftGroupIndex];
                    if(leftKeyIndex == group.GetSize())
                    {
                        leftKeyIndex = 0;
                    }
                    if(leftGroupIndex == left.Get().GetSize())
                    {
                        leftGroupIndex = 0;
                        left.Reset();
                    }
                }
                if(right.Exist())
                {

                }
            }
            while (left.Exist() || right.Exist());
            return root;
        }
    };
}

#endif