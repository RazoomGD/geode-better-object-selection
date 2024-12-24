#pragma once

#include "config.hpp"

enum class CloseOption {
    NONE, LINE, STIPPLE_LINE, 
};

class BasicSelectionTool : public cocos2d::CCNode {
protected:
    std::vector<CCPoint> m_points;

    void drawPolygon(CloseOption closeOption);

public:
    virtual void handleTouchStart(CCTouch* touch) = 0;
    virtual void handleTouchMove(CCTouch* touch) = 0;
    virtual void handleTouchEnd(CCTouch* touch) = 0;
    virtual void drawPolygon() = 0;
    
    void applySelection();
};