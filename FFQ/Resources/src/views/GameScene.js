var GameLayer = cc.Layer.extend({
    isMouseDown:false,
    helloImg:null,
    helloLabel:null,
    circle:null,
    sprite:null,

    ctor:function() {
        this._super();
        cc.associateWithNative( this, cc.Layer );
    },

    init:function () {

        //////////////////////////////
        // 1. super init first
        this._super();

        /////////////////////////////
        // 2. add a menu item with "X" image, which is clicked to quit the program
        //    you may modify it.
        // ask director the window size
        var size = cc.Director.getInstance().getWinSize();

        // add a "close" icon to exit the progress. it's an autorelease object
        var closeItem = cc.MenuItemImage.create(
            "res/CloseNormal.png",
            "res/CloseSelected.png",
            function () {
                cc.log("close button was clicked.");
                cc.Director.getInstance().end();
            },this);
        closeItem.setAnchorPoint(cc.p(0.5, 0.5));

        var menu = cc.Menu.create(closeItem);
        menu.setPosition(cc.p(0, 0));
        this.addChild(menu, 1);
        closeItem.setPosition(cc.p(size.width - 20, 20));


        var scrollLayer = cc.LayerColor.create(cc.c4b(255,128,128, 128));
        scrollLayer.setTag(124);
        // scrollLayer.setAnchorPoint(cc.p(0.0, 1.0));

        var scrollView = game.ScrollView.create(cc.size(640, 960), scrollLayer);
        // scrollView.setDirection(cc.SCROLLVIEW_DIRECTION_VERTICAL);
        scrollView.setDirection(cc.SCROLLVIEW_DIRECTION_BOTH);
        this.addChild(scrollView);

        // scrollView.setContentOffset(cc.p(0,0));
        // scrollView.setBounceable(false);

        scrollView.setDelegate(this);
        // scrollView.setTouchEnabled(false);

        var container = scrollView.getContainer();
        cc.log(container.getTag());
        var label = cc.LabelTTF.create("Hello ScrollView.", "Airal", 40);
        container.addChild(label);

        // for(var i = 0; i < 10; i++) {
        //     var res = "res/ui_bg_a036.png";
        //     // if (i == 1) res = "res/ui_bg_a037.png";
        //     var cell = cc.Sprite.create(res);
        //     var label = cc.LabelTTF.create("idx:"+ i, "Arial", 40);
        //     cell.addChild(label);
        //     // cell.setAnchorPoint(cc.p(0, 0));
        //     label.setPosition(cc.p(cell.getContentSize().width / 2, cell.getContentSize().height / 2));
        //     scrollView.addCell(cell);
        // }


        // cc.log(JSON.stringify(scrollView.getContentOffset()));
        // cc.log("getViewSize"+ JSON.stringify(scrollView.getViewSize()));
        // cc.log(JSON.stringify(scrollLayer.getContentSize()));

            

        return true;
    }

});

var GameScene = cc.Scene.extend({
    ctor:function() {
        this._super();
        cc.associateWithNative( this, cc.Scene );
    },

    onEnter:function () {
        this._super();
        var layer = new GameLayer();
        this.addChild(layer);
        layer.init();
    }
});
