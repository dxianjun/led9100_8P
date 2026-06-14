# Led9100_8P 工程说明

## 1. 项目概述

本工程是基于 `CIU32F003F5Px` 的 LED9100 8P PWM 转换固件，使用 Keil MDK + ARMCC 工具链。当前代码以 `硬件脚位定义说明.md` 为硬件依据，将两路输入 PWM 转换为两路 TIM1 输出 PWM。

核心功能：

- 采集两路 PWM 输入：`PWM_IN1 = PB0/TIM3_CH1`，`PWM_IN2 = PA7/TIM3_CH2`。
- 输出两路 PWM：`PWM1 = PB2/TIM1_CH1`，`PWM2 = PB1/TIM1_CH2N`。
- UART1 调试通信：`PA3/UART1_TX`，`PA4/UART1_RX`。
- PB4/PB5 作为 UART1 预留脚，当前不使用。
- 使用 SysTick 维护 1 ms、10 ms、1 s 软件计时。
- 支持串口命令打开输入/输出打印、切换模式、手动设置 PWM 等级。

## 2. 工程环境

| 项目 | 说明 |
| --- | --- |
| IDE | Keil MDK |
| 工程文件 | `Led9100ciu/MDK/ciu32f003.uvprojx` |
| Device | `CIU32F003F5Px` |
| CPU | Cortex-M0+ |
| 标准库 | `Drivers/CIU32F003_Lib` |
| CMSIS | `Drivers/CMSIS` |
| 输出名 | `led9100ciu` |
| 主版本标识 | `LED9100_8P_V1.00` |

## 3. 目录结构

```text
Led9100_8P/
├─ Drivers/
│  ├─ CMSIS/                  CIU32F003 CMSIS Core、Device、启动文件
│  └─ CIU32F003_Lib/          CIU32F003 标准外设库
├─ Led9100ciu/
│  ├─ Include/                应用层和 BSP 头文件
│  ├─ Source/                 应用层和 BSP 源文件
│  ├─ MDK/                    Keil 工程、启动文件、输出目录
│  └─ readme.txt              原始示例说明
├─ 硬件脚位定义说明.md        CIU32F003F5P6-TSSOP20 脚位定义
└─ 开发规范.md                编码、代码格式、注释保护规则
```

## 4. 当前工程源码分层

| 文件 | 分层 | 作用 |
| --- | --- | --- |
| `main.c` | 程序入口 | 系统初始化、版本打印、主循环喂狗和服务调度 |
| `common.c/.h` | 公共 BSP | RCH 系统时钟、IWDG 初始化 |
| `SysTick_bsp.c/.h` | 计时 BSP | SysTick 1 ms 节拍、软件超时数组、延时函数 |
| `usart_bsp.c/.h` | 串口 BSP | UART1 PA3/PA4 初始化、RXNE 接收、printf 重定向 |
| `tim_bsp.c/.h` | 定时器 BSP | TIM3 输入采样、TIM1 PWM 输出、输入超时处理 |
| `app_service.c/.h` | 应用层 | 亮度曲线、输出脉冲计算、输入状态处理、主服务函数 |
| `app_tcp.c/.h` | 通信应用层 | UART1 命令解析和接收帧处理 |

`input_bsp.c`、`output_bsp.c`、`rtc_bsp.c`、`exti_bsp.c`、I2C/EEPROM 相关文件仍保留在源码目录中，但当前 F003 主流程未启用。

## 5. 启动流程

`main()` 当前执行顺序：

1. `system_clock_config()`：启用 RCH，并设置系统时钟为 `RCH_VALUE`。
2. `SysTick_init()`：配置 1 ms SysTick。
3. `uart_gpio_init()`、`uart_init()`、`usart_init_int()`：初始化 UART1 和 RXNE 中断。
4. 打印硬件版本、编译日期时间、软件版本。
5. `app_init()`：初始化应用状态、PWM 目标值和模式。
6. `tim3_gpio_init()`：配置 PB0/PA7 为 TIM3 输入脚。
7. `uc_sel_ch = 1U`，`tim3_input_init()`，`bsp_tim3_capture_start()`：先启动 PWM_IN1 采样。
8. `tim1_gpio_init()`、`tim1_output_init()`、`tim1_output_start()`：启动 TIM1 输出。
9. `iwdg_init()`：启动独立看门狗。
10. 主循环执行 `TimFlg_Hand()` 和 `user_serv()`。

## 6. 外设与脚位

| 功能 | MCU 引脚 | 外设/通道 | 说明 |
| --- | --- | --- | --- |
| PWM_IN1 | PB0 | TIM3_CH1 | PWM 输入 1，允许 800 Hz - 24 kHz |
| PWM_IN2 | PA7 | TIM3_CH2 | PWM 输入 2，允许 800 Hz - 24 kHz |
| PWM1 | PB2 | TIM1_CH1 | 输出通道 1 |
| PWM2 | PB1 | TIM1_CH2N | 输出通道 2，互补通道输出 |
| UART1_TX | PA3 | UART1_TX | 调试发送 |
| UART1_RX | PA4 | UART1_RX | 调试接收 |
| SWDIO | PB6 | SWDIO | 下载/调试 |
| SWCLK | PA2 | SWCLK | 下载/调试 |
| 预留 TX | PB4 | UART1_TX 可选复用 | 当前不使用 |
| 预留 RX | PB5 | UART1_RX 可选复用 | 当前不使用 |

## 7. PWM 输入采样

当前 TIM3 采用单定时器轮流采样两路输入：

