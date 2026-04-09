//dif jingyan strat
#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "Common.h" 

// ==========================================
// 场景一：开场视频 (OpenCV)
// ==========================================
class VideoPlayer {
public:
    VideoPlayer(SDL_Renderer* r, const std::string& path)
        : renderer(r)
    {
        cap.open(path);
        if (cap.isOpened()) {
            int w = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
            int h = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
            // 创建流式纹理，用于频繁更新
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, w, h);
            videoAspect = (double)w / h;
        }
        else {
            SDL_Log("VideoPlayer: Failed to open %s", path.c_str());
        }
    }

    ~VideoPlayer() {
        if (texture) SDL_DestroyTexture(texture);
        cap.release();
    }

    // 重置播放进度到开头
    void reset() {
        if (cap.isOpened()) {
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);
            hasStarted = false;
            startTime = 0;
            isFinishedFlag = false;
        }
    }

    // 更新并渲染一帧
    // 返回值: true 表示正在播放, false 表示播放结束
    bool updateAndRender() {
        if (!cap.isOpened()) return false;
        if (isFinishedFlag) return false;

        // 初始化起始时间
        if (!hasStarted) {
            startTime = SDL_GetTicks();
            hasStarted = true;
        }

        // 简单的音画同步逻辑
        Uint32 now = SDL_GetTicks();
        double timeElapsed = (double)(now - startTime);
        double videoTime = cap.get(cv::CAP_PROP_POS_MSEC);

        // 如果视频进度落后于真实时间，则读取新帧
        if (videoTime <= timeElapsed) {
            if (cap.read(frame)) {
                // 更新纹理
                void* pixels; int pitch;
                SDL_LockTexture(texture, NULL, &pixels, &pitch);
                memcpy(pixels, frame.data, frame.total() * frame.elemSize());
                SDL_UnlockTexture(texture);
            }
            else {
                // 读取失败，说明视频结束
                isFinishedFlag = true;
                return false;
            }
        }

        renderLetterbox();
        return true;
    }

private:
    SDL_Renderer* renderer;
    cv::VideoCapture cap;
    cv::Mat frame;
    SDL_Texture* texture = nullptr;

    bool hasStarted = false;
    bool isFinishedFlag = false;
    Uint32 startTime = 0;
    double videoAspect = 1.0;

    // 保持比例渲染，上下或左右留黑边
    void renderLetterbox() {
        if (!texture) return;

        int winW, winH;
        SDL_GetRendererOutputSize(renderer, &winW, &winH);

        int targetW, targetH, targetX, targetY;
        double windowAspect = (double)winW / winH;

        if (windowAspect > videoAspect) {
            // 窗口比视频宽，左右留黑
            targetH = winH;
            targetW = (int)(winH * videoAspect);
            targetY = 0;
            targetX = (winW - targetW) / 2;
        }
        else {
            // 窗口比视频窄，上下留黑
            targetW = winW;
            targetH = (int)(winW / videoAspect);
            targetX = 0;
            targetY = (winH - targetH) / 2;
        }

        SDL_Rect dstRect = { targetX, targetY, targetW, targetH };
        SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    }
};

// ==========================================
// 场景二：登陆场景
// ==========================================
class LoginScene {
public:
    bool isLoginSuccess = false; // 标记是否登录成功

    LoginScene(SDL_Renderer* r, int w, int h) : renderer(r), winW(w), winH(h) {
        // 加载背景图 (视频21.3s处的截图)
        SDL_Surface* surf = IMG_Load("assets/texture/menu_bg_blur.jpg");
        if (surf) {
            bgTexture = SDL_CreateTextureFromSurface(renderer, surf);
            // 获取图片原始宽高，后续用于计算等比缩放
            bgOriginalW = surf->w;
            bgOriginalH = surf->h;
            SDL_FreeSurface(surf);
        }
        else {
            SDL_Log("LoginScene: Failed to load menu_bg_blur.jpg");
        }

        // 加载字体资源
        loginFont = TTF_OpenFont(u8"assets/font/汉仪文黑-85W Heavy.ttf", 32);
        tipFont = TTF_OpenFont(u8"assets/font/汉仪文黑-85W Heavy.ttf", 20);
		if (!loginFont || !tipFont) {
            SDL_Log("LoginScene: Failed to load fonts: %s", TTF_GetError());
        }

        // 开启SDL文本输入模式
        SDL_StartTextInput();
    }

    ~LoginScene() {
        // 释放纹理和字体资源
        if (bgTexture) SDL_DestroyTexture(bgTexture);
        if (loginFont) TTF_CloseFont(loginFont);
        if (tipFont) TTF_CloseFont(tipFont);
        // 关闭文本输入模式
        SDL_StopTextInput();
    }

