#include "BasicSelectionTool.hpp"

void BasicSelectionTool::drawPolygon(CloseOption closeOption) {
    if (m_points.size() <= 1) return;
    ccDrawColor4B(0,255,0,255);
    auto prev = m_points[0];
    for (unsigned i = 1; i < m_points.size(); i++) {
        auto point = m_points[i];
        ccDrawLine(prev, point);
        prev = point;
    }

    if (closeOption == CloseOption::LINE) {
        ccDrawLine(prev, m_points[0]);
    } else if (closeOption == CloseOption::STIPPLE_LINE) {
        // math code alert!
        const float streak = 12, gap = 7;
        const CCPoint B = prev, A = m_points[0];
        const float AB = std::sqrt(std::pow(B.x-A.x, 2) + std::pow(B.y - A.y, 2));
        const float sinA = (B.y-A.y) / AB;
        const float cosA = (B.x-A.x) / AB;
        float x0 = A.x, y0 = A.y;
        float x1, y1;
        for(float dist = 0;;) {
            x1 = x0 + streak * cosA;
            y1 = y0 + streak * sinA;

            dist += streak;
            if (dist >= AB) {
                ccDrawLine(ccp(x0, y0), B); // draw last streak
                break;
            }
            ccDrawLine(ccp(x0, y0), ccp(x1, y1)); // draw streak
            
            dist += gap;
            if (dist >= AB) {
                break;
            }

            x0 += (streak + gap) * cosA;
            y0 += (streak + gap) * sinA;
        }
    }
} 


// get rectangle for NOT SCALED NOR ROTATED NOR FLIPPED object
// (decompiled + fixed + rewritten version of LevelEditorLayer::getObjectRect)
std::array<CCPoint, 4> getNotTransformedObjectBox(GameObject* gameObj) {
	CCRect rect;
	CCPoint posOffset;
    CCSize size;
    // get size
    if (gameObj->m_unk4ad == false) {
        if (gameObj->m_unk4ac == true) {
            rect = gameObj->getObjectRect();
			goto m1;
        }
        if (gameObj->m_unk4F8 == false && (gameObj->m_colorSprite != nullptr || gameObj->m_hasCustomChild == true || gameObj->m_unk367 == true)) {
            size = gameObj->getContentSize();
		} else {
			size = gameObj->getContentSize() - gameObj->getOffsetPosition();
        }
    } else {
		size = gameObj->m_unk4b0;
    }
	// get offset
	posOffset = ccp(0,0);
    if ((gameObj->m_colorSprite == nullptr && gameObj->m_hasCustomChild == false) || gameObj->m_unk4F8 == true) {
		posOffset = *(CCPoint *)((long long)gameObj + 0x1c4); // todo
    }
    rect.origin = posOffset + gameObj->getPosition() - size * 0.5f;
	rect.size = size;

m1: 
	std::array<CCPoint, 4> ret = {
	    rect.origin,
	    rect.origin + ccp(rect.size.width, 0),
	    rect.origin + rect.size,
	    rect.origin + ccp(0, rect.size.height),
    };
	return ret;
}

std::array<CCPoint, 4> getTransformedObjectBox(GameObject* obj) {
	auto rotX = obj->getRotationX();
	auto rotY = obj->getRotationY();
	auto scaleX = obj->getScaleX();
	auto scaleY = obj->getScaleY();

    auto ret = getNotTransformedObjectBox(obj);
	
	auto center = obj->getPosition();

	double cosX = std::cos(rotX*M_PI/180);
	double sinX = std::sin(rotX*M_PI/180);
	double cosY = std::cos(rotY*M_PI/180);
	double sinY = std::sin(rotY*M_PI/180);

	for (int i = 0; i < 4; i++) {
		auto point = ret[i];
		point -= center;
		point.x *= scaleX;
		point.y *= scaleY;
		float px = point.x;
		point.x = (point.x * cosY + point.y * sinX);
		point.y = (point.y * cosX - px * sinY);
		point += center;
		ret[i] = point;
	}
	return ret;
}

// find out if two segments intersect
// thanks: https://users.livejournal.com/-winnie/152327.html
bool segmentsIntersect(CCPoint start1, CCPoint end1, CCPoint start2, CCPoint end2) {
    CCPoint dir1 = end1 - start1;
    CCPoint dir2 = end2 - start2;

    float a1 = -dir1.y;
    float b1 = +dir1.x;
    float d1 = -(a1*start1.x + b1*start1.y);

    float a2 = -dir2.y;
    float b2 = +dir2.x;
    float d2 = -(a2*start2.x + b2*start2.y);

    float seg1Line2Start = a2*start1.x + b2*start1.y + d2;
    float seg1Line2End = a2*end1.x + b2*end1.y + d2;

    float seg2Line1Start = a1*start2.x + b1*start2.y + d1;
    float seg2Line1End = a1*end2.x + b1*end2.y + d1;

    if (seg1Line2Start * seg1Line2End >= 0 || seg2Line1Start * seg2Line1End > 0) 
        return false;

    // float u = seg1Line2Start / (seg1Line2Start - seg1Line2End);
    // intersection =  start1 + u*dir1;
    return true;
}

