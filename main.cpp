#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
//dif jingyan start
#include <SDL_ttf.h>
#include <opencv2/opencv.hpp>
#include <Windows.h>
#include <vector>
//dif jingyan end
#include <iostream>
#include <fstream>      // 引入文件流，替代 FILE*
#include <string>

#include "Common.h"
#include "GameBoard.h"
//dif jingyan start
#include "ExtraScenes.h"
//dif jingyan end

// 屏幕尺寸配置
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 900;

// 全局音乐指针
Mix_Music* bgm = nullptr;// 局内背景音乐
//dif jingyan strat
Mix_Music* bgm_part1 = nullptr;// 开场音乐
Mix_Music* bgm_part2 = nullptr;// 过场音乐
Mix_Chunk* bgm_main = nullptr;// 局外背景音乐
int ChunkChannel = -1;

//全局用户名
std::string currentUsername = "Guest";
//dif jingyan end

// ==========================================================
// 读取记录文件 (C++ fstream 风格)
// ==========================================================
//dif jingyan start
//double loadBestRecord()
//{
//    std::string filePath = "assets/Data/record.bin";
//    std::ifstream inFile(filePath, std::ios::binary);
//
//    double bestTime = 9999.0;
//
//    if (inFile.is_open())
//    {
//        inFile.read(reinterpret_cast<char*>(&bestTime), sizeof(double));
//        inFile.close();
//    }
//    else
//    {
//        SDL_Log("未找到记录文件，将创建新记录。");
//    }
//
//    return bestTime;
//}

// ==========================================================
// 保存记录文件
// ==========================================================
//void saveBestRecord(double time)
//{
//    std::string filePath = "assets/Data/record.bin";
//    std::ofstream outFile(filePath, std::ios::binary);
//
//    if (outFile.is_open())
//    {
//        outFile.write(reinterpret_cast<const char*>(&time), sizeof(double));
//        outFile.close();
//    }
//}

// 1. 读取整个用户数据库到 vector
std::vector<UserData> loadAllUsers() {
    std::vector<UserData> allUsers;
    std::ifstream inFile("assets/Data/record.bin", std::ios::binary);
    if (inFile.is_open()) {
        // 获取文件大小
        inFile.seekg(0, std::ios::end);
        size_t fileSize = inFile.tellg();
        inFile.seekg(0, std::ios::beg);

        if (fileSize > 0) {
            size_t userCount = fileSize / sizeof(UserData);
            allUsers.resize(userCount);
            inFile.read(reinterpret_cast<char*>(allUsers.data()), fileSize);
        }
        inFile.close();
    }
   
    return allUsers;
}

// 2. 将整个用户数据库写回文件
void saveAllUsers(const std::vector<UserData>& allUsers) {
    std::ofstream outFile("assets/Data/record.bin", std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(reinterpret_cast<const char*>(allUsers.data()), allUsers.size() * sizeof(UserData));
        outFile.close();
    }
}

// 3. 查找当前用户的 Records
Records getCurrentUserRecords(std::vector<UserData>& allUsers) {
    for (auto& u : allUsers) {
        // 比较用户名 (std::string 与 char数组比较)
        if (currentUsername == u.username) {
            return u.records;
        }
    }
    // 未找到用户，返回默认空记录
    return Records();
}

// 4. 更新当前用户的 Records 并保存到文件
void saveCurrentUserRecords(Records newRecords) {
    // 重新读取所有数据，确保是覆盖最新状态
    std::vector<UserData> allUsers = loadAllUsers();
    bool found = false;

    // 查找并更新
    for (auto& u : allUsers) {
        if (currentUsername == u.username) {
            u.records = newRecords;
            found = true;
            break;
        }
    }

    // 如果是新用户，追加到末尾
    if (!found) {
        UserData newUser;
        // 安全拷贝字符串到定长数组
        strncpy_s(newUser.username, currentUsername.c_str(), 16);
        newUser.records = newRecords;
        allUsers.push_back(newUser);
        SDL_Log("New User Created: %s", currentUsername.c_str());
    }

    // 统一写回文件
    saveAllUsers(allUsers);
    SDL_Log("Records Saved for: %s", currentUsername.c_str());
}

//dif jingyan end

