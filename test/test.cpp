#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include "../src/conv.hpp"

#include <string>
#include <sstream>
#include <typeinfo>
#include <cstdlib>
#include <memory>
//#include <cxxabi.h>
#include <iostream>
#include <vector>

TEST_CASE("Forward Convolution")
{
    xt::xarray<double> a1{
        {
            {
                {2, 3},
                {3, 4},
            },
        },
        {
            {
                {2, 1},
                {3, 2},
            },
        }};

    // (3,1,2,2)
    xt::xarray<double> a2{
        {{
            {2, 1},
            {1, 2},
        }},
        {{
            {3, 4},
            {3, 2},
        }},
        {{
            {3, 5},
            {5, 2},
        }}};

    xt::xarray<double> e1{
        {{{18}},
         {{35}},
         {{44}}},
        {{{12}},
         {{23}},
         {{30}}}};

    auto &&x = conv2d_forward(a1, a2, 2);
    bool match = (x == e1);
    if (!match)
    {
        std::stringstream ss;
        ss << "Convolution result did not match expected.\n";
        ss << "Convolution Result:\n";
        ss << x << "\n";
        ss << "Expected:\n";
        ss << e1 << "\n";
        FAIL(ss.str());
    }
    else
    {
        SUCCEED();
    }

    //REQUIRE( x == 1 );
}