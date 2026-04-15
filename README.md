# 🌟 原神连连看 Genshin-LinkLink-SDL-v2
初始写于2025.12，现作归档。鉴于时间间隔久远，且代码语言、文件结构、资源文件均有较大变动，故另开一个项目。（作为程序设计大一上大作业）

 相比v1.0，用C++面向对象、模块化架构重构了C语言面条式结构，新增难度选择、迷雾模式玩法等功能，以及多用户记录、启动游戏动画、设置界面、游戏内实时计时器、结算窗口表现优化等（存储占用主要在视频和音乐）
 
---

基于 **C++** 和 **SDL2** 引擎开发的一款《原神》主题连连看小游戏。本项目不仅实现了经典的连连看核心玩法，还一定程度上复刻了《原神》风格的 UI 界面，并加入了开场/过场视频、多用户本地存档、以及特色“迷雾模式”（鹤观寻航）。

无论是作为 C++/SDL2 的学习实战项目，还是作为原神同人小游戏，都具有参考与游玩价值。

👤 **开发作者：** Wenyaowo & Jingyan

## 📸 游戏截图 (Screenshots)

*(提示：上传到 GitHub 后，请将这里的图片路径替换为你仓库中存放截图的实际路径)*

| 登录界面 | 主菜单 |
| :---: | :---: |
| <img width="400" alt="image" src="https://github.com/user-attachments/assets/6f447dc8-fcdb-43d8-ab3b-6a6810945e6f" /> |  <img width="400" alt="image" src="https://github.com/user-attachments/assets/9deda365-b98a-45f3-b1bd-eb86fa66f531" /> |
| **游戏对局 (含连线与计时)** | **结算界面 (破纪录提示)** |
| <img width="400" alt="image" src="https://github.com/user-attachments/assets/6e28eeb8-da82-45d0-a4e3-095f33253238" /> | <img width="400" alt="image" src="https://github.com/user-attachments/assets/c738323a-bd99-4119-9d2e-d961d6579fe6" /> |

## ✨ 核心特性 (Features)

*   🎬 **沉浸式视听体验：**
    *   使用 OpenCV 解码并渲染 `.mp4` 格式的开场与过场动画。
    *   包含高度还原的《原神》UI、字体渲染，以及全程原声 BGM 和交互音效。
*   🎮 **丰富的游戏模式：**
    *   **难度分级：** 简单 (8x8) / 标准 (10x10) / 困难 (12x12)。
    *   **经典模式：** 传统的连连看消除玩法。
    *   **迷雾模式：** 独创玩法，棋盘上会周期性生成遮挡视野的迷雾（动态刷新），增加挑战性！
*   💾 **多用户存档系统：**
    *   通过用户名登录，游戏自动为不同用户创建独立的存档文件 (`record.bin`)。
    *   分别记录每个用户在不同“模式 x 难度”组合下的 **历史最快通关时间**。
*   ⚙️ **完善的状态机与 UI 交互：**
    *   严格的 `GameState` 状态机管理（开场->登录->菜单->游戏->结算）。
    *   手写的下拉菜单、悬停动画、文本输入框、带描边和抗锯齿的字体渲染。
*   🧠 **高效的寻路算法：**
    *   支持 0折（直连）、1折、2折的连连看经典寻路校验算法。

## 🛠️ 技术栈与依赖 (Tech Stack & Dependencies)

*   **开发语言：** C++
*   **图形与媒体库：** 
    *   [SDL2](https://libsdl.org/) (核心窗口与渲染) - SDL2-2.32.10-VC
    *   [SDL2_image](https://github.com/libsdl-org/SDL_image) (PNG/JPG 图片加载) - SDL2_image-2.8.8-VC
    *   [SDL2_mixer](https://github.com/libsdl-org/SDL_mixer) (音频播放, FLAC/WAV 支持) - SDL2_mixer-2.8.1-VC
    *[SDL2_ttf](https://github.com/libsdl-org/SDL_ttf) (TrueType 字体渲染) - SDL2_ttf-2.24.0-VC
*   **视频解码：** [OpenCV 4.x](https://opencv.org/) (用于读取帧并转化为 SDL 纹理流)
*   **开发环境：** 推荐使用 Visual Studio (Windows)

## 🚀 编译与运行 (Build & Run)

1.  **配置环境：**
    *   在 Visual Studio 中创建一个 C++ 空项目。
    *   通过 vcpkg 或手动配置安装 `SDL2`, `SDL2_image`, `SDL2_mixer`, `SDL2_ttf` 以及 `OpenCV`。
    *   将包含目录和库目录配置到项目中。
2.  **添加源码：**
    *   将本仓库中的 `.cpp` 和 `.h` 文件添加到项目中。
3.  **放置资源：**
    *   下载资源包（如果有提供 Release 压缩包），将 `assets` 文件夹放置在生成的 `.exe` 同级目录，或者配置 VS 的工作目录到项目根目录。
4.  **编译：**
    *   建议在 `Release` 模式下编译，以获得最佳帧率体验。
    *   控制台已设置为 UTF-8 (`SetConsoleOutputCP(65001)`) 防止控制台乱码。

## 🕹️ 游戏操作 (Controls)

*   **登录界面：** 键盘输入昵称，按 `Enter` 键确认登录。
*   **主菜单：** 鼠标左键点击下拉菜单选择模式与难度，点击“开始”。
*   **游戏内：** 
    *   `鼠标左键`：点击选中两个相同的角色头像进行消除。
    *   `ESC`：在游戏局内按下可放弃当前对局，返回主菜单。全局按下可退出游戏。
*   **结算界面：** 点击“返回菜单”继续挑战。

## 📜 许可证 (License)

本项目代码部分采用 [MIT License](LICENSE) 开源协议。

> **⚠️ 免责声明 (Disclaimer)：**
> 本项目仅供编程学习与技术交流使用。游戏中使用的所有美术素材、音乐、视频、字体及“原神”相关 IP 均属于 **[上海米哈游网络科技股份有限公司 (miHoYo)](https://www.mihoyo.com/)** 及相关版权方所有。请勿将本项目用于任何商业用途！



## 实机演示：

https://github.com/user-attachments/assets/35246d6f-9c3b-492b-980a-3fdc89f52d8c

---
## 日期记录：
<img height="400" alt="ScreenShot_2026-04-09_082746_695" src="https://github.com/user-attachments/assets/6385db07-3a5c-40fd-9de9-0c8b26c248c2" /> <img height="400" alt="ScreenShot_2026-04-09_082958_601" src="https://github.com/user-attachments/assets/457b7855-34fd-456e-af97-87a5b3d81e69" />

---

*If you like this project, please consider giving it a ⭐!*

---
