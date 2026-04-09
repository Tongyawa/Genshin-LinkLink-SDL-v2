#include "GameBoard.h"
#include <algorithm>
#include <ctime>
#include <iostream>

// ==========================================================
// 构造函数
// 负责初始化游戏的各项参数，并加载资源
// ==========================================================
GameBoard::GameBoard(SDL_Renderer* render, int winW, int winH, Difficulty diff, GameMode mode)
{
    // 保存渲染器指针
    this->renderer = render;
    this->currentDifficulty = diff;
    this->currentMode = mode;

    // 初始化随机数种子
    srand((unsigned int)time(NULL));

    // 根据难度设置矩阵大小
    // 注意：这里的 rows 和 cols 是实际有效区域
    // 后面我们会申请 +2 的空间作为辅助边界
    switch (diff)
    {
    case Difficulty::EASY:
        rows = 8;
        cols = 8;
        break;
    case Difficulty::NORMAL:
        rows = 10;
        cols = 10;
        break;
    case Difficulty::HARD:
        rows = 12;
        cols = 12;
        break;
    default:
        rows = 10;
        cols = 10;
        break;
    }

    // 计算居中偏移量
    // 算法：(窗口总宽 - (列数 * 格子宽)) / 2
    this->offsetX = (winW - (cols * ANIMAL_WIDTH)) / 2;
    this->offsetY = (winH - (rows * ANIMAL_HEIGHT)) / 2;

    // 初始化状态变量
    this->isRecord = false;
    this->beginPos = { -2, -2 };
    this->endPos = { -2, -2 };
    this->elementNum = rows * cols;
    this->lastFogUpdateTime = 0;
    //diffv2
    this->drawLinesTimer = 0; // 连线显示倒计时（帧数）初始化为0

    // 加载资源
    loadResources();

    // 初始化地图数据
    initMapData();
}

// ==========================================================
// 析构函数
// 负责清理申请的SDL资源，防止内存泄漏
// ==========================================================
GameBoard::~GameBoard()
{
    // 释放背景纹理
    if (texBg != nullptr)
    {
        SDL_DestroyTexture(texBg);
        texBg = nullptr;
    }

    // 释放迷雾纹理
    if (texFog != nullptr)
    {
        SDL_DestroyTexture(texFog);
        texFog = nullptr;
    }

    // 释放角色纹理数组
    // 使用迭代器遍历 vector
    for (auto tex : texAnimals)
    {
        if (tex != nullptr)
        {
            SDL_DestroyTexture(tex);
        }
    }
    texAnimals.clear();

    // 释放音效资源
    // 检查指针是否为空是一个好习惯
    if (soundClick) Mix_FreeChunk(soundClick);
    if (soundEliminate) Mix_FreeChunk(soundEliminate);
    if (soundWin) Mix_FreeChunk(soundWin);
}

// ==========================================================
// 资源加载函数
// 统一管理图片和音频的读取
// ==========================================================
void GameBoard::loadResources()
{
    // 1. 加载背景
    // 使用 assets/texture/bg.png
    texBg = loadTexture("assets/texture/bg.png");

    // 2. 加载迷雾图片（如果没有专用图，暂时用 null，渲染时用色块代替）
    // 使用 fog.png，没有的话也没关系
    texFog = loadTexture("assets/texture/fog.png");

    // 3. 加载角色图片
    // 预留 0 号位置为空，因为 map 中 0 代表空地
    texAnimals.push_back(nullptr);

    char pathBuffer[128];
    for (int i = 1; i <= ANIMAL_TYPES; i++)
    {
        // 格式化字符串路径 assets/texture/1.png ...
        SDL_snprintf(pathBuffer, sizeof(pathBuffer), "assets/texture/%d.png", i);

        // 加载并存入 vector
        SDL_Texture* tex = loadTexture(pathBuffer);
        texAnimals.push_back(tex);
    }

    // 4. 加载音效
    soundClick = Mix_LoadWAV("assets/audio/Click.wav"); // 备用

    soundEliminate = Mix_LoadWAV("assets/audio/Eliminate.wav");

    soundWin = Mix_LoadWAV("assets/audio/Win.wav");
}

