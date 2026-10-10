#pragma once
#include <uiutils/activity.h>
#include <type_traits>
namespace uiutils {
using ActivityFactory = std::function<std::unique_ptr<ActivityFrame>()>;
int RunApplication(int argc, char** argv, ActivityFactory factory);
template<class T> int RunApplication(int argc, char** argv) {
    static_assert(std::is_base_of_v<ActivityFrame, T>, "T must derive from ActivityFrame");
    return RunApplication(argc, argv, [] { return std::make_unique<T>(); });
}
}
