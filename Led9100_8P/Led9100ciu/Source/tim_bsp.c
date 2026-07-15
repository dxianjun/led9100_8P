/************************************************************************************************/
/**
* @file               tim_bsp.c
* @brief              CIU32F003 timer and PWM BSP for LED9100 8P.
************************************************************************************************/

#include "main.h"

#define TIM3_ARR_VALUE          (0xFFFFU)

#define INPUT_PERIOD_MIN        (2000U)     /* 24 kHz at 48 MHz */
#define INPUT_PERIOD_MAX        (60000U)    /* 800 Hz at 48 MHz */

#define DET_CNT                 (4U)
#define BIT_SHIFT               (2U)

#define INPUT_TIMEOUT_1MS      (8U)
#define INPUT_TIMEOUT_DEBOUNCE_READS  (3U)
#define INPUT_TIMEOUT_DEBOUNCE_DELAY  (4U)


typedef struct
{
	uint32_t period_sum;
	uint32_t duty_sum;

    uint16_t freq_value;
    uint16_t duty_value;

	uint8_t cap_cnt;
    uint8_t updated;
} pwm_input_channel_t;

typedef enum
{
	INPUT_STATE_CAPTURE_CH1 = 0,
	INPUT_STATE_CALCULATE_CH1,
	INPUT_STATE_CAPTURE_CH2,
	INPUT_STATE_CALCULATE_CH2
} input_state_t;

static volatile pwm_input_channel_t pwm_in1 = {0};
static volatile pwm_input_channel_t pwm_in2 = {0};
static volatile input_state_t input_state = INPUT_STATE_CAPTURE_CH1;

volatile uint32_t g_ch1_ccx_value = 0U;
volatile uint32_t g_ch2_ccx_value = 0U;

static void reset_pwm_capture_state(volatile pwm_input_channel_t *input)
{
	input->period_sum = 0U;
	input->duty_sum = 0U;
	input->cap_cnt = 0U;
	input->updated = 0U;

}

static uint8_t read_pin_debounced(GPIO_t *gpiox, uint32_t pin)
{
    uint8_t i;
    uint8_t high_count = 0U;

    for (i = 0U; i < INPUT_TIMEOUT_DEBOUNCE_READS; i++)
        {
        if (std_gpio_get_input_pin(gpiox, pin))
            {
            high_count++;
            }

        if ((i + 1U) < INPUT_TIMEOUT_DEBOUNCE_READS)
            {
            delay_clk(INPUT_TIMEOUT_DEBOUNCE_DELAY);
            }
        }

    return (high_count > (INPUT_TIMEOUT_DEBOUNCE_READS / 2U)) ? 1U : 0U;
}

uint16_t tim1_period_ticks = DUTY_LIMIT_PERIOD_TICKS;

static uint16_t normalize_pulse_ticks(uint16_t pulse_ticks, uint16_t period_ticks)
{
	if (pulse_ticks == 0U)
		{
		return 0U;
		}
	else if (pulse_ticks < MIN_PULSE)
		{
		pulse_ticks = MIN_PULSE;
		}
	else if (pulse_ticks > period_ticks)
		{
		pulse_ticks = period_ticks;
		}

	#if (DUTY_MAX_NO_ADJ == 0)
	uint16_t max_pulse_ticks;
	max_pulse_ticks = (uint16_t)(period_ticks - DUTY_ADJ_OUT);
	if (pulse_ticks >= max_pulse_ticks)
		{
		pulse_ticks = max_pulse_ticks;
		}
	#endif

	return pulse_ticks;
}

static uint16_t clamp_output_period(uint32_t period_ticks)
{
	if (period_ticks < ARR_16K)
		{
		return ARR_16K;
		}

	if (period_ticks > ARR_2K)
		{
		return ARR_2K;
		}

	return (uint16_t)period_ticks;
}

