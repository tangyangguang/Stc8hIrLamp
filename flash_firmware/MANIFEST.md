# 固件清单

生成时间：2026-05-12

固件源码提交：`a692eb3`

生成命令：

```sh
cd firmware/lamp && make clean && make
cd firmware/remote && pio run -t clean -e STC8H1K08 && pio run -e STC8H1K08
cd firmware/remote && pio run -t clean -e STC8H1K08_2TY_NoLED && pio run -e STC8H1K08_2TY_NoLED
```

| 文件 | 来源产物 | 大小 bytes | SHA1 |
| --- | --- | ---: | --- |
| `ir_lamp.ihx` | `firmware/lamp/build/ir_lamp.ihx` | 14784 | `418f3e52f583dbc8d2deb3fe451fa37003436d1a` |
| `remote_J3Y_LED.hex` | `firmware/remote/.pio/build/STC8H1K08/firmware.hex` | 9400 | `d3ab85841856d665ea98e351cbd8b41b9aacf94e` |
| `remote_2TY_NoLED.hex` | `firmware/remote/.pio/build/STC8H1K08_2TY_NoLED/firmware.hex` | 9348 | `82cbec9946a10d058668f00173e68af9061042b1` |
