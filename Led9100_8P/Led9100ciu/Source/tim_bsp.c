/************************************************************************************************/
/**
* @file               tim_bsp.c
* @brief              CIU32F003 timer and PWM BSP for LED9100 8P.
************************************************************************************************/

#include "main.h"

#define TIM3_ARR_VALUE          (0xFFFFU)

#define INPUT_PERIOD_MIN        (2000U)     /* 24 kHz at 48 MHz */
#define INPUT_PERIOD_MAX        (60000U)    /* 800 Hz at 48 MHz */
#define INPUT_TIMEOUT_10MS      (6U)

#define DET_CNT                 (8U)
#define BIT_SHIFT               (3U)

typedef struct
{
	uint32_t period_sum;
	uint32_t duty_sum;

    uint16_t freq_value;
    uint16_t duty_value;

	uint8_t cap_cnt;
    uint8_t updated;
} pwm_input_channel_t;

static volatile pwm_input_channel_t pwm_in1 = {0};
static volatile pwm_input_channel_t pwm_in2 = {0};


// 如果需要限制最大输出不能是高电平，将DUTY_MAX_ADJ设置为1；如果要求最大是高电平，将DUTY_MAX_ADJ设置为0
#define DUTY_MAX_NO_ADJ			1	

static uint16_t pulse_to_ccr1(uint16_t pulse_ticks, uint16_t period_ticks)
{
	if (pulse_ticks == 0U)
		{
		return 0U;
		}

	if (pulse_ticks > period_ticks)
		{
		pulse_ticks = period_ticks;
		}

	if (pulse_ticks > MIN_PULSE)
		{
		#if DUTY_MAX_NO_ADJ
		if ((uint32_t)DUTY_DELT + pulse_ticks >= period_ticks)
			{
			return period_ticks;
			}
		#else
		if ((uint32_t)DUTY_DELT + pulse_ticks >= (period_ticks - 1U))
			{
			return (uint16_t)(period_ticks - 1U);
			}
		#endif

		return (uint16_t)(DUTY_DELT + pulse_ticks);
		}

	return (uint16_t)(DUTY_DELT + MIN_PULSE);
}

// pwm2_n: CNT > CCRx 时输出为高电平。 因此，要用(period - pulse - DUTY_DELT)
static uint16_t pulse_to_ccr2n(uint16_t pulse_ticks, uint16_t period_ticks)
{
	if (pulse_ticks == 0U)
		{
		return period_ticks;
		}

	if (pulse_ticks > period_ticks)
		{
		pulse_ticks = period_ticks;
		}

	if (pulse_ticks > MIN_PULSE)
		{
		#if DUTY_MAX_NO_ADJ
		if ((uint32_t)DUTY_DELT + pulse_ticks >= period_ticks)
			{
			return 0U;
			}
		#else
		if ((uint32_t)DUTY_DELT + pulse_ticks >= (period_ticks - 1U))
			{
			return 1U;
			}
		#endif

		return (uint16_t)(period_ticks - (DUTY_DELT + pulse_ticks));
		}


	return (uint16_t)(period_ticks - (DUTY_DELT + MIN_PULSE));
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
	uint16_t ccr1;
	uint16_t ccr2;

	period_ticks = clamp_output_period(period_ticks);

	if (pulse1_ticks > period_ticks)
		{
		pulse1_ticks = period_ticks;
		}

	if (pulse2_ticks > period_ticks)
		{
		pulse2_ticks = period_ticks;
		}

	ccr1 = pulse_to_ccr1(pulse1_ticks, period_ticks);
	ccr2 = pulse_to_ccr2n(pulse2_ticks, period_ticks);

	std_tim_set_autoreload(TIM1, (uint16_t)(period_ticks - 1U));
	std_tim_set_ccx_value(TIM1, TIM_CHANNEL_1, ccr1);
	std_tim_set_ccx_value(TIM1, TIM_CHANNEL_2, ccr2);
}


void tim3_gpio_init(void)
{
    std_gpio_init_t gpio_init = {0};

    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOA);
    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOB);

    gpio_init.pin = GPIO_PIN_0;
    gpio_init.mode = GPIO_MODE_ALTERNATE;
    gpio_init.pull = GPIO_PULLDOWN;
    gpio_init.output_type = GPIO_OUTPUT_PUSHPULL;
    gpio_init.alternate = GPIO_AF3_TIM3;
    std_gpio_init(GPIOB, &gpio_init);

    gpio_init.pin = GPIO_PIN_7;
    gpio_init.alternate = GPIO_AF3_TIM3;
    std_gpio_init(GPIOA, &gpio_init);
}

