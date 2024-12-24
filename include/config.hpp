#pragma once

#include <Geode/Geode.hpp>

#include <vector>
#include <cmath>

using namespace geode::prelude;


enum class SelMode {
    RECT = 0,
    CIRCLE,
    AREA,
    // RECT_PLUS
    // POLYGON
};


// iterate through SelMode enum
#define SEL_MODE_ENUM_FOR_LOOP(i) for(SelMode i  = SelMode::RECT; \
    (int) i <= (int) SelMode::AREA; i = (SelMode)((int) i + 1))