    // 处理输入事件
    void handleEvent(SDL_Event* ev) {
        if (isLoginSuccess) return;

        // 1. 处理文本输入 (SDL_TEXTINPUT)
        if (ev->type == SDL_TEXTINPUT) {
            // 限制长度为16个字符
            if (inputText.length() < 16) {
                char c = ev->text.text[0];
                // 过滤非法字符 (仅允许 ASCII 可见字符)
                if (c >= 33 && c <= 126) inputText += c;
            }
        }
        // 2. 处理按键 (回车确认 / 退格删除)
        else if (ev->type == SDL_KEYDOWN) {
            if (ev->key.keysym.sym == SDLK_BACKSPACE && !inputText.empty()) {
                inputText.pop_back();
            }
            else if (ev->key.keysym.sym == SDLK_RETURN || ev->key.keysym.sym == SDLK_KP_ENTER) {
                if (!inputText.empty()) {
                    // 确认登录，保存用户名
                    currentUsername = inputText;
                    isLoginSuccess = true;
                    SDL_StopTextInput();
                }
            }
        }
    }

    void render() {
        // 1. 绘制背景 (保持比例缩放，Letterbox模式)
        SDL_Rect bgRect = getBgRect();
        if (bgTexture)
            SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect);
        else {
            // 如果背景加载失败，使用纯黑填充
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
        }

        // 2. 绘制登录框 (相对于计算出的背景图区域居中)
        drawLoginBox(bgRect);
    }

private:
    SDL_Renderer* renderer;
    SDL_Texture* bgTexture = nullptr;
    int winW, winH;
    int bgOriginalW = 1920, bgOriginalH = 1080; // 默认值，加载后会更新

    TTF_Font* loginFont = nullptr;
    TTF_Font* tipFont = nullptr;
    std::string inputText = "";

    // 核心函数：计算背景图在当前窗口中的 Letterbox 区域
    // 保证图片不变形，居中显示，上下或左右留黑边
    SDL_Rect getBgRect() {
        double windowAspect = (double)winW / winH;
        double bgAspect = (double)bgOriginalW / bgOriginalH;
        int targetW, targetH, targetX, targetY;

        if (windowAspect > bgAspect) {
            // 窗口更宽 -> 高度撑满，左右留黑
            targetH = winH;
            targetW = (int)(winH * bgAspect);
            targetY = 0;
            targetX = (winW - targetW) / 2;
        }
        else {
            // 窗口更高 -> 宽度撑满，上下留黑
            targetW = winW;
            targetH = (int)(winW / bgAspect);
            targetX = 0;
            targetY = (winH - targetH) / 2;
        }
        return { targetX, targetY, targetW, targetH };
    }

    // 绘制登录框 UI
    void drawLoginBox(SDL_Rect bgRect) {
        // 计算登录框中心点 (基于实际背景区域的中心)
        int cx = bgRect.x + bgRect.w / 2;
        int cy = bgRect.y + bgRect.h / 2;

        SDL_Rect boxRect = { cx - 200, cy - 80, 400, 160 };

        // 绘制半透明黑底背景
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 40, 44, 52, 220);
        SDL_RenderFillRect(renderer, &boxRect);

        // 绘制金色边框
        SDL_SetRenderDrawColor(renderer, 218, 165, 32, 255);
        SDL_RenderDrawRect(renderer, &boxRect);

        // 绘制 "User Name:" 提示文字
        SDL_Color gold = { 218, 165, 32, 255 };
        drawTextCentered(tipFont, u8"设置昵称:", { boxRect.x, boxRect.y + 20, boxRect.w, 30 }, gold);

        // 绘制输入框深色背景
        SDL_Rect inputArea = { boxRect.x + 50, boxRect.y + 70, 300, 50 };
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderFillRect(renderer, &inputArea);

        // 绘制用户输入的文字 (带光标闪烁效果)
        SDL_Color white = { 255, 255, 255, 255 };
        std::string displayStr = inputText;
        // 每500ms闪烁一次光标 "|"
        if (inputText.length() < 16 && (SDL_GetTicks64() / 500) % 2 == 0) displayStr += "|";
        drawTextCentered(loginFont, displayStr, inputArea, white);

        // 绘制底部 "Press Enter" 提示
        SDL_Color gray = { 150, 150, 150, 255 };
        drawTextCentered(tipFont, u8"[ 按 Enter 进入游戏 ]", { boxRect.x, boxRect.y + 125, boxRect.w, 30 }, gray);
    }

    // 辅助函数：绘制居中文字
    void drawTextCentered(TTF_Font* f, std::string t, SDL_Rect r, SDL_Color c) {
        if (!f || t.empty()) return;
        SDL_Surface* s = TTF_RenderUTF8_Blended(f, t.c_str(), c);
        if (s) {
            SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, s);
            SDL_Rect dst = { r.x + (r.w - s->w) / 2, r.y + (r.h - s->h) / 2, s->w, s->h };
            SDL_RenderCopy(renderer, tx, NULL, &dst);
            SDL_DestroyTexture(tx);
            SDL_FreeSurface(s);
        }
    }
};

