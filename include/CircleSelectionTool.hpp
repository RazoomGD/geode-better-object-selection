#pragma once

#include "BasicSelectionTool.hpp"

class CircleSelectionTool : public BasicSelectionTool {
protected:
    const int m_pointLimit = 50;

public:
    static CircleSelectionTool* create();
    void handleTouchStart(CCTouch* touch) override;
    void handleTouchMove(CCTouch* touch) override;
    void handleTouchEnd(CCTouch* touch) override;
    void drawPolygon() override;
};