#pragma once

// Standard C++ includes
#include <charconv>

namespace MiniParse::Utils
{
    template<class... Ts> struct Overload : Ts... { using Ts::operator()...; };
    template<class... Ts> Overload(Ts...) -> Overload<Ts...>; // line not needed in

    template<typename T>
    T toCharsThrow(std::string_view input, std::chars_format format = std::chars_format::general)
    {
        T out;
        const auto result = std::from_chars(input.data(), input.data() + input.size(), out);
        if(result.ec == std::errc::invalid_argument) {
            throw std::invalid_argument("Unable to convert chars '" + std::string{input} + "'");
        }
        else if(result.ec == std::errc::result_out_of_range) {
            throw std::out_of_range("Unable to convert chars '" + std::string{input} + "'");
        }
        return out;
    }
}