- `uc_sel_ch = 1U` 时采样 PWM_IN1。
- `uc_sel_ch = 2U` 时采样 PWM_IN2。
- `Capture_switch()` 在两路输入之间切换 TIM3 输入捕获配置。
- 输入捕获使用上升沿作为周期捕获，下降沿作为高电平宽度捕获。
- `DET_CNT = 8`，`BIT_SHIFT = 3`，对周期和高电平计数做 8 次平均。
- `input_timeout_check()` 在无信号时根据 GPIO 当前电平给出 0 或满量程输入，并定期切换检测通道。

注意：当前 `TIM3_IRQHandler()` 里的占空比计算使用百分比形式：

```text
duty_value = ((duty_sum + 1) * 100) / (period_sum + 1)
```

应用层仍以 `PWM_SCALE = 12000` 为亮度刻度，后续如果希望输入采样直接输出 0..12000，需要同步修改 TIM3 采样换算。

## 8. PWM 输出

TIM1 输出由 `tim_bsp.c` 负责寄存器写入：

- `TIM1_CH1` 使用 PWM1 模式。
- `TIM1_CH2N` 使用 PWM2 模式和互补输出。
- `tim1_apply_output(period_ticks, pulse1_ticks, pulse2_ticks)` 根据目标周期和脉宽写入 ARR/CCR。
- 输出周期限制在 `ARR_16K` 到 `ARR_2K` 范围内。
- `pulse_to_ccr1()` 和 `pulse_to_ccr2n()` 会结合 `MIN_PULSE`、`DUTY_DELT`、`DT_SET` 处理死区补偿和极限输出。

当前 `CHIP_VER` 默认：

```c
#define CHIP_VER CHIP_9100
```

不同芯片版本会影响 `MIN_PULSE`、`DUTY_DELT`、`DT_SET`。

## 9. 应用层逻辑

`app_service.c` 管理 LED9100 应用状态：

- `brightness_curve_anchors[]`：亮度到输出周期/总脉宽的曲线表。
- `compute_output_pulses()`：根据模式和两路输入计算 PWM1/PWM2 实际输出脉宽。
- `app_task()`：处理输入更新、输入超时、输出计算和调试打印。
- `pwm_update_isr()`：在 TIM1 更新中断中平滑追踪目标占空比，并置位计算标志。
- `user_serv()`：主循环服务函数，依次执行 `app_task()` 和 `tcp_hand()`。

当前支持两种模式：

| 模式 | 宏 | 说明 |
| --- | --- | --- |
| CCT | `MODE_CCT` | 按色温/比例方式分配总脉宽 |
| Direct | `MODE_DIRECT` | 默认模式，两路输入按总量比例分配输出 |

当前默认：

```c
#define MODE_DEFAULT MODE_DIRECT
```

## 10. UART1 通信

UART1 配置：

- TX：PA3
- RX：PA4
- 波特率：115200
- 数据格式：8N1
- 中断：RXNE

接收帧结束条件：

- 收到 `$`
- 收到 `\r`
- 收到 `\n`
- 缓冲区满
- 10 ms 内无新字节，软件空闲判帧

`app_tcp.c` 当前命令：

| 命令 | 作用 |
| --- | --- |
| `inpEn=1` | 打开输入占空比打印 |
| `inpEn=0` | 关闭输入占空比打印 |
| `outEn=1` | 打开输出状态打印 |
| `outEn=0` | 关闭输出状态打印 |
| `mode=2` | 切换到 `MODE_CCT` |
| `mode=3` | 切换到 `MODE_DIRECT` |
| `pwmLv=0..100` | 手动设置两路 PWM 目标等级 |

## 11. SysTick 软件计时

`SysTick_bsp.c` 独立维护软件计时：

- `Tim1ms_flg`
- `Tim10ms_flg`
- `Tim1s_flg`
- `TimOut1mS[]`
- `TimOut10mS[]`

`TimFlg_Hand()` 在主循环中调用，负责：

- 递增 1 ms 软件计数。
- 递增 10 ms 软件计数。
- 调用 `uart1_rx_idle_check()` 做 UART1 10 ms 空闲收帧。

## 12. 编译与验证

使用 Keil MDK 打开：

```text
Led9100ciu/MDK/ciu32f003.uvprojx
```

当前工程文件已加入：

- `main.c`
- `app_tcp.c`
- `app_service.c`
- `common.c`
- `tim_bsp.c`
- `SysTick_bsp.c`
- `usart_bsp.c`
- CIU32F003 标准库源码
- `system_ciu32f003.c`
- `startup_ciu32f003.s`

本次 README 更新前，已用 `armcc` 对以下文件做单文件编译检查并通过：

- `main.c`
- `SysTick_bsp.c`
- `tim_bsp.c`
- `usart_bsp.c`
- `app_service.c`
- `app_tcp.c`
- `common.c`

该检查验证了头文件引用和 C 语法，但不等同于 Keil 完整链接和上板波形验证。

## 13. 开发注意事项

1. C 源码按 `开发规范.md` 使用 ANSI/GBK 编码；Markdown 使用 UTF-8 no BOM。
2. 不修改模板工程中已有中文备注。
3. 不修改 `/* ... */` 形式注释掉的备注。
4. 修改硬件脚位、复用功能或外设通道时，必须同步更新 `硬件脚位定义说明.md`、源码和 README。
5. `Objects/`、`Listings/`、Source Insight cache 等生成物不应作为发布源码提交。
6. 发布前需要完成 Keil 全量编译，并用示波器确认 PWM_IN1/PWM_IN2 输入采样和 PWM1/PWM2 输出波形。