// ==========================================
// 场景三：主菜单 
// ==========================================
class MainMenu
{
public:
    // --- 游戏配置状态变量 ---
    GameMode selectedMode = GameMode::CLASSIC;    // 当前选中的游戏模式（经典/迷雾）
    Difficulty selectedDiff = Difficulty::NORMAL; // 当前选中的游戏难度（简单/普通/困难）
    bool isStartClicked = false;                  // 标志位：是否点击了开始游戏按钮
    double currentDisplayRecord = 9999.0;         // 当前模式下需要显示的最高纪录

    // 构造函数：初始化资源和菜单结构
    MainMenu(SDL_Renderer* r, int w, int h) : renderer(r), winW(w), winH(h)
    {
        // 1. 加载背景图片
        // 这张图片需要与视频 21.3秒 处的画面保持视觉一致，以实现无缝过渡
        SDL_Surface* surf = IMG_Load("assets/texture/menu_bg_blur.jpg");
        if (surf)
        {
            bgTexture = SDL_CreateTextureFromSurface(renderer, surf);
            // 记录图片的原始尺寸，用于后续计算等比缩放（Letterbox模式）
            bgOriginalW = surf->w;
            bgOriginalH = surf->h;
            SDL_FreeSurface(surf);
        }
        else
        {
            SDL_Log("MainMenu: Failed to load menu_bg_blur.jpg");
        }

        // 2. 加载交互音效 (WAV格式)
        clickSound1 = Mix_LoadWAV("assets/audio/TabClick_1.wav"); // 音效1：下拉菜单展开/收起
        clickSound2 = Mix_LoadWAV("assets/audio/TabClick_2.wav"); // 音效2：选项被选中点击
        clickSound3 = Mix_LoadWAV("assets/audio/TabClick_3.wav"); // 音效3：开始按钮点击

        // 3. 加载各级字体资源
        // 标题大字体
        titleFont = TTF_OpenFont(u8"assets/font/汉仪文黑-85W Heavy.ttf", 64);
        // 菜单项中等字体
        itemFont = TTF_OpenFont(u8"assets/font/汉仪文黑-85W Heavy.ttf", 26);
        // 选项小字体
        optionFont = TTF_OpenFont(u8"assets/font/汉仪文黑-85W Heavy.ttf", 22);
        // 记录显示专用字体
        recordFont = TTF_OpenFont(u8"assets/font/汉仪文黑-85W Heavy.ttf", 20);
        // 用户名显示字体
        userFont = TTF_OpenFont(u8"assets/font/汉仪文黑-85W Heavy.ttf", 34);

		if (!titleFont || !itemFont || !optionFont || !recordFont || !userFont)
        {
            SDL_Log("MainMenu: Failed to load fonts: %s", TTF_GetError());
        }

        // 4. 初始化下拉菜单的数据结构（标题、选项内容）
        initMenus();
    }

    // 析构函数：释放所有占用的 SDL 资源
    ~MainMenu()
    {
        // 释放纹理资源
        if (bgTexture) SDL_DestroyTexture(bgTexture);

        // 释放音效资源
        if (clickSound1) Mix_FreeChunk(clickSound1);
        if (clickSound2) Mix_FreeChunk(clickSound2);
        if (clickSound3) Mix_FreeChunk(clickSound3);

        // 释放字体资源
        if (titleFont) TTF_CloseFont(titleFont);
        if (itemFont) TTF_CloseFont(itemFont);
        if (optionFont) TTF_CloseFont(optionFont);
        if (recordFont) TTF_CloseFont(recordFont);
        if (userFont) TTF_CloseFont(userFont);
    }

