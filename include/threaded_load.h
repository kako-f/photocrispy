#pragma once
#include "raw_processing.h"
#include <string>
#include <future>
#include <optional>
#include <mutex>
#include <atomic>

namespace ThreadLoader
{
    class ThreadedImageLoader
    {
    private:
        // loading = std::atomic ensures thread-safe access to this variable
        // loaderFuture = std::future holds the result of the async task (in the created thread)
        // loadedResult =  std::optional stores the loaded image information once it's available.
        // resultMutex = A mutex (mutual exclusion) to protect loadedResult from concurrent access issues when multiple threads try to read or write to it.
        std::future<RawProcessor::RawImageInfo> loaderFuture;
        std::optional<RawProcessor::RawImageInfo> loadedResult;
        std::mutex resultMutex;
        std::atomic<bool> loading{false};

    public:
        // ThreadedImageLoader(/* args */);
        //  Destructor
        //~ThreadedImageLoader();

        // Starts asynchronous loading of the image
        // New Thread
        void startAsyncLoad(const std::string &filePath);

        // Polls whether the result is ready; if so, returns it and resets the internal state
        bool pollLoadResult(RawProcessor::RawImageInfo &outImage);

        // Checks if the loader is still working
        bool isLoading() const;

        // Placeholder for cancellation logic (not implemented yet)
        void cancel();
    };

}
