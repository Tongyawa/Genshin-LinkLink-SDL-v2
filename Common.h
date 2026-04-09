#pragma once
// 防止头文件重复包含

#include <SDL.h>
// 引入SDL核心库
#include <string>

struct Point
{
    // 行坐标
    int r;
    // 列坐标
    int c;

    // 重载 == 运算符，方便比较两个点是否相同
    bool operator==(const Point& other) const
    {
        return r == other.r && c == other.c;
    }

    // 重载 != 运算符
    bool operator!=(const Point& other) const
    {
        return !(*this == other);
    }
};

//dif jingyan start
enum class GameState {
    //开场动画播放
	INTRO_PART1,
	//开场动画暂停&&登录界面
    LOGIN_SCREEN,
    //主菜单
    MAIN_MENU,
	//开场动画继续播放
    INTRO_PART2,
	//游戏中
    GAME_PLAYING,
    //游戏结算
    GAME_OVER
};

// 简单的纪录存储结构：每种模式+难度组合对应一个 double 时间
struct Records { // 【修改】名称改为 Records
    // 二维数组： [难度索引][模式索引]
    // 难度: 0=EASY, 1=NORMAL, 2=HARD
    // 模式: 0=CLASSIC, 1=FOG
    double values[3][3];

    Records() {
        // 初始化所有记录为 9999.0
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                values[i][j] = 9999.0;
    }
};

// 我们将所有用户的数据存在同一个Records.bin中
// 这个结构体代表文件中的“一行”数据
struct UserData {
    char username[17]; // 用户名 (16字符 + 1结束符)
    Records records;   // 该用户的专属纪录

    UserData() {
        memset(username, 0, sizeof(username));
    }
};
//dif jingyan end

// 游戏模式枚举
enum class GameMode
{
    CLASSIC,        // 传统模式
    //PROP,           // 道具模式（蹦蹦炸弹）
    FOG,            // 迷雾模式（鹤观寻航）
};

// 难度枚举
enum class Difficulty
{
    EASY,           // 简单 (8x8)
    NORMAL,         // 中等 (10x10)
    HARD            // 困难 (12x12)
};

//dif jingyan start
// 辅助函数：将枚举转为数组索引
inline int getDiffIndex(Difficulty d) {
    return (int)d; 
    // EASY=0, NORMAL=1, HARD=2
}
// 辅助函数：将模式转为数组索引
inline int getModeIndex(GameMode m) {
    // GameMode 定义是 CLASSIC=0, PROP=1, FOG=2
    // 没有 PROP 了也不要紧，这里直接转 int 即可，只要不越界
    return (int)m;
}

// 全局变量
// 当前登录的用户名 (在 main.cpp 中定义)
extern std::string currentUsername;
//dif jingyan end

// 基础配置常量
// 这里的宽和高可以作为基础单位
const int ANIMAL_WIDTH = 80;
const int ANIMAL_HEIGHT = 80;
const int ANIMAL_TYPES = 30; // 图片种类数量