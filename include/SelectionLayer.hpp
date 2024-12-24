#pragma once

#include "config.hpp"

struct MyEditorUI;

class SelectionLayer : public CCNode {
private:
	MyEditorUI* m_editor;
public:
	bool init(MyEditorUI* editor) {
        this->m_editor = editor;
        return true;
    }
	static SelectionLayer* create(MyEditorUI* editor) {
        auto ret = new SelectionLayer();
        if (ret && ret->init(editor)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
	void draw() override;
};