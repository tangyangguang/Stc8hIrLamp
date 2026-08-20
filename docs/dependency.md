# 依赖记录

## Stc8hBase

- 引用方式：相邻目录引用，不使用 submodule。
- 默认位置：与本仓库同级的 `Stc8hBase/`
- 夜灯/遥控器验收时记录的基础库提交：`135ce30`
- 创建本产品仓库时本机基础库当前提交：`50df0a7`
- 当前本机基础库提交：`bf072c8`
- 本次适配：遥控器与夜灯已迁移到 PWM 的 `group + channel` 通用 API，并按该版本的
  `STC8H_PWM_GROUP_MASK` / `STC8H_PWM_A_CHANNEL_MASK` 裁剪为仅 PWMA CH1。

后续如果升级基础库，应先在 `Stc8hBase` 中完成提交，再更新本文件并重新构建、验收 `firmware/lamp` 和 `firmware/remote`。

## 路径约定

- `firmware/lamp/Makefile` 默认 `BASE=../../../Stc8hBase`。
- `firmware/remote/platformio.ini` include path 使用 `../../../Stc8hBase/...`。
- `firmware/remote/src/base/*.c` wrapper include 使用 `../../../../../Stc8hBase/...`。