void tim1_apply_output(uint16_t period_ticks, uint16_t pulse1_ticks, uint16_t pulse2_ticks)
{
	uint16_t normalized_pulse1;
	uint16_t normalized_pulse2;
	uint16_t ccr3;
	uint16_t ccr4;
	uint16_t final_ccr3;
	uint16_t final_ccr4;

	period_ticks = clamp_output_period(period_ticks);
	normalized_pulse1 = normalize_pulse_ticks(pulse1_ticks, period_ticks);
	normalized_pulse2 = normalize_pulse_ticks(pulse2_ticks, period_ticks);
	ccr3 = normalized_pulse1;
	ccr4 = (uint16_t)(period_ticks - normalized_pulse2);

	if (normalized_pulse1 == 0U)
		{
		final_ccr3 = 0U;
		}
	else if (((uint32_t)ccr3 + DUTY_DELT) >= period_ticks)
		{
		final_ccr3 = period_ticks;
		}
	else
		{
		final_ccr3 = (uint16_t)(ccr3 + DUTY_DELT);
		}

	if (normalized_pulse2 == 0U)
		{
		final_ccr4 = period_ticks;
		}
	else if (ccr4 <= DUTY_DELT)		// ccr4 >= DUTY_ADJ_OUT, if DUTY_ADJ_OUT==0; ccr4 可能会<=DUTY_DELT;
		{
		final_ccr4 = 0U;
		}
	else
		{
		final_ccr4 = (uint16_t)(ccr4 - DUTY_DELT);
		}

	std_tim_set_autoreload(TIM1, (uint16_t)(period_ticks - 1U));
	std_tim_set_ccx_value(TIM1, TIM_CHANNEL_3, final_ccr3);
	std_tim_set_ccx_value(TIM1, TIM_CHANNEL_4, final_ccr4);
	tim1_period_ticks = period_ticks;
}


void tim3_gpio_init(void)
{
    std_gpio_init_t gpio_init = {0};

    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOA);
    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOB);

	// TIM3_CH1 --> PB0
    gpio_init.pin = GPIO_PIN_0;
    gpio_init.mode = GPIO_MODE_ALTERNATE;
    gpio_init.pull = GPIO_PULLDOWN;
    gpio_init.output_type = GPIO_OUTPUT_PUSHPULL;
    gpio_init.alternate = GPIO_AF3_TIM3;
    std_gpio_init(GPIOB, &gpio_init);

	// TIM3_CH2 --> PA7
    gpio_init.pin = GPIO_PIN_7;
    gpio_init.alternate = GPIO_AF3_TIM3;
    std_gpio_init(GPIOA, &gpio_init);
}

void tim1_gpio_init(void)
{
    std_gpio_init_t gpio_init = {0};

    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOA);
    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOB);
    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOC);

    gpio_init.mode = GPIO_MODE_ALTERNATE;
    gpio_init.pull = GPIO_NOPULL;
    gpio_init.output_type = GPIO_OUTPUT_PUSHPULL;

	// 输入是PB0和PA7，输出是PB1/PB2
	// TIM1_CH3 --> PB2  --> PB0 --> TIM3_CH1
	// TIM1_CH4 --> PB1  --> PA7 --> TIM3_CH2
	
	// PB0-->PB2; PA7-->PB1
    // Follow TIM3 inputs: TIM1_CH3(PB0/PB2), TIM1_CH4(PA7/PB1)

	#if (DEBUG_PWM_OUTPUT == 1)

    // Test PWM outputs: TIM1_CH1(PA0), TIM1_CH2(PA1)
    gpio_init.pin = GPIO_PIN_0;
    gpio_init.alternate = GPIO_AF2_TIM1;
    std_gpio_init(GPIOA, &gpio_init);

    gpio_init.pin = GPIO_PIN_1;
    gpio_init.alternate = GPIO_AF2_TIM1;
    std_gpio_init(GPIOA, &gpio_init);
	#endif
	
	gpio_init.pin = GPIO_PIN_2 | GPIO_PIN_1;			
	gpio_init.alternate = GPIO_AF4_TIM1;
    std_gpio_init(GPIOB, &gpio_init);

}

