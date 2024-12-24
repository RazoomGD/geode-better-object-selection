#pragma once

#include "CircleSelectionTool.hpp"

CircleSelectionTool* CircleSelectionTool::create() {
    auto ret = new CircleSelectionTool();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void CircleSelectionTool::drawPolygon() {
    BasicSelectionTool::drawPolygon(CloseOption::LINE);
}

void CircleSelectionTool::handleTouchStart(CCTouch* touch) {

}

void CircleSelectionTool::handleTouchMove(CCTouch* touch) {

}

void CircleSelectionTool::handleTouchEnd(CCTouch* touch) {

}

