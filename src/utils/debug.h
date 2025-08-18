#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>
#include <string>
#include <utility>
#include <limits>
#include <source_location>
#include <mutex>
#include <thread>
#include <memory>


#ifdef _OPENMP
    #include <omp.h>
#endif

#if __has_include(<stacktrace>)
    #include <stacktrace>
    constexpr bool IsStackTraceAvailableDebug = true;
#else
    #pragma message("warning: <stacktrace> not available — stack dumps will be disabled")
    constexpr bool IsStackTraceAvailableDebug = false;
#endif

class Debug {
public:
    template <bool stop = true, typename... Args>
    static inline void Breakpoint(
        const std::source_location& location,
        const bool condition = true,
        const std::string expr = "", 
        const Args&... args 
    ) {
        if (condition) {
            std::lock_guard<std::mutex> lock(sMutex);
            if (expr != "") {
                std::cerr << "\n[BREAKPOINT on thread " << GetThreadID() << " ("<< expr <<")" << "]\n";
            } else { 
                std::cerr << "\n[BREAKPOINT on thread " << GetThreadID() << "]\n";
            }

            std::cerr << "\t[Stacktrace]" << "\n";
            if constexpr (IsStackTraceAvailableDebug) {
                auto trace = std::stacktrace::current();
                for (std::size_t i = 1; i < trace.size(); ++i)
                    std::cerr << "\t\t" << trace[i] << '\n';
            } else {
                std::cerr << "\t\tFile: " << location.file_name() << ":"
                    << location.line() << ":" << location.column() << "\n";
                std::cerr << "\t\tFunction: " << location.function_name() << "\n";
            }
            std::cerr << "\t[Variables]\n";
            std::cerr << PrintVariables(args...).str();

            if constexpr (stop) {
                std::cerr << "\n[Press Enter to continue]" << std::endl;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
    }

private:
    static inline std::mutex sMutex;

    template <typename T>
    static std::ostringstream PrintVariable(const std::pair<std::string, std::vector<T>>& var) {
        std::ostringstream os;
        os << var.first
           << " [addr: " << &var.second << "] = [";

        for (size_t i = 0; i < var.second.size(); ++i) {
            auto pair = std::make_pair(std::to_string(i), var.second[i]); 
            os << PrintVariable(pair).str();
            if (i + 1 < var.second.size()) os << ", ";
        }
        os << "]";
        return os;
    }
    
    template <typename T>
    static std::ostringstream PrintVariable(const std::pair<std::string, T>& var) {
        std::ostringstream os;
        os << var.first
           << " [addr: " << &var.second << "] = "
           << var.second;
        return os;
    }


    template <typename T, typename... Args>
    static std::ostringstream PrintVariables(const std::pair<std::string, T>& var, const Args&... args) {
        std::ostringstream os;
        os << "\t\t" << PrintVariable(var).str() << "\n";
        os << PrintVariables(args...).str();
        return os;
    }

    static std::ostringstream PrintVariables() { return std::ostringstream{}; }

    static std::string GetThreadID() 
    {
        #ifdef _OPENMP
        if (omp_in_parallel())
            return std::to_string(omp_get_thread_num());
        #endif
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        return oss.str();
    }
};

#ifndef NDEBUG
    #define VAR(var) std::make_pair(std::string(#var), var)

    #define BREAK(...) ::Debug::Breakpoint(std::source_location::current(), true, "" __VA_OPT__(, __VA_ARGS__))

    #define BREAK_RUN(...) ::Debug::Breakpoint<false>(std::source_location::current(), true, "" __VA_OPT__(, __VA_ARGS__))

    #define BREAK_COND(condition, ...) \
        ::Debug::Breakpoint(std::source_location::current(), (condition), #condition __VA_OPT__(, __VA_ARGS__))
#else
    #pragma message("Debug are disabled")
    #define DEBUG_VAR(var)
    #define BREAK(...) ((void)0)
    #define BREAK_RUN(...) ((void)0)
    #define BREAK_COND(...) ((void)0)
#endif

#endif
