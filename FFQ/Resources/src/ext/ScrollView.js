/**
 * 分页列表
 */
var MOVE_INCH = 7.0/160.0;

game.ScrollView = cc.Layer.extend({
	
	dragThreshold:40,
    bouncThreshold:140,
    defaultAnimateTime:0.4,
    defaultAnimateEasing:"backOut",

    touchRect:null,
    offsetX:0,
    offsetY:0,

	cells:[],
	currentIndex:0,

    // 阈值反弹?
    bouncThreshold:140,
    defaultAnimateTime:0.4,
    
    _delegate:null,
    
    _dragging:false,
    _contentOffset:null,
    _container:null,
    _touchMoved:false,

    _bounceable:false,
    _clippingToBounds:false,
    _scrollDistance:null,
    _touchPoint:null,
    _touches:null,
    _viewSize:null,
    _minScale:0,
    _maxScale:0,

    _touchMoved:false,
    _touchPoint:null,
    _touches:null,

    _direction:cc.SCROLLVIEW_DIRECTION_BOTH,
    // 自动检测滑动方向
    _auto_direction:cc.SCROLLVIEW_DIRECTION_BOTH,

	ctor:function() {
        this._super();
        cc.associateWithNative( this, cc.Layer );

        this._touches = [];
        this._touchPoint = cc.p(0, 0);
        this.offsetX = 0;
        this.offsetY = 0;

        cc.log("_touches:"+ this._touches.length);
    },

    init:function () {

    	this._super();

        cc.log("init.");

    	return true;
    },

    /**
     * initialized whether success or fail
     * @param {cc.size} size
     * @param {cc.Node} container
     * @return {Boolean}
     */
    initWithViewSize:function (size, container) {
        var pZero = cc.p(0,0);
        if (cc.Layer.prototype.init.call(this)) {
            this._container = container;

            if (!this._container) {
                this._container = cc.Layer.create();
                this._container.ignoreAnchorPointForPosition(false);
                this._container.setAnchorPoint(pZero);
            }

            this.setViewSize(size);

            this.setTouchEnabled(true);
            this._touches.length = 0;
            this._delegate = null;
            this._bounceable = true;
            this._clippingToBounds = true;

            //this._container.setContentSize(CCSizeZero);
            this._direction = cc.SCROLLVIEW_DIRECTION_BOTH;
            this._container.setPosition(pZero);

            this.addChild(this._container);
            this._minScale = this._maxScale = 1.0;
            return true;
        }
        return false;
    },

    /**
     * Sets a new content offset. It ignores max/min offset. It just sets what's given. (just like UIKit's UIScrollView)
     *
     * @param {cc.Point} offset new offset
     * @param {Number} [animated=] If true, the view will scroll to the new offset
     */
    setContentOffset: function (offset, animated) {
        var ox = this.offsetX, oy = this.offsetY;
        var x = ox, y = oy;

        if (this.getDirection() == cc.SCROLLVIEW_DIRECTION_HORIZONTAL) {
            this.offsetX = offset;
            x = offset;

            var maxX = this.bouncThreshold;
            var minX = -this.getContainer().getContentSize().width - this.bouncThreshold + this.getViewRect().width;
            if (x > maxX) {
                x = maxX;
            } else if (x < minX) {
                x = minX;
            }
        } else {
            this.offsetY = offset;
            y = offset;

            var maxY = this.getContainer().getContentSize().height + this.bouncThreshold + this.getViewRect().height;
            var minY = -this.bouncThreshold;
            if (y > maxY) {
                y = maxY;
            } else if (y < minY) {
                y = minY;
            }
        }

        if (animated) {
            var scroll = cc.MoveTo.create(this.defaultAnimateTime, cc.p(x, y));
            this.getContainer().stopAllActions();
            this.getContainer().runAction(scroll);
        } else {
            this.getContainer().setPosition(cc.p(x, y));
        }
    },

    getContentOffset:function () {
        return this._container.getPosition();
    },

    /**
     * <p>
     * size to clip. CCNode boundingBox uses contentSize directly.                   <br/>
     * It's semantically different what it actually means to common scroll views.    <br/>
     * Hence, this scroll view will use a separate size property.
     * </p>
     */
    getViewSize:function () {
        return this._viewSize;
    },

    setViewSize:function (size) {
        this._viewSize = size;
        cc.Node.prototype.setContentSize.call(this,size);
    },

    getContainer:function () {
        return this._container;
    },

    setContainer:function (container) {
        // Make sure that 'm_pContainer' has a non-NULL value since there are
        // lots of logic that use 'm_pContainer'.
        if (!container)
            return;

        this.removeAllChildren(true);

        this._container = container;
        container.ignoreAnchorPointForPosition(false);
        container.setAnchorPoint(cc.p(0.0, 0.0));

        this.addChild(container);
        this.setViewSize(this._viewSize);
    },

    /**
     * direction allowed to scroll. CCScrollViewDirectionBoth by default.
     */
    getDirection:function () {
        return this._direction;
    },
    setDirection:function (direction) {
        this._direction = direction;
    },

    getDelegate:function () {
        return this._delegate;
    },
    setDelegate:function (delegate) {
        this._delegate = delegate;
    },


    setContentSize: function (size) {
        if (this.getContainer() != null) {
            this.getContainer().setContentSize(size);
        }
    },

    getContentSize:function () {
        return this._container.getContentSize();
    },

    /**
     * Determines whether it clips its children or not.
     */
    isClippingToBounds:function () {
        return this._clippingToBounds;
    },

    setClippingToBounds:function (clippingToBounds) {
        this._clippingToBounds = clippingToBounds;
    },



    onEnter:function() {
        cc.registerTargetedDelegate(this.getTouchPriority(), false, this);
    },

    onEnterTransitionDidFinish:function() {
    	this._super();
    	// cc.log("ScrollView onEnterTransitionDidFinish.");
    	// var _container = this.getContainer();
    	// this.setContentOffset(cc.p(0, -(_container.getContentSize().height - this.getViewSize().height)));
    },

    getViewRect:function() {
        var s = this.getViewSize();
        var screenPos = this.convertToWorldSpace(cc.p(0, 0));
        return cc.rect(screenPos.x, screenPos.y, s.width, s.height);
    },

    onTouchBegan:function (touch, event) {
    	cc.log("onTouchBegan:function (touch, event);");
        this._touchPoint = this.convertTouchToNodeSpace(touch);
        this._touchMoved = false;
    	return true;
    },

    onTouchMoved:function (touch, event) {
        
        var newPoint = this.convertTouchToNodeSpace(touch);
        var moveDistance = cc.p(newPoint.x - this._touchPoint.x, newPoint.y - this._touchPoint.y);
        var frame = this.getViewRect();

        var dis = 0.0, locDirection = this.getDirection();
        if (locDirection === cc.SCROLLVIEW_DIRECTION_VERTICAL)
            dis = moveDistance.y;
        else if (locDirection === cc.SCROLLVIEW_DIRECTION_HORIZONTAL)
            dis = moveDistance.x;
        else
            dis = Math.sqrt(moveDistance.x * moveDistance.x + moveDistance.y * moveDistance.y);

        // if (!this._touchMoved && Math.abs(cc.convertDistanceFromPointToInch(dis)) < MOVE_INCH ) {
        //     cc.log("!MOVE_INCH");
        //     return;
        // }

        if (!this._touchMoved){
            moveDistance.x = 0;
            moveDistance.y = 0;
        }

        this._touchPoint = newPoint;
        this._touchMoved = true;

        if (cc.rectContainsPoint(frame, this.convertToWorldSpace(newPoint))) {
            switch (locDirection) {
                case cc.SCROLLVIEW_DIRECTION_VERTICAL:
                    moveDistance.x = 0.0;
                    break;
                case cc.SCROLLVIEW_DIRECTION_HORIZONTAL:
                    moveDistance.y = 0.0;
                    break;
                default:
                    break;
            }

            var locPosition = this.getContainer().getPosition();
            var newX = locPosition.x + moveDistance.x;
            var newY = locPosition.y + moveDistance.y;

            this._scrollDistance = moveDistance;
            this.setContentOffset(cc.p(newX, newY));
        }

        /*return ;

        if (!this.isVisible())
            return;

        this._touchMoved = true;
        //var frameOriginal = this.getParent().convertToWorldSpace(this.getPosition());
        //var frame = cc.rect(frameOriginal.x, frameOriginal.y, this._viewSize.width, this._viewSize.height);
        var frame = this.getViewRect();

        //var newPoint = this.convertTouchToNodeSpace(this._touches[0]);
        var newPoint = this.convertTouchToNodeSpace(touch);
        var moveDistance = cc.p(newPoint.x - this._touchPoint.x, newPoint.y - this._touchPoint.y);

        var x = newPoint.x - this._touchPoint.x;
        var y = newPoint.y - this._touchPoint.y;

        


        if (cc.rectContainsPoint(frame, this.convertToWorldSpace(newPoint))) {
            
            
            if (Math.abs(x) > Math.abs(y) && Math.abs(x) > 0) {
                cc.log("水平移动");
                moveDistance.x = 0.0;
            } else if (Math.abs(x) < Math.abs(y) && Math.abs(y) > 0) {
                cc.log("检测到是垂直滑动");
                moveDistance.y = 0.0;
            } else {
                cc.log("原点。");
                moveDistance.x = 0.0;
                moveDistance.y = 0.0;
            }


            var locPosition = this.getContainer().getPosition();
            var newX = locPosition.x + moveDistance.x;
            var newY = locPosition.y + moveDistance.y;

            this._scrollDistance = moveDistance;

            var touchPoint = touch.getLocation();
            this.setPosition(cc.p(newX, newY));
        }*/
    },

    addCell:function(cell) {
    	cell.objectId = this.cells.length;
    	this.getContainer().addChild(cell);
    	this.cells[this.cells.length] = cell;
    	this.reorderAllCells();
    	// this.dispatchEvent({name:"addCell", count:this.cells.length})
    },

    /* 重新排列列表 */
    reorderAllCells:function() {
    	var count = this.cells.length;
    	var x = 0, y = 0;
    	var maxWidth = 0, maxHeight = 0;

    	this.cells.sort(function(a, b) {
    		return (a.objectId < b.objectId);
    	});

    	for(var i = 0; i < count; i++) {
    		var cell = this.cells[i];
    		cell.setPosition(x, y);
    		if (this.getDirection() == cc.SCROLLVIEW_DIRECTION_HORIZONTAL) {
    			var width = cell.getContentSize().width;
    			if (width > maxWidth) maxWidth = width;
    			x = x + width;
    		} else {
    			var height = cell.getContentSize().height;
    			if (height > maxHeight) maxHeight = height;
    			y = y - height;
    		}
    	}

    	if (count > 0) {
    		if (this.currentIndex < 1) {
    			this.currentIndex = 1;
    		} else if (this.currentIndex > count) {
    			this.currentIndex = count;
    		}
    	} else {
    		this.currentIndex = 0;
    	}

    	var size = null;
    	if (this.getDirection() == cc.SCROLLVIEW_DIRECTION_HORIZONTAL) {
    		size = cc.size(x, maxHeight);
    	} else {
    		size = cc.size(maxWidth, Math.abs(y));
    	}

    	this.getContainer().setContentSize(size);

    }
});


game.ScrollView.create = function (size, container) {
    var pRet = new game.ScrollView();
    if (arguments.length == 2) {
        if (pRet && pRet.initWithViewSize(size, container))
            return pRet;
    } else {
        if (pRet && pRet.init())
            return pRet;
    }
    return null;
};