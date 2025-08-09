#ifndef PROFILING_H
#define PROFILING_H

#include "utils/debug.h"
#include "utils/massert.h"
#include <chrono>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>


struct ProfNode {
    using children_type = std::vector<std::shared_ptr<ProfNode>>; 

    std::string mName = "";
    std::variant<children_type, float> mValues = 0.0f;

    ProfNode(const std::string& name = "")
        : mName(name), mValues(children_type{}) {}

    ProfNode(const std::string& name, float time)
        : mName(name), mValues(time) {}

    children_type GetChildren() const {
        ASSERT(!IsLeaf(), "This node is a leaf, it has not children");
        return std::get<children_type>(mValues);
    }

    children_type& GetChildren() {
        return std::get<children_type>(mValues);
    }

    float GetTime() const {
        ASSERT(IsLeaf(), "This node is a leaf, it has not children");
        return std::get<float>(mValues);
    }

    float& GetTime() {
        return std::get<float>(mValues);
    }

    inline bool IsLeaf() const { return mValues.index() == 1; }
};
   
thread_local inline std::shared_ptr<ProfNode> __ProfilingRoot =
    std::make_shared<ProfNode>("ROOT");
thread_local inline std::vector<std::shared_ptr<ProfNode>> __ProfilingStack = {
    __ProfilingRoot
};


class Profiling {
    std::string mMsg;
    std::chrono::high_resolution_clock::time_point mStart;
    std::shared_ptr<ProfNode> mNode;

public:
    Profiling(const std::string& msg = "")
        : mMsg(msg), mStart(std::chrono::high_resolution_clock::now())
    {
        auto parent = __ProfilingStack.back();
        auto child = std::make_shared<ProfNode>(msg);
        parent->GetChildren().push_back(child);
        __ProfilingStack.push_back(child);
        mNode = child;
    }

    ~Profiling()
    {
        auto end = std::chrono::high_resolution_clock::now();
        float delta = static_cast<float>(
            std::chrono::duration<double, std::milli>(end - mStart).count());
        if (mNode->GetChildren().size() == 0) 
            mNode->GetTime() = delta;
        __ProfilingStack.pop_back();
    }
    
};

inline void ProfilingPrint(const std::shared_ptr<ProfNode> node, int depth = 0) {
   
    std::string tabs(depth, '\t');
    std::cout << tabs << "[" << node->mName << "]" << "\n";
    for(const auto& el : node->GetChildren()) {
        if (el->IsLeaf()) {
            std::cout << tabs << "["<< el->mName <<"]" << el->GetTime();
        } else {
            ProfilingPrint(el, depth + 1);
        }
    }
}

#define PROFILING_SCOPE(msg) Profiling timer##__LINE__(msg)

#endif // !PROFILING_H
