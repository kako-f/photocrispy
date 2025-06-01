#pragma once
#include "raw_processing.h"
#include <string>
#include <future>
#include <optional>
#include <mutex>
#include <atomic>

namespace ThreadLoader
{
    class threaded_load
    {
    private:
        std::future<RawProcessor::RawImageInfo> loaderFuture;
        std::optional<RawProcessor::RawImageInfo> loadedResult;
        std::mutex resultMutex;
        std::atomic<bool> loading;

    public:
        threaded_load(/* args */);
        // Destructor
        ~threaded_load();
        void startLoadingAsync(const std::string &filePath);
        bool pollLoadResult(RawProcessor::RawImageInfo &outImage);
        bool isLoading() const;
        // Placeholder for future cancellation
        void cancel(); 
    };

    threaded_load::threaded_load(/* args */)
    {
    }

    threaded_load::~threaded_load()
    {
    }

}
