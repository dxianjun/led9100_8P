#include <stdio.h>

#include "app_service.h"
#include "app_tcp.h"
#include "tim_bsp.h"


typedef struct
{
    uint16_t brightness;
    uint16_t pulse;
    uint16_t period;
} brightness_curve_anchor_t;

bit16_field_t g_app_flag;

static volatile uint16_t pwm_duty = 0;
static volatile uint16_t pwm_duty2 = 0;

static uint16_t pwm_duty_tag = 0;
static uint16_t pwm_duty2_tag = 0;


static uint8_t printf_input_en = 1;
static uint8_t printf_output_en = 0;
volatile uint8_t uc_modecfg = MODE_DEFAULT;

#if (FIX_16K==1)
static const brightness_curve_anchor_t brightness_curve_anchors[] =
{
    {  12U,   3U, 3000U }, // 0.10% @ 16.00kHz
    {  24U,   6U, 3000U }, // 0.20% @ 16.00kHz
    {  36U,   9U, 3000U }, // 0.30% @ 16.00kHz
    {  60U,  15U, 3000U }, // 0.50% @ 16.00kHz
    { 120U,  30U, 3000U }, // 1.00% @ 16.00kHz
    { 240U,  60U, 3000U }, // 2.00% @ 16.00kHz
    { 300U,  75U, 3000U }, // 2.50% @ 16.00kHz
    { 360U,  90U, 3000U }, // 3.00% @ 16.00kHz
    { 600U, 150U, 3000U }, // 5.00% @ 16.00kHz
    { 960U, 240U, 3000U }, // 8.00% @ 16.00kHz
    {1200U, 300U, 3000U }, // 10.0% @ 16.00kHz
    {1440U, 360U, 3000U }, // 12.0% @ 16.00kHz
    {1920U, 480U, 3000U }, // 16.0% @ 16.00kHz
    {2400U, 600U, 3000U }, // 20.0% @ 16.00kHz
    {2765U, 692U, 3000U }, // 23.04% @ 16.00kHz
    #if (DUTY_MAX_NO_ADJ == 1)
    {10800U, 2700U, 3000U }, // 逻辑脉宽达到90% @ 16.00kHz
    {11400U, 2850U, 3000U }, // 逻辑脉宽达到95% @ 16.00kHz
    {12000U, 3000U, 3000U }, // 逻辑脉宽达到100% @ 16.00kHz
    #elif (MAX_OUTPUT_PERCENT == 90U)
    {10800U, 2700U, 3000U }, // 逻辑脉宽达到90% @ 16.00kHz
    {12000U, 2700U, 3000U }, // 高亮端保持在90%平台 @ 16.00kHz
    #elif (MAX_OUTPUT_PERCENT == 95U)
    {10800U, 2700U, 3000U }, // 逻辑脉宽达到90% @ 16.00kHz
    {11400U, 2850U, 3000U }, // 逻辑脉宽达到95% @ 16.00kHz
    {12000U, 2850U, 3000U }, // 高亮端保持在95%平台 @ 16.00kHz
    #endif
};
#elif (FIX_2K==1)
static const brightness_curve_anchor_t brightness_curve_anchors[] =
{
    {  12U,    24U, 24000U }, // 0.10% @ 2.00kHz
    {  24U,    48U, 24000U }, // 0.20% @ 2.00kHz
    {  36U,    72U, 24000U }, // 0.30% @ 2.00kHz
    {  60U,   120U, 24000U }, // 0.50% @ 2.00kHz
    { 120U,   240U, 24000U }, // 1.00% @ 2.00kHz
    { 240U,   480U, 24000U }, // 2.00% @ 2.00kHz
    { 300U,   600U, 24000U }, // 2.50% @ 2.00kHz
    { 360U,   720U, 24000U }, // 3.00% @ 2.00kHz
    { 600U,  1200U, 24000U }, // 5.00% @ 2.00kHz
    { 960U,  1920U, 24000U }, // 8.00% @ 2.00kHz
    {1200U,  2400U, 24000U }, // 10.0% @ 2.00kHz
    {1440U,  2880U, 24000U }, // 12.0% @ 2.00kHz
    {1920U,  3840U, 24000U }, // 16.0% @ 2.00kHz
    {2400U,  4800U, 24000U }, // 20.0% @ 2.00kHz
    {2765U,  5536U, 24000U }, // 23.04% @ 2.00kHz
    #if (DUTY_MAX_NO_ADJ == 1)
    {10800U, 21600U, 24000U }, // 逻辑脉宽达到90% @ 2.00kHz
    {11400U, 22800U, 24000U }, // 逻辑脉宽达到95% @ 2.00kHz
    {12000U, 24000U, 24000U }, // 逻辑脉宽达到100% @ 2.00kHz
    #elif (MAX_OUTPUT_PERCENT == 90U)
    {10800U, 21600U, 24000U }, // 逻辑脉宽达到90% @ 2.00kHz
    {12000U, 21600U, 24000U }, // 高亮端保持在90%平台 @ 2.00kHz
    #elif (MAX_OUTPUT_PERCENT == 95U)
    {10800U, 21600U, 24000U }, // 逻辑脉宽达到90% @ 2.00kHz
    {11400U, 22800U, 24000U }, // 逻辑脉宽达到95% @ 2.00kHz
    {12000U, 22800U, 24000U }, // 高亮端保持在95%平台 @ 2.00kHz
    #endif
};

