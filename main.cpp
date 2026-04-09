#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <iostream>
#include <fstream>      // 引入文件流，替代 FILE*
#include <string>

#include "Common.h"
#include "GameBoard.h"

// 屏幕尺寸配置
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 900;

// 全局音乐指针
Mix_Music* bgm = nullptr;

// 排行榜数据结构
struct PlayerRecord
{
    double timeUsed;
    // 可以在这里加更多字段，如 Difficulty diff;
};

// ==========================================================
// 读取记录文件 (C++ fstream 风格)
// ==========================================================
double loadBestRecord()
{
    std::string filePath = "assets/Data/record.bin";
    std::ifstream inFile(filePath, std::ios::binary);

    double bestTime = 9999.0;

    if (inFile.is_open())
    {
        inFile.read(reinterpret_cast<char*>(&bestTime), sizeof(double));
        inFile.close();
    }
    else
    {
        SDL_Log("未找到记录文件，将创建新记录。");
    }

    return bestTime;
}

// ==========================================================
// 保存记录文件
// ==========================================================
void saveBestRecord(double time)
{
    std::string filePath = "assets/Data/record.bin";
    std::ofstream outFile(filePath, std::ios::binary);

    if (outFile.is_open())
    {
        outFile.write(reinterpret_cast<const char*>(&time), sizeof(double));
        outFile.close();
    }
}

// ==========================================================
// 主函数
// ==========================================================
int main(int argc, char* argv[])
{
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

    // 4. 创建窗口
    SDL_Window* window = SDL_CreateWindow(
        u8"原神连连看 v2.0 - Made by Wenyao", // 标题
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

    // 6. 加载并播放背景音乐 (示例)
    // 注意：请确保文件路径正确，支持中文路径需转码，这里简化处理
    bgm = Mix_LoadMUS(u8"assets/audio/陈致逸,HOYO-MiX - Moonlike Smile 皎洁的笑颜.flac");
    if (bgm)
    {
        Mix_PlayMusic(bgm, -1); // -1 表示循环播放
    }


    // ==========================================
    // 游戏主循环控制变量
    // ==========================================
    bool isAppRunning = true;

    // 这里模拟菜单选择，之后搭档会把菜单UI做在这里
    // 假设玩家选择了：困难难度 + 迷雾模式
    Difficulty selectedDiff = Difficulty::NORMAL; // 可以改成 HARD 测试
    GameMode selectedMode = GameMode::FOG;     // 可以改成 CLASSIC 测试

    // 创建游戏局实例 (使用 new 在堆上创建)
    GameBoard* game = new GameBoard(renderer, WINDOW_WIDTH, WINDOW_HEIGHT, selectedDiff, selectedMode);

    // 计时器
    Uint64 startTime = SDL_GetTicks64();
    double bestRecord = loadBestRecord();
    bool isGameOver = false;

    while (isAppRunning)
    {
        // 1. 事件处理
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
            {
                isAppRunning = false;
            }
            else if (ev.type == SDL_KEYDOWN)
            {
                if (ev.key.keysym.sym == SDLK_ESCAPE)
                {
                    isAppRunning = false;
                }
                // 测试按键：按 R 重开
                if (ev.key.keysym.sym == SDLK_r)
                {
                    delete game;
                    game = new GameBoard(renderer, WINDOW_WIDTH, WINDOW_HEIGHT, selectedDiff, selectedMode);
                    startTime = SDL_GetTicks64();
                    isGameOver = false;
                }
            }

            // 将事件传递给游戏局处理
            if (!isGameOver && game)
            {
                game->handleEvent(&ev);
            }
        }

        // 2. 逻辑更新
        if (!isGameOver && game)
        {
            game->update();

            // 检查胜利
            if (game->isVictory())
            {
                isGameOver = true;
                Uint64 endTime = SDL_GetTicks64();
                double currentDuration = (endTime - startTime) / 1000.0;

                // 播放胜利音效在 GameBoard 内部或者这里处理
                SDL_Log("Victory! Time: %.2f s", currentDuration);

                if (currentDuration < bestRecord)
                {
                    saveBestRecord(currentDuration);
                    bestRecord = currentDuration;
                }

                // 弹出提示框 (简单实现，后续应改为图形化结算界面)
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, u8"恭喜", u8"你赢了！按 R 重来", window);
            }
        }

        // 3. 渲染绘制
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // 黑底
        SDL_RenderClear(renderer);

        if (game)
        {
            game->render();
        }

        // 可以在这里绘制全局 UI，如计时器文字等
        // ...

        SDL_RenderPresent(renderer);
    }

    // ==========================================
    // 清理资源
    // ==========================================
    if (game) delete game;
    if (bgm) Mix_FreeMusic(bgm);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}