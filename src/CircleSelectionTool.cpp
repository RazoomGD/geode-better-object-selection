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

// get all circle vertices by top left and bottom right vertices 
void pushCircleByTwoPoints(std::vector<CCPoint>* points, CCPoint tl, CCPoint br) {
    CCPoint tr = ccp(br.x, tl.y);
    CCPoint bl = ccp(tl.x, br.y);
    points->push_back(tl);
    points->push_back(tr);
    points->push_back(br);
    points->push_back(bl); // todo: replace with the code for circle
}

void CircleSelectionTool::drawPolygon() {
    BasicSelectionTool::drawPolygon(CloseOption::LINE);
}

void CircleSelectionTool::handleTouchStart(CCTouch* touch) {
    CCPoint pointStart = LevelEditorLayer::get()->m_drawGridLayer->convertToNodeSpace(touch->getStartLocation());
    CCPoint pointEnd = LevelEditorLayer::get()->m_drawGridLayer->convertToNodeSpace(touch->getLocation());
    m_startPoint = pointStart;
    m_points.clear();
    pushCircleByTwoPoints(&m_points, pointStart, pointEnd);
}

void CircleSelectionTool::handleTouchMove(CCTouch* touch) {
    CCPoint point = LevelEditorLayer::get()->m_drawGridLayer->convertToNodeSpace(touch->getLocation());
    m_points.clear();
    pushCircleByTwoPoints(&m_points, m_startPoint, point);
}

void CircleSelectionTool::handleTouchEnd(CCTouch* touch) {
    applySelection();
    m_points.clear();
}

