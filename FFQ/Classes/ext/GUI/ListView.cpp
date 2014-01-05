/****************************************************************************
 Copyright (c) 2012 cocos2d-x.org
 Copyright (c) 2010 Sangwoo Im
 
 http://www.cocos2d-x.org
 
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
 ****************************************************************************/

#include "ListView.h"

namespace my
{

	#define SCROLL_DEACCEL_RATE  0.95f
	#define SCROLL_DEACCEL_DIST  1.0f
	#define BOUNCE_DURATION      0.15f
	#define INSET_RATIO          0.2f
	#define MOVE_INCH            7.0f/160.0f

	static float convertDistanceFromPointToInch(float pointDis)
	{
		float factor = ( CCEGLView::sharedOpenGLView()->getScaleX() + CCEGLView::sharedOpenGLView()->getScaleY() ) / 2;
		return pointDis * factor / CCDevice::getDPI();
	}


	ListView::ListView()
	: m_fZoomScale(0.0f)
	, m_fMinZoomScale(0.0f)
	, m_fMaxZoomScale(0.0f)
	, m_pDelegate(NULL)
	, m_eDirection(kListViewDirectionBoth)
	, m_bDragging(false)
	, m_pContainer(NULL)
	, m_bTouchMoved(false)
	, m_bBounceable(false)
	, m_bClippingToBounds(false)
	, m_fTouchLength(0.0f)
	, m_pTouches(NULL)
	, m_fMinScale(0.0f)
	, m_fMaxScale(0.0f)
	, m_bDistanceHorizontal(false)
	, m_bDirectionVertical(false)
	, m_eAutoDirection(kListViewDirectionBoth)
	, m_pCells(NULL)
	{

	}

	ListView::~ListView()
	{
		CC_SAFE_RELEASE(m_pTouches);
		CC_SAFE_RELEASE(m_pCells);
		this->unregisterScriptHandler(kScrollViewScroll);
		this->unregisterScriptHandler(kScrollViewZoom);
	}

	ListView* ListView::create(CCSize size, CCNode* container/* = NULL*/)
	{
		ListView* pRet = new ListView();
		if (pRet && pRet->initWithViewSize(size, container))
		{
			pRet->autorelease();
		}
		else
		{
			CC_SAFE_DELETE(pRet);
		}
		return pRet;
	}

	ListView* ListView::create()
	{
		ListView* pRet = new ListView();
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


	bool ListView::initWithViewSize(CCSize size, CCNode *container/* = NULL*/)
	{
		if (CCLayer::init())
		{
			m_pContainer = container;
        
			if (!this->m_pContainer)
			{
				m_pContainer = CCLayer::create();
				this->m_pContainer->ignoreAnchorPointForPosition(false);
				this->m_pContainer->setAnchorPoint(ccp(0.0f, 0.0f));
			}

			this->setViewSize(size);

			setTouchEnabled(true);
			m_pTouches = new CCArray();
			m_pDelegate = NULL;
			m_bBounceable = true;
			m_bClippingToBounds = true;
			//m_pContainer->setContentSize(CCSizeZero);
			m_eDirection  = kListViewDirectionBoth;
			m_pContainer->setPosition(ccp(0.0f, 0.0f));
			m_fTouchLength = 0.0f;
        
			this->addChild(m_pContainer);
			m_fMinScale = m_fMaxScale = 1.0f;
			m_mapScriptHandler.clear();

			m_bDistanceHorizontal = false;
			m_bDirectionVertical = false;
			m_eAutoDirection = kListViewDirectionBoth;
			m_pCells = new CCArray();

			return true;
		}
		return false;
	}

	bool ListView::init()
	{
		return this->initWithViewSize(CCSizeMake(200, 200), NULL);
	}

	void ListView::registerWithTouchDispatcher()
	{
		CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, CCLayer::getTouchPriority(), false);
	}