/*
PWM1模式：CNT < CCRx 时输出为高电平。因此，在计数器的30个周期内输出高电平，剩余70个周期为低电平
PWM2模式：CNT > CCRx 时输出为高电平。因此，在计数器的70个周期内输出高电平，剩余30个周期为低电平
如果配置模式2，同时极性为NEG
PWM2模式：CNT > CCRx 时输出为低电平。
// pwm2_n: CNT > CCRx 时输出为高电平。 因此，在计数器的70个周期内输出高电平，剩余30个周期为低电平
*/
void tim1_output_init(void)
{
    std_tim_basic_init_t basic_init = {0};
    std_tim_output_compare_init_t oc_init = {0};
    std_tim_break_init_t break_init = {0};

    /* TIM1时钟使能 */
    std_rcc_apb2_clk_enable(RCC_PERIPH_CLK_TIM1);

    /* 配置TIM1计数器参数 */
    basic_init.prescaler = 0U;
    basic_init.counter_mode = TIM_COUNTER_MODE_UP;
    basic_init.period = tim1_period_ticks - 1U;
    basic_init.clock_div = TIM_CLOCK_DTS_DIV1;
    basic_init.repeat_counter = 0U;
    std_tim_init(TIM1, &basic_init);

    oc_init.output_state = TIM_OUTPUT_ENABLE;
    oc_init.output_negtive_state = TIM_OUTPUT_NEGTIVE_ENABLE;
    oc_init.output_pol = TIM_OUTPUT_POL_HIGH;
    oc_init.output_negtive_pol = TIM_OUTPUT_NEGTIVE_POL_HIGH;
    oc_init.output_idle_state = TIM_OUTPUT_IDLE_RESET;
    oc_init.output_negtive_idle_state = TIM_OUTPUT_NEGTIVE_IDLE_RESET;

	#if (DEBUG_PWM_OUTPUT == 1)
    /* TIM1_CH1(PA0): test PWM, PWM1 high active */
    oc_init.output_compare_mode = TIM_OUTPUT_MODE_PWM1;
    oc_init.pulse = 2400U;
    std_tim_output_compare_init(TIM1, &oc_init, TIM_CHANNEL_1);

    /* TIM1_CH2(PC1): test PWM, PWM1 high active */
    std_tim_output_compare_init(TIM1, &oc_init, TIM_CHANNEL_2);

	std_tim_preloadccx_channel_enable(TIM1, TIM_CHANNEL_1);
    std_tim_preloadccx_channel_enable(TIM1, TIM_CHANNEL_2);
	#endif
	
    /* TIM1_CH3(PA1/PB2): follows TIM3 input, PWM1 high active */
    oc_init.output_compare_mode = TIM_OUTPUT_MODE_PWM1;
    oc_init.pulse = 0U;
    std_tim_output_compare_init(TIM1, &oc_init, TIM_CHANNEL_3);

    /* TIM1_CH4(PB7/PB1): follows TIM3 input, PWM2 high active */
    oc_init.output_compare_mode = TIM_OUTPUT_MODE_PWM2;
    oc_init.pulse = tim1_period_ticks;
    std_tim_output_compare_init(TIM1, &oc_init, TIM_CHANNEL_4);
	
    std_tim_arrpreload_enable(TIM1);

    std_tim_preloadccx_channel_enable(TIM1, TIM_CHANNEL_3);
    std_tim_preloadccx_channel_enable(TIM1, TIM_CHANNEL_4);

    /* 配置断路输入参数 */
    break_init.off_state_run_mode = TIM_OSSR_ENABLE;
    break_init.off_state_idle_mode = TIM_OSSI_ENABLE;
    break_init.lock_level = TIM_LOCK_LEVEL_OFF;
    break_init.dead_time = DT_SET;
    break_init.break_state = TIM_BREAK_DISABLE;
    std_tim_bdt_init(TIM1, &break_init);

    std_tim_interrupt_enable(TIM1, TIM_INTERRUPT_UPDATE);
    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, NVIC_PRIO_1);
    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
}

