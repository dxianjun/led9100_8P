# Led9100_8P 工程说明

## 1. 项目概述

本工程是基于 `CIU32F003F5Px` 的 LED9100 8P PWM 转换固件，使用 Keil MDK + ARMCC 工具链。当前代码以 `硬件脚位定义说明.md` 为硬件依据，将两路输入 PWM 转换为两路 TIM1 输出 PWM。

核心功能：

- 采集两路 PWM 输入：`PWM_IN1 = PB0/TIM3_CH1`，`PWM_IN2 = PA7/TIM3_CH2`。
- 支持 `TSSOP20` / SOP8 两种输出脚位配置；当前默认 `TSSOP20 = 1`。
- TSSOP20 跟随输入输出 PWM：`PWM_OUT1 = PA1/TIM1_CH3`，`PWM_OUT2 = PB7/TIM1_CH4`。
- TSSOP20 测试 PWM 输出：`TEST_PWM1 = PA0/TIM1_CH1`，`TEST_PWM2 = PC1/TIM1_CH2(AF2)`。
- SOP8 跟随输入输出 PWM：`PWM_OUT1 = PB2/TIM1_CH3`，`PWM_OUT2 = PB1/TIM1_CH4`；测试 PWM 和 LED 调试不启用。
- UART1 调试通信：`PA3/UART1_TX`，`PA4/UART1_RX`。
- 使用 SysTick 维护 125 us、1 ms、10 ms、1 s 软件计时。
- 支持串口命令打开输入/输出打印、切换模式、分别设置两路测试 PWM 等级。

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
| 主版本标识 | `LED9101_8P_V1.01`（当前 `CHIP_VER = CHIP_9101`，`TSSOP20 = 1`） |

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
```

仓库根目录还包含 `硬件脚位定义说明.md`、`开发规范.md`、`github版本管理规范.md`、历史备份目录和烧录档目录。

## 4. 当前工程源码分层

| 文件 | 分层 | 作用 |
| --- | --- | --- |
| `main.c` | 程序入口 | 系统初始化、版本打印、主循环喂狗和服务调度 |
| `common.c/.h` | 公共 BSP | RCH 系统时钟、IWDG 初始化 |
| `SysTick_bsp.c/.h` | 计时 BSP | SysTick 125 us 节拍、软件超时数组、延时函数 |
| `usart_bsp.c/.h` | 串口 BSP | UART1 PA3/PA4 初始化、RXNE 接收、printf 重定向 |
| `tim_bsp.c/.h` | 定时器 BSP | TIM3 输入采样、TIM1 PWM 输出、输入超时处理 |
| `app_service.c/.h` | 应用层 | 亮度曲线、输出脉冲计算、输入状态处理、主服务函数 |
| `app_tcp.c/.h` | 通信应用层 | UART1 命令解析和接收帧处理 |

`input_bsp.c`、`rtc_bsp.c`、`exti_bsp.c`、I2C/EEPROM 相关文件仍保留在源码目录中，但当前 F003 主流程未启用。`output_bsp.c` 当前仅启用 `led_init()`，历史输出控制代码已用条件编译保留。

## 5. 启动流程

`main()` 当前执行顺序：

1. `system_clock_config()`：启用 RCH，并设置系统时钟为 `RCH_VALUE`。
2. `SysTick_init()`：配置 125 us SysTick，并在中断中派生 1 ms、10 ms、1 s 标志。
3. `uart_gpio_init()`、`uart_init()`、`usart_init_int()`：初始化 UART1 和 RXNE 中断。
4. 打印硬件版本、编译日期时间、软件版本。
5. `app_init()`：初始化应用状态、PWM 目标值和模式。
6. `tim3_gpio_init()`：配置 PB0/PA7 为 TIM3 输入脚。
7. `uc_sel_ch = 1U`，`tim3_input_init()`，`bsp_tim3_capture_start()`：先启动 PWM_IN1 采样。
8. `tim1_gpio_init()`、`tim1_output_init()`、`tim1_output_start()`：启动 TIM1 输出。
9. `led_init()`：初始化 LED1。
10. `iwdg_init()`：启动独立看门狗，当前使用 RCL 时钟并配置 `IWDG_OVERFLOW_PERIOD_2048`。
11. 主循环执行 `TimFlg_Hand()` 和 `user_serv()`。

## 6. 外设与脚位

| 功能 | MCU 引脚 | 外设/通道 | 说明 |
| --- | --- | --- | --- |
| PWM_IN1 | PB0 | TIM3_CH1 | PWM 输入 1，允许 800 Hz - 24 kHz |
| PWM_IN2 | PA7 | TIM3_CH2 | PWM 输入 2，允许 800 Hz - 24 kHz |
| PWM_OUT1 | PA1 | TIM1_CH3 | TSSOP20 跟随输入输出通道 1，PWM1 模式，高电平有效 |
| PWM_OUT2 | PB7 | TIM1_CH4 | TSSOP20 跟随输入输出通道 2，PWM2 模式，高电平有效 |
| PWM_OUT1 | PB2 | TIM1_CH3 | SOP8 跟随输入输出通道 1，PWM1 模式，高电平有效 |
| PWM_OUT2 | PB1 | TIM1_CH4 | SOP8 跟随输入输出通道 2，PWM2 模式，高电平有效 |
| TEST_PWM1 | PA0 | TIM1_CH1 | TSSOP20 测试 PWM 输出 1，PWM1 模式，高电平有效 |
| TEST_PWM2 | PC1 | TIM1_CH2(AF2) | TSSOP20 测试 PWM 输出 2，PWM1 模式，高电平有效 |
| LED1 | PB1 | GPIO 输出 | 仅 TSSOP20 初始化，SysTick 1 s 翻转一次 |
| UART1_TX | PA3 | UART1_TX | 调试发送 |
| UART1_RX | PA4 | UART1_RX | 调试接收 |
| SWDIO | PB6 | SWDIO | 下载/调试 |
| SWCLK | PA2 | SWCLK | 下载/调试 |

## 7. PWM 输入采样

当前 TIM3 采用单定时器轮流采样两路输入：

- `uc_sel_ch = 1U` 时采样 PWM_IN1。
- `uc_sel_ch = 2U` 时采样 PWM_IN2。
- `Capture_switch()` 在两路输入之间切换 TIM3 输入捕获配置。
- 输入捕获使用上升沿作为周期捕获，下降沿作为高电平宽度捕获。
- `DET_CNT = 8`，`BIT_SHIFT = 3`，对周期和高电平计数做 8 次平均。
- 输入周期限制为 800 Hz - 24 kHz；超出范围时本次输入占空比按 0 处理。
- `input_timeout_check()` 在无信号时根据 GPIO 消抖后的当前电平给出 0 或满量程输入，并定期切换检测通道。
- `Capture_switch()` 切换采样通道时会清理当前通道的周期/占空累加状态，但保留最近一次有效 `freq_value`/`duty_value`。

当前 `TIM3_IRQHandler()` 里的占空比按 `PWM_SCALE = 12000` 换算：

```text
duty_value = (duty_sum * PWM_SCALE + 1) / (period_sum + 1)
```

## 8. PWM 输出

TIM1 输出由 `tim_bsp.c` 负责寄存器写入：

- `TSSOP20 = 1` 时，`TIM1_CH1(PA0)` 和 `TIM1_CH2(PC1)` 为测试 PWM，均使用 PWM1 模式。
- `TSSOP20 = 1` 时，`TIM1_CH3(PA1)` 为跟随输入输出 1，`TIM1_CH4(PB7)` 为跟随输入输出 2。
- `TSSOP20 = 0` 时，`TIM1_CH3(PB2)` 为跟随输入输出 1，`TIM1_CH4(PB1)` 为跟随输入输出 2，不启用 CH1/CH2 测试 PWM。
- CH1 - CH3 使用 PWM1 模式，CH4 使用 PWM2 模式；已启用 ARR 预装载，实际启用的 CCR 通道也会打开预装载。
- `tim1_apply_output(period_ticks, pulse1_ticks, pulse2_ticks)` 根据目标周期和脉宽写入 ARR、CH3 CCR、CH4 CCR。
- 输出周期限制在 `ARR_16K` 到 `ARR_2K` 范围内。
- `pulse_to_ccr1()` 和 `pulse_to_ccr2n()` 会结合 `MIN_PULSE`、`DUTY_DELT`、`DT_SET` 处理死区补偿和极限输出。
- `pwmLv1`、`pwmLv2` 命令在 TSSOP20 配置下设置测试 PWM CH1/CH2 等级；跟随输出 CH3/CH4 仍由输入采样和亮度曲线计算。

当前 `CHIP_VER` 默认：

```c
#define CHIP_VER CHIP_9101
#define TSSOP20  1
```

不同芯片版本会影响 `MIN_PULSE`、`DUTY_DELT`、`DT_SET`。

## 9. 应用层逻辑

`app_service.c` 管理 LED9100 应用状态：

- `brightness_curve_anchors[]`：亮度到输出周期/总脉宽的曲线表。
- 当前非 `FIX_16K` 曲线中，pulse 和 period 相对早期 2 kHz 表已放大 2 倍；`FIX_16K=1` 时使用固定 3000 tick 周期表。
- `compute_output_pulses()`：根据模式和两路输入计算 PWM1/PWM2 实际输出脉宽。
- `app_task()`：处理输入更新、输入超时、输出计算和调试打印，计算完成后由 `uc_cal_step` 协调到 TIM1 更新中断写入。
- `pwm_update_isr()`：在 TIM1 更新中断中平滑追踪目标占空比，并在合适时机写入 TIM1 输出；TSSOP20 配置下同时刷新测试 PWM。
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
| `pwmLv1=0..100` | 手动设置 TSSOP20 测试 PWM1 等级 |
| `pwmLv2=0..100` | 手动设置 TSSOP20 测试 PWM2 等级 |

未知命令会打印命令提示；当前提示字符串中仍保留旧的 `pwmLv=0..100` 文案，实际解析命令以 `pwmLv1=`、`pwmLv2=` 为准。

## 11. SysTick 软件计时

`SysTick_bsp.c` 独立维护软件计时。当前 SysTick 硬件中断周期为 125 us：

- `Tim0_1ms_flg`
- `Tim1ms_flg`
- `Tim10ms_flg`
- `Tim1s_flg`
- `TimOut1mS[]`
- `TimOut10mS[]`

`TimFlg_Hand()` 在主循环中调用，负责：

- 递增 1 ms 软件计数。
- 递增 10 ms 软件计数。
- 调用 `uart1_rx_idle_check()` 做 UART1 10 ms 空闲收帧。

`Delay_125us()` 和 `Delay_1ms()` 均按分段方式等待，并在等待过程中保留喂狗调用。`SysTick_Handler()` 当前在 TSSOP20 配置下每 1 s 翻转一次 LED1。

## 12. 看门狗

`iwdg_init()` 当前流程：

- 使能 RCL，并等待 `RCC_CSR2_RCLRDY`。
- 打开 IWDG 写权限。
- 设置溢出周期为 `IWDG_OVERFLOW_PERIOD_2048`。
- 启动前先执行一次 `std_iwdg_refresh()`。
- 主循环、`user_serv()` 和延时函数中保留喂狗调用。

## 13. 编译与验证

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

代码已更新到 `LED9101_8P_V1.01` 方向，`功能介绍.txt` 中记录了 20260616 版本的程序大小、看门狗验证、TSSOP20/SOP8 配置和输出口调整备注。README 更新本身未执行 Keil 全量编译。

## 14. 开发注意事项

1. C 源码按 `开发规范.md` 使用 ANSI/GBK 编码；Markdown 使用 UTF-8 no BOM。
2. 不修改模板工程中已有中文备注。
3. 不修改 `/* ... */` 形式注释掉的备注。
4. 修改硬件脚位、复用功能或外设通道时，必须同步更新 `硬件脚位定义说明.md`、源码和 README。
5. `Objects/`、`Listings/`、Source Insight cache 等生成物不应作为发布源码提交。
6. 发布前需要完成 Keil 全量编译，并用示波器确认 PWM_IN1/PWM_IN2 输入采样和 PWM1/PWM2 输出波形。
