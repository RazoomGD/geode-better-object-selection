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

std::array<CCPoint, 4> getTransformedObjectBox(GameObject* obj) {
	auto rotX = obj->getRotationX();
	auto rotY = obj->getRotationY();
	auto scaleX = obj->getScaleX();
	auto scaleY = obj->getScaleY();
	obj->setRotation(0); // rotation is bugged in RobTop's function
    obj->setScaleX(scaleX > 0 ? 1 : -1); // scale is bugged in RobTop's function as well
    obj->setScaleY(scaleY > 0 ? 1 : -1);
	auto box = LevelEditorLayer::get()->getObjectRect(obj, 0, 0);
	obj->setRotationX(rotX);
	obj->setRotationY(rotY);
	obj->setScaleX(scaleX);
	obj->setScaleY(scaleY);
	
	std::array<CCPoint, 4> ret = {
		box.origin,
		box.origin + ccp(box.size.width, 0),
		box.origin + box.size,
		box.origin + ccp(0, box.size.height)
	};

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

    // check edges
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


// REQUIREMENTS LIST FOR THIS FUNCTION:
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
    auto objectsInBox = CCArray::create();
    const short currLayer = levelLayer->m_currentLayer;

    auto selectedBefore = CCSet::create();
    if (auto selObjects = editor->getSelectedObjects()) {
        for (int i = 0; i < selObjects->count(); i++) {
            selectedBefore->addObject(selObjects->objectAtIndex(i));
        }
    }

    for (int i = 0; i < allObjects->count(); i++) {
        auto obj = static_cast<GameObject*>(allObjects->objectAtIndex(i));

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

        // precisely check object box
        auto objBox = getTransformedObjectBox(obj);
        if (preciseBoxCheck(&m_points, &objBox)) {
            objectsInBox->addObject(obj);
        }
    }

    // select objects in box, create undo objects and update interface
    if (objectsInBox->count()) {
        auto undoObj = UndoObject::createWithArray(editor->getSelectedObjects(), UndoCommand::Select);
        undoObj->m_redo = false;
        levelLayer->m_undoObjects->addObject(undoObj);
        levelLayer->m_redoObjects->removeAllObjects();
        // todo: check for previously selected objects

        int maxUndoRedo = levelLayer->m_increaseMaxUndoRedo ? 1000 : 200;
        if (levelLayer->m_undoObjects->count() >= maxUndoRedo) {
            levelLayer->m_undoObjects->removeObjectAtIndex(0);
        }

        editor->selectObjects(objectsInBox, false); // 2nd arg is ignoreSelectFilter

        editor->updateButtons();
        editor->updateObjectInfoLabel();
        // editor->deactivateRotationControl();
        editor->deactivateScaleControl();
        editor->deactivateTransformControl();
        
    }

}
/*
// decompiled function
CCRect * FUN_1402c1df0(LevelEditorLayer* levelLayer,CCRect *retRect, GameObject *gameObj) {
    CCSize tmpSize;
    CCRect *pCVar1;
    long long lVar2;
    CCNode* lVar3;
    undefined8 uVar4;
    CCPoint *pCVar5;
    float *pfVar6;
    float fVar7;
    float objRScaleX;
    float objRScaleY;
    unsigned uVar8;
    CCSize size;
    float local_res18;
    float local_res1c;
    CCSize gameObjSizeCopy;
    CCSize gameObjSize;
    CCSize gameObjSizeCopy2;
    CCSize sizesArr56 [0x38];
    
    *(levelLayer + 0x37ad) = 0x0;
    if (gameObj->m_unk4ad == false) {
        if (gameObj->m_unk4ac == true) {
            *retRect = gameObj->getObjectRect();
            return retRect;
        }
        if ((gameObj->m_unk4F8 == false) &&
           (gameObj->m_colorSprite != nullptr || gameObj->m_hasCustomChild == true || gameObj->m_unk367 == true)) {
                    // 130 - get content size
            tmpSize = gameObj->getContentSize();
            gameObjSizeCopy = tmpSize;
            CCSize objContentSize = gameObj->getContentSize();
            objRScaleY = gameObj->getRScaleY();
            objRScaleX = gameObj->getRScaleX();
            sizesArr56[0].width = objRScaleX * objContentSize.width;
            sizesArr56[0].height = objRScaleY * objContentSize.height;
            size = tmpSize;
        }
        else {
            cocos2d::CCSize::operator=(&gameObjSizeCopy,gameObj + 0x36);
                    // 518 - getRScaleY, 510 - getRScaleX
            objRScaleY = gameObj->getRScaleY();
            fVar7 = *(gameObj + 0x1b4);
            objRScaleX = gameObj->getRScaleX();
            tmpSize = CCSizeMake(objRScaleX * *(gameObj + 0x36),objRScaleY * fVar7) ;
            size = tmpSize;
            sizesArr56[0] = tmpSize;
        }
    }
    else {
        gameObjSize = gameObj->m_unk4b0;
        gameObjSizeCopy = gameObjSize;
        gameObjSizeCopy2 = gameObj->m_unk4b0;
        fVar7 = gameObj->getRScaleY();
        objRScaleY = gameObj->getRScaleX();
        tmpSize = CCSizeMake(objRScaleY * gameObjSizeCopy2.width, fVar7 * gameObjSize.height);
        sizesArr56[0] = tmpSize;
        // cocos2d::CCSize::operator=(&size,tmpSize);
        size = tmpSize;
    }
    fVar7 = 7.5;
    if (7.5 <= size.width) {
        fVar7 = size.width;
    }
    objRScaleY = 7.5;
    if (7.5 <= size.height) {
        objRScaleY = size.height;
    }
    size.width = fVar7;
    size.height = objRScaleY;
    cocos2d::CCPoint::CCPoint(&local_res18,&DAT_140687068);
    if (gameObj->m_unk4ad == false) {
        // fVar7 = (**(*gameObj + 0x158))(gameObj);
        fVar7 = gameObj->getRotation();
        if ((fVar7 != (fVar7 / 0x5a) * 0x5a) && (lVar3 = gameObj->getParent(), lVar3 != nullptr)) {
            if ((gameObj->m_colorSprite == nullptr && gameObj->m_hasCustomChild == false) || gameObj->m_unk4F8 == true) {
                cocos2d::CCPoint::operator=(&local_res18,gameObj + 0x1bc);
            }
            uVar4 = cocos2d::CCPoint::CCPoint(sizesArr56,gameObjSizeCopy.width * 0.5 + local_res18,gameObjSizeCopy.height * 0.5 + local_res1c);
            cocos2d::CCNode::convertToWorldSpace(gameObj,gameObjSize,uVar4);
            pCVar5 = cocos2d::CCNode::convertToNodeSpace(*(levelLayer + 0xfe0),sizesArr56,gameObjSize);
            gameObjSize = *pCVar5;
            // uVar8 = (**(*gameObj + 0x158))(gameObj);
            uVar8 = gameObj->getRotation();
            uVar4 = cocos2d::CCPoint::CCPoint(sizesArr56,gameObjSize);
            // OBB2D : void calculateWithCenter(cocos2d::CCPoint, float, float, float)
            FUN_14006c560(*(levelLayer + 0x3790),uVar4,size,size.height,(uVar8 ^ 0x80000000) * 0.01745329);
            *(levelLayer + 0x37ad) = 0x1;
            // OBB2D : TodoREturn getBoundingRect()
            FUN_14006cd50(*(levelLayer + 0x3790),retRect);
            return retRect;
        }
        fVar7 = size.width;
        if (gameObj->m_isRotationAligned == true) {
            size.width = size.height;
            size.height = fVar7;
        }
    }
    if (((gameObj->m_colorSprite == nullptr) && gameObj->m_hasCustomChild == false) || gameObj->m_unk4F8 == true) {
        uVar4 = cocos2d::CCPoint::CCPoint(sizesArr56,gameObj + 0x1c4);
        // GameToolbox : static TodoReturn getRelativeOffset(GameObject*, cocos2d::CCPoint)
        // FUN_140063380(gameObjSizeCopy2,gameObj,uVar4);
        gameObjSizeCopy2 = GameToolbox::getRelativeOffset(gameObj, uVar4); // todo: it returns ccsize
        local_res18 = gameObjSizeCopy2.width;
    }
    // lVar3 = (**(*gameObj + 0xc8))(gameObj);
    CCPoint posPoint = gameObj->getPosition();
    objRScaleY = size.height * 0.5;
    // fVar7 = *(lVar3 + 0x4);
    fVar7 = posPoint.y;
    // pfVar6 = (**(*gameObj + 0xc8))(gameObj);
    // cocos2d::CCRect::CCRect(retRect,(local_res18 + *pfVar6) - size.width * 0.5,(fVar7 + local_res1c) - objRScaleY,size.width,size.height);
    retRect->origin = ccp((local_res18 + posPoint.x) - size.width * 0.5,(fVar7 + local_res1c) - objRScaleY);
    retRect->size = size;
    return retRect;
}
*/