volatile uint8_t uc_sel_ch=0;		// 哪个都不选
/**
* @brief  TIM3初始化
* @retval 无
*/
void tim3_input_init(void)
{
    std_tim_basic_init_t basic_init_struct = {0};
	input_state = (uc_sel_ch == 2U) ? INPUT_STATE_CAPTURE_CH2 : INPUT_STATE_CAPTURE_CH1;
    
    /* TIM3时钟使能 */
    std_rcc_apb1_clk_enable(RCC_PERIPH_CLK_TIM3);
    
    /* TIM3基本定时器配置 */
    basic_init_struct.period = TIM3_ARR_VALUE;
    basic_init_struct.clock_div = TIM_CLOCK_DTS_DIV1;
    std_tim_init(TIM3, &basic_init_struct);

	std_tim_input_capture_init_t input_capture_struct = {0};
	if (uc_sel_ch==2U)	// 此时采样硬件ch2上的pwm信号
		{
	    /* 配置为复位模式，且触发源为TI2FP2，触发极性为上升沿触发 */
	    std_tim_slave_mode_config(TIM3, TIM_SLAVE_MODE_RESET);
	    std_tim_trig_source_config(TIM3, TIM_TRIG_SOURCE_TI2FP2);
	    std_tim_set_input_pol(TIM3, TIM_CHANNEL_2, TIM_INPUT_POL_RISING);
	    
	    /* 配置TI2FP2映射到IC2上，且上升沿有效 */
	    input_capture_struct.input_capture_pol = TIM_INPUT_POL_RISING;
	    input_capture_struct.input_capture_sel = TIM_INPUT_CAPTURE_SEL_DIRECTTI;
	    input_capture_struct.input_capture_prescaler = TIM_INPUT_CAPTURE_PSC_DIV1;
	    input_capture_struct.input_capture_filter = 0x00;
	    std_tim_input_capture_init(TIM3, &input_capture_struct, TIM_CHANNEL_2);
	    
	    /* 配置TI2FP2映射到IC1上，且下降沿有效 */
	    input_capture_struct.input_capture_pol = TIM_INPUT_POL_FALLING;
	    input_capture_struct.input_capture_sel = TIM_INPUT_CAPTURE_SEL_INDIRECTTI;
	    std_tim_input_capture_init(TIM3, &input_capture_struct, TIM_CHANNEL_1);
		}
    else if (uc_sel_ch==1U)	// 此时采样硬件ch1上的pwm信号
		{
	    /* 配置为复位模式，且触发源为TI1FP1，触发极性为上升沿触发 */
	    std_tim_slave_mode_config(TIM3, TIM_SLAVE_MODE_RESET);
	    std_tim_trig_source_config(TIM3, TIM_TRIG_SOURCE_TI1FP1);
	    std_tim_set_input_pol(TIM3, TIM_CHANNEL_1, TIM_INPUT_POL_RISING);
	    
	    /* 配置TI1FP1映射到IC1上，且上升沿有效 */
	    input_capture_struct.input_capture_pol = TIM_INPUT_POL_RISING;
	    input_capture_struct.input_capture_sel = TIM_INPUT_CAPTURE_SEL_DIRECTTI;
	    input_capture_struct.input_capture_prescaler = TIM_INPUT_CAPTURE_PSC_DIV1;
	    input_capture_struct.input_capture_filter = 0x00;
	    std_tim_input_capture_init(TIM3, &input_capture_struct, TIM_CHANNEL_1);
	    
	    /* 配置TI1FP1映射到IC2上，且下降沿有效 */
	    input_capture_struct.input_capture_pol = TIM_INPUT_POL_FALLING;
	    input_capture_struct.input_capture_sel = TIM_INPUT_CAPTURE_SEL_INDIRECTTI;
	    std_tim_input_capture_init(TIM3, &input_capture_struct, TIM_CHANNEL_2);		
		}

    /* 使能输入捕获 */
    // std_tim_ccx_channel_enable(TIM3, TIM_CHANNEL_1);
    // std_tim_ccx_channel_enable(TIM3, TIM_CHANNEL_2);   

    /* NVIC初始化 */ 
    NVIC_SetPriority(TIM3_IRQn, NVIC_PRIO_0);
    NVIC_EnableIRQ(TIM3_IRQn);
}

volatile uint32_t g_enter_cnt = 0U;