// precisely check object box
// returns true if object is in the box by any part
bool preciseBoxCheck(std::vector<CCPoint>* polygon, std::array<CCPoint, 4>* objBox) {
    if (polygon->size() <= 1) return false;

    // check edges intersections
    CCPoint a, b;
    bool areIntersect = false;
    b = polygon->at(polygon->size()-1); // last
    for (int i = 0; i < polygon->size(); i++) {
        a = polygon->at(i);
        if (segmentsIntersect(a, b, objBox->at(0), objBox->at(1)) ||
            segmentsIntersect(a, b, objBox->at(1), objBox->at(2)) ||
            segmentsIntersect(a, b, objBox->at(2), objBox->at(3)) ||
            segmentsIntersect(a, b, objBox->at(3), objBox->at(0))) {
            areIntersect = true;
            break;
        }
        b = a;
    }
    if (areIntersect) return true;

    // is the box fully inside the polygon
    const auto boxControlPoint = objBox->at(0);
    const auto boxControlPoint2 = boxControlPoint + ccp(0, 5000000);
    int intersectCount = 0;
    b = polygon->at(polygon->size()-1); // last
    for (int i = 0; i < polygon->size(); i++) {
        a = polygon->at(i);
        if (segmentsIntersect(a, b, boxControlPoint, boxControlPoint2)) {
            intersectCount++;
        }
        b = a;
    }
    if (intersectCount % 2 == 1) return true;

    // is the polygon fully inside the box
    const auto polygonControlPoint = polygon->at(0);
    const auto polygonControlPoint2 = polygonControlPoint + ccp(0, 5000000);
    intersectCount = 0;
    if(segmentsIntersect(polygonControlPoint, polygonControlPoint2, objBox->at(0), objBox->at(1))) intersectCount++;
    if(segmentsIntersect(polygonControlPoint, polygonControlPoint2, objBox->at(1), objBox->at(2))) intersectCount++;
    if(segmentsIntersect(polygonControlPoint, polygonControlPoint2, objBox->at(2), objBox->at(3))) intersectCount++;
    if(segmentsIntersect(polygonControlPoint, polygonControlPoint2, objBox->at(3), objBox->at(0))) intersectCount++;
    if (intersectCount % 2 == 1) return true;

    return false;
}


// REQUIREMENTS FOR THIS FUNCTION:
// groupId filter - works
// custom filter - works
// static/details filter - deferred (not in vanilla)
// editor layers - works
// layer lock - works
// undo-redo - works
// append selection - works
// swipe cycle mode - works
void BasicSelectionTool::applySelection() {
    if (m_points.size() == 0) return;
    auto editor = EditorUI::get();
    auto levelLayer = LevelEditorLayer::get();

    // get selection bounding box
    float xMin = m_points[0].x;
    float yMax = m_points[0].y;
    float xMax = m_points[0].x;
    float yMin = m_points[0].y;

    for (int i = 1; i < m_points.size(); i++) {
        auto point = m_points[i];
        if (point.x > xMax) xMax = point.x;
        else if (point.x < xMin) xMin = point.x;
        if (point.y < yMin) yMin = point.y;
        else if (point.y > yMax) yMax = point.y;
    }

    // get objects in selection bounding box
    auto allObjects = levelLayer->m_objects;
    auto objectsInPolygon = CCArray::create();
    auto selectedBefore = CCSet::create();
    const short currLayer = levelLayer->m_currentLayer;

    if (auto selObjects = editor->getSelectedObjects()) {
        for (int i = 0; i < selObjects->count(); i++) {
            selectedBefore->addObject(selObjects->objectAtIndex(i));
        }
    }

    for (int i = 0; i < allObjects->count(); i++) {
        auto obj = static_cast<GameObject*>(allObjects->objectAtIndex(i));
        /*
        // check editor layer
        if (currLayer != -1 && currLayer != obj->m_editorLayer && currLayer != obj->m_editorLayer2) {
            continue;
        }

        // locked layers
        if (levelLayer->isLayerLocked(obj->m_editorLayer) || levelLayer->isLayerLocked(obj->m_editorLayer2)) {
            continue;
        }

        // todo: maybe add static/detail filter (although it works only for deleting in vanilla GD)
        // it filters by obj->m_objectType (static - are all except 7, detail - are all except 0 and 25)

        // roughly check object box
        auto roughObjBox = obj->boundingBox();
        if (roughObjBox.getMinX() > xMax || roughObjBox.getMaxX() < xMin || roughObjBox.getMinY() > yMax || roughObjBox.getMaxY() < yMin) {
            continue; // obj is 100% outside the selection area
        }

        // object is already selected
        if (selectedBefore->containsObject(obj)) {
            continue;
        }
        */
        // precisely check object box
        auto objBox = getTransformedObjectBox(obj);
        if (preciseBoxCheck(&m_points, &objBox)) {
            objectsInPolygon->addObject(obj);
        }
    }

    // select objects in box, create undo objects and update interface
    if (objectsInPolygon->count()) {
        auto undoObj = UndoObject::createWithArray(editor->getSelectedObjects(), UndoCommand::Select);
        undoObj->m_redo = false;
        levelLayer->m_undoObjects->addObject(undoObj);
        levelLayer->m_redoObjects->removeAllObjects();

        int maxUndoRedo = levelLayer->m_increaseMaxUndoRedo ? 1000 : 200;
        if (levelLayer->m_undoObjects->count() >= maxUndoRedo) {
            levelLayer->m_undoObjects->removeObjectAtIndex(0);
        }

        editor->selectObjects(objectsInPolygon, false); // 2nd arg is ignoreSelectFilter

        editor->updateButtons();
        editor->updateObjectInfoLabel();
        // editor->deactivateRotationControl();
        editor->deactivateScaleControl();
        editor->deactivateTransformControl();
        
    }
}




