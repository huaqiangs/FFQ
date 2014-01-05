#include "Game.h"
#include "ext\GUI\ListView.h"

namespace my
{
	Game::Game()
	{
	}

	Game::~Game()
	{
	}

	bool Game::init() {
		bool pRet = false;
		do {
			CCLOG("my::Game::init().");

			CCLayerColor* layer = CCLayerColor::create(ccc4(128,128,128,128));

			my::ListView* listView = my::ListView::create(cocos2d::CCSize(640, 960), layer);

			this->addChild(listView);

			for (int i = 0; i < 10; i++)
			{
				CCSprite* sprite = CCSprite::create("res/ui_bg_a036.png");
				// sprite->setAnchorPoint(ccp(0.0f, 0.0f));
				listView->addCell(sprite);

				CCLabelTTF* m_pLabel = CCLabelTTF::create(CCString::createWithFormat("IDX:%d", i)->getCString(), "Arial", 40);
				sprite->addChild(m_pLabel);
				m_pLabel->setPosition(ccp(sprite->getContentSize().width / 2, sprite->getContentSize().height / 2));
			}

			listView->reorderAllCells();

			// layer->setPosition(ccp(listView->getViewSize().width / 2, 0));

			pRet = true;
		} while(0);
		return pRet;
	}

	Game* Game::create() {
		Game* pRet = new Game();
		if (pRet && pRet->init())
		{
			pRet->autorelease();
		}
		else
		{
			CC_SAFE_DELETE(pRet);
		}
		return pRet;
	}
}