#else
static const brightness_curve_anchor_t brightness_curve_anchors[] =
{
    {  12U,   24U, 24000U }, // 0.10% -> 0.50us @ 2.00kHz
    {  24U,   48U, 24000U }, // 0.20% -> 1.00us @ 2.00kHz
    {  36U,   72U, 24000U }, // 0.30% -> 1.50us @ 2.00kHz
    {  60U,  120U, 24000U }, // 0.50% -> 2.50us @ 2.00kHz
    { 120U,  240U, 24000U }, // 1.00% -> 5.00us @ 2.00kHz
    { 240U,  480U, 24000U }, // 2.00% -> 10.0us @ 2.00kHz
    { 300U,  600U, 24000U }, // 2.50% -> 12.5us @ 2.00kHz
    { 360U,  600U, 20000U }, // 3.00% -> 12.5us @ 2.40kHz
    { 600U,  600U, 12000U }, // 5.00% -> 12.5us @ 4.00kHz
    { 960U,  600U,  7500U }, // 8.00% -> 12.5us @ 6.40kHz
    {1200U,  600U,  6000U }, // 10.0% -> 12.5us @ 8.00kHz
    {1440U,  600U,  5000U }, // 12.0% -> 12.5us @ 9.60kHz
    {1920U,  600U,  3750U }, // 16.0% -> 12.5us @ 12.80kHz
    {2400U,  600U,  3000U }, // 20.0% -> 12.5us @ 16.00kHz
    {2765U,  692U,  3000U }, // 23.04% -> 14.42us @ 16.00kHz
    #if (DUTY_MAX_NO_ADJ == 1)
		{10800U, 2700U, 3000U }, // 逻辑脉宽达到90% @ 16.00kHz
		{11400U, 2850U, 3000U }, // 逻辑脉宽达到95% @ 16.00kHz
		{12000U, 3000U, 3000U }, // 逻辑脉宽达到100% @ 16.00kHz
	#else
	    #if (MAX_OUTPUT_PERCENT == 90U)
	    {10800U, 2700U, 3000U }, // 逻辑脉宽达到90% @ 16.00kHz
	    {12000U, 2700U, 3000U }, // 高亮端保持在90%平台 @ 16.00kHz
	    #elif (MAX_OUTPUT_PERCENT == 95U)
	    {10800U, 2700U, 3000U }, // 逻辑脉宽达到90% @ 16.00kHz
	    {11400U, 2850U, 3000U }, // 逻辑脉宽达到95% @ 16.00kHz
	    {12000U, 2850U, 3000U }, // 高亮端保持在95%平台 @ 16.00kHz
	    #endif
	#endif
};