    // 处理鼠标交互事件
    void handleEvent(SDL_Event* ev)
    {
        // 更新鼠标位置记录，用于处理悬停（Hover）高亮效果
        if (ev->type == SDL_MOUSEMOTION)
        {
            mousePos = { ev->motion.x, ev->motion.y };
        }

        // 处理鼠标点击事件
        if (ev->type == SDL_MOUSEBUTTONDOWN)
        {
            SDL_Point p = { ev->button.x, ev->button.y };

            // 1. 处理“模式选择”下拉菜单的点击逻辑
            bool handled = handleDropdownClick(modeMenu, p, selectedMode);

            // 2. 如果模式菜单没有截获点击（即没点中它），继续判断“难度选择”菜单
            if (!handled)
            {
                handleDropdownClick(diffMenu, p, selectedDiff);
            }

            // 3. 处理“开始游戏”按钮的点击
            // 只有当两个下拉菜单都处于关闭状态时，才允许点击开始按钮，防止误触
            if (!modeMenu.isOpen && !diffMenu.isOpen)
            {
                if (isPointInRect(p, startBtnRect))
                {
                    isStartClicked = true;
                    // 播放开始按钮的专属音效 (TabClick_3)
                    if (clickSound3)
                    {
                        Mix_PlayChannel(-1, clickSound3, 0);
                    }
                }
            }
        }
    }

    // 核心渲染函数：绘制菜单的每一帧
    void render()
    {
        // 1. 绘制背景图
        // 使用 Letterbox 算法计算保持比例后的显示区域
        SDL_Rect bgRect = getBgRect();
        if (bgTexture)
        {
            SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect);
        }

        // 2. 动态更新 UI 元素的位置
        // 这一步非常关键：UI 坐标是基于 bgRect（背景图实际位置）计算的
        // 确保当窗口大小改变导致背景图缩放或产生黑边时，按钮依然贴合在背景图的正确位置
        updateUIPositions(bgRect);

        // 3. 绘制左侧的装饰线条 (随背景图位置移动)
        int lineX = bgRect.x + 100; // 假设装饰线在背景左侧偏移100px处
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 20); // 淡淡的白色
        SDL_RenderDrawLine(renderer, lineX, 0, lineX, winH);

        // 绘制左侧的装饰性图标
        drawSideIcon(bgRect.x + 50, bgRect.y + 120);

        // 4. 绘制游戏大标题
        drawText(titleFont, u8"原神连连看", bgRect.x + 150, bgRect.y + 80, { 255, 255, 255, 255 });
        // 绘制副标题/标语
        drawText(itemFont, u8"欢迎旅行者的到来！", bgRect.x + 155, bgRect.y + 155, { 218, 165, 32, 255 });

        // 5. 显示当前登录的用户名
        drawUserInfo(bgRect);

        // 6. 显示当前选择模式的历史最佳纪录
        drawBestRecord(bgRect);

        // 7. 绘制下拉菜单
        // 注意绘制顺序：先判断哪个菜单是激活（展开）状态
        // 展开的菜单必须最后绘制，确保它浮在最上层，不被其他 UI 遮挡
        Dropdown* active = nullptr;
        if (modeMenu.isOpen)
        {
            active = &modeMenu;
        }
        else if (diffMenu.isOpen)
        {
            active = &diffMenu;
        }

        // 绘制处于收起状态的菜单
        if (!modeMenu.isOpen)
        {
            drawDropdown(modeMenu, getModeName(selectedMode));
        }
        if (!diffMenu.isOpen)
        {
            drawDropdown(diffMenu, getDiffName(selectedDiff));
        }

        // 最后绘制处于展开状态的菜单
        if (active)
        {
            // 获取当前选中的值的文本表示
            std::string currentStr = (active == &modeMenu) ? getModeName(selectedMode) : getDiffName(selectedDiff);
            drawDropdown(*active, currentStr);
        }

        // 8. 绘制开始游戏按钮
        drawStartButton();
    }

