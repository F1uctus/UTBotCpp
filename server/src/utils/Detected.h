#ifndef UNITTESTBOT_DETECTED_H
#define UNITTESTBOT_DETECTED_H

#include <type_traits>
#include <utility>

namespace Utils {
    template <template <typename...> class Operation, typename, typename... Arguments>
    struct IsDetected : std::false_type {
    };

    template <template <typename...> class Operation, typename... Arguments>
    struct IsDetected<Operation, std::void_t<Operation<Arguments...>>, Arguments...> : std::true_type {
    };

    template <template <typename...> class Operation, typename... Arguments>
    inline constexpr bool isDetectedV = IsDetected<Operation, void, Arguments...>::value;
}

#endif // UNITTESTBOT_DETECTED_H
