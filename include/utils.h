#pragma once

namespace MiniParse::Utils
{
    template<class... Ts> struct Overload : Ts... { using Ts::operator()...; };
    template<class... Ts> Overload(Ts...) -> Overload<Ts...>; // line not needed in 
}