private:
    // --- 内部数据结构定义 ---
    // 下拉菜单的选项结构
    struct Option
    {
        int value;          // 选项对应的枚举值
        std::string label;  // 选项显示的文本
    };
    // 下拉菜单组件结构
    struct Dropdown
    {
        SDL_Rect headRect;           // 标题栏区域
        std::string title;           // 菜单标题
        bool isOpen = false;         // 是否展开
        std::vector<Option> options; // 内部选项列表
    };

    // --- 渲染器与资源指针 ---
    SDL_Renderer* renderer;
    SDL_Texture* bgTexture = nullptr;
    int bgOriginalW = 1920, bgOriginalH = 1080; // 背景图原始尺寸
    int winW, winH;                             // 窗口当前尺寸
    SDL_Point mousePos = { 0, 0 };              // 鼠标当前位置

    // --- 音效资源 ---
    Mix_Chunk* clickSound1 = nullptr;
    Mix_Chunk* clickSound2 = nullptr;
    Mix_Chunk* clickSound3 = nullptr;

    // --- 字体资源 ---
    TTF_Font* titleFont;
    TTF_Font* itemFont;
    TTF_Font* optionFont;
    TTF_Font* recordFont;
    TTF_Font* userFont;

    // --- UI 组件实例 ---
    Dropdown modeMenu;
    Dropdown diffMenu;
    SDL_Rect startBtnRect;

    // 辅助函数：根据窗口比例和图片比例，计算背景图的 Letterbox 区域
    // 保证图片始终保持原始比例显示，不足部分留黑
    SDL_Rect getBgRect()
    {
        double windowAspect = (double)winW / winH;
        double bgAspect = (double)bgOriginalW / bgOriginalH;
        int targetW, targetH, targetX, targetY;

        // 如果窗口比图片更“宽”，则以窗口高度为基准，左右留黑
        if (windowAspect > bgAspect)
        {
            targetH = winH;
            targetW = (int)(winH * bgAspect);
            targetY = 0;
            targetX = (winW - targetW) / 2;
        }
        // 如果窗口比图片更“高”，则以窗口宽度为基准，上下留黑
        else
        {
            targetW = winW;
            targetH = (int)(winW / bgAspect);
            targetX = 0;
            targetY = (winH - targetH) / 2;
        }
        return { targetX, targetY, targetW, targetH };
    }

    // 关键逻辑：根据计算出的背景区域 (bgRect)，动态更新按钮和菜单的坐标
    void updateUIPositions(SDL_Rect bgRect)
    {
        // 设定 UI 元素相对于背景图左上角的偏移量
        int leftX = bgRect.x + 150;  // 距离背景左边界 150px
        int startY = bgRect.y + 280; // 距离背景上边界 280px
        int w = 300;                 // 菜单宽度
        int h = 50;                  // 菜单高度
        int gap = 30;                // 菜单间距

        // 更新下拉菜单的头部区域
        modeMenu.headRect = { leftX, startY, w, h };
        diffMenu.headRect = { leftX, startY + h + gap, w, h };

        // 更新开始按钮的位置 (定位在背景图的右下角区域)
        startBtnRect = { bgRect.x + bgRect.w - 350, bgRect.y + bgRect.h - 150, 200, 60 };
    }

    // 辅助工具：将枚举转换为可读字符串
    std::string getModeName(GameMode m)
    {
        switch (m)
        {
        case GameMode::CLASSIC:
            return u8"经典模式";
        case GameMode::FOG:
            return u8"迷雾模式";
        default:
            return u8"Unknown";
        }
    }

    std::string getDiffName(Difficulty d)
    {
        switch (d)
        {
        case Difficulty::EASY:
            return u8"简单";
        case Difficulty::NORMAL:
            return u8"标准";
        case Difficulty::HARD:
            return u8"困难";
        default:
            return "Unknown";
        }
    }

    // 初始化菜单的数据内容
    void initMenus()
    {
        // 设置模式菜单的标题和选项
        modeMenu.title = u8"模式选择";
        modeMenu.options = { { (int)GameMode::CLASSIC, u8"经典模式" }, { (int)GameMode::FOG, u8"迷雾模式" } };

        // 设置难度菜单的标题和选项
        diffMenu.title = u8"难度选择";
        diffMenu.options = { { (int)Difficulty::EASY, u8"简单" }, { (int)Difficulty::NORMAL, u8"标准" }, { (int)Difficulty::HARD, u8"困难" } };
    }

    // 绘制当前用户信息
    void drawUserInfo(SDL_Rect bgRect)
    {
        if (!userFont)
        {
            return;
        }
        std::string helloStr = u8"你好 , " + currentUsername + " !";
        SDL_Color c = { 200, 200, 200, 255 };
        // 名字位置也随背景区域动态移动
        drawText(userFont, helloStr, bgRect.x + 140, bgRect.y + 530, c);
    }

    // 绘制最佳纪录信息
    void drawBestRecord(SDL_Rect bgRect)
    {
        std::string label = u8"最佳纪录";
        std::string timeStr;

        // 如果纪录是初始值 9999.0，则显示占位符
        if (currentDisplayRecord >= 9999.0)
        {
            timeStr = "--.--";
        }
        else
        {
            // 否则格式化为两位小数
            char buf[32];
            snprintf(buf, 32, "%.2f s", currentDisplayRecord);
            timeStr = buf;
        }

        // 计算显示位置：位于开始按钮的上方
        int centerX = startBtnRect.x + startBtnRect.w / 2;
        int bottomY = startBtnRect.y - 20;

        // 绘制标签 (灰色)
        drawTextCentered(recordFont, label, { centerX, bottomY - 40, 0, 0 }, { 150, 150, 160, 255 });

        // 根据是否有纪录，选择不同的颜色绘制时间 (有纪录为金色，无纪录为灰色)
        SDL_Color timeColor = (currentDisplayRecord < 9999.0) ? SDL_Color{ 218, 165, 32, 255 } : SDL_Color{ 100, 100, 100, 255 };
        drawTextCentered(itemFont, timeStr, { centerX, bottomY - 10, 0, 0 }, timeColor);
    }

    // 通用函数：处理下拉菜单的点击逻辑，并播放音效
    template<typename T>
    bool handleDropdownClick(Dropdown& dd, SDL_Point p, T& targetVal)
    {
        // 1. 如果菜单已展开，检查是否点击了内部选项
        if (dd.isOpen)
        {
            int currentY = dd.headRect.y + dd.headRect.h;
            for (auto& opt : dd.options)
            {
                // 计算每个选项的矩形区域
                SDL_Rect optRect = { dd.headRect.x, currentY, dd.headRect.w, dd.headRect.h };
                if (isPointInRect(p, optRect))
                {
                    targetVal = (T)opt.value; // 更新目标变量的值
                    dd.isOpen = false;        // 选择后自动关闭菜单

                    // 播放选项点击音效 (TabClick_2)
                    if (clickSound2)
                        Mix_PlayChannel(-1, clickSound2, 0);

                    return true;
                }
                currentY += dd.headRect.h; // 下移 Y 坐标，准备判断下一个选项
            }
        }

        // 2. 检查是否点击了菜单的标题栏 (用于展开或收起)
        if (isPointInRect(p, dd.headRect))
        {
            dd.isOpen = !dd.isOpen;

            // 实现互斥逻辑：当展开一个菜单时，强制关闭另一个
            if (dd.isOpen)
            {
                if (&dd == &modeMenu)
                    diffMenu.isOpen = false;
                else
                    modeMenu.isOpen = false;
            }

            // 播放菜单展开/收起音效 (TabClick_1)
            if (clickSound1)
                Mix_PlayChannel(-1, clickSound1, 0);

            return true;
        }

        // 3. 如果点击了菜单外部，自动关闭当前菜单
        if (dd.isOpen)
            dd.isOpen = false;

        return false;
    }

    // 绘制装饰用的几何图标
    // 在指定坐标绘制一个由线条和点组成的金色十字星图案
    void drawSideIcon(int cx, int cy)
    {
        int size = 15; // 图标半径大小

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // 绘制金色部分
        SDL_SetRenderDrawColor(renderer, 218, 165, 32, 200);
        // 上下左右四条对角线
        SDL_RenderDrawLine(renderer, cx, cy - size, cx - size, cy);
        SDL_RenderDrawLine(renderer, cx, cy - size, cx + size, cy);
        SDL_RenderDrawLine(renderer, cx, cy + size, cx - size, cy);
        SDL_RenderDrawLine(renderer, cx, cy + size, cx + size, cy);
        // 中心点
        SDL_RenderDrawPoint(renderer, cx, cy);

        // 绘制白色垂直贯穿线
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
        SDL_RenderDrawLine(renderer, cx, cy - size - 20, cx, cy + size + 20);
    }

    // 绘制下拉菜单组件
    void drawDropdown(const Dropdown& dd, std::string currentValStr)
    {
        // 绘制菜单顶部的标题文字 (灰色)
        drawText(itemFont, dd.title, dd.headRect.x, dd.headRect.y - 30, { 200, 200, 200, 255 });

        bool isHoverHead = isPointInRect(mousePos, dd.headRect);

        // 绘制标题栏背景 (半透明深色)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 50, 50, 55, 200);
        SDL_RenderFillRect(renderer, &dd.headRect);

        // 绘制边框：展开或悬停时为金色，否则为灰色
        if (dd.isOpen || isHoverHead)
        {
            SDL_SetRenderDrawColor(renderer, 218, 165, 32, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        }
        SDL_RenderDrawRect(renderer, &dd.headRect);

        // 绘制当前选中的值
        drawText(optionFont, currentValStr, dd.headRect.x + 15, dd.headRect.y + 12, { 255, 255, 255, 255 });

        // 绘制右侧的小箭头指示器
        int arrowX = dd.headRect.x + dd.headRect.w - 20;
        int arrowY = dd.headRect.y + 20;
        if (dd.isOpen)
        {
            // 展开状态：向上箭头 (^)
            SDL_RenderDrawLine(renderer, arrowX, arrowY, arrowX + 5, arrowY - 5);
            SDL_RenderDrawLine(renderer, arrowX + 5, arrowY - 5, arrowX + 10, arrowY);
        }
        else
        {
            // 收起状态：向下箭头 (v)
            SDL_RenderDrawLine(renderer, arrowX, arrowY - 5, arrowX + 5, arrowY);
            SDL_RenderDrawLine(renderer, arrowX + 5, arrowY, arrowX + 10, arrowY - 5);
        }

        // 如果菜单处于展开状态，绘制下拉列表
        if (dd.isOpen)
        {
            int itemY = dd.headRect.y + dd.headRect.h;
            // 计算列表总高度
            SDL_Rect listBg = { dd.headRect.x, itemY, dd.headRect.w, (int)dd.options.size() * dd.headRect.h };

            // 绘制列表背景
            SDL_SetRenderDrawColor(renderer, 40, 40, 45, 250);
            SDL_RenderFillRect(renderer, &listBg);
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderDrawRect(renderer, &listBg);

            // 遍历并绘制每个选项
            for (const auto& opt : dd.options)
            {
                SDL_Rect itemRect = { dd.headRect.x, itemY, dd.headRect.w, dd.headRect.h };
                bool isItemHover = isPointInRect(mousePos, itemRect);

                // 如果鼠标悬停在选项上，绘制高亮背景和左侧指示条
                if (isItemHover)
                {
                    // 高亮背景
                    SDL_SetRenderDrawColor(renderer, 218, 165, 32, 50);
                    SDL_RenderFillRect(renderer, &itemRect);
                    // 左侧金色指示条
                    SDL_Rect indicator = { itemRect.x, itemRect.y + 10, 4, itemRect.h - 20 };
                    SDL_SetRenderDrawColor(renderer, 218, 165, 32, 255);
                    SDL_RenderFillRect(renderer, &indicator);
                }

                // 绘制选项文字 (悬停时更亮)
                SDL_Color c = isItemHover ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 180, 180, 180, 255 };
                drawText(optionFont, opt.label, itemRect.x + 20, itemRect.y + 12, c);

                // 绘制选项底部的分割线
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 20);
                SDL_RenderDrawLine(renderer, itemRect.x + 10, itemRect.y + itemRect.h, itemRect.x + itemRect.w - 10, itemRect.y + itemRect.h);

                itemY += dd.headRect.h;
            }
        }
    }

    // 绘制开始游戏按钮
    void drawStartButton()
    {
        bool isHover = isPointInRect(mousePos, startBtnRect);

        // 绘制按钮背景 (悬停时完全不透明，否则稍微透明)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, isHover ? 255 : 220);
        SDL_RenderFillRect(renderer, &startBtnRect);

        // 绘制按钮外边框 (深金色)
        SDL_SetRenderDrawColor(renderer, 200, 180, 120, 255);
        SDL_Rect border = startBtnRect;
        // 稍微扩大边框以制造双层效果
        border.x -= 2;
        border.y -= 2;
        border.w += 4;
        border.h += 4;
        SDL_RenderDrawRect(renderer, &border);

        // 绘制按钮文字
        SDL_Color textColor = { 60, 50, 40, 255 };
        drawTextCentered(itemFont, u8"开始", startBtnRect, textColor);
    }

    // 辅助函数：检测点是否在矩形内
    bool isPointInRect(SDL_Point p, SDL_Rect r)
    {
        return (p.x >= r.x && p.x <= r.x + r.w && p.y >= r.y && p.y <= r.y + r.h);
    }

    // 辅助函数：绘制指定坐标的文本
    void drawText(TTF_Font* f, std::string t, int x, int y, SDL_Color c)
    {
        if (!f || t.empty())
            return;

        SDL_Surface* s = TTF_RenderUTF8_Blended(f, t.c_str(), c);
        if (s)
        {
            SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, s);
            SDL_Rect dst = { x, y, s->w, s->h };
            SDL_RenderCopy(renderer, tx, NULL, &dst);
            SDL_DestroyTexture(tx);
            SDL_FreeSurface(s);
        }
    }

    // 辅助函数：在矩形区域内绘制居中文本
    void drawTextCentered(TTF_Font* f, std::string t, SDL_Rect r, SDL_Color c)
    {
        if (!f || t.empty())
            return;

        SDL_Surface* s = TTF_RenderUTF8_Blended(f, t.c_str(), c);
        if (s)
        {
            SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, s);
            // 计算居中坐标
            SDL_Rect dst = { r.x + (r.w - s->w) / 2, r.y + (r.h - s->h) / 2, s->w, s->h };
            SDL_RenderCopy(renderer, tx, NULL, &dst);
            SDL_DestroyTexture(tx);
            SDL_FreeSurface(s);
        }
    }
};

