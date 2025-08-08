#ifndef MASSERT_H
#define MASSERT_H

#include <iostream>
#include <atomic>
#include <string>
#include <string_view>
#include <mutex>
#include <thread>
#include <sstream>
#include <source_location>

#ifdef _OPENMP
    #include <omp.h>
#endif

#if __has_include(<stacktrace>)
#  include <stacktrace>
   constexpr bool IsStackTraceAvailable = true;
#else
#  pragma message("warning: <stacktrace> not available — stack dumps will be disabled")
   constexpr bool IsStackTraceAvailable = false;
#endif

class Assert {
    static inline std::mutex mtx;

public:
    static inline void Check(bool condition, const std::string expr, 
                             const std::string& message, 
                             const std::source_location& location) 
    {
        if (!condition) {
            std::lock_guard<std::mutex> lock(mtx);
            std::cerr << "\n[Assertion failed] " << expr  << "\n";
            std::cerr << "\t Thread ID: " << GetThreadID() << "\n";
            std::cerr << "\t Msg: " << message << "\n\n";

            if constexpr (IsStackTraceAvailable) {
                std::cerr << "[Stack trace]\n";
                auto trace = std::stacktrace::current();
                for (std::size_t i = 1; i < trace.size(); ++i) {
                    std::cerr << trace[i] << '\n';
                }
            } else {
                std::cerr << "\t File: " << location.file_name() << ":" << location.line() << ":" << location.column() << "\n";
                std::cerr << "\t Function: " << location.function_name() << std::endl;
            }

            throw std::runtime_error(message);
        }
    }

private:
    static std::string GetThreadID() {
        #ifdef _OPENMP
        if (omp_in_parallel())
            return std::to_string(omp_get_thread_num());
        #endif
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        return oss.str();
    }
};

#define ASSERT(condition, message) \
    ::Assert::Check((condition), #condition ,message, std::source_location::current())

#endif // !MASSERT_H
