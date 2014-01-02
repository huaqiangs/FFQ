/**
 * 分页列表
 */

game.ScrollView = cc.ScrollView.extend({
	
	dragThreshold:40,
    bouncThreshold:140,
    defaultAnimateTime:0.4,
    defaultAnimateEasing:"backOut",

    touchRect:null,
    offsetX:0,
    offsetY:0,

	cells:[],
	currentIndex:0,

	ctor:function() {
        this._super();
        cc.associateWithNative( this, cc.ScrollView );
    },

    init:function () {

    	this._super();

    	return true;
    },

    onEnterTransitionDidFinish:function() {
    	this._super();
    	// cc.log("ScrollView onEnterTransitionDidFinish.");
    	var _container = this.getContainer();
    	this.setContentOffset(cc.p(0, -(_container.getContentSize().height - this.getViewSize().height)));
    },

    onTouchBegan:function (touch, event) {
    	cc.log("onTouchBegan:function (touch, event);");
    	this._super();
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
    		if (i == 0) {
    		}
    		cell.setPosition(x, y);
    		if (this.getDirection() == cc.SCROLLVIEW_DIRECTION_HORIZONTAL) {
    			var width = cell.getContentSize().width;
    			if (width > maxWidth) maxWidth = width;
    			x = x + width;
    		} else {
    			var height = cell.getContentSize().height;
    			if (height > maxHeight) maxHeight = height;
    			y = y + height;
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

    	// if (this.getDirection() == cc.SCROLLVIEW_DIRECTION_HORIZONTAL) {
    	// 	this.setContentOffset(cc.p(0, 0));
    	// } else {
    	// 	this.setContentOffset(cc.p(0, this.minContainerOffset().y));
    	// }
    	this.updateInset();
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