// =================================================================
// 场景三：游戏结束结算界面 (原神风格)
// =================================================================
class GameOverScene {
public:
    bool isReturnClicked = false;

    GameOverScene(SDL_Renderer* r, int w, int h) : renderer(r), winW(w), winH(h) {
        // 加载字体
        titleFont = TTF_OpenFont(u8"assets/font/汉仪文黑-85W Heavy.ttf", 60);
        infoFont = TTF_OpenFont(u8"assets/font/汉仪文黑-85W Heavy.ttf", 50);
        btnFont = TTF_OpenFont(u8"assets/font/汉仪文黑-85W Heavy.ttf", 28);

		if (!titleFont || !infoFont || !btnFont) {
            SDL_Log("GameOverScene: Failed to load fonts: %s", TTF_GetError());
        }
    }

    ~GameOverScene() {
        if (titleFont) TTF_CloseFont(titleFont);
        if (infoFont) TTF_CloseFont(infoFont);
        if (btnFont) TTF_CloseFont(btnFont);
    }

    void handleEvent(SDL_Event* ev) {
        if (ev->type == SDL_MOUSEMOTION) {
            mousePos = { ev->motion.x, ev->motion.y };
        }
        if (ev->type == SDL_MOUSEBUTTONDOWN) {
            if (isPointInRect({ ev->button.x, ev->button.y }, btnRect)) {
                isReturnClicked = true;
            }
        }
    }

