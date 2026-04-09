#pragma once
#include "Common.h"
#include <vector>
#include <string>
#include <SDL_mixer.h>
#include <SDL_image.h>

// GameBoard 类：负责处理游戏局内的所有逻辑、渲染和输入
class GameBoard
{
private:
    // ==========================================
    // 核心数据成员
    // ==========================================
    
    
    //diffv2
    int drawLinesTimer; // 新增：连线显示倒计时（帧数）

    // 渲染器指针，由外部传入，不负责销毁
    SDL_Renderer* renderer;

    // 游戏地图数据
    // 使用 vector 实现动态数组，方便不同难度调整大小
    // map[r][c] 存储的是图案的 ID，0 表示空
    std::vector<std::vector<int>> map;

    // 迷雾遮罩数据
    // fogMask[r][c] 为 true 表示该位置被迷雾覆盖
    std::vector<std::vector<bool>> fogMask;

    // 地图的行数 (不含边界)
    int rows;

    // 地图的列数 (不含边界)
    int cols;

    // 当前剩余的图块对数 (用于判断胜利)
    int elementNum;

    // 居中偏移量 X (像素)
    int offsetX;

    // 居中偏移量 Y (像素)
    int offsetY;

    // ==========================================
    // 游戏状态成员
    // ==========================================

    // 当前选中的第一个点
    Point beginPos;

    // 当前选中的第二个点
    Point endPos;

    // 消除路径的点集，用于绘制连线
    // 最多是 4 个点 (起点 -> 拐点1 -> 拐点2 -> 终点)
    std::vector<Point> lines;

    // 是否已经选中了第一个点
    bool isRecord;

    // 当前游戏模式
    GameMode currentMode;

    // 当前难度
    Difficulty currentDifficulty;

    // 迷雾更新的计时器
    Uint64 lastFogUpdateTime;

    // ==========================================
    // 资源成员
    // ==========================================

    // 背景图片纹理
    SDL_Texture* texBg;

    // 角色图片纹理数组
    // 索引 1-30 对应具体的角色
    std::vector<SDL_Texture*> texAnimals;

    // 迷雾纹理 (可选)
    SDL_Texture* texFog;

    // 音效资源
    Mix_Chunk* soundClick;
    Mix_Chunk* soundEliminate;
    Mix_Chunk* soundWin;

public:
    // ==========================================
    // 构造与析构
    // ==========================================

    // 构造函数：初始化游戏局
    // 参数：渲染器、窗口宽、窗口高、难度、模式
    GameBoard(SDL_Renderer* render, int winW, int winH, Difficulty diff, GameMode mode);

    // 析构函数：释放纹理和音频资源
    ~GameBoard();

    // ==========================================
    // 主循环接口
    // ==========================================

    // 处理输入事件
    // 返回值：如果点击了有效区域返回 true
    bool handleEvent(SDL_Event* ev);

    // 更新每一帧的逻辑
    // 例如：迷雾的流动、时间的计算、道具的冷却
    void update();

    // 渲染画面
    // 包括背景、角色、连线、UI等
    void render();

    // 检查游戏是否获胜
    bool isVictory() const;

private:
    // ==========================================
    // 内部逻辑函数 (私有)
    // ==========================================

    // 加载所有图片和音频资源
    void loadResources();

    // 初始化地图数据
    void initMapData();

    // 打乱地图 (洗牌算法)
    void shuffleMap();

    // 尝试消除两个点
    // 如果可以消除，更新地图并返回 true
    bool tryEliminate(Point p1, Point p2);

    // 路径判断算法封装
    bool canConnect(Point p1, Point p2);

    // 直连判断 (0折)
    bool checkStraight(Point p1, Point p2);

    // 单折判断 (1折)
    bool checkOneTurn(Point p1, Point p2);

    // 双折判断 (2折)
    bool checkTwoTurns(Point p1, Point p2);

    // 判断两点之间水平是否通路
    bool isHorizontalClear(int r, int c1, int c2);

    // 判断两点之间垂直是否通路
    bool isVerticalClear(int c, int r1, int r2);

    // 绘制连接线
    void drawLinkLine();

    // 更新迷雾状态 (随机生成迷雾)
    void updateFog();

    // 辅助函数：加载单个纹理
    SDL_Texture* loadTexture(const std::string& path);

    // 辅助函数：坐标转换 (屏幕像素 -> 矩阵行列)
    Point screenToGrid(int x, int y);

    // 辅助函数：坐标转换 (矩阵行列 -> 屏幕像素)
    SDL_Rect gridToScreenRect(int r, int c);
};