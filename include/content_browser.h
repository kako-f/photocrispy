#pragma once
#include <filesystem>

namespace ContentBrowser
{
    class RawBrowser
    {
    private:
        std::filesystem::path baseDirectory;
        std::filesystem::path currentDirectory;

    public:
        RawBrowser();
        //  destructor
        //~RawBrowser();
        void browseImages();
    };

}