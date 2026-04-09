#pragma once
// 防止头文件重复包含

#include <SDL.h>
// 引入SDL核心库

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

// 游戏模式枚举
enum class GameMode
{
    CLASSIC,        // 传统模式
    PROP,           // 道具模式（蹦蹦炸弹）
    FOG,            // 迷雾模式（鹤观寻航）
};

// 难度枚举
enum class Difficulty
{
    EASY,           // 简单 (8x8)
    NORMAL,         // 中等 (10x10)
    HARD            // 困难 (12x12)
};

// 基础配置常量
// 这里的宽和高可以作为基础单位
const int ANIMAL_WIDTH = 80;
const int ANIMAL_HEIGHT = 80;
const int ANIMAL_TYPES = 30; // 图片种类数量