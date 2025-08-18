#ifndef PROFILING_H
#define PROFILING_H

#include <chrono>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "debug.h"
#include "massert.h"
#include "logging.h"

struct ProfNode {
    using children_type = std::vector<std::shared_ptr<ProfNode>>; 

    std::string mName = "";
    float mValue  = 0.0;
    children_type mChildren;

    inline bool IsLeaf() const { return mChildren.size() == 0; }
};

inline std::vector<std::shared_ptr<ProfNode>>               __SavedProfilingStack;
inline std::shared_ptr<ProfNode>                            __SavedProfilingRoot;
inline float                                                __SavedDeltaTime = 0;

inline std::shared_ptr<ProfNode>                            __GlobalProfilingRoot;
inline unsigned int                                         __GlobalProfilingRootCount;
inline std::mutex                                           __GlobalProfilingRootMutex;

thread_local inline std::shared_ptr<ProfNode>               __LocalProfilingRoot;
thread_local inline std::vector<std::shared_ptr<ProfNode>>  __LocalProfilingStack;

inline std::string ProfilingPrint(const std::shared_ptr<ProfNode>& node, int depth = 0) {
    std::string tabs(depth, '\t');
    std::ostringstream oss;

    oss << tabs << "[" << node->mName << "]: " << node->mValue << " ms \n";
    if (!node->IsLeaf()) {
        for (auto& el : node->mChildren) {
            oss << ProfilingPrint(el, depth + 1);
        }
    }

    return oss.str();
}

inline void ProfilingCleanup() {
    __GlobalProfilingRoot.reset();
    __GlobalProfilingRootCount = 0;
    __LocalProfilingRoot.reset();
    __LocalProfilingStack.clear();
}

inline void ProfilingLock()
{
    __SavedProfilingStack.clear();
    __SavedProfilingRoot.reset();

    for (const auto& ptr : __LocalProfilingStack)
        __SavedProfilingStack.push_back(ptr);
    __SavedProfilingRoot = __LocalProfilingRoot;

    __GlobalProfilingRoot.reset();
    __GlobalProfilingRootCount = 0;

    __LocalProfilingStack.clear();
    __LocalProfilingRoot.reset();
}

inline void ProfilingUnLock()
{
    __LocalProfilingStack.clear();
    __LocalProfilingRoot.reset();

    for (const auto& ptr : __SavedProfilingStack)
        __LocalProfilingStack.push_back(ptr);
    __LocalProfilingRoot = __SavedProfilingRoot;

    auto parent = __LocalProfilingStack.back();
    parent->mChildren.push_back(__GlobalProfilingRoot);

    __GlobalProfilingRoot.reset();
    __GlobalProfilingRootCount = 0;
}

inline void ProfilingMergeTree(const std::shared_ptr<ProfNode>& global, 
                               const std::shared_ptr<ProfNode>& local, 
                               int count) 
{
    ASSERT(global->mName == local->mName, 
           "Trees are not equal (" + global->mName + "!=" + local->mName + ")");

    if (global->IsLeaf()) {
        global->mValue += (local->mValue - global->mValue) / count;
    } else {
        ASSERT(global->mChildren.size() == local->mChildren.size(), 
               "Trees are not the same children number");

        for (int i = 0; i < global->mChildren.size(); ++i) {
            ProfilingMergeTree(global->mChildren[i], local->mChildren[i], count);
        }
    }
}


class Profiling {
    std::string mMsg;
    std::chrono::high_resolution_clock::time_point mStart;
    std::shared_ptr<ProfNode> mNode;

public:
    Profiling(const std::string& msg = "")
        : mMsg(msg), mStart(std::chrono::high_resolution_clock::now())
    {
        auto child = std::make_shared<ProfNode>(msg);
        if (!__LocalProfilingStack.empty()) {
            auto parent = __LocalProfilingStack.back();
            parent->mChildren.push_back(child);
        } else {
            __LocalProfilingRoot = child;
        }
        __LocalProfilingStack.push_back(child);
        mNode = child;
    }

    ~Profiling()
    {
        auto end = std::chrono::high_resolution_clock::now();
        float delta = static_cast<float>(
            std::chrono::duration<double, std::milli>(end - mStart).count()
        );
        mNode->mValue += delta;
        __LocalProfilingStack.pop_back();

        if (__LocalProfilingStack.empty()) {
            std::lock_guard<std::mutex> lock(__GlobalProfilingRootMutex);
            __GlobalProfilingRootCount++;
            if (!__GlobalProfilingRoot) {
                __GlobalProfilingRoot = std::make_shared<ProfNode>(*__LocalProfilingRoot);
            } else {
                ProfilingMergeTree(
                    __GlobalProfilingRoot, 
                    __LocalProfilingRoot, 
                    __GlobalProfilingRootCount
                );
            }
        } 
    }
    
};

#define PROFILING_PRINT() {                                                           \
    std::lock_guard<std::mutex> lock(__GlobalProfilingRootMutex);                     \
    ASSERT(__GlobalProfilingRoot != nullptr, "Global Profiling Root are invalid");    \
    std::cout << ProfilingPrint(__GlobalProfilingRoot);                               \
    ProfilingCleanup();                                                               \
}


#define PROFILING_LOCK() {                                              \
    auto start = std::chrono::high_resolution_clock::now();             \
    ProfilingLock();                                                    \
    auto end = std::chrono::high_resolution_clock::now();               \
    __SavedDeltaTime += static_cast<float>(                             \
        std::chrono::duration<double, std::milli>(end - mStart).count() \
    );                                                                  \
}

#define PROFILING_UNLOCK() {                                            \
    auto start = std::chrono::high_resolution_clock::now();             \
    ProfilingUnLock();                                                  \
    auto end = std::chrono::high_resolution_clock::now();               \
    __SavedDeltaTime += static_cast<float>(                             \
        std::chrono::duration<double, std::milli>(end - mStart).count() \
    );                                                                  \
    __LocalProfilingStack.back().mValue -= __SavedDeltaTime;            \
    __SavedDeltaTime = 0;                                               \
}

#define PROFILING_SCOPE(msg) Profiling timer##__LINE__(msg)

#endif // !PROFILING_H
