# 构建与烧录

## 前置目录

默认目录结构：

```text
<workspace>/
  Stc8hBase/
  Stc8hIrLamp/
```

## 夜灯固件

```sh
cd firmware/lamp
make clean && make
```

生成文件位于 `firmware/lamp/build/`，该目录不提交。

## 遥控器固件

```sh
cd firmware/remote
pio run -t clean
pio run
```

生成文件位于 `firmware/remote/.pio/`，该目录不提交。

## 烧录

夜灯和遥控器的烧录说明分别见各自 `firmware/*/docs/` 下的验证或构建文档。STC 下载需要目标板重新上电进入 bootloader。