void Capture_switch(uint8_t channel)
{
    /* 断开TIM3输入捕获通道1和通道2中断 */
    std_tim_interrupt_disable(TIM3, TIM_INTERRUPT_CC1 | TIM_INTERRUPT_CC2);
    std_tim_clear_flag(TIM3, TIM_FLAG_CC1 | TIM_FLAG_CC2);
	
    /* 断开输入捕获 */
    std_tim_ccx_channel_disable(TIM3, TIM_CHANNEL_1);
    std_tim_ccx_channel_disable(TIM3, TIM_CHANNEL_2);

	/* 停止TIM3计数 */
    std_tim_disable(TIM3);	
	g_enter_cnt = 0;
    g_ch1_ccx_value = 0U;
    g_ch2_ccx_value = 0U;
	uc_sel_ch = channel;
	input_state = (channel == 2U) ? INPUT_STATE_CAPTURE_CH2 : INPUT_STATE_CAPTURE_CH1;

	std_tim_input_capture_init_t input_capture_struct = {0};
	if (channel==2U)	// 此时采样硬件ch2上的pwm信号
		{
		reset_pwm_capture_state(&pwm_in2);
	    /* 配置为复位模式，且触发源为TI2FP2，触发极性为上升沿触发 */
	    std_tim_slave_mode_config(TIM3, TIM_SLAVE_MODE_RESET);
	    std_tim_trig_source_config(TIM3, TIM_TRIG_SOURCE_TI2FP2);
	    std_tim_set_input_pol(TIM3, TIM_CHANNEL_2, TIM_INPUT_POL_RISING);
	    
	    /* 配置TI2FP2映射到IC2上，且上升沿有效 */
	    input_capture_struct.input_capture_pol = TIM_INPUT_POL_RISING;
	    input_capture_struct.input_capture_sel = TIM_INPUT_CAPTURE_SEL_DIRECTTI;
	    input_capture_struct.input_capture_prescaler = TIM_INPUT_CAPTURE_PSC_DIV1;
	    input_capture_struct.input_capture_filter = 0x00;
	    std_tim_input_capture_init(TIM3, &input_capture_struct, TIM_CHANNEL_2);
	    
	    /* 配置TI2FP2映射到IC1上，且下降沿有效 */
	    input_capture_struct.input_capture_pol = TIM_INPUT_POL_FALLING;
	    input_capture_struct.input_capture_sel = TIM_INPUT_CAPTURE_SEL_INDIRECTTI;
	    std_tim_input_capture_init(TIM3, &input_capture_struct, TIM_CHANNEL_1);
		}
	else if (channel==1U)	// 此时采样硬件ch1上的pwm信号
		{
		reset_pwm_capture_state(&pwm_in1);
	    /* 配置为复位模式，且触发源为TI1FP1，触发极性为上升沿触发 */
	    std_tim_slave_mode_config(TIM3, TIM_SLAVE_MODE_RESET);
	    std_tim_trig_source_config(TIM3, TIM_TRIG_SOURCE_TI1FP1);
	    std_tim_set_input_pol(TIM3, TIM_CHANNEL_1, TIM_INPUT_POL_RISING);
	    
	    /* 配置TI1FP1映射到IC1上，且上升沿有效 */
	    input_capture_struct.input_capture_pol = TIM_INPUT_POL_RISING;
	    input_capture_struct.input_capture_sel = TIM_INPUT_CAPTURE_SEL_DIRECTTI;
	    input_capture_struct.input_capture_prescaler = TIM_INPUT_CAPTURE_PSC_DIV1;
	    input_capture_struct.input_capture_filter = 0x00;
	    std_tim_input_capture_init(TIM3, &input_capture_struct, TIM_CHANNEL_1);
	    
	    /* 配置TI1FP1映射到IC2上，且下降沿有效 */
	    input_capture_struct.input_capture_pol = TIM_INPUT_POL_FALLING;
	    input_capture_struct.input_capture_sel = TIM_INPUT_CAPTURE_SEL_INDIRECTTI;
	    std_tim_input_capture_init(TIM3, &input_capture_struct, TIM_CHANNEL_2);		
		}

    /* 使能TIM3输入捕获通道1和通道2中断 */
    std_tim_interrupt_enable(TIM3, TIM_INTERRUPT_CC1 | TIM_INTERRUPT_CC2);
    
    /* 使能输入捕获 */
    std_tim_ccx_channel_enable(TIM3, TIM_CHANNEL_1);
    std_tim_ccx_channel_enable(TIM3, TIM_CHANNEL_2);

	/* 启动TIM3计数 */
    std_tim_enable(TIM3);	
}

/**
* @brief  TIM3启动计数，并使能捕获事件中断
* @retval 无
*/
void bsp_tim3_capture_start(void)
{
    /* 使能TIM3输入捕获通道1和通道2中断 */
    std_tim_interrupt_enable(TIM3, TIM_INTERRUPT_CC1 | TIM_INTERRUPT_CC2);
    
    /* 使能输入捕获 */
    std_tim_ccx_channel_enable(TIM3, TIM_CHANNEL_1);
    std_tim_ccx_channel_enable(TIM3, TIM_CHANNEL_2);

	/* 启动TIM3计数 */
    std_tim_enable(TIM3);	
}

void tim1_output_start(void)
{
	#if (DEBUG_PWM_OUTPUT == 1)
	std_tim_ccx_channel_enable(TIM1, TIM_CHANNEL_1);
	std_tim_ccx_channel_enable(TIM1, TIM_CHANNEL_2);
	#endif
	
	std_tim_ccx_channel_enable(TIM1, TIM_CHANNEL_3);
	std_tim_ccx_channel_enable(TIM1, TIM_CHANNEL_4);

	std_tim_moen_enable(TIM1);
	std_tim_enable(TIM1);
}