#endif
static uint16_t limit_u16(uint32_t value, uint16_t max_value)
{
    if (value > max_value)
        {
        value = max_value;
        }

    return (uint16_t)value;
}

static uint16_t interpolate(uint16_t x, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    int32_t delta;
    uint16_t offset;
    uint16_t range;

    if (x1 <= x0)
        {
        return y0;
        }

    offset = (uint16_t)(x - x0);
    range = (uint16_t)(x1 - x0);
    delta = (int32_t)y1 - (int32_t)y0;

    if (delta >= 0)
        {
        return (uint16_t)(y0 + (uint16_t)((((uint32_t)delta * offset) + range / 2U) / range));
        }

    return (uint16_t)(y0 - (uint16_t)((((uint32_t)(-delta) * offset) + range / 2U) / range));
}

static void brightness_to_output(uint16_t brightness, uint16_t *pulse, uint16_t *period)
{
    uint8_t i;
    uint8_t last;

    last = (uint8_t)((sizeof(brightness_curve_anchors) / sizeof(brightness_curve_anchors[0])) - 1U);

    if (brightness == 0U)
        {
        *pulse = 0U;
        *period = brightness_curve_anchors[0].period;
        return;
        }

    if (brightness <= brightness_curve_anchors[0].brightness)
        {
        *pulse = brightness_curve_anchors[0].pulse;
        *period = brightness_curve_anchors[0].period;
        return;
        }

    if (brightness >= brightness_curve_anchors[last].brightness)
        {
        *pulse = brightness_curve_anchors[last].pulse;
        *period = brightness_curve_anchors[last].period;
        return;
        }

    for (i = 1U; i <= last; i++)
        {
        if (brightness <= brightness_curve_anchors[i].brightness)
            {
            *pulse = interpolate(brightness,
                                 brightness_curve_anchors[i - 1U].brightness,
                                 brightness_curve_anchors[i - 1U].pulse,
                                 brightness_curve_anchors[i].brightness,
                                 brightness_curve_anchors[i].pulse);
            *period = interpolate(brightness,
                                  brightness_curve_anchors[i - 1U].brightness,
                                  brightness_curve_anchors[i - 1U].period,
                                  brightness_curve_anchors[i].brightness,
                                  brightness_curve_anchors[i].period);
            return;
            }
        }

    *pulse = brightness_curve_anchors[last].pulse;
    *period = brightness_curve_anchors[last].period;

	//*pulse = limit_u16(*pulse, *period);
	
}

void sys_flg_init(void)
{
    g_app_flag.val = 0U;
}

void app_init(void)
{
    sys_flg_init();
    pwm_duty = 0U;
    pwm_duty2 = 0U;
    pwm_duty_tag = 0U;
    pwm_duty2_tag = 0U;

    uc_modecfg = MODE_DEFAULT;
    TimOut1mS[TTPWM_CH] = 0U;
//    TimOut1mS[TTPWM_CH2] = 0U;
}

// 主循环计算完成后发布，TIM1更新中断在周期边界统一应用。
// 从中断里边移出来的参数
static volatile uint8_t uc_cal_step = 0;

static volatile uint16_t next_level1=0;
static volatile uint16_t next_level2=0;
static volatile uint32_t brightness=0;

static volatile uint16_t period_ticks=0;
static volatile uint16_t pulse1_ticks=0;
static volatile uint16_t pulse2_ticks=0;

#if (DEBUG_PWM_OUTPUT == 1)
static uint8_t test_level_ch1 = 0U;
static uint8_t test_level_ch2 = 0U;