	bool ListView::isNodeVisible(CCNode* node)
	{
		const CCPoint offset = this->getContentOffset();
		const CCSize  size   = this->getViewSize();
		const float   scale  = this->getZoomScale();
    
		CCRect viewRect;
    
		viewRect = CCRectMake(-offset.x/scale, -offset.y/scale, size.width/scale, size.height/scale); 
    
		return viewRect.intersectsRect(node->boundingBox());
	}

	void ListView::pause(CCObject* sender)
	{
		m_pContainer->pauseSchedulerAndActions();

		CCObject* pObj = NULL;
		CCArray* pChildren = m_pContainer->getChildren();

		CCARRAY_FOREACH(pChildren, pObj)
		{
			CCNode* pChild = (CCNode*)pObj;
			pChild->pauseSchedulerAndActions();
		}
	}

	void ListView::resume(CCObject* sender)
	{
		CCObject* pObj = NULL;
		CCArray* pChildren = m_pContainer->getChildren();

		CCARRAY_FOREACH(pChildren, pObj)
		{
			CCNode* pChild = (CCNode*)pObj;
			pChild->resumeSchedulerAndActions();
		}

		m_pContainer->resumeSchedulerAndActions();
	}

	void ListView::setTouchEnabled(bool e)
	{
		CCLayer::setTouchEnabled(e);
		if (!e)
		{
			m_bDragging = false;
			m_bTouchMoved = false;
			m_pTouches->removeAllObjects();
		}
	}

	void ListView::setContentOffset(CCPoint offset, bool animated/* = false*/)
	{
		if (animated)
		{ //animate scrolling
			this->setContentOffsetInDuration(offset, BOUNCE_DURATION);
		} 
		else
		{ //set the container position directly
			if (!m_bBounceable)
			{
				const CCPoint minOffset = this->minContainerOffset();
				const CCPoint maxOffset = this->maxContainerOffset();
            
				offset.x = MAX(minOffset.x, MIN(maxOffset.x, offset.x));
				offset.y = MAX(minOffset.y, MIN(maxOffset.y, offset.y));
			}

			m_pContainer->setPosition(offset);

			if (m_pDelegate != NULL)
			{
				m_pDelegate->scrollViewDidScroll(this);   
			}
		}
	}

	void ListView::setContentOffsetInDuration(CCPoint offset, float dt)
	{
		CCFiniteTimeAction *scroll, *expire;
    
		scroll = CCMoveTo::create(dt, offset);
		expire = CCCallFuncN::create(this, callfuncN_selector(ListView::stoppedAnimatedScroll));
		m_pContainer->runAction(CCSequence::create(scroll, expire, NULL));
		this->schedule(schedule_selector(ListView::performedAnimatedScroll));
	}

	CCPoint ListView::getContentOffset()
	{
		return m_pContainer->getPosition();
	}

	void ListView::setZoomScale(float s)
	{
		if (m_pContainer->getScale() != s)
		{
			CCPoint oldCenter, newCenter;
			CCPoint center;
        
			if (m_fTouchLength == 0.0f) 
			{
				center = ccp(m_tViewSize.width*0.5f, m_tViewSize.height*0.5f);
				center = this->convertToWorldSpace(center);
			}
			else
			{
				center = m_tTouchPoint;
			}
        
			oldCenter = m_pContainer->convertToNodeSpace(center);
			m_pContainer->setScale(MAX(m_fMinScale, MIN(m_fMaxScale, s)));
			newCenter = m_pContainer->convertToWorldSpace(oldCenter);
        
			const CCPoint offset = ccpSub(center, newCenter);
			if (m_pDelegate != NULL)
			{
				m_pDelegate->scrollViewDidZoom(this);
			}
			this->setContentOffset(ccpAdd(m_pContainer->getPosition(),offset));
		}
	}

	float ListView::getZoomScale()
	{
		return m_pContainer->getScale();
	}

	void ListView::setZoomScale(float s, bool animated)
	{
		if (animated)
		{
			this->setZoomScaleInDuration(s, BOUNCE_DURATION);
		}
		else
		{
			this->setZoomScale(s);
		}
	}

