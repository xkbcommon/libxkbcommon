#include <charconv>
#include <cctype>
#include <string>
#include <cerrno>

#include "utils-numbers.h"

long double
strtold_lc(const char *first, const char *last, char **end_out) {
    while (std::isspace(*first))
        ++first;

    if (*first == '+')
        ++first;

    long double d = 0.0;
    auto result = std::from_chars(first, last, d, std::chars_format::fixed);

    if (end_out)
        *end_out = const_cast<char *>(result.ptr);
    errno = static_cast<int>(result.ec);
    return d;
}
