#ifndef SAI_COMMON_THREAD_BARRIER_H
#define SAI_COMMON_THREAD_BARRIER_H

#include <condition_variable>
#include <mutex>

namespace SaiCommon
{

    /**
     * @brief Simple barrier for thread synchronization at initialization.
     * Allows one thread to signal when it's ready, and other threads to wait for that signal.
     */
    class ThreadBarrier
    {
    public:
        ThreadBarrier() : ready_(false) {}

        /**
         * @brief Signal that the thread/resource is ready
         */
        void signal()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ready_ = true;
            cv_.notify_all();
        }

        /**
         * @brief Wait until signal() has been called
         */
        void wait()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]
                     { return ready_; });
        }

        /**
         * @brief Reset the barrier to not-ready state
         */
        void reset()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ready_ = false;
        }

        /**
         * @brief Check if barrier has been signaled (non-blocking)
         */
        bool isReady() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return ready_;
        }

    private:
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        bool ready_;
    };

} // namespace SaiCommon

#endif // SAI_COMMON_THREAD_BARRIER_H