SDL_Texture* GameBoard::loadTexture(const std::string& path)
{
    SDL_Surface* tempSurface = IMG_Load(path.c_str());
    if (tempSurface == nullptr)
    {
        // 如果加载失败，在控制台输出错误信息
        SDL_Log("无法加载图片: %s, 错误: %s", path.c_str(), SDL_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface);

    return texture;
}

// ==========================================================
// 地图初始化函数
// 生成成对的图标并打乱
// ==========================================================
void GameBoard::initMapData()
{
    // 1. 调整 vector 大小
    // 我们需要一个 (rows + 2) * (cols + 2) 的矩阵
    // 这里的 +2 是为了在四周形成一圈“空气墙”，
    // 方便边缘消除的算法逻辑（不用特判边界）
    map.resize(rows + 2);
    fogMask.resize(rows + 2);

    for (int i = 0; i < rows + 2; i++)
    {
        map[i].resize(cols + 2, 0);       // 默认填充 0
        fogMask[i].resize(cols + 2, false); // 默认没有迷雾
    }

    // 2. 生成成对的数据
    // 临时容器，用于存放所有需要的图块ID
    std::vector<int> tiles;
    int totalPairs = (rows * cols) / 2;

    for (int i = 0; i < totalPairs; i++)
    {
        // 随机选择一个角色 ID (1 到 ANIMAL_TYPES)
        int id = (rand() % ANIMAL_TYPES) + 1;

        // 每次加入两个，保证成对
        tiles.push_back(id);
        tiles.push_back(id);
    }

    // 3. 随机打乱临时容器
    // 使用 std::random_shuffle 或手动交换
    for (size_t i = 0; i < tiles.size(); i++)
    {
        int target = rand() % tiles.size();
        std::swap(tiles[i], tiles[target]);
    }

    // 4. 将打乱后的数据填入地图核心区域
    // 注意：跳过第 0 行/列，和最后一行/列
    int index = 0;
    for (int r = 1; r <= rows; r++)
    {
        for (int c = 1; c <= cols; c++)
        {
            map[r][c] = tiles[index];
            index++;
        }
    }

    // 5. 如果是迷雾模式，初始化迷雾
    if (currentMode == GameMode::FOG)
    {
        updateFog();
    }
}

// ==========================================================
// 输入事件处理
// ==========================================================
bool GameBoard::handleEvent(SDL_Event* ev)
{
    // 只处理鼠标左键按下
    if (ev->type == SDL_MOUSEBUTTONDOWN && ev->button.button == SDL_BUTTON_LEFT)
    {
        // 获取鼠标点击的屏幕坐标
        int mouseX = ev->button.x;
        int mouseY = ev->button.y;

        // 转换为网格坐标
        Point gridPos = screenToGrid(mouseX, mouseY);

        // 1. 越界检查
        if (gridPos.r < 1 || gridPos.r > rows || gridPos.c < 1 || gridPos.c > cols)
        {
            return false;
        }

        // 2. 空地检查 (如果点击的地方没有方块)
        if (map[gridPos.r][gridPos.c] == 0)
        {
            return false;
        }

        // 3. 播放点击音效
        Mix_PlayChannel(-1, soundClick, 0);

        // 4. 游戏逻辑状态机
        if (!isRecord)
        {
            // 状态：第一次点击
            beginPos = gridPos;
            isRecord = true;

            // 清空上一次的连线显示
            lines.clear();
        }
        else
        {
            // 状态：第二次点击
            endPos = gridPos;

            // 情况A：点击了同一个方块 -> 取消选中
            if (beginPos == endPos)
            {
                isRecord = false;
                beginPos = { -2, -2 };
                endPos = { -2, -2 };
            }
            // 情况B：点击了不同种类的方块 -> 更新起点为新的点
            else if (map[beginPos.r][beginPos.c] != map[endPos.r][endPos.c])
            {
                // 这里的设计是：如果玩家点了一个不一样的，
                // 那个新的不一样的点就变成新的“第一次点击”
                beginPos = endPos;
                endPos = { -2, -2 };
                // isRecord 保持为 true
            }
            // 情况C：点击了相同种类的方块 -> 尝试消除
            else
            {
                bool success = tryEliminate(beginPos, endPos);
                if (success)
                {
                    // 消除成功后，重置状态
                    isRecord = false;
                    beginPos = { -2, -2 };
                    endPos = { -2, -2 };
                }
                else
                {
                    // 消除失败（路不通），将第二个点设为新的起点
                    beginPos = endPos;
                    endPos = { -2, -2 };
                }
            }
        }
        return true;
    }
    return false;
}

// ==========================================================
// 核心逻辑：尝试消除
// ==========================================================
// diffv2替换原有的 tryEliminate
bool GameBoard::tryEliminate(Point p1, Point p2)
{
    if (map[p1.r][p1.c] != map[p2.r][p2.c]) return false;

    // 使用 canConnect 判断，若成功，lines 会被正确填充
    if (canConnect(p1, p2))
    {
        Mix_PlayChannel(-1, soundEliminate, 0);
        map[p1.r][p1.c] = 0;
        map[p2.r][p2.c] = 0;
        elementNum -= 2;

        // 设置连线显示时间（例如显示 30 帧，约 0.5 秒）
        drawLinesTimer = 30;

        return true;
    }

    // 重点：如果连不通，必须清空 lines，防止幽灵线
    lines.clear();
    return false;
}

// ==========================================================
// 寻路算法：判断两点是否可连
// ==========================================================
bool GameBoard::canConnect(Point p1, Point p2)
{
    lines.clear(); // 清空旧路径

    // 1. 检查直连 (无拐弯)
    if (checkStraight(p1, p2))
    {
        return true;
    }

    // 2. 检查一次拐弯
    if (checkOneTurn(p1, p2))
    {
        return true;
    }

    // 3. 检查两次拐弯
    if (checkTwoTurns(p1, p2))
    {
        return true;
    }

    return false;
}

// 直连逻辑
bool GameBoard::checkStraight(Point p1, Point p2)
{
    // 如果在同一行
    if (p1.r == p2.r)
    {
        if (isHorizontalClear(p1.r, p1.c, p2.c))
        {
            lines.push_back(p1);
            lines.push_back(p2);
            return true;
        }
    }
    // 如果在同一列
    else if (p1.c == p2.c)
    {
        if (isVerticalClear(p1.c, p1.r, p2.r))
        {
            lines.push_back(p1);
            lines.push_back(p2);
            return true;
        }
    }
    return false;
}

// 一折逻辑
bool GameBoard::checkOneTurn(Point p1, Point p2)
{
    // 一个拐点必定是 (p1.r, p2.c) 或者 (p2.r, p1.c)
    // 我们把这两个潜在拐点称为 C 和 D

    Point C = { p1.r, p2.c };

    // 检查拐点 C 是否为空 (或者就是终点本身，虽然这里不太可能)
    // 且 p1->C 通路，C->p2 通路
    if (map[C.r][C.c] == 0)
    {
        bool path1 = checkStraight(p1, C);
        // 注意：checkStraight 会修改 lines，需要处理
        // 这里为了简化，我们在 checkStraight 成功后清空 lines 重新填
        if (path1)
        {
            // 为了防止 lines 被污染，这里需要特殊的逻辑
            // 简单做法：复用 checkStraight 的逻辑但不依赖它的 lines 输出
            // 或者直接调用底层 Clear 函数
            if (isVerticalClear(C.c, C.r, p2.r)) // C->p2 是垂直的
            {
                lines.clear();
                lines.push_back(p1);
                lines.push_back(C);
                lines.push_back(p2);
                return true;
            }
        }
    }

    Point D = { p2.r, p1.c };
    if (map[D.r][D.c] == 0)
    {
        // p1 -> D (垂直), D -> p2 (水平)
        if (isVerticalClear(p1.c, p1.r, D.r) && isHorizontalClear(D.r, D.c, p2.c))
        {
            lines.clear();
            lines.push_back(p1);
            lines.push_back(D);
            lines.push_back(p2);
            return true;
        }
    }

    return false;
}

// 二折逻辑 (最复杂的部分)
bool GameBoard::checkTwoTurns(Point p1, Point p2)
{
    // 遍历 p1 所在行的所有点，寻找可以通过“一折”连到 p2 的点
    // 这里的 i 范围是 0 到 cols + 1 (包括辅助区)
    for (int i = 0; i < cols + 2; i++)
    {
        Point temp = { p1.r, i };

        // 如果 temp 点为空，且 p1 到 temp 直连
        if (map[temp.r][temp.c] == 0 && isHorizontalClear(p1.r, p1.c, temp.c))
        {
            // 检查 temp 到 p2 是否可以一折连通
            // 这里不能直接调 checkOneTurn，因为那会修改 lines
            // 我们手动模拟 logic

            // temp -> 拐点2 -> p2
            // 拐点2 必须是 {p2.r, temp.c}
            Point turn2 = { p2.r, temp.c };

            if (map[turn2.r][turn2.c] == 0)
            {
                if (isVerticalClear(temp.c, temp.r, turn2.r) &&
                    isHorizontalClear(turn2.r, turn2.c, p2.c))
                {
                    lines.clear();
                    lines.push_back(p1);
                    lines.push_back(temp);
                    lines.push_back(turn2);
                    lines.push_back(p2);
                    return true;
                }
            }
        }
    }

    // 遍历 p1 所在列的所有点
    for (int i = 0; i < rows + 2; i++)
    {
        Point temp = { i, p1.c };

        if (map[temp.r][temp.c] == 0 && isVerticalClear(p1.c, p1.r, temp.r))
        {
            // temp -> 拐点2 ({temp.r, p2.c}) -> p2
            Point turn2 = { temp.r, p2.c };

            if (map[turn2.r][turn2.c] == 0)
            {
                if (isHorizontalClear(temp.r, temp.c, turn2.c) &&
                    isVerticalClear(turn2.c, turn2.r, p2.r))
                {
                    lines.clear();
                    lines.push_back(p1);
                    lines.push_back(temp);
                    lines.push_back(turn2);
                    lines.push_back(p2);
                    return true;
                }
            }
        }
    }

    return false;
}

bool GameBoard::isHorizontalClear(int r, int c1, int c2)
{
    if (c1 > c2) std::swap(c1, c2);

    // 遍历中间的格子 (不包含起点和终点)
    for (int c = c1 + 1; c < c2; c++)
    {
        if (map[r][c] != 0) return false;
    }
    return true;
}

bool GameBoard::isVerticalClear(int c, int r1, int r2)
{
    if (r1 > r2) std::swap(r1, r2);

    for (int r = r1 + 1; r < r2; r++)
    {
        if (map[r][c] != 0) return false;
    }
    return true;
}

// ==========================================================
// 辅助函数：坐标转换
// ==========================================================
Point GameBoard::screenToGrid(int x, int y)
{
    Point p;
    // 减去偏移量
    x -= offsetX;
    y -= offsetY;

    // 除以格子宽高
    // 注意：这里计算出的是 0-based 的逻辑坐标
    // 但我们的 map 是 1-based (带padding)，所以需要 +1
    p.c = (x / ANIMAL_WIDTH) + 1;
    p.r = (y / ANIMAL_HEIGHT) + 1;

    return p;
}

SDL_Rect GameBoard::gridToScreenRect(int r, int c)
{
    SDL_Rect rect;
    // 这里的 r, c 是 map 中的下标 (1-based)
    // 转回屏幕坐标需要先 -1
    rect.x = (c - 1) * ANIMAL_WIDTH + offsetX;
    rect.y = (r - 1) * ANIMAL_HEIGHT + offsetY;
    rect.w = ANIMAL_WIDTH;
    rect.h = ANIMAL_HEIGHT;
    return rect;
}

// ==========================================================
// 逻辑更新 (Update)
// ==========================================================
void GameBoard::update()
{
    // 如果是迷雾模式，处理迷雾更新逻辑
    if (currentMode == GameMode::FOG)
    {
        Uint64 currentTime = SDL_GetTicks64();

        // 每 5000 毫秒 (5秒) 更新一次迷雾
        if (currentTime - lastFogUpdateTime > 5000)
        {
            updateFog();
            lastFogUpdateTime = currentTime;
        }
    }
}

void GameBoard::updateFog()
{
    // 简单迷雾逻辑：随机遮盖 20% 的格子
    for (int r = 1; r <= rows; r++)
    {
        for (int c = 1; c <= cols; c++)
        {
            // 20% 概率生成迷雾
            if (rand() % 5 == 0)
            {
                fogMask[r][c] = true;
            }
            else
            {
                fogMask[r][c] = false;
            }
        }
    }
}

// ==========================================================
// 渲染 (Render)
// ==========================================================
// 替换原有的 render (整合了所有优化)
void GameBoard::render()
{
    // 1. 背景绘制 (裁切版)
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h); // 获取当前窗口大小
    int bgW, bgH;
    SDL_QueryTexture(texBg, NULL, NULL, &bgW, &bgH);
    SDL_Rect src = { 0, 0, std::min(bgW, w), std::min(bgH, h) };
    SDL_Rect dst = { 0, 0, src.w, src.h };
    SDL_RenderCopy(renderer, texBg, &src, &dst);

    // 2. 绘制角色与迷雾
    for (int r = 1; r <= rows; r++)
    {
        for (int c = 1; c <= cols; c++)
        {
            int id = map[r][c];
            if (id > 0)
            {
                SDL_Rect cell = gridToScreenRect(r, c);

                // --- 优化1：绘制角色 ---
                if (id < (int)texAnimals.size() && texAnimals[id])
                    SDL_RenderCopy(renderer, texAnimals[id], NULL, &cell);

                // --- 优化2：边缘区分框 (白色半透明描边) ---
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
                SDL_RenderDrawRect(renderer, &cell);

                // --- 优化3：迷雾视觉效果 ---
                if (currentMode == GameMode::FOG && fogMask[r][c])
                {
                    if (texFog) // 如果有 fog.png
                    {
                        // 稍微设置一点透明度，让下面的图案隐约可见（难度降低）或者全不透明
                        SDL_SetTextureAlphaMod(texFog, 240);
                        SDL_RenderCopy(renderer, texFog, NULL, &cell);
                    }
                    else // 没有图就用灰色块
                    {
                        SDL_SetRenderDrawColor(renderer, 50, 50, 60, 230); // 深灰色
                        SDL_RenderFillRect(renderer, &cell);
                    }
                }
            }
        }
    }

    // 3. 绘制选中框 (红色)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    if (beginPos.r != -2) {
        SDL_Rect r = gridToScreenRect(beginPos.r, beginPos.c);
        SDL_RenderDrawRect(renderer, &r);
        // 加粗
        SDL_Rect r2 = { r.x + 1, r.y + 1, r.w - 2, r.h - 2 };
        SDL_RenderDrawRect(renderer, &r2);
    }
    if (endPos.r != -2) {
        SDL_Rect r = gridToScreenRect(endPos.r, endPos.c);
        SDL_RenderDrawRect(renderer, &r);
    }

    // 4. 绘制连线 (带倒计时)
    if (drawLinesTimer > 0 && !lines.empty())
    {
        drawLinkLine(); // 调用抽离出来的画线函数
        drawLinesTimer--; // 倒计时递减
    }
    else
    {
        // 时间到，清空线条数据
        lines.clear();
    }
}

