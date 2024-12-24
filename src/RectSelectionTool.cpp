#pragma once

#include "RectSelectionTool.hpp"

RectSelectionTool* RectSelectionTool::create() {
    auto ret = new RectSelectionTool();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void RectSelectionTool::drawPolygon() {
    BasicSelectionTool::drawPolygon(CloseOption::STIPPLE_LINE);
    // BasicSelectionTool::drawPolygon(CloseOption::LINE);
}

// get all rectangle vertices by top left and bottom right vertices 
void pushRectByTwoPoints(std::vector<CCPoint>* points, CCPoint tl, CCPoint br) {
    CCPoint tr = ccp(br.x, tl.y);
    CCPoint bl = ccp(tl.x, br.y);
    points->push_back(tl);
    points->push_back(tr);
    points->push_back(br);
    // points->push_back(bl);
}

void RectSelectionTool::handleTouchStart(CCTouch* touch) {
    CCPoint pointStart = LevelEditorLayer::get()->m_drawGridLayer->convertToNodeSpace(touch->getStartLocation());
    CCPoint pointEnd = LevelEditorLayer::get()->m_drawGridLayer->convertToNodeSpace(touch->getLocation());
    m_startPoint = pointStart;
    m_points.clear();
    pushRectByTwoPoints(&m_points, pointStart, pointEnd);
}

void RectSelectionTool::handleTouchMove(CCTouch* touch) {
    CCPoint point = LevelEditorLayer::get()->m_drawGridLayer->convertToNodeSpace(touch->getLocation());
    m_points.clear();
    pushRectByTwoPoints(&m_points, m_startPoint, point);
}

void RectSelectionTool::handleTouchEnd(CCTouch* touch) {
    applySelection();
    m_points.clear();
}