	void ListView::setZoomScaleInDuration(float s, float dt)
	{
		if (dt > 0)
		{
			if (m_pContainer->getScale() != s)
			{
				CCActionTween *scaleAction;
				scaleAction = CCActionTween::create(dt, "zoomScale", m_pContainer->getScale(), s);
				this->runAction(scaleAction);
			}
		}
		else
		{
			this->setZoomScale(s);
		}
	}

	void ListView::setViewSize(CCSize size)
	{
		m_tViewSize = size;
		CCLayer::setContentSize(size);
	}

	CCNode * ListView::getContainer()
	{
		return this->m_pContainer;
	}

	void ListView::setContainer(CCNode * pContainer)
	{
		// Make sure that 'm_pContainer' has a non-NULL value since there are
		// lots of logic that use 'm_pContainer'.
		if (NULL == pContainer)
			return;

		this->removeAllChildrenWithCleanup(true);
		this->m_pContainer = pContainer;

		this->m_pContainer->ignoreAnchorPointForPosition(false);
		this->m_pContainer->setAnchorPoint(ccp(0.0f, 0.0f));

		this->addChild(this->m_pContainer);

		this->setViewSize(this->m_tViewSize);
	}

	void ListView::relocateContainer(bool animated)
	{
		CCPoint oldPoint, min, max;
		float newX, newY;
    
		min = this->minContainerOffset();
		max = this->maxContainerOffset();
    
		oldPoint = m_pContainer->getPosition();

		newX     = oldPoint.x;
		newY     = oldPoint.y;
		if (m_eDirection == kListViewDirectionBoth || m_eDirection == kListViewDirectionHorizontal)
		{
			newX     = MAX(newX, min.x);
			newX     = MIN(newX, max.x);
		}

		if (m_eDirection == kListViewDirectionBoth || m_eDirection == kListViewDirectionVertical)
		{
			newY     = MIN(newY, max.y);
			newY     = MAX(newY, min.y);
		}

		if (newY != oldPoint.y || newX != oldPoint.x)
		{
			this->setContentOffset(ccp(newX, newY), animated);
		}
	}

	CCPoint ListView::maxContainerOffset()
	{
		return ccp(0.0f, 0.0f);
	}

	CCPoint ListView::minContainerOffset()
	{
		return ccp(m_tViewSize.width - m_pContainer->getContentSize().width*m_pContainer->getScaleX(), 
				   m_tViewSize.height - m_pContainer->getContentSize().height*m_pContainer->getScaleY());
	}

	void ListView::deaccelerateScrolling(float dt)
	{
		if (m_bDragging)
		{
			this->unschedule(schedule_selector(ListView::deaccelerateScrolling));
			return;
		}
    
		float newX, newY;
		CCPoint maxInset, minInset;
    
		m_pContainer->setPosition(ccpAdd(m_pContainer->getPosition(), m_tScrollDistance));
    
		if (m_bBounceable)
		{
			maxInset = m_fMaxInset;
			minInset = m_fMinInset;
		}
		else
		{
			maxInset = this->maxContainerOffset();
			minInset = this->minContainerOffset();
		}
    
		//check to see if offset lies within the inset bounds
		newX     = MIN(m_pContainer->getPosition().x, maxInset.x);
		newX     = MAX(newX, minInset.x);
		newY     = MIN(m_pContainer->getPosition().y, maxInset.y);
		newY     = MAX(newY, minInset.y);
    
		newX = m_pContainer->getPosition().x;
		newY = m_pContainer->getPosition().y;
    
		m_tScrollDistance     = ccpSub(m_tScrollDistance, ccp(newX - m_pContainer->getPosition().x, newY - m_pContainer->getPosition().y));
		m_tScrollDistance     = ccpMult(m_tScrollDistance, SCROLL_DEACCEL_RATE);
		this->setContentOffset(ccp(newX,newY));
    
		if ((fabsf(m_tScrollDistance.x) <= SCROLL_DEACCEL_DIST &&
			 fabsf(m_tScrollDistance.y) <= SCROLL_DEACCEL_DIST) ||
			newY > maxInset.y || newY < minInset.y ||
			newX > maxInset.x || newX < minInset.x ||
			newX == maxInset.x || newX == minInset.x ||
			newY == maxInset.y || newY == minInset.y)
		{
			this->unschedule(schedule_selector(ListView::deaccelerateScrolling));
			this->relocateContainer(true);
		}
	}