bool GameBoard::isVictory() const
{
    return elementNum == 0;
}

// 补全：洗牌算法 (其实之前的 initMapData 里写了，这里单独抽离更规范)
void GameBoard::shuffleMap()
{
    // 提取当前地图上所有非0的元素
    std::vector<int> elements;
    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            if (map[r][c] != 0) elements.push_back(map[r][c]);
        }
    }

    // 打乱
    // std::random_shuffle 在C++14后弃用，建议用 std::shuffle
    // 但为了兼容大作业环境，手写交换或者 random_shuffle 都可以
    for (size_t i = 0; i < elements.size(); i++) {
        int target = rand() % elements.size();
        std::swap(elements[i], elements[target]);
    }

    // 填回地图
    int idx = 0;
    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            if (map[r][c] != 0) { // 保持原来的位置结构（如果有死局重开功能，这很有用）
                map[r][c] = elements[idx++];
            }
        }
    }
}

// 补全：绘制连线
void GameBoard::drawLinkLine()
{
    // 设置线条颜色：青蓝色，高亮
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);

    if (lines.empty()) return;

    for (size_t i = 0; i < lines.size() - 1; i++)
    {
        Point p1 = lines[i];
        Point p2 = lines[i + 1];

        // 计算格子中心点
        SDL_Rect r1 = gridToScreenRect(p1.r, p1.c);
        SDL_Rect r2 = gridToScreenRect(p2.r, p2.c);

        int x1 = r1.x + ANIMAL_WIDTH / 2;
        int y1 = r1.y + ANIMAL_HEIGHT / 2;
        int x2 = r2.x + ANIMAL_WIDTH / 2;
        int y2 = r2.y + ANIMAL_HEIGHT / 2;

        // 画线（画粗一点，多画几次偏移）
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        SDL_RenderDrawLine(renderer, x1 + 1, y1, x2 + 1, y2);
        SDL_RenderDrawLine(renderer, x1, y1 + 1, x2, y2 + 1);
    }
}