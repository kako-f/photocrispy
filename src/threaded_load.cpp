#include "threaded_load.h"

namespace ThreadLoader
{
    // function to initiate the asynchronous loading of a RAW image.
    void ThreadedImageLoader::startAsyncLoad(const std::string &filePath)
    {
        // Check for loading
        // avoiding multiple concurrent loads
        if (loading.load())
            return;

        // load operation has started.
        loading.store(true);
        // reset any previous result.
        loadedResult.reset();

        // std::async launches a new task to execute the provided lambda function.
        // std::launch::async force new thread creation
        // In this case the lambda function "[filePath]..." reads the RAW image in the path and returns
        // the RawProcessor::RawImageInfo object.
        loaderFuture = std::async(std::launch::async, [filePath]() -> RawProcessor::RawImageInfo
                                  { return RawProcessor::loadRaw(filePath); });
        // this part create another thread, which is used to wait for loaderFuture
        // to complete and store the result
        std::thread([this]()
                    {
            RawProcessor::RawImageInfo result = loaderFuture.get();
            {
                std::lock_guard<std::mutex> lock(resultMutex);
                loadedResult = std::move(result);
            }
            loading.store(false); })
            .detach();
        // Comments for my understanding // 
        // "this" allows a pointer by value, enabling the lambda to access member 
        // variables of the ThreadedImageLoader instance.
        // --
        // loaderFuture.get(); blocks the current thread (created by std::thread) 
        // until the std::async task (the one loading the image) has completed 
        // and its result is available. Once available, it retrieves the RawImageInfo.
        // --
        // The "resultMutex" part, applies a lock on the thread.
        // This is for thread safety. Ensures that only one thread can access "loadedResult" at a time, 
        // preventing data corruption if "pollLoadResult" were called concurrently. 
        // --
        // "loadedResult", use std::move to move the result to this variable.
        // --
        // loading.store(false), indicates the processis complete.
        // --
        // .detach(), This detaches the newly created thread. Meaning that can run independently in the background.

    }
    // This function checks if the loaded image is available and retrieves it. 
    // it does this periodically (polling)
    bool ThreadedImageLoader::pollLoadResult(RawProcessor::RawImageInfo &outImage)
    {
        std::lock_guard<std::mutex> lock(resultMutex);
        if (loadedResult.has_value())
        {
            outImage = std::move(loadedResult.value());
            loadedResult.reset();
            return true;
        }
        return false;
    }
    // comments for my (or future) understanding //
    // std::lock_guard, Acquires a lock to safely access loadedResult.
    // --
    // loadedResult.has_value(), we check for value
    // --
    // If has any result is moved to "outImage", and gets cleared. 
    // Lastly return true for a positive outcome or false if no result is obtained.

    // return the current state of loading
    bool ThreadedImageLoader::isLoading() const
    {
        return loading.load();
    }
    // cancel the execution of the task in the thread.
    // this has to be linked to a cancel task in libraw
    void ThreadedImageLoader::cancel()
    {
        // todo
    }

}