#include "config.hpp"

#include "SelectionLayer.hpp"
#include "BasicSelectionTool.hpp"
#include "CircleSelectionTool.hpp"
#include "RectSelectionTool.hpp"

#include <Geode/modify/EditorUI.hpp>


// state[0] - 1 when 'swipe' button (at the right corner) is active, else 0
// state[1] - 1 when 'swipe' rectangle should be drawn and objects should be selected after, else 0
// state[2] - 1 when touching screen with 'shift' or with active 'swipe' button, else 0
// state[3] - ?
// values are invalid before the first touch
#define GET_SEL_STATE ((uint8_t*) &this->m_unk244);

class MyObj : public GameObject {
public:
	CCSize getOffset36Value() {
		log::debug("offset field = {}", (uint64_t) (&(m_obRect.size)) - (uint64_t)this);
		return m_obRect.size;
	}
};

class $modify(MyEditorUI, EditorUI) {
	struct Fields {
		SelMode m_mode;
		SelectionLayer* m_selLayer;
		Ref<BasicSelectionTool> m_selTool = nullptr;
		Ref<CCMenu> m_bar;
		bool m_inTouch = false;

		Fields() {
			m_mode = (SelMode) Mod::get()->getSavedValue<int>("mode");
			// validate value 
			bool valid = false;
			SEL_MODE_ENUM_FOR_LOOP(m) {
				if (m_mode == m) {
					valid = true;
					break;
				}
			}
			if (!valid) {
				m_mode = SelMode::RECT;
			}
		}
		~Fields() {
			Mod::get()->setSavedValue<int>("mode", (int) m_mode);
		}
	};

	$override 
	bool init(LevelEditorLayer *editorLayer) {
		if(!EditorUI::init(editorLayer)) return false;
		CCSize winSize = CCDirector::sharedDirector()->getWinSize();

		// bar
		auto bar = CCMenu::create();
		m_fields->m_bar = bar;
		bar->setLayout(RowLayout::create());
		bar->setID("razoom.swipe-options-menu");
		float scale = 1;
		if (auto ch = this->getChildByID("undo-menu")) {
			scale *= ch->getScaleX(); // interface scale
			// todo: check it work - nope
		}
		bar->setScale(scale * 0.8f);
		this->addChild(bar, 2);
		bar->setPosition(winSize.width / 2, winSize.height - 50);

		// buttons
		auto btnRect = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_copyStateBtn_001.png"), 
			this, menu_selector(MyEditorUI::onModeButton));

		auto btnCirc = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_pasteBtn2_001.png"),
			this, menu_selector(MyEditorUI::onModeButton));

		auto btnArea = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_editObjBtn4_001.png"), 
			this, menu_selector(MyEditorUI::onModeButton));

		auto infoBtn = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"), 
			this, menu_selector(MyEditorUI::onInfoButton));
		
		bar->addChild(btnRect, 0, (int) SelMode::RECT);
		bar->addChild(btnCirc, 0, (int) SelMode::CIRCLE);
		bar->addChild(btnArea, 0, (int) SelMode::AREA);
		bar->addChild(infoBtn, 0);

		bar->updateLayout();

		// initial mode
		switch (m_fields->m_mode) {
			case SelMode::RECT: onModeButton(btnRect); break;
			case SelMode::CIRCLE: onModeButton(btnCirc); break;
			case SelMode::AREA: onModeButton(btnArea); break;
			default: break;
		}

		// drawing layer
		m_fields->m_selLayer = SelectionLayer::create(this);
		editorLayer->m_drawGridLayer->getParent()->addChild(m_fields->m_selLayer, 2240);
		m_fields->m_selLayer->setID("razoom.selection-layer");

		return true;
	}

	// void printState(const char* msg) {
	// 	uint8_t* state = GET_SEL_STATE;
	// 	log::debug("state - {} - {} {} {} {}", msg, state[0], state[1], state[2], state[3]);
	// }

	// Is called every frame
	// The only thing that this function does in source code is to draw a selection rectangle
	$override
	void draw() {
		
		uint8_t* state = GET_SEL_STATE;
		auto oldVal = state[1];
		state[1] = 0; // don't draw selection rectangle
		EditorUI::draw(); // call it in case somebody wants to hook it too
		state[1] = oldVal;
	}

	void drawSelectionThings() {
		uint8_t* state = GET_SEL_STATE;
		if (state[1]) {
			m_fields->m_selTool->drawPolygon();
		}
	}

	// bool ccTouchBegan(CCTouch* p0, CCEvent* p1) {
		// uint8_t* state = GET_SEL_STATE; 
		// if (state[1]) {
		// 	m_fields->m_selTool->handleTouchStart(p0);
		// 	m_fields->m_inTouch = true;
		// 	return true; // todo: or false...
		// }
		// return EditorUI::ccTouchBegan(p0, p1);
	// }

	$override
	void ccTouchMoved(CCTouch* p0, CCEvent* p1) {
		uint8_t* state = GET_SEL_STATE; 
		if (state[1] || m_fields->m_inTouch) {
			if (!m_fields->m_inTouch) {
				m_fields->m_selTool->handleTouchStart(p0);
				m_fields->m_inTouch = true;
			} else {
				m_fields->m_selTool->handleTouchMove(p0);
			}
			return;
		}
		EditorUI::ccTouchMoved(p0, p1);
	}

	$override
	void ccTouchEnded(CCTouch* p0, CCEvent* p1) {
		if (m_fields->m_inTouch) {
			uint8_t* state = GET_SEL_STATE;
			state[1] = 0; // set to 0 so that there is no selection of objects
			m_fields->m_selTool->handleTouchEnd(p0);
			m_fields->m_inTouch = false;
		}

		EditorUI::ccTouchEnded(p0, p1);
	}

	void onModeButton(CCObject* sender) {
		auto btn = static_cast<CCNodeRGBA*>(sender);
		auto mode = (SelMode) btn->getTag(); // tag is mode
		if (mode == m_fields->m_mode && m_fields->m_selTool != nullptr) return;

		log::debug("mode = {}", (int) mode);

		switch (mode) {
			case SelMode::RECT: {
				m_fields->m_selTool = RectSelectionTool::create();
				break;
			}
			case SelMode::CIRCLE: {
				m_fields->m_selTool = CircleSelectionTool::create();
				break;
			}
			case SelMode::AREA: {
				// todo: add area tool
				break;
			}
		}

		SEL_MODE_ENUM_FOR_LOOP(m) {
			static_cast<CCNodeRGBA*>(m_fields->m_bar->getChildByTag((int) m))->setColor({255,255,255});
		}

		btn->setColor({128,128,128});
		m_fields->m_mode = mode;
	}

	void onInfoButton(CCObject* sender) {
		// FLAlertLayer::create(
		// 	"title",
		// 	"description",
		// 	"Ok"
		// )->show();
		// log::debug("all obj 1 = {}", LevelEditorLayer::get()->m_objects->count());
		// log::debug("all obj 2 = {}", LevelEditorLayer::get()->getAllObjects());
		// auto obj = static_cast<GameObject*>(this->getSelectedObjects()->objectAtIndex(0));
		// log::debug("object: {}", obj->getObjectRect());
		// log::debug("object: {}", obj->getObjectRectDirty());
		// log::debug("object: {}", obj->getObjectRect2(0,0));
		// log::debug("object: {}", obj->getBoundingRect());
		if (!this->getSelectedObjects() || this->getSelectedObjects()->count() == 0) return;
		auto selected = this->getSelectedObjects();
		auto obj = static_cast<GameObject*>(selected->objectAtIndex(0));
		// log::debug("anchor {}", obj->get);
		// log::debug("size = {} --- {}", *((CCSize*) obj+0x36), obj->getOffsetPosition());

		// log::debug("size = {} == {}", ((MyObj*) obj)->getOffset36Value(), obj->getTextureRect().size);
		// log::debug("flip = {} --- {}", obj->getUnflippedOffsetPosition(), obj->getOffsetPosition());
		// auto levelLayer = LevelEditorLayer::get();
		// log::debug("QQQQQQQqqq = {}", (uint64_t) &(levelLayer->__pad1378[1]) - (uint64_t) levelLayer);
	}
};