// ==========================================================
// 主函数
// ==========================================================
int main(int argc, char* argv[])
{
    SetConsoleOutputCP(65001);

    // 1. 初始化 SDL 系统
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        SDL_Log("SDL Init Failed: %s", SDL_GetError());
        return -1;
    }

    // 2. 初始化图片库 (PNG/JPG)
    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG)))
    {
        SDL_Log("SDL_image Init Failed: %s", IMG_GetError());
        return -1;
    }

    // 3. 初始化音频库
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        SDL_Log("SDL_mixer Init Failed: %s", Mix_GetError());
        return -1;
    }

	//dif jingyan start
    // 3.5 初始化字体库
    if (TTF_Init() == -1)
    {
        SDL_Log("SDL_ttf Init Failed: %s", TTF_GetError());
        return -1;
    }
	//dif jingyan end

    // 4. 创建窗口
    SDL_Window* window = SDL_CreateWindow(
        u8"原神连连看 v2.1 - By Wenyaowo & Jingyan", // 标题
        SDL_WINDOWPOS_CENTERED,            // X
        SDL_WINDOWPOS_CENTERED,            // Y
        WINDOW_WIDTH,                      // W
        WINDOW_HEIGHT,                     // H
        SDL_WINDOW_SHOWN                   // Flags
    );

    if (!window)
    {
        SDL_Log("Create Window Failed: %s", SDL_GetError());
        return -1;
    }

    // 5. 创建渲染器 (启用垂直同步 VSYNC)
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    //dif jingyan strat
    // 
    // 6. 加载开场场景视频
    VideoPlayer* videoP1 = new VideoPlayer(renderer, "assets/video/Launch_Part1.mp4");
    VideoPlayer* videoP2 = new VideoPlayer(renderer, "assets/video/Launch_Part2.mp4");

    // 7. 加载音频资源 (示例)
    // 注意：请确保文件路径正确，支持中文路径需转码，这里简化处理
    bgm = Mix_LoadMUS(u8"assets/audio/陈致逸,HOYO-MiX - Moonlike Smile 皎洁的笑颜.flac");
	bgm_part1 = Mix_LoadMUS("assets/audio/Launch_Part1.flac");
	bgm_part2 = Mix_LoadMUS("assets/audio/Launch_Part2.flac");
    bgm_main = Mix_LoadWAV("assets/audio/Main.flac");
    if (bgm_part1)
    {
        Mix_PlayMusic(bgm_part1, 0); // 播放一次
    }
    else
    {
        SDL_Log("Failed to load intro BGM: %s", Mix_GetError());
	}
    if (bgm_main) {
        Mix_VolumeChunk(bgm_main, 16);
		ChunkChannel=Mix_PlayChannel(1, bgm_main, -1); // 循环播放
    }
    //if (bgm)
    //{
    //    Mix_PlayMusic(bgm, -1); // -1 表示循环播放
    //}
    //暂时先不播放游戏bgm

	//8. 加载界面场景
    LoginScene* loginScene = new LoginScene(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    MainMenu* menuScene = new MainMenu(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    GameOverScene* overScene = new GameOverScene(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // ==========================================
    // 游戏主循环控制变量
    // ==========================================
    bool isAppRunning = true;

    //先不进菜单
    // 
    //// 这里模拟菜单选择，之后你的搭档会把菜单UI做在这里
    //// 假设玩家选择了：困难难度 + 迷雾模式
    //Difficulty selectedDiff = Difficulty::NORMAL; // 可以改成 HARD 测试
    //GameMode selectedMode = GameMode::FOG;     // 可以改成 CLASSIC 测试

    // 创建游戏局实例，但先不创建 (使用 new 在堆上创建)
    GameBoard* game = nullptr;

    // 纪录数据缓存
    Records currentUserRecords;
	std::vector<UserData> all = loadAllUsers();
	//dif jingyan end

    // 计时器
    Uint64 startTime = SDL_GetTicks64();
    bool isGameOver = false;

	//dif jingyan start
    double currentDuration = 0.0;
    
    //初始化状态机，见common.h
     GameState currentState = GameState::INTRO_PART1;
    // 调试Debug Mode接口
    //currentState = GameState::MAIN_MENU;

	//调试模式：直接跳过开场动画和登录
    if (currentState == GameState::MAIN_MENU) {
        // 1. 初始化菜单和结算场景
        menuScene = new MainMenu(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
        overScene = new GameOverScene(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

        //// 2. 加载用户数据
        //std::vector<UserData> all = loadAllUsers();
        //currentUserRecords = getCurrentUserRecords(all);

        // 3. 确保音频正常
        if (bgm_main && ChunkChannel == -1) {
            ChunkChannel = Mix_PlayChannel(1, bgm_main, -1);
        }

        SDL_Log("Debug Mode: Skipped Intro, initialized Menu directly.");
    }

    while (isAppRunning)
   {
        // 如果处于主菜单，实时刷新显示的记录数据 (应对用户切换后的数据更新)
        if (currentState == GameState::MAIN_MENU)
        {
            currentUserRecords = getCurrentUserRecords(all);
            int dIdx = getDiffIndex(menuScene->selectedDiff);
            int mIdx = getModeIndex(menuScene->selectedMode);
            menuScene->currentDisplayRecord = currentUserRecords.values[dIdx][mIdx];
        }

        // 1. 事件处理
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            // 手动点击退出响应
            if (ev.type == SDL_QUIT) isAppRunning = false;

            // 全局 ESC 键响应
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
                //局内ESC返回菜单
                if (currentState == GameState::GAME_PLAYING) {
                    if (game) {
                        delete game;
                        game = nullptr;
                    }

                    delete menuScene;
                    menuScene = new MainMenu(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

                    currentState = GameState::MAIN_MENU;
                    menuScene->isStartClicked = false;

                    if (bgm) Mix_HaltMusic();
                    if (bgm_main) {
                        // 局外bgm循环播放
                        ChunkChannel = Mix_PlayChannel(1, bgm_main, -1);
                    }
                }
                else {
                    isAppRunning = false;
                }
            }

            // --- 根据状态分发事件 ---
            switch (currentState)
            {
            case GameState::LOGIN_SCREEN:
                // 暂停时，将登录事件传给 introScene 处理
                loginScene->handleEvent(&ev);

                // 检查登录是否成功
                if (loginScene->isLoginSuccess) {
                    // 加载用户数据
                    all = loadAllUsers();
                    currentUserRecords = getCurrentUserRecords(all);

                    // 切换状态机到主菜单
                    currentState = GameState::MAIN_MENU;

                    // 停止开场音乐
                    if (Mix_PlayingMusic())
                    {
                        Mix_HaltMusic();
                    }
                    // 播放菜单背景音
                    if (bgm_main && ChunkChannel == -1)
                    {
                        ChunkChannel = Mix_PlayChannel(1, bgm_main, -1);
                    }
                }
                break;

            case GameState::MAIN_MENU:
            {
                //交给主菜单响应点击
                menuScene->handleEvent(&ev);

                //检查是否点击了开始
                if (menuScene->isStartClicked) {
                    // 保存当前用户纪录（处理新用户情况，创建记录）
                    saveCurrentUserRecords(currentUserRecords);
                    all = loadAllUsers();

                    //停止局外bgm
                    if (ChunkChannel != -1) {
                        Mix_HaltChannel(ChunkChannel);
                        ChunkChannel = -1;
                    }

                    //播放p2视频
                    videoP2->reset();

                    //播放p2音乐
                    if (bgm_part2) {
                        Mix_PlayMusic(bgm_part2, 0);
                    }

                    // 切换状态机到过场动画
                    currentState = GameState::INTRO_PART2;
                }
                break;

            case GameState::GAME_PLAYING:
                if (game && !isGameOver) {
                    //交给游戏内响应点击
                    game->handleEvent(&ev);

                    //按R重置游戏(暂未实现)
                    //if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_r) {
                    //    delete game;
                    //    game = new GameBoard(renderer, WINDOW_WIDTH, WINDOW_HEIGHT,
                    //        menuScene->selectedDiff, menuScene->selectedMode);
                    //    startTime = SDL_GetTicks64();
                    //}
                }
                break;

            case GameState::GAME_OVER:
                //交给结算界面响应点击
                overScene->handleEvent(&ev);
                break;
            }
            }
        }
            // 2. 逻辑更新与渲染
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            //清空画面

            switch (currentState)
            {
                // 1. 状态：开场动画
            case GameState::INTRO_PART1:
                // 更新并渲染视频帧
                // updateAndRender 返回 false 表示视频播放完毕（意外结束）
                if (!videoP1->updateAndRender()) {
                    // 视频结束，切换到登录界面
                    currentState = GameState::LOGIN_SCREEN;
                }
                break;
                // 2. 状态：等待登录
            case GameState::LOGIN_SCREEN:
                loginScene->render();
                break;

                // 3. 状态：主菜单
            case GameState::MAIN_MENU:
                menuScene->render();
                break;

                // 4. 状态：过场动画
            case GameState::INTRO_PART2:
                if (!videoP2->updateAndRender())
                {
                    // 过场动画结束，真正开始游戏

                    // 销毁旧游戏对象 (防止内存泄漏)
                    if (game)
                    {
                        delete game;
                    }

                    // 停止 Part 2 的音乐
                    Mix_HaltMusic();

                    // 播放游戏核心 BGM (循环)
                    Mix_VolumeMusic(32);
                    if (bgm)
                    {
                        Mix_PlayMusic(bgm, -1);
                    }

                    // 创建新游戏局
                    game = new GameBoard(renderer, WINDOW_WIDTH, WINDOW_HEIGHT,
                        menuScene->selectedDiff, menuScene->selectedMode);

                    // 设置状态到游戏中
                    currentState = GameState::GAME_PLAYING;
                    startTime = SDL_GetTicks64();
                    isGameOver = false;
                }
                break;

                // 5. 状态：游戏进行中
            case GameState::GAME_PLAYING:
                if (game) {
                    Uint64 nowTime = SDL_GetTicks64();
                    currentDuration = (nowTime - startTime) / 1000.0;
                    game->render(currentDuration); // 绘制棋盘、连线、计时器等

                    if (!isGameOver) {
                        game->update(); // 更新迷雾、倒计时等逻辑

                        // 检查胜利条件
                        if (game->isVictory()) {
                            isGameOver = true;

                            Uint64 endTime = SDL_GetTicks64();
                            currentDuration = (endTime - startTime) / 1000.0;

                            // 停止游戏 BGM
                            if (bgm)
                            {
                                Mix_HaltMusic();
                            }

                            //// 播放胜利音效在 GameBoard 内部或者这里处理
                            // 播放胜利音效
                            game->playWinSound();
                            //SDL_Log("Victory! Time: %.2f s", currentDuration);
                            {
                                int dIdx = getDiffIndex(menuScene->selectedDiff);
                                int mIdx = getModeIndex(menuScene->selectedMode);

                                // 检查是否打破纪录并且更新上传
                                if (currentDuration < currentUserRecords.values[dIdx][mIdx]) {
                                    currentUserRecords.values[dIdx][mIdx] = currentDuration;
                                    saveCurrentUserRecords(currentUserRecords);
                                    all = loadAllUsers();
                                }
                            }
                            //// 弹出提示框 (简单实现，后续应改为图形化结算界面)
                            //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, u8"恭喜", u8"你赢了！按 R 重来", window);

                            // 重置结算页状态并跳转，不销毁 game 对象（为了画背景）
                            overScene->reset();
                            currentState = GameState::GAME_OVER;
                        }
                    }
                }
                break;

            case GameState::GAME_OVER:
                // 1. 先画游戏背景 (定格) 
                // 最终时间即为当前时间
                if (game) game->render(currentDuration);

                {
                    // 2. 画结算蒙版和UI
                    int dIdx = getDiffIndex(menuScene->selectedDiff);
                    int mIdx = getModeIndex(menuScene->selectedMode);
                    overScene->render(currentDuration, currentUserRecords.values[dIdx][mIdx]);
                }

                // 3. 检查是否点了返回
                if (overScene->isReturnClicked) {
                    // 销毁这一局
                    if (game) { delete game; game = nullptr; }

                    // 返回菜单
                    currentState = GameState::MAIN_MENU;
                    menuScene->isStartClicked = false;

                    // 恢复菜单背景音
                    if (bgm_main && ChunkChannel == -1)
                    {
                        ChunkChannel = Mix_PlayChannel(1, bgm_main, -1);
                    }
                }
                break;
            }
		//dif jingyan end
        
        // 可以在这里绘制全局 UI，如计时器文字等
        // ...

        SDL_RenderPresent(renderer);
    }
    // ==========================================
    // 清理资源
    // ==========================================
	if (videoP1) delete videoP1;
	if (videoP2) delete videoP2;
    if (loginScene) delete loginScene;
    if (menuScene) delete menuScene;
    if (game) delete game;
	if (overScene) delete overScene;

    if (bgm) Mix_FreeMusic(bgm);
    if (bgm_part1) Mix_FreeMusic(bgm_part1);
	if (bgm_part2) Mix_FreeMusic(bgm_part2);    
    if (bgm_main) Mix_FreeChunk(bgm_main);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}