static void test_pwm_apply(void)
{
    uint16_t ccr;

    ccr = (uint16_t)((uint32_t)tim1_period_ticks * test_level_ch1 / 100U);
    std_tim_set_ccx_value(TIM1, TIM_CHANNEL_1, ccr);

    ccr = (uint16_t)((uint32_t)tim1_period_ticks * test_level_ch2 / 100U);
    std_tim_set_ccx_value(TIM1, TIM_CHANNEL_2, ccr);
}
#endif

uint16_t abs_delt(uint16_t a, uint16_t b)
{
	if (a>=b) 
		return a-b;
	else
		return b-a;
}

uint16_t level1_bk=0, pulse1_ticks_bk=0; 
uint16_t level2_bk=0, pulse2_ticks_bk=0; 

static void compute_output_pulses(uint8_t mode, uint16_t level1, uint16_t level2,
	uint16_t total_pulse_ticks, uint16_t output_period_ticks,
	uint16_t *output_pulse1_ticks, uint16_t *output_pulse2_ticks)
{
	uint32_t pulse1_u32 = 0;
	uint32_t pulse2_u32 = 0;
	uint32_t total_pulse_u32 = 0;
	uint32_t level_sum_u32 = 0;
	uint8_t pulse1_requested = 0U;
	uint8_t pulse2_requested = 0U;

	switch (mode)
		{
		case MODE_CCT:
			pulse1_requested = ((level1 > 0U) && (level2 > 0U));
			pulse2_requested = ((level1 > 0U) && (level2 < PWM_SCALE));
			total_pulse_u32 = total_pulse_ticks;
			
			pulse1_u32 = (total_pulse_u32 * level2 + PWM_SCALE / 2U) / PWM_SCALE;
			pulse2_u32 = total_pulse_u32 - pulse1_u32;
			break;

		case MODE_DIRECT:
		default:
			pulse1_requested = (level1 > 0U);
			pulse2_requested = (level2 > 0U);
			level_sum_u32 = (uint32_t)level1 + level2;
		
			if (level_sum_u32 != 0U)
				{
				total_pulse_u32 = total_pulse_ticks;
					
				pulse1_u32 = (total_pulse_u32 * level1 + level_sum_u32 / 2U) / level_sum_u32;
				pulse2_u32 = total_pulse_u32 - pulse1_u32;

				// 输入下降时，禁止对应通道的输出脉宽反向增大；输入上升时允许因配比变化而减小。
				if ((level1 <= level1_bk) && (pulse1_u32 >= pulse1_ticks_bk))	// 通道1输入下降
					{
					pulse1_u32 = pulse1_ticks_bk;
					}
				else
					{
					level1_bk = level1;
					pulse1_ticks_bk = pulse1_u32;
					}

				if ((level2 <= level2_bk) && (pulse2_u32 >= pulse2_ticks_bk))	// 通道2输入下降
					{
					pulse2_u32 = pulse2_ticks_bk;
					}
				else
					{
					level2_bk = level2;
					pulse2_ticks_bk = pulse2_u32;
					}
				}
			break;
		}

	if (pulse1_u32 > output_period_ticks)
		{
		pulse1_u32 = output_period_ticks;
		}
	else if (pulse1_requested && (pulse1_u32 == 0U))
		{
		pulse1_u32 = 1;
		}

	if (pulse2_u32 > output_period_ticks)
		{
		pulse2_u32 = output_period_ticks;
		}
	else if (pulse2_requested && (pulse2_u32 == 0U))
		{
		pulse2_u32 = 1;
		}
		
	*output_pulse1_ticks = (uint16_t)pulse1_u32;
	*output_pulse2_ticks = (uint16_t)pulse2_u32;
}

