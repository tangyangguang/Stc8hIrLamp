# 项目规则

- 本仓库是 STC8H 红外遥控夜灯产品仓库，包含夜灯固件、遥控器固件和老 Keil 工程归档。
- `firmware/lamp` 和 `firmware/remote` 使用相邻目录中的 `Stc8hBase` 基础库，不复制基础库源码。
- `legacy/` 为历史归档，除非用户明确要求，不修改老 Keil 工程内容。
- 新固件构建产物不提交：`firmware/lamp/build/`、`firmware/remote/.pio/`。
- 修改固件后必须分别构建对应项目，并说明是否需要硬件复测。
- 依赖 `Stc8hBase` 的版本变更必须记录到 `docs/dependency.md`。
- 不为旧 Keil 工程的不合理设计增加兼容层；新实现以当前硬件和验收行为为准。
- 本文件保持在 100 行以内。