void tim1_gpio_init(void)
{
    std_gpio_init_t gpio_init = {0};

	std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOA);
    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOB);

    gpio_init.pin = GPIO_PIN_2;
    gpio_init.mode = GPIO_MODE_ALTERNATE;
    gpio_init.pull = GPIO_NOPULL;
    gpio_init.output_type = GPIO_OUTPUT_PUSHPULL;
    gpio_init.alternate = GPIO_AF2_TIM1;
    std_gpio_init(GPIOB, &gpio_init);

    gpio_init.pin = GPIO_PIN_1;
    gpio_init.alternate = GPIO_AF3_TIM1;
    std_gpio_init(GPIOB, &gpio_init);

#if 1	// 测试用
	// tim1_ch4
    gpio_init.pin = GPIO_PIN_7;
    gpio_init.alternate = GPIO_AF4_TIM1;
    std_gpio_init(GPIOB, &gpio_init);

	// tim1_ch3
    gpio_init.pin = GPIO_PIN_1;
    gpio_init.alternate = GPIO_AF4_TIM1;
    std_gpio_init(GPIOA, &gpio_init);
#endif
}


/*
PWM1模式：CNT < CCRx 时输出为高电平。因此，在计数器的30个周期内输出高电平，剩余70个周期为低电平
PWM2模式：CNT > CCRx 时输出为高电平。因此，在计数器的70个周期内输出高电平，剩余30个周期为低电平
如果配置模式2，同时极性为NEG
PWM2模式：CNT > CCRx 时输出为低电平。
pwm2_n: CNT > CCRx 时输出为高电平。 因此，在计数器的70个周期内输出高电平，剩余30个周期为低电平
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
    basic_init.period = ARR_2K - 1U;
    basic_init.clock_div = TIM_CLOCK_DTS_DIV1;
    basic_init.repeat_counter = 0U;
    std_tim_init(TIM1, &basic_init);
		
	/* 配置通道1输出模式为PWM1模式 */
    oc_init.output_compare_mode = TIM_OUTPUT_MODE_PWM1;
    oc_init.output_state = TIM_OUTPUT_ENABLE;
    oc_init.output_negtive_state = TIM_OUTPUT_NEGTIVE_DISABLE;
    oc_init.pulse = 0U;
    oc_init.output_pol = TIM_OUTPUT_POL_HIGH;
    oc_init.output_negtive_pol = TIM_OUTPUT_NEGTIVE_POL_HIGH;
    oc_init.output_idle_state = TIM_OUTPUT_IDLE_RESET;
    oc_init.output_negtive_idle_state = TIM_OUTPUT_NEGTIVE_IDLE_RESET;
    std_tim_output_compare_init(TIM1, &oc_init, TIM_CHANNEL_1);
		
	/* 配置通道2输出模式为PWM2模式 */
	oc_init.output_compare_mode = TIM_OUTPUT_MODE_PWM2;
    oc_init.output_state = TIM_OUTPUT_DISABLE;
    oc_init.output_negtive_state = TIM_OUTPUT_NEGTIVE_ENABLE;
	oc_init.output_negtive_pol = TIM_OUTPUT_NEGTIVE_POL_LOW;
    oc_init.pulse = ARR_2K;
    std_tim_output_compare_init(TIM1, &oc_init, TIM_CHANNEL_2);

	#if 1
	//测试A1,B7 tim1_ch3, tim1_ch4, 输出pwm
	oc_init.output_compare_mode = TIM_OUTPUT_MODE_PWM1;
	oc_init.output_state = TIM_OUTPUT_ENABLE;
	oc_init.output_negtive_state = TIM_OUTPUT_NEGTIVE_DISABLE;
	oc_init.pulse = 1000U;
	oc_init.output_pol = TIM_OUTPUT_POL_HIGH;
	oc_init.output_negtive_pol = TIM_OUTPUT_NEGTIVE_POL_HIGH;
	oc_init.output_idle_state = TIM_OUTPUT_IDLE_RESET;
	oc_init.output_negtive_idle_state = TIM_OUTPUT_NEGTIVE_IDLE_RESET;
	
	std_tim_output_compare_init(TIM1, &oc_init, TIM_CHANNEL_3);
	std_tim_output_compare_init(TIM1, &oc_init, TIM_CHANNEL_4);
	
	#endif

	
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