static void limit_levels_for_deadtime(uint8_t mode, uint16_t level1, uint16_t level2,
	uint16_t *limited_level1, uint16_t *limited_level2, uint32_t *limited_brightness)
{
	uint32_t level_sum;

	*limited_level1 = level1;
	*limited_level2 = level2;

	if (mode == MODE_CCT)
		{
		if (*limited_level1 > CONST_INPUT_MAX)
			{
			*limited_level1 = CONST_INPUT_MAX;
			}
		*limited_brightness = *limited_level1;
		return;
		}

	level_sum = (uint32_t)level1 + level2;
	*limited_brightness = level_sum;

	float f_delt = 0.00L;
	if (level_sum >= CONST_INPUT_MAX)
		{
		f_delt = (float)1.00L * CONST_INPUT_MAX/level_sum;
		*limited_brightness = CONST_INPUT_MAX;
		*limited_level1 = (uint16_t)(f_delt * level1);
		*limited_level2 = (uint16_t)(f_delt * level2);
		}
}

#if 0
// 调用者必须位于TIM1中断中，或已经通过PRIMASK进入临界区。
static void queue_output_calculation(uint8_t mode, uint16_t level1, uint16_t level2)
{
	// 模式切换可以覆盖尚未应用的旧结果；普通调变请求仅在流水线空闲时进入。
	output_ready = 0U;
	calc_mode = mode;
	calc_level1 = level1;
	calc_level2 = level2;
	calc_generation++;
	calc_pending = 1U;
}
#endif

#if WAKE_UP_CHX
uint8_t uc_ch1_en=0, uc_ch2_en=0;	// 标记两路输入是否已经触发PWM输出唤醒。
void chx_add_wakeUp(void)
{
	printf("wake up\n");
	// 在正式输出CH3上产生约50us的唤醒高脉冲。
	// 2470 ticks包含死区补偿，对应约50us的实际高电平。
	std_tim_set_ccx_value(TIM1, TIM_CHANNEL_3, 2470);
//	std_tim_set_ccx_value(TIM1, TIM_CHANNEL_4, 24000-2470);
}
#endif



uint16_t limited_level1=0;
uint16_t limited_level2=0;
uint32_t local_brightness=0;
uint16_t local_total_pulse=0;
uint16_t local_period=0;

uint16_t local_pulse1=0;
uint16_t local_pulse2=0;
uint16_t request_level1, request_level2;

