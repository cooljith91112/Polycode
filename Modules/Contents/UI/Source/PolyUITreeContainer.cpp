/*
 Copyright (C) 2012 by Ivan Safrin
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "PolyUITreeContainer.h"
#include "PolyConfig.h"
#include "PolyInputEvent.h"
#include "PolyLabel.h"
#include "PolyCoreServices.h"
#include "PolyCore.h"

using namespace Polycode;

UITreeContainer::UITreeContainer(String icon, String text, Number treeWidth, Number treeHeight) : UIElement() {
	
	Config *conf = CoreServices::getInstance()->getConfig();
	
	Number st = conf->getNumericValue("Polycode", "uiTreeContainerSkinT");
	Number sr = conf->getNumericValue("Polycode", "uiTreeContainerSkinR");
	Number sb = conf->getNumericValue("Polycode", "uiTreeContainerSkinB");
	Number sl = conf->getNumericValue("Polycode", "uiTreeContainerSkinL");	
	Number padding = conf->getNumericValue("Polycode", "uiTreeContainerSkinPadding");	
	
//	Number scrollBarOffset = conf->getNumericValue("Polycode", "uiTreeContainerScrollBarOffset");	
	
	bgBox = new UIBox(conf->getStringValue("Polycode", "uiTreeContainerSkin"),
						  st,sr,sb,sl,
						  treeWidth, treeHeight);
	
	addChild(bgBox);
	bgBox->blockMouseInput = true;
	blockMouseInput = true;
	//bgBox->setPosition(-padding, -padding);
	
	scrollChild = new ScreenEntity();
	scrollChild->processInputEvents = true;
	
	rootNode = new UITree(icon, text, treeWidth,0);		
	rootNode->addEventListener(this, UITreeEvent::NEED_REFRESH_EVENT);	
	scrollChild->addChild(rootNode);
	
	mainContainer = new UIScrollContainer(scrollChild, false, true, treeWidth-conf->getNumericValue("Polycode", "uiScrollDefaultSize"), treeHeight);
	addChild(mainContainer);
	
	width = treeWidth;
	height = treeHeight;
	setHitbox(width, height);
	
	Resize(width, height);

	CoreServices::getInstance()->getCore()->getInput()->addEventListener(this, InputEvent::EVENT_KEYDOWN);
	this->addEventListener(this, InputEvent::EVENT_MOUSEDOWN);
	focusable = true;
	keyNavigable = true;
}

void UITreeContainer::Resize(Number width, Number height) {
	mainContainer->Resize(width,height);
	bgBox->resizeBox(width, height);
	mainContainer->setPositionY(0);

	rootNode->Resize(width);
//	width = x;
	//	height = y;
	setHitbox(width, height);
}

void UITreeContainer::handleEvent(Event *event) {
	if(event->getDispatcher() == rootNode) {
		if(event->getEventCode() == UITreeEvent::NEED_REFRESH_EVENT) {
			mainContainer->setContentSize(rootNode->getWidth(), rootNode->getHeight());
		}
	}
	
	if (!hasFocus && event->getDispatcher() == this) {
		if (event->getEventCode() == InputEvent::EVENT_MOUSEDOWN) {
			if (parentEntity && isFocusable())
				((ScreenEntity*)parentEntity)->focusChild(this);
		}
	}

}

UITree *UITreeContainer::getRootNode() {
	return rootNode;
}

UITreeContainer::~UITreeContainer() {
	if(!ownsChildren) {
		delete bgBox;
		delete scrollChild;
		delete rootNode;
		delete mainContainer;
	}
}

void UITreeContainer::onGainFocus() {
	if (!getRootNode()->getSelectedNode())
		getRootNode()->setSelected();
}

// RECURSIVE HELPER FUNCTIONS
//
// used in KEY_UP tree nav
UITree *UITreeContainer::findLastOpenNode(UITree *tree) {
	if (!tree->hasTreeChildren() || tree->isCollapsed())
		return tree;
	UITree *t = tree->getTreeChild(tree->getNumTreeChildren()-1);
	if (t->isCollapsed() || !t->hasTreeChildren())
		return t;
	else
		return findLastOpenNode(t);
}
// used in KEY_DOWN tree nav
UITree *UITreeContainer::findNextParentSibling(UITree *parent) {
	UITree *sibling = parent->getNextSibling();
	if (sibling)
		return sibling;
	else {
		if (parent->getParent())
			return findNextParentSibling(parent->getParent());
		else
			return NULL;
	}
}
//
// END RECURSIVE HELPER FUNCTIONS

void UITreeContainer::onKeyDown(PolyKEY key, wchar_t charCode) {
	if (hasFocus) {
		
		// KEYBOARD NAV STUFF
		// TODO: add ability to jump to prev/next sibling via holding down some button
		//
		if (keyNavigable) {
			enum { NONE, UP, DOWN } scrollDir = NONE;
			UITree *currentSelection = getRootNode()->getSelectedNode();
			if (!currentSelection)
				return;
			UITree *parent = currentSelection->getParent();
			
			// - select parent if currently selected node is first in the tree
			// - else, select the last opened node in a sibling above
			if (key == KEY_UP) {
				if (parent) {
					for (int i=0; i < parent->getNumTreeChildren(); i++) {
						if (parent->getTreeChild(i) == currentSelection) {
							if (i == 0)
								parent->setSelected();
							else
								findLastOpenNode((parent->getTreeChild(i-1)))->setSelected();
							scrollDir = UP;
							break;
						}
					}
				}
			}
			
			// - select first child of currently selected if expanded and has children
			// - or select next sibling
			// - else, select the next sibling of some ancestor node
			if (key == KEY_DOWN) {
				if (currentSelection == getRootNode())
					parent = getRootNode();

				if (currentSelection->hasTreeChildren() && !currentSelection->isCollapsed()) {
					currentSelection->getTreeChild(0)->setSelected();
					scrollDir = DOWN;
				}
				else {
					for (int i=0; i < parent->getNumTreeChildren(); i++) {
						if (parent->getTreeChild(i) == currentSelection) {
							if (i == parent->getNumTreeChildren()-1) {
								UITree *psib = findNextParentSibling(parent);
								if (psib)
									psib->setSelected();
							} else {
								parent->getTreeChild(i+1)->setSelected();
							}
							scrollDir = DOWN;
							break;
						}
					}
				}
			}
			
			if (key == KEY_LEFT) {
				if (currentSelection->hasTreeChildren() && !currentSelection->isCollapsed())
					currentSelection->toggleCollapsed();
				else if (parent) {
					parent->setSelected();
					scrollDir = UP;
				}
			}
			
			if (key == KEY_RIGHT) {
				if (currentSelection->hasTreeChildren()) {
					if (currentSelection->isCollapsed())
						currentSelection->toggleCollapsed();
					else {
						currentSelection->getTreeChild(0)->setSelected();
						scrollDir = DOWN;
					}
				}
			}
			
			if (scrollDir != NONE)
				scrollToNode(getRootNode()->getSelectedNode(), (scrollDir == UP) ? true : false, false);
		}
		//
		// END KEYBOARD NAV STUFF
		
	}
}

void UITreeContainer::scrollToNode(UITree *node, bool showAtTop, bool select) {
	
	Number nodeY = node->getScreenPosition().y - getRootNode()->getScreenPosition().y;
	Number contentHeight = mainContainer->getContentSize().y;
	Number scrollHeight = contentHeight - mainContainer->getHeight();
	Number viewTop = (contentHeight - mainContainer->getHeight()) * mainContainer->getVScrollBar()->getScrollValue();
	Number viewBottom = viewTop + mainContainer->getHeight();
	
	if (select)
		node->setSelected();
	
	if (nodeY < viewTop || nodeY+node->getCellHeight() > viewBottom) {
		if (showAtTop)
			mainContainer->scrollVertical((nodeY-viewTop) / scrollHeight);
		else
			mainContainer->scrollVertical((nodeY+node->getCellHeight()-viewBottom) / scrollHeight);
	}
}
