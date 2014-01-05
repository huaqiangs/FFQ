#ifndef _MY_GAME_H__
#define _MY_GAME_H__

#include "cocos2d.h"

namespace my
{
	class Game : public cocos2d::CCLayer
	{
	public:
		Game();
		virtual ~Game();

		bool init();

		static Game* create();

	private:

	};
}

#endif