	void ListView::stoppedAnimatedScroll(CCNode * node)
	{
		this->unschedule(schedule_selector(ListView::performedAnimatedScroll));
		// After the animation stopped, "scrollViewDidScroll" should be invoked, this could fix the bug of lack of tableview cells.
		if (m_pDelegate != NULL)
		{
			m_pDelegate->scrollViewDidScroll(this);
		}
	}

	void ListView::performedAnimatedScroll(float dt)
	{
		if (m_bDragging)
		{
			this->unschedule(schedule_selector(ListView::performedAnimatedScroll));
			return;
		}

		if (m_pDelegate != NULL)
		{
			m_pDelegate->scrollViewDidScroll(this);
		}
	}


	const CCSize& ListView::getContentSize() const
	{
		return m_pContainer->getContentSize();
	}

	void ListView::setContentSize(const CCSize & size)
	{
		if (this->getContainer() != NULL)
		{
			this->getContainer()->setContentSize(size);
			this->updateInset();
		}
	}

	void ListView::updateInset()
	{
		if (this->getContainer() != NULL)
		{
			m_fMaxInset = this->maxContainerOffset();
			m_fMaxInset = ccp(m_fMaxInset.x + m_tViewSize.width * INSET_RATIO,
				m_fMaxInset.y + m_tViewSize.height * INSET_RATIO);
			m_fMinInset = this->minContainerOffset();
			m_fMinInset = ccp(m_fMinInset.x - m_tViewSize.width * INSET_RATIO,
				m_fMinInset.y - m_tViewSize.height * INSET_RATIO);
		}
	}

	/**
	 * make sure all children go to the container
	 */
	void ListView::addChild(CCNode * child, int zOrder, int tag)
	{
		child->ignoreAnchorPointForPosition(false);
		child->setAnchorPoint(ccp(0.0f, 0.0f));
		if (m_pContainer != child) {
			m_pContainer->addChild(child, zOrder, tag);
		} else {
			CCLayer::addChild(child, zOrder, tag);
		}
	}

	void ListView::addChild(CCNode * child, int zOrder)
	{
		this->addChild(child, zOrder, child->getTag());
	}

	void ListView::addChild(CCNode * child)
	{
		this->addChild(child, child->getZOrder(), child->getTag());
	}

