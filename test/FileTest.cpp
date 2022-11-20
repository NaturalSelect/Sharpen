#include <cassert>
#include <cstdio>
#include <cstring>

#include <sharpen/IFileChannel.hpp>
#include <sharpen/EventEngine.hpp>
#include <sharpen/FileOps.hpp>
#include <sharpen/AlignedAlloc.hpp>
#include <sharpen/MemoryPage.hpp>

void Test()
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupSingleThreadEngine();
    std::printf("file test begin\n");
    sharpen::FileChannelPtr channel = sharpen::MakeFileChannel("./hello.txt", sharpen::FileAccessMethod::Write, sharpen::FileOpenMethod::CreateOrOpen);
    channel->Register(engine);
    char str[] = "hello";
    std::size_t size = channel->WriteAsync(str, sizeof(str) - 1, 0);
    std::printf("write size is %zu\n", size);
    assert(size == sizeof(str) - 1);
    std::printf("pass\n");
    char buf[sizeof(str)] = {0};
    channel->Close();
    channel = sharpen::MakeFileChannel("./hello.txt", sharpen::FileAccessMethod::Read, sharpen::FileOpenMethod::Open);
    channel->Register(engine);
    size = channel->ReadAsync(buf, sizeof(buf) - 1, 0);
    std::printf("read size is %zu\n", size);
    for (std::size_t i = 0; i != sizeof(str) - 1; ++i)
    {
        assert(buf[i] == str[i]);
    }
    std::printf("pass\n");
    std::printf("zero memory test\n");
    channel = sharpen::MakeFileChannel("./buf.log", sharpen::FileAccessMethod::Write, sharpen::FileOpenMethod::CreateNew);
    channel->Register(engine);
    channel->ZeroMemoryAsync(64 * 1024, 0);
    assert(channel->GetFileSize() == 64 * 1024);
    channel->Close();
    std::printf("pass\n");
    std::printf("exist test\n");
    assert(sharpen::ExistFile("./buf.log"));
    std::printf("pass\n");
    std::printf("access test\n");
    assert(sharpen::AccessFile("./buf.log", sharpen::FileAccessMethod::Read));
    std::printf("pass\n");
    std::printf("rename test\n");
    sharpen::RenameFile("./buf.log", "./buf1.log");
    assert(!sharpen::ExistFile("./buf.log"));
    assert(sharpen::ExistFile("./buf1.log"));
    std::printf("pass\n");
    std::printf("remove test\n");
    sharpen::RemoveFile("./buf1.log");
    assert(!sharpen::ExistFile("./buf1.log"));
    std::printf("pass\n");
    std::printf("map file test\n");
    channel = sharpen::MakeFileChannel("./buf.log", sharpen::FileAccessMethod::All, sharpen::FileOpenMethod::CreateNew);
    channel->Register(engine);
    channel->ZeroMemoryAsync(64*1024, 0);
    {
        auto mem = channel->MapMemory(12, 0);
        std::memcpy(mem.Get(), "Hello World", 11);
        mem.Flush();
        channel->Close();
    }
    sharpen::RemoveFile("./buf.log");
    sharpen::RemoveFile("./hello.txt");
    std::printf("pass\n");
    std::puts("resolve path test");
    {
        char curr[] = "/";
        char path[] = "./abc/def/.././a.txt/.a";
        char resolved[sizeof(curr) + sizeof(path) - 1] = {0};
        // /abc/a.txt/.a
        sharpen::ResolvePath(curr,sizeof(curr) - 1,path,sizeof(path) - 1,resolved,sizeof(resolved) - 1);
        std::printf("current is %s\n"
                    "path is %s\n"
                    "resolved is %s\n",curr,path,resolved);
        assert(!std::strcmp(resolved,"/abc/a.txt/.a"));
    }
    {
        char curr[] = "C:/";
        char path[] = "./abc/def/.././a.txt/.a";
        char resolved[sizeof(curr) + sizeof(path) - 1] = {0};
        // C:/abc/a.txt/.a
        sharpen::ResolvePath(curr,sizeof(curr) - 1,path,sizeof(path) - 1,resolved,sizeof(resolved) - 1);
        std::printf("current is %s\n"
                    "path is %s\n"
                    "resolved is %s\n",curr,path,resolved);
        assert(!std::strcmp(resolved,"C:/abc/a.txt/.a"));
    }
    {
        char path[] = "./abc/def/.././a.txt/.a";
        char resolved[sizeof(path)] = {0};
        // abc/a.txt/.a
        sharpen::ResolvePath(nullptr,0,path,sizeof(path) - 1,resolved,sizeof(resolved) - 1);
        std::printf("path is %s\n"
                    "resolved is %s\n",path,resolved);
        assert(!std::strcmp(resolved,"abc/a.txt/.a"));
    }
    do
    {
        const char *sparesFileName = "./sparesTestFile";
        channel = sharpen::MakeFileChannel(sparesFileName,sharpen::FileAccessMethod::All,sharpen::FileOpenMethod::CreateNew);
        channel->Register(sharpen::EventEngine::GetEngine());
        try
        {
            channel->Allocate(0,1*1024*1024);
        }
        catch(const std::system_error &error)
        {
            if(error.code().value() == sharpen::ErrorOperationNotSupport)
            {
                std::puts("allocate operation not support");
                channel->Close();
                sharpen::RemoveFile(sparesFileName);
                break;
            }
            throw;
        }
        std::uint64_t offset{0};
        for(std::size_t i = 0;i != 10;++i)
        {
            offset += channel->WriteAsync(str,sizeof(str),offset);
        }
        channel->Deallocate(0,offset - 5*sizeof(str));
        channel->Close();
        sharpen::RemoveFile(sparesFileName);
    } while(0);
    {
        channel = sharpen::MakeFileChannel("./raw_file.tmp",sharpen::FileAccessMethod::Write,sharpen::FileOpenMethod::CreateNew,sharpen::FileIoMethod::DirectAndSync);
        channel->Register(engine);
        sharpen::MemoryPage content{1};
        std::memcpy(content.Data(),"1234",4);
        channel->WriteAsync(content.Data(),content.GetSize(),0);
        channel->Close();
        sharpen::RemoveFile("./raw_file.tmp");
    }
    std::puts("pass");
    std::printf("file test pass\n");
}

void FileTest()
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupSingleThreadEngine();
    engine.Startup([&engine]() {
        try
        {
            Test();
        }
        catch(const std::exception& e)
        {
            std::printf("error: %s\n",e.what());   
        }
    });
}

int main()
{
    FileTest();
    return 0;
}
