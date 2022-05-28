//-----------------------------------------------------------------------------
// Multithreaded Job scheduling system
//-----------------------------------------------------------------------------
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace core {
    class JobSystem
    {
    public:
        using JobHandle = uint64_t;
        using Job = std::function<void()>;

        // --- Singleton interface ---
        static JobSystem& Get();
        // If numWorkers = -1, the system will use the system's count of logical counts -1.
        // If 0 worker threads are requested, then you must manually poll the job system for tasks,
        // or no work will be executed.
        static void Init(int numWorkers = -1);

        // Waits for execution of all scheduled work to finish and joins all worker threads
        static void End();

        // Executes the
        // Returns true if there was work available for execution.
        bool runNext();

        JobHandle addJob(Job&& workload);

    private:
        JobSystem(int numWorkers);
        ~JobSystem();
        JobSystem(const JobSystem&) = delete;
        void operator=(const JobSystem&) = delete;

        // Singleton instance
        inline static JobSystem* s_Instance = nullptr;

        std::atomic<bool> m_quit = false;

        std::vector<std::thread> m_workerThreads;

        std::mutex m_jobQueueMutex;
        std::vector<Job> m_jobQueue;
        JobHandle m_jobCount = 0;
    };
}