    // 渲染结算界面
    // current: 本局用时, best: 历史最佳
    void render(double current, double best) {
        // 1. 绘制半透明黑色蒙版 (覆盖在游戏画面上)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200); // 很黑的遮罩
        SDL_Rect fullScreen = { 0, 0, winW, winH };
        SDL_RenderFillRect(renderer, &fullScreen);

        // 2. 绘制 "VICTORY" 标题 (金色)
        drawTextCentered(titleFont, u8"游戏结束", winW / 2, 200, { 218, 165, 32, 255 });

        // 3. 绘制装饰线
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
        SDL_RenderDrawLine(renderer, 200, 280, winW - 200, 280);

        // 4. 绘制时间数据
        std::string timeStr = u8"本次用时: " + formatTime(current);
        std::string bestStr = u8"最佳纪录: " + formatTime(best);

        // 如果打破纪录，显示 "New Record!"
        if (current <= best && best != 9999.0) {
            drawTextCentered(infoFont, u8"新纪录产生！", winW / 2, 360, { 255, 100, 100, 255 }); // 红色高亮
        }

        drawTextCentered(infoFont, timeStr, winW / 2, 440, { 255, 255, 255, 255 });
        drawTextCentered(infoFont, bestStr, winW / 2, 500, { 180, 180, 180, 255 });

        // 5. 绘制返回按钮
        drawButton();
    }