	/**
	 * clip this view so that outside of the visible bounds can be hidden.
	 */
	void ListView::beforeDraw()
	{
		if (m_bClippingToBounds)
		{
			m_bScissorRestored = false;
			CCRect frame = getViewRect();
			if (CCEGLView::sharedOpenGLView()->isScissorEnabled()) {
				m_bScissorRestored = true;
				m_tParentScissorRect = CCEGLView::sharedOpenGLView()->getScissorRect();
				//set the intersection of m_tParentScissorRect and frame as the new scissor rect
				if (frame.intersectsRect(m_tParentScissorRect)) {
					float x = MAX(frame.origin.x, m_tParentScissorRect.origin.x);
					float y = MAX(frame.origin.y, m_tParentScissorRect.origin.y);
					float xx = MIN(frame.origin.x+frame.size.width, m_tParentScissorRect.origin.x+m_tParentScissorRect.size.width);
					float yy = MIN(frame.origin.y+frame.size.height, m_tParentScissorRect.origin.y+m_tParentScissorRect.size.height);
					CCEGLView::sharedOpenGLView()->setScissorInPoints(x, y, xx-x, yy-y);
				}
			}
			else {
				glEnable(GL_SCISSOR_TEST);
				CCEGLView::sharedOpenGLView()->setScissorInPoints(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
			}
		}
	}

	/**
	 * retract what's done in beforeDraw so that there's no side effect to
	 * other nodes.
	 */
	void ListView::afterDraw()
	{
		if (m_bClippingToBounds)
		{
			if (m_bScissorRestored) {//restore the parent's scissor rect
				CCEGLView::sharedOpenGLView()->setScissorInPoints(m_tParentScissorRect.origin.x, m_tParentScissorRect.origin.y, m_tParentScissorRect.size.width, m_tParentScissorRect.size.height);
			}
			else {
				glDisable(GL_SCISSOR_TEST);
			}
		}
	}

	void ListView::visit()
	{
		// quick return if not visible
		if (!isVisible())
		{
			return;
		}

		kmGLPushMatrix();
	
		if (m_pGrid && m_pGrid->isActive())
		{
			m_pGrid->beforeDraw();
			this->transformAncestors();
		}

		this->transform();
		this->beforeDraw();

		if(m_pChildren)
		{
			ccArray *arrayData = m_pChildren->data;
			unsigned int i=0;
		
			// draw children zOrder < 0
			for( ; i < arrayData->num; i++ )
			{
				CCNode *child =  (CCNode*)arrayData->arr[i];
				if ( child->getZOrder() < 0 )
				{
					child->visit();
				}
				else
				{
					break;
				}
			}
		
			// this draw
			this->draw();
		
			// draw children zOrder >= 0
			for( ; i < arrayData->num; i++ )
			{
				CCNode* child = (CCNode*)arrayData->arr[i];
				child->visit();
			}
        
		}
		else
		{
			this->draw();
		}

		this->afterDraw();
		if ( m_pGrid && m_pGrid->isActive())
		{
			m_pGrid->afterDraw(this);
		}

		kmGLPopMatrix();
	}

	bool ListView::ccTouchBegan(CCTouch* touch, CCEvent* event)
	{
		if (!this->isVisible())
		{
			return false;
		}
    
		CCRect frame = getViewRect();

		//dispatcher does not know about clipping. reject touches outside visible bounds.
		if (m_pTouches->count() > 2 ||
			m_bTouchMoved          ||
			!frame.containsPoint(m_pContainer->convertToWorldSpace(m_pContainer->convertTouchToNodeSpace(touch))))
		{
			return false;
		}

		if (!m_pTouches->containsObject(touch))
		{
			m_pTouches->addObject(touch);
		}

		if (m_pTouches->count() == 1)
		{ // scrolling
			m_tTouchPoint     = this->convertTouchToNodeSpace(touch);
			m_bTouchMoved     = false;
			m_bDragging     = true; //dragging started
			m_tScrollDistance = ccp(0.0f, 0.0f);
			m_fTouchLength    = 0.0f;
		}
		else if (m_pTouches->count() == 2)
		{
			m_tTouchPoint  = ccpMidpoint(this->convertTouchToNodeSpace((CCTouch*)m_pTouches->objectAtIndex(0)),
									   this->convertTouchToNodeSpace((CCTouch*)m_pTouches->objectAtIndex(1)));
			m_fTouchLength = ccpDistance(m_pContainer->convertTouchToNodeSpace((CCTouch*)m_pTouches->objectAtIndex(0)),
									   m_pContainer->convertTouchToNodeSpace((CCTouch*)m_pTouches->objectAtIndex(1)));
			m_bDragging  = false;
		} 

		// Reset m_eAutoDirection
		m_eAutoDirection = kListViewDirectionBoth;

		return true;
	}

	void ListView::ccTouchMoved(CCTouch* touch, CCEvent* event)
	{
		if (!this->isVisible())
		{
			return;
		}

		if (m_pTouches->containsObject(touch))
		{
			if (m_pTouches->count() == 1 && m_bDragging)
			{ // scrolling
				CCPoint moveDistance, newPoint, maxInset, minInset;
				CCRect  frame;
				float newX, newY;
            
				frame = getViewRect();

				newPoint     = this->convertTouchToNodeSpace((CCTouch*)m_pTouches->objectAtIndex(0));
				moveDistance = ccpSub(newPoint, m_tTouchPoint);
            
				float dis = 0.0f;
				if (m_eDirection == kListViewDirectionVertical)
				{
					dis = moveDistance.y;
				}
				else if (m_eDirection == kListViewDirectionHorizontal)
				{
					dis = moveDistance.x;
				}
				else
				{
					dis = sqrtf(moveDistance.x*moveDistance.x + moveDistance.y*moveDistance.y);
				}

				if (!m_bTouchMoved && fabs(convertDistanceFromPointToInch(dis)) < MOVE_INCH )
				{
					//CCLOG("Invalid movement, distance = [%f, %f], disInch = %f", moveDistance.x, moveDistance.y);
					return;
				}
            
				if (!m_bTouchMoved)
				{
					moveDistance = CCPointZero;
				}
            
				m_tTouchPoint = newPoint;
				m_bTouchMoved = true;
            
				if (frame.containsPoint(this->convertToWorldSpace(newPoint)))
				{
					/*
					switch (m_eDirection)
					{
						case kListViewDirectionVertical:
							moveDistance = ccp(0.0f, moveDistance.y);
							break;
						case kListViewDirectionHorizontal:
							moveDistance = ccp(moveDistance.x, 0.0f);
							break;
						default:
							break;
					}
					*/

					if (
						fabs(moveDistance.x) > fabs(moveDistance.y) 
						&& fabs(moveDistance.x) > 0
						&& m_eAutoDirection == kListViewDirectionBoth
					) {
						CCLOG("kListViewDirectionHorizontal");
						m_eAutoDirection = kListViewDirectionHorizontal;
					} else if (
						fabs(moveDistance.y) > fabs(moveDistance.x) 
						&& fabs(moveDistance.y) > 0
						&& m_eAutoDirection == kListViewDirectionBoth
					) {
						CCLOG("kListViewDirectionVertical");
						m_eAutoDirection = kListViewDirectionVertical;
					}

					switch (m_eAutoDirection)
					{
						case kListViewDirectionVertical:
							moveDistance = ccp(0.0f, moveDistance.y);
							break;
						case kListViewDirectionHorizontal:
							moveDistance = ccp(moveDistance.x, 0.0f);
							break;
						default:
							break;
					}

					/*
					if (Math.abs(x) > Math.abs(y) && Math.abs(x) > 0) {
						cc.log("ˮƽ�ƶ�");
						moveDistance.x = 0.0;
					} else if (Math.abs(x) < Math.abs(y) && Math.abs(y) > 0) {
						cc.log("��⵽�Ǵ�ֱ����");
						moveDistance.y = 0.0;
					} else {
						cc.log("ԭ�㡣");
						moveDistance.x = 0.0;
						moveDistance.y = 0.0;
					}
					*/
                
					maxInset = m_fMaxInset;
					minInset = m_fMinInset;

					newX     = m_pContainer->getPosition().x + moveDistance.x;
					newY     = m_pContainer->getPosition().y + moveDistance.y;

					m_tScrollDistance = moveDistance;
					this->setContentOffset(ccp(newX, newY));
				}
			}
			else if (m_pTouches->count() == 2 && !m_bDragging)
			{
				const float len = ccpDistance(m_pContainer->convertTouchToNodeSpace((CCTouch*)m_pTouches->objectAtIndex(0)),
												m_pContainer->convertTouchToNodeSpace((CCTouch*)m_pTouches->objectAtIndex(1)));
				this->setZoomScale(this->getZoomScale()*len/m_fTouchLength);
			}
		}
	}

	void ListView::ccTouchEnded(CCTouch* touch, CCEvent* event)
	{
		if (!this->isVisible())
		{
			return;
		}
		if (m_pTouches->containsObject(touch))
		{
			if (m_pTouches->count() == 1 && m_bTouchMoved)
			{
				this->schedule(schedule_selector(ListView::deaccelerateScrolling));
			}
			m_pTouches->removeObject(touch);
		} 

		if (m_pTouches->count() == 0)
		{
			m_bDragging = false;    
			m_bTouchMoved = false;
		}
	}

	void ListView::ccTouchCancelled(CCTouch* touch, CCEvent* event)
	{
		if (!this->isVisible())
		{
			return;
		}
		m_pTouches->removeObject(touch); 
		if (m_pTouches->count() == 0)
		{
			m_bDragging = false;    
			m_bTouchMoved = false;
		}
	}

	CCRect ListView::getViewRect()
	{
		CCPoint screenPos = this->convertToWorldSpace(CCPointZero);
    
		float scaleX = this->getScaleX();
		float scaleY = this->getScaleY();
    
		for (CCNode *p = m_pParent; p != NULL; p = p->getParent()) {
			scaleX *= p->getScaleX();
			scaleY *= p->getScaleY();
		}

		// Support negative scaling. Not doing so causes intersectsRect calls
		// (eg: to check if the touch was within the bounds) to return false.
		// Note, CCNode::getScale will assert if X and Y scales are different.
		if(scaleX<0.f) {
			screenPos.x += m_tViewSize.width*scaleX;
			scaleX = -scaleX;
		}
		if(scaleY<0.f) {
			screenPos.y += m_tViewSize.height*scaleY;
			scaleY = -scaleY;
		}

		return CCRectMake(screenPos.x, screenPos.y, m_tViewSize.width*scaleX, m_tViewSize.height*scaleY);
	}

	void ListView::registerScriptHandler(int nFunID,int nScriptEventType)
	{
		this->unregisterScriptHandler(nScriptEventType);
		m_mapScriptHandler[nScriptEventType] = nFunID;
	}
	void ListView::unregisterScriptHandler(int nScriptEventType)
	{
		std::map<int,int>::iterator iter = m_mapScriptHandler.find(nScriptEventType);
    
		if (m_mapScriptHandler.end() != iter)
		{
			m_mapScriptHandler.erase(iter);
		}
	}
	int  ListView::getScriptHandler(int nScriptEventType)
	{
		std::map<int,int>::iterator iter = m_mapScriptHandler.find(nScriptEventType);
    
		if (m_mapScriptHandler.end() != iter)
			return iter->second;
    
		return 0;
	}



	void ListView::addCell(CCNode* cell)
	{
		this->getContainer()->addChild(cell);
		this->m_pCells->addObject(cell);
	}

	void ListView::reorderAllCells()
	{
		unsigned int count = m_pCells->count();
		float x = 0.0f;
		float y = this->getContainer()->getContentSize().height;
		float maxWidth = 0.0f;
		float maxHeight = 0.0f;
		CCSize size;

		CCNode* m_pCell = dynamic_cast<CCNode*>(m_pCells->objectAtIndex(0));
		if (m_pCell) {
			y = y - m_pCell->getContentSize().height * (1 - m_pCell->getAnchorPoint().y);
		}

		CCObject* it;
		CCARRAY_FOREACH(m_pCells, it)
		{
			CCNode* cell = dynamic_cast<CCNode*>(it);
			cell->setPosition(ccp(x, y));
			if (this->getDirection() == kListViewDirectionHorizontal) {
				float width = cell->getContentSize().width;
				if (width > maxWidth) maxWidth = width;
				x = x + width;
			} else {
				float height = cell->getContentSize().height;
				if (height > maxHeight) maxHeight = height;
				y = y - height;
			}
		}

		if (count > 0) {
			/*
			if (this.currentIndex < 1) {
    			this.currentIndex = 1;
    		} else if (this.currentIndex > count) {
    			this.currentIndex = count;
    		}
			*/
		} else {
			// this.currentIndex = 0;
		}

		if (this->getDirection() == kListViewDirectionHorizontal) {
			size = CCSizeMake(x, maxHeight);
		} else {
			size = CCSizeMake(maxWidth, fabs(y));
		}
		
		this->getContainer()->setContentSize(size);
		// this->setContentOffset(ccp(0, 0));
		// this.setContentOffset(cc.p(0, -(_container.getContentSize().height - this.getViewSize().height)));
	}
};