// 实际有信号时4个周期，仅仅需要4ms, 就切一轮；
static void input_timeout_process(void)
{
#if 0
	// 避免在无信号时，一直检测ch1_in, 不检测ch2_in, 如果没信号时，最短20ms才能切一次, 结果就是20ms, 60ms轮流;
	if (TimOut1mS[TTMR_CK_CH] < 20) return;
	TimOut1mS[TTMR_CK_CH] = 0;
#endif

	if (TimOut1mS[TTPWM_CH] < INPUT_TIMEOUT_1MS) return;
		TimOut1mS[TTPWM_CH] = 0;
		
	if (input_state == INPUT_STATE_CAPTURE_CH1)
		{
			pwm_in1.duty_value = read_pin_debounced(GPIOB, GPIO_PIN_0) ? PWM_SCALE : 0U;
			pwm_in1.updated = 1U;
			Capture_switch(2U);
		}
	else if (input_state == INPUT_STATE_CAPTURE_CH2)
		{
			pwm_in2.duty_value = read_pin_debounced(GPIOA, GPIO_PIN_7) ? PWM_SCALE : 0U;
			pwm_in2.updated = 1U;
			Capture_switch(1U);
		}
}

static void calculate_pwm_input(volatile pwm_input_channel_t *input)
{
	uint32_t period_average;
	uint32_t duty_average;

	period_average = input->period_sum >> BIT_SHIFT;
	duty_average = input->duty_sum >> BIT_SHIFT;

	if ((period_average >= INPUT_PERIOD_MIN) && (period_average <= INPUT_PERIOD_MAX))
		{
		input->duty_value = (duty_average * PWM_SCALE + period_average / 2U) / period_average;
		if (input->duty_value > PWM_SCALE)
			{
			input->duty_value = PWM_SCALE;
			}

		input->freq_value = (uint16_t)(std_rcc_get_pclkfreq() / period_average);
		if (input->duty_value == 0U)
			{
			input->duty_value = 1U;
			}
		}
	else
		{
		input->duty_value = 0U;
		}

	reset_pwm_capture_state(input);
	input->updated = 1U;
}

void input_service_process(void)
{
	// 计算、发布、切换和重启均由输入状态机完成，input_get()只负责读取结果。
	if (input_state == INPUT_STATE_CALCULATE_CH1)
		{
		calculate_pwm_input(&pwm_in1);
		Capture_switch(2U);
		}
	else if (input_state == INPUT_STATE_CALCULATE_CH2)
		{
		calculate_pwm_input(&pwm_in2);
		Capture_switch(1U);
		}
	else
		{
		input_timeout_process();
		}
}

uint8_t input_get(uint8_t channel, uint16_t *duty)
{
    volatile pwm_input_channel_t *input;

    if (duty == 0)
        {
        return 0U;
        }

    input = (channel == 1U) ? &pwm_in1 : &pwm_in2;
    if (!input->updated)
        {
        return 0U;
        }

    input->updated = 0U;
    *duty = input->duty_value;
    return 1U;
}

void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    if (std_tim_get_flag(TIM1, TIM_FLAG_UPDATE))
        {
        std_tim_clear_flag(TIM1, TIM_FLAG_UPDATE);
        pwm_update_isr();
        }
}

/*------------------------------------------variables-------------------------------------------*/


//uint32_t g_pwm_duty = 0x0U;
//uint32_t g_pwm_frequency = 0x0U;