    // 重置状态（每次进入GameOver时调用）
    void reset() {
        isReturnClicked = false;
    }

private:
    SDL_Renderer* renderer;
    int winW, winH;
    SDL_Point mousePos = { 0, 0 };
    SDL_Rect btnRect = { 0, 0, 0, 0 }; // 在 drawButton 中计算

    TTF_Font* titleFont;
    TTF_Font* infoFont;
    TTF_Font* btnFont;

    std::string formatTime(double t) {
        char buf[64];
        snprintf(buf, 64, "%.2f s", t);
        return std::string(buf);
    }

    void drawButton() {
        int w = 240; int h = 60;
        btnRect = { (winW - w) / 2, 600, w, h };

        bool isHover = isPointInRect(mousePos, btnRect);

        // 按钮背景 (白)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, isHover ? 255 : 200);
        SDL_RenderFillRect(renderer, &btnRect);

        // 按钮文字 (黑)
        SDL_Color textColor = { 30, 30, 30, 255 };
        drawTextCentered(btnFont, u8"返回菜单", btnRect.x + w / 2, btnRect.y + h / 2, textColor);
    }

    bool isPointInRect(SDL_Point p, SDL_Rect r) {
        return (p.x >= r.x && p.x <= r.x + r.w && p.y >= r.y && p.y <= r.y + r.h);
    }

    void drawTextCentered(TTF_Font* f, std::string t, int cx, int cy, SDL_Color c) {
        if (!f) return;
        SDL_Surface* s = TTF_RenderUTF8_Blended(f, t.c_str(), c);
        if (s) {
            SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, s);
            SDL_Rect dst = { cx - s->w / 2, cy - s->h / 2, s->w, s->h };
            SDL_RenderCopy(renderer, tx, NULL, &dst);
            SDL_DestroyTexture(tx); SDL_FreeSurface(s);
        }
    }
};
//dif jingyan end