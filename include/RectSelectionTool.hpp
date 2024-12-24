#pragma once

#include "BasicSelectionTool.hpp"

class RectSelectionTool : public BasicSelectionTool {
protected:
    CCPoint m_startPoint;

public:
    static RectSelectionTool* create();
    void handleTouchStart(CCTouch* touch) override;
    void handleTouchMove(CCTouch* touch) override;
    void handleTouchEnd(CCTouch* touch) override;
    void drawPolygon() override;
};