/**
* @brief  TIM3中断处理程序
* @retval 无
*/
void TIM3_IRQHandler(void)
{
	if (input_state == INPUT_STATE_CAPTURE_CH2)
		{
		/* CC2上升沿捕获 */
		if (std_tim_get_flag(TIM3, TIM_FLAG_CC2))
			{
			/* 从模式控制器配置为复位模式，且TI1FP1信号用作触发输入信号，故首次捕获的CC值忽略 */
			if (g_enter_cnt == 0)
				{
				g_enter_cnt = 1;
				std_tim_get_ccx_value(TIM3, TIM_CHANNEL_2);
				std_tim_clear_flag(TIM3, TIM_FLAG_CC2);
				}

			/* 获取第二次捕获的CC值 */
			else if (g_enter_cnt == 1)
				{
				/* 关闭CC1中断 */
				// std_tim_interrupt_disable(TIM3, TIM_INTERRUPT_CC1);
				g_ch2_ccx_value = std_tim_get_ccx_value(TIM3, TIM_CHANNEL_2);
				std_tim_clear_flag(TIM3, TIM_FLAG_CC2);
				
				pwm_in2.period_sum += g_ch2_ccx_value;
				pwm_in2.duty_sum += g_ch1_ccx_value;
				pwm_in2.cap_cnt++;
				if (pwm_in2.cap_cnt >= DET_CNT)
					{
					// 冻结本批4个周期，主循环计算完成后再切换到另一输入通道。
					input_state = INPUT_STATE_CALCULATE_CH2;
					// std_tim_interrupt_disable(TIM3, TIM_INTERRUPT_CC1 | TIM_INTERRUPT_CC2);
					}
				
				// 每个周期，都要重置
				TimOut1mS[TTPWM_CH] = 0;
				}
			}

		/* CC1下降沿捕获 */
		if (std_tim_get_flag(TIM3, TIM_FLAG_CC1))
			{
			/* 关闭CC2中断 */
			// std_tim_interrupt_disable(TIM3, TIM_INTERRUPT_CC2);
			g_ch1_ccx_value = std_tim_get_ccx_value(TIM3, TIM_CHANNEL_1);    
			std_tim_clear_flag(TIM3, TIM_FLAG_CC1);
			}
		}
	else if (input_state == INPUT_STATE_CAPTURE_CH1)
		{
		/* CC1上升沿捕获 */
		if (std_tim_get_flag(TIM3, TIM_FLAG_CC1))
			{
			/* 从模式控制器配置为复位模式，且TI1FP1信号用作触发输入信号，故首次捕获的CC值忽略 */
			if (g_enter_cnt == 0)
				{
				g_enter_cnt = 1;
				std_tim_get_ccx_value(TIM3, TIM_CHANNEL_1);
				std_tim_clear_flag(TIM3, TIM_FLAG_CC1);
				}

			/* 获取第二次捕获的CC值 */
			else if (g_enter_cnt == 1)
				{
				/* 关闭CC1中断 */
				// std_tim_interrupt_disable(TIM3, TIM_INTERRUPT_CC1);
				g_ch1_ccx_value = std_tim_get_ccx_value(TIM3, TIM_CHANNEL_1);
				std_tim_clear_flag(TIM3, TIM_FLAG_CC1);

				pwm_in1.period_sum += g_ch1_ccx_value;
				pwm_in1.duty_sum += g_ch2_ccx_value;
				pwm_in1.cap_cnt++;

				if (pwm_in1.cap_cnt >= DET_CNT)
					{
					// 冻结本批4个周期，主循环计算完成后再切换到另一输入通道。
					input_state = INPUT_STATE_CALCULATE_CH1;
					// std_tim_interrupt_disable(TIM3, TIM_INTERRUPT_CC1 | TIM_INTERRUPT_CC2);
					}
				
				// 每个周期，都要重置
				TimOut1mS[TTPWM_CH] = 0;	
				}
			}

		/* CC2下降沿捕获 */
		if (std_tim_get_flag(TIM3, TIM_FLAG_CC2))
			{
			/* 关闭CC2中断 */
			// std_tim_interrupt_disable(TIM3, TIM_INTERRUPT_CC2);
			g_ch2_ccx_value = std_tim_get_ccx_value(TIM3, TIM_CHANNEL_2);  
			std_tim_clear_flag(TIM3, TIM_FLAG_CC2);
			} 		
		}
		else
		{
		if (std_tim_get_flag(TIM3, TIM_FLAG_CC2))
			{
			/* 关闭CC2中断 */
			// std_tim_interrupt_disable(TIM3, TIM_INTERRUPT_CC2);
			g_ch2_ccx_value = std_tim_get_ccx_value(TIM3, TIM_CHANNEL_2);  
			std_tim_clear_flag(TIM3, TIM_FLAG_CC2);
			} 

			if (std_tim_get_flag(TIM3, TIM_FLAG_CC1))
			{
			/* 关闭CC2中断 */
			// std_tim_interrupt_disable(TIM3, TIM_INTERRUPT_CC2);
			g_ch1_ccx_value = std_tim_get_ccx_value(TIM3, TIM_CHANNEL_1);    
			std_tim_clear_flag(TIM3, TIM_FLAG_CC1);
			}
		}
}

