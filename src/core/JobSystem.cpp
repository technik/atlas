//-----------------------------------------------------------------------------
// Multithreaded Job scheduling system
//-----------------------------------------------------------------------------
#include <cassert>
#include <core/JobSystem.h>

// Already defined in the CMake project.
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace core
{
    JobSystem& JobSystem::Get()
    {
        assert(s_Instance != nullptr && "The singleton needs to be initialized before querying");
        return *s_Instance;
    }

    void JobSystem::Init(int numWorkers)
    {
        assert(!s_Instance && "Trying to initialize the singleton multiple times");
        s_Instance = new JobSystem(numWorkers);
    }

    void JobSystem::End()
    {
        assert(s_Instance && "Singleton was not initialized. Trying to end it multiple times?");
        delete s_Instance;
        s_Instance = nullptr;
    }

    bool JobSystem::runNext()
    {
        // Grab next job
        Job nextJob;
        { // Critical section
            auto lock = std::scoped_lock(m_jobQueueMutex);

            if (m_jobQueue.empty())
                return false;

            auto job = m_jobQueue.back();
            m_jobQueue.pop_back();
        }

        // Execute the job
        nextJob();

        return true;
    }

    auto JobSystem::addJob(Job&& workload) -> JobHandle
    {
        auto lock = std::scoped_lock(m_jobQueueMutex);
        m_jobQueue.emplace_back(std::move(workload));
        return ++m_jobCount;
    }

    JobSystem::JobSystem(int numWorkers)
    {
        // Detect the system's logical processor count
        if (numWorkers < 0)
        {
            numWorkers = 0;

            constexpr size_t kMaxLogicalProcessors = 64;
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION processorInfo[kMaxLogicalProcessors]; // The system call is limited to 64 logical processors
            // Find out how many logical cores the system has
            DWORD numLogicalProcessors = kMaxLogicalProcessors;
            auto success = GetLogicalProcessorInformation(processorInfo, &numLogicalProcessors);
            assert(success && "Failed to gather system information");
            if(success) // Guard against failed system calls, reverting to 0 workers
            {
                numWorkers = (numLogicalProcessors>0) ? (numLogicalProcessors - 1) : 0; // Leave room for the main thread
            }
        }

        // Spawn worker threads
        m_workerThreads.resize(numWorkers);
        for (auto& worker : m_workerThreads)
        {
            worker = std::thread([this]()
                {
                    while (!m_quit)
                    {
                        runNext();
                    }
                });
        }
    }

    JobSystem::~JobSystem()
    {
        // Signal we want to end execution
        m_quit = true;

        // Join all worker threads
        for (auto& worker : m_workerThreads)
        {
            worker.join();
        }
    }
} // namespace core