std::array<CCPoint, 4> getTransformedObjectBox(GameObject* obj);
std::array<CCPoint, 4> getNotTransformedObjectBox(GameObject* gameObj);

// function for debug
void drawDebugObjectBoxes(CCArray* objects) {
	if (!objects) return;

	ccDrawColor4B(0,0,255,255);
	for (int i = 0; i < objects->count(); i++) {
		auto obj = static_cast<GameObject*>(objects->objectAtIndex(i));
		// function that I use for calculating precise object boxes
		// 	auto rect = LevelEditorLayer::get()->getObjectRect(obj, 0, 0);
		auto corners = getTransformedObjectBox(obj);
		ccDrawLine(corners[0], corners[1]);
		ccDrawLine(corners[1], corners[2]);
		ccDrawLine(corners[2], corners[3]);
		ccDrawLine(corners[3], corners[0]);
	}

	ccDrawColor4B(255,0,0,255);
	for (int i = 0; i < objects->count(); i++) {
		auto obj = static_cast<GameObject*>(objects->objectAtIndex(i));
		// function that I use for calculating precise object boxes
		auto rect = LevelEditorLayer::get()->getObjectRect(obj, 0, 0);
		std::array<CCPoint, 4> corners = {
			rect.origin,
			rect.origin + ccp(rect.size.width, 0),
			rect.origin + rect.size,
			rect.origin + ccp(0, rect.size.height),
		};
		// auto corners = getTransformedObjectBox(obj);
		ccDrawLine(corners[0], corners[1]);
		ccDrawLine(corners[1], corners[2]);
		ccDrawLine(corners[2], corners[3]);
		ccDrawLine(corners[3], corners[0]);
	}

	ccDrawColor4B(255,255,0,255);
	for (int i = 0; i < objects->count(); i++) {
		auto obj = static_cast<GameObject*>(objects->objectAtIndex(i));
		// function that I use for calculating precise object boxes
		// 	auto rect = LevelEditorLayer::get()->getObjectRect(obj, 0, 0);
		auto corners = getNotTransformedObjectBox(obj);
		ccDrawLine(corners[0], corners[1]);
		ccDrawLine(corners[1], corners[2]);
		ccDrawLine(corners[2], corners[3]);
		ccDrawLine(corners[3], corners[0]);
	}
}

void SelectionLayer::draw() {
	// calls editorUI drawSelectionThings() function on 
	// this node, so that it affects this node
	m_editor->drawSelectionThings(); 
	auto selected = m_editor->getSelectedObjects();
	drawDebugObjectBoxes(selected);
};
