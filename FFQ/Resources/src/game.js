/*
 * 游戏配置、辅助
 */


var game = game || {};



game.size = cc.size(640, 960);
game.winSize = cc.Director.getInstance().getWinSize();
game.offset = cc.p((game.winSize.width - game.size.width) / 2, (game.winSize.height - game.size.height) / 2);