void app_task(void)
{
    uint16_t duty;

	if (uc_cal_step == 1)
		{
		// 固化ISR中的变量，防止发生中断后，数据被改到
		request_level1 = next_level1;
		request_level2 = next_level2;
		
		limit_levels_for_deadtime(uc_modecfg, next_level1, next_level2,
			&limited_level1, &limited_level2, &local_brightness);
		brightness_to_output((uint16_t)local_brightness, &local_total_pulse, &local_period);

		if ((local_brightness <= 240UL) && (abs_delt(local_period, tim1_period_ticks) < 160U))
			{
			local_period = tim1_period_ticks;
			}
		
		// 根据周期和总脉宽，以及插值后的pwm值，计算实际应该输出的脉冲
		compute_output_pulses(uc_modecfg, limited_level1, limited_level2,
			local_total_pulse, local_period, &local_pulse1, &local_pulse2);

		// 设置中断中用到的全局变量
		period_ticks = local_period; pulse1_ticks = local_pulse1; pulse2_ticks = local_pulse2;
		
		if (printf_output_en && (TimOut10mS[TTMR_DLY] >= 100U))
	        {
	        TimOut10mS[TTMR_DLY] = 0U;
	        printf("cur1=%u cur2=%u period=%u pulse1=%u pulse2=%u\r\n",
				request_level1, request_level2, local_period, local_pulse1, local_pulse2);
	        }
		
		uc_cal_step = 2;
		}

	// 输入状态机完成当前通道计算、结果发布、另一通道切换及TIM3重启。
	input_service_process();

    if (input_get(1U, &duty))
        {		
        pwm_duty_tag = limit_u16(duty, PWM_SCALE);
        if (printf_input_en)
			{
			static uint16_t ch1_bk=0;
			uint16_t ch1_duty=pwm_duty_tag/12;
			if (ch1_duty != ch1_bk) 
				{
				ch1_bk = ch1_duty;
				
				printf("IN1 duty=%.01f%%\r\n", (float)ch1_duty/10);
				}
			}

		#if WAKE_UP_CHX
		if (pwm_duty_tag > 0)
			{
			if (uc_ch1_en==0)
				{
				// 仅当两路都尚未唤醒时，才需要发送一次公共唤醒脉冲。
				if (uc_ch2_en==0)
					{
					chx_add_wakeUp();
					}
				// 唤醒脉冲发送完成后，再更新通道状态。
				uc_ch1_en = 1;
				}
			}
		else if (uc_ch1_en)
			{
			uc_ch1_en = 0;
			printf("ch1 off\n");
			}
		#endif
		
        }

    if (input_get(2U, &duty))
        {
        pwm_duty2_tag = limit_u16(duty, PWM_SCALE);
        if (printf_input_en)
			{
			static uint16_t ch2_bk=0;
			uint16_t ch2_duty=pwm_duty2_tag/12;
			if (ch2_duty != ch2_bk) 
				{
				ch2_bk = ch2_duty;
				
				printf("IN2 duty=%.01f%%\r\n", (float)ch2_duty/10);
				}
			}
		
		#if WAKE_UP_CHX
		if (pwm_duty2_tag > 0)
			{
			if (uc_ch2_en==0)
				{
				if (uc_ch1_en==0)
					{
					chx_add_wakeUp();
					}
				
				uc_ch2_en = 1;
				}
			}
		else if (uc_ch2_en)
			{
			uc_ch2_en = 0;
			printf("ch2 off\n");
			}
		#endif
		
        }
}

static uint16_t step_level_toward_target(uint16_t current_level, uint16_t target_level)
{
	if (current_level < target_level)
		{
		current_level++;
		}
	else if (current_level > target_level)
		{
		current_level--;
		}

	return current_level;
}



void pwm_update_isr(void)
{
	if (uc_cal_step == 2)
		{
		tim1_apply_output(period_ticks, pulse1_ticks, pulse2_ticks);
		
		#if (DEBUG_PWM_OUTPUT == 1)
		test_pwm_apply();
		#endif
		uc_cal_step = 0;		
		}

	if (uc_cal_step == 1) return;

	if ((pwm_duty == pwm_duty_tag) && (pwm_duty2 == pwm_duty2_tag))
		{
		return;
		}
	
	#if 1
	next_level1 = step_level_toward_target(pwm_duty, pwm_duty_tag);
	next_level2 = step_level_toward_target(pwm_duty2, pwm_duty2_tag);

	pwm_duty = next_level1;
	pwm_duty2 = next_level2;

	#else
	pwm_duty = pwm_duty_tag; pwm_duty2 = pwm_duty2_tag;
	#endif
	// 计算搬到主循环里
	uc_cal_step = 1;
	// queue_output_calculation(uc_modecfg, next_level1, next_level2);
}

void app_set_input_print(uint8_t enable)
{
    printf_input_en = enable ? 1U : 0U;
}

void app_set_output_print(uint8_t enable)
{
    printf_output_en = enable ? 1U : 0U;
}

void app_set_mode(uint8_t mode)
{

}

void app_set_manual_level(uint8_t ch,uint8_t level)
{
    if (level > 100U)
        {
        level = 100U;
        }
	
	#if (DEBUG_PWM_OUTPUT == 1)
    printf("tim1 period %d\n", tim1_period_ticks);

    if (ch==1)
        {
        test_level_ch1 = level;
        }
    else if (ch==2)
        {
        test_level_ch2 = level;
        }

    test_pwm_apply();
	#endif
}
void user_serv(void)
{
    WDG_ReloadCounter;
    app_task();

    WDG_ReloadCounter;
    tcp_hand();
}