uint8_t uc_sel_ch=0;		// 哪个都不选
/**
* @brief  TIM3初始化
* @retval 无
*/
void tim3_input_init(void)
{
    std_tim_basic_init_t basic_init_struct = {0};
    
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

void Capture_switch(uint8_t uc_sel_ch)
{
    /* 断开TIM3输入捕获通道1和通道2中断 */
    std_tim_interrupt_disable(TIM3, TIM_INTERRUPT_CC1 | TIM_INTERRUPT_CC2);
    std_tim_clear_flag(TIM3, TIM_FLAG_CC1 | TIM_INTERRUPT_CC2);
	
    /* 断开输入捕获 */
    std_tim_ccx_channel_disable(TIM3, TIM_CHANNEL_1);
    std_tim_ccx_channel_disable(TIM3, TIM_CHANNEL_2);

	/* 停止TIM3计数 */
    std_tim_disable(TIM3);	
	
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
    std_tim_ccx_channel_enable(TIM1, TIM_CHANNEL_1);
    std_tim_ccxn_channel_enable(TIM1, TIM_CHANNEL_2);
    std_tim_moen_enable(TIM1);
    std_tim_enable(TIM1);
}

void input_timeout_check(void)
{
	// 避免在无信号时，一直检测ch1_in, 不检测ch2_in, 如果没信号时，最短20ms才能切一次, 结果就是20ms, 60ms轮流;
	if (TimOut1mS[TTMR_CK_CH] < 20) return;
	TimOut1mS[TTMR_CK_CH] = 0;

	if (uc_sel_ch== 1U)
		{
		if (TimOut10mS[TTPWM_CH1] >= INPUT_TIMEOUT_10MS)
			{
			TimOut10mS[TTPWM_CH1] = 0; 
			
			pwm_in1.updated = 1U;
			pwm_in1.duty_value = std_gpio_get_input_pin(GPIOB, GPIO_PIN_0) ? PWM_SCALE : 0U;
			}
		}
	else if (uc_sel_ch== 2U)
		{
		if (TimOut10mS[TTPWM_CH2] >= INPUT_TIMEOUT_10MS)
			{
			TimOut10mS[TTPWM_CH2] = 0; 
			
			pwm_in2.updated = 1U;
			pwm_in2.duty_value = std_gpio_get_input_pin(GPIOA, GPIO_PIN_7) ? PWM_SCALE : 0U;
			}
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
uint32_t g_enter_cnt = 0U;

uint32_t g_ch1_ccx_value = 0U;
uint32_t g_ch2_ccx_value = 0U;

//uint32_t g_pwm_duty = 0x0U;
//uint32_t g_pwm_frequency = 0x0U;

/**
* @brief  TIM3中断处理程序
* @retval 无
*/
void TIM3_IRQHandler(void)
{
	if (uc_sel_ch)
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
					pwm_in2.cap_cnt=0;

					pwm_in2.period_sum = (pwm_in2.period_sum>>BIT_SHIFT);
					pwm_in2.duty_sum = (pwm_in2.duty_sum>>BIT_SHIFT);
					/* 计算占空比 */
					pwm_in2.duty_value = ((pwm_in2.duty_sum + 1) * 100) / (pwm_in2.period_sum + 1);

					/* 计算输入频率 */
					pwm_in2.freq_value = (std_rcc_get_pclkfreq() / (pwm_in2.period_sum + 1));

					// 计算完成后，把累加值清0
					pwm_in2.period_sum = 0; pwm_in2.duty_sum = 0;

					pwm_in2.updated = 1U; 	
					}
				
				// 每个周期，都要重置
				TimOut10mS[TTPWM_CH2] = 0;
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
	else
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
					pwm_in1.cap_cnt=0;

					pwm_in1.period_sum = (pwm_in1.period_sum>>BIT_SHIFT);
					pwm_in1.duty_sum = (pwm_in1.duty_sum>>BIT_SHIFT);
					/* 计算占空比 */
					pwm_in1.duty_value = ((pwm_in1.duty_sum + 1) * 100) / (pwm_in1.period_sum + 1);

					/* 计算输入频率 */
					pwm_in1.freq_value = (std_rcc_get_pclkfreq() / (pwm_in1.period_sum + 1));

					// 计算完成后，把累加值清0
					pwm_in1.period_sum = 0; pwm_in1.duty_sum = 0;

					pwm_in1.updated = 1U; 
					}
				
				// 每个周期，都要重置
				TimOut10mS[TTPWM_CH1] = 0;	
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
}

