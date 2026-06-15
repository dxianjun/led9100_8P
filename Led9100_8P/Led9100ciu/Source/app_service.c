#include <stdio.h>

#include "app_service.h"
#include "app_tcp.h"



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
//uint16_t tim1_period_ticks = ARR_2K;

static uint8_t printf_input_en = 0;
static uint8_t printf_output_en = 0;
uint8_t uc_modecfg = MODE_DEFAULT;

#if (FIX_16K==1)
static const brightness_curve_anchor_t brightness_curve_anchors[] =
{
    {  12U,   3U, 3000U }, // 0.10% -> 1.00us @ 1.00kHz
    {  24U,   6U, 3000U }, // 0.20% -> 2.00us @ 1.00kHz
    {  36U,   9U, 3000U }, // 0.30% -> 3.00us @ 1.00kHz
    {  60U,  15U, 3000U }, // 0.50% -> 5.00us @ 1.00kHz
    { 120U,  30U, 3000U }, // 1.00% -> 10.0us @ 1.00kHz
    { 240U,  60U, 3000U }, // 2.00% -> 20.0us @ 1.00kHz
    { 300U,  75U, 3000U }, // 2.50% -> 25.0us @ 1.00kHz
    { 360U,  90U, 3000U }, // 3.00% -> 25.0us @ 1.20kHz
    { 600U,  150U, 3000U }, // 5.00% -> 25.0us @ 2.00kHz
    { 960U,  240U,  3000U }, // 8.00% -> 25.0us @ 3.20kHz
    {1200U,  300U,  3000U }, // 10.0% -> 25.0us @ 4.00kHz
    {1440U,  360U,  3000U }, // 12.0% -> 25.0us @ 4.80kHz
    {1920U,  480U,  3000U }, // 16.0% -> 25.0us @ 6.40kHz
    {2400U,  600U,  3000U }, // 20.0% -> 25.0us @ 8.00kHz
    {2765U,  692U,  3000U }, // 23.04% -> 28.8us @ 8.00kHz
    {11328U, 2832U, 3000U }, // 94.40% -> 118us @ 8.00kHz
    {12000U, 2980U, 3000U }, // 100.0% -> max output
};
#else
static const brightness_curve_anchor_t brightness_curve_anchors[] =
{
    {  12U,   24U, 24000U }, // 0.10% -> 1.00us @ 1.00kHz
    {  24U,   48U, 24000U }, // 0.20% -> 2.00us @ 1.00kHz
    {  36U,   72U, 24000U }, // 0.30% -> 3.00us @ 1.00kHz
    {  60U,  120U, 24000U }, // 0.50% -> 5.00us @ 1.00kHz
    { 120U,  240U, 24000U }, // 1.00% -> 10.0us @ 1.00kHz
    { 240U,  480U, 24000U }, // 2.00% -> 20.0us @ 1.00kHz
    { 300U,  600U, 24000U }, // 2.50% -> 25.0us @ 1.00kHz
    { 360U,  600U, 20000U }, // 3.00% -> 25.0us @ 1.20kHz
    { 600U,  600U, 12000U }, // 5.00% -> 25.0us @ 2.00kHz
    { 960U,  600U,  7500U }, // 8.00% -> 25.0us @ 3.20kHz
    {1200U,  600U,  6000U }, // 10.0% -> 25.0us @ 4.00kHz
    {1440U,  600U,  5000U }, // 12.0% -> 25.0us @ 4.80kHz
    {1920U,  600U,  3750U }, // 16.0% -> 25.0us @ 6.40kHz
    {2400U,  600U,  3000U }, // 20.0% -> 25.0us @ 8.00kHz
    {2765U,  692U,  3000U }, // 23.04% -> 28.8us @ 8.00kHz
    {11328U, 2832U, 3000U }, // 94.40% -> 118us @ 8.00kHz
    {12000U, 2980U, 3000U }, // 100.0% -> max output
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
    uint32_t y;

    if (x1 <= x0)
        {
        return y0;
        }

    y = (uint32_t)(x - x0) * (uint32_t)(y1 - y0);
    y = y / (uint32_t)(x1 - x0) + y0;

    return (uint16_t)y;
}
#if 0
static uint16_t duty_to_pulse(uint16_t duty, uint16_t period)
{
    uint32_t pulse;

    pulse = ((uint32_t)duty * period + (PWM_SCALE / 2U)) / PWM_SCALE;

    if ((duty > 0U) && (pulse < MIN_PULSE))
        {
        pulse = MIN_PULSE;
        }

    return limit_u16(pulse, period);
}
#endif

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
    TimOut10mS[TTPWM_CH1] = 0U;
    TimOut10mS[TTPWM_CH2] = 0U;
}

// 从中断里边移出来的参数
volatile uint32_t brightness=0;

uint16_t pulse_total_ticks=0;
uint16_t period_ticks=0;
uint16_t pulse1_ticks=0;
uint16_t pulse2_ticks=0;
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

uint16_t abs_delt(uint16_t a, uint16_t b)
{
	if (a>=b) 
		return a-b;
	else
		return b-a;
}

uint16_t level1_bk=0, pulse1_ticks_bk=0; 
uint16_t level2_bk=0, pulse2_ticks_bk=0; 

void compute_output_pulses(uint16_t level1, uint16_t level2, uint16_t total_pulse_ticks, uint16_t period_ticks, uint16_t *pulse1_ticks, uint16_t *pulse2_ticks)
{
	uint32_t pulse1_u32 = 0;
	uint32_t pulse2_u32 = 0;
	uint32_t total_pulse_u32 = 0;
	uint32_t level_sum_u32 = 0;

	switch (uc_modecfg)
		{
		case MODE_CCT:
			total_pulse_u32 = total_pulse_ticks;
			
			pulse1_u32 = (total_pulse_u32 * level2 + PWM_SCALE / 2U) / PWM_SCALE;
			pulse2_u32 = total_pulse_u32 - pulse1_u32;
			break;

		case MODE_DIRECT:
		default:
			level_sum_u32 = (uint32_t)level1 + level2;
		
			if (level_sum_u32 != 0U)
				{
				total_pulse_u32 = total_pulse_ticks;
					
				pulse1_u32 = (total_pulse_u32 * level1 + level_sum_u32 / 2U) / level_sum_u32;
				pulse2_u32 = total_pulse_u32 - pulse1_u32;

				// 如果输入在减小，输出不应该变大； 如果输入在增加，输出允许减小，此时应该是色温再变化
				if ((level1 <= level1_bk) && (pulse1_u32 >= pulse1_ticks_bk))	// 减小
					{
					pulse1_u32 = pulse1_ticks_bk;
					}
				else
					{
					level1_bk = level1;
					pulse1_ticks_bk = pulse1_u32;
					}

				if ((level2 <= level2_bk) && (pulse2_u32 >= pulse2_ticks_bk))	// 减小
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

	if (pulse1_u32 > period_ticks)
		{
		pulse1_u32 = period_ticks;
		}
	else if ((level1>0) && (pulse1_u32==0))
		{
		pulse1_u32 = 1;
		}

	if (pulse2_u32 > period_ticks)
		{
		pulse2_u32 = period_ticks;
		}
	else if ((level2>0) && (pulse2_u32==0))
		{
		pulse2_u32 = 1;
		}
		
	*pulse1_ticks = (uint16_t)pulse1_u32;
	*pulse2_ticks = (uint16_t)pulse2_u32;
}

#if WAKE_UP_CHX
uint8_t uc_ch1_en=0, uc_ch2_en=0;	// 这2个标记为是否将io口映射到pwm 输出；
void chx_add_wakeUp(void)
{
	printf("wake up\n");
	// 输出50us高电平

	std_tim_set_ccx_value(TIM1, TIM_CHANNEL_3, 2470);
	
	// 无法实现同时高，除非占空大于50%
	// TIM1->CCR2 = (uint16_t)(ui_duty_out_max - 1238);	// (uint16_t)(period_ticks - (DUTY_DELT + pulse_ticks));
}
#endif

void app_task(void)
{
    uint16_t duty;

	if (f_cal_ok)
		{
		// 根据总亮度获取输出频率和总脉宽
		brightness_to_output(brightness, &pulse_total_ticks, &period_ticks);

		#if 1
		if ((brightness <= 240UL) && (abs_delt(period_ticks, tim1_period_ticks) < 160U))
			{
			// Keep low-brightness steady states from chattering due to tiny sampled duty changes.
			period_ticks = tim1_period_ticks;
			}
		else
			{
			tim1_period_ticks = period_ticks;
			}
		#endif
		
		// 根据周期和总脉宽，以及插值后的pwm值，计算实际应该输出的脉冲
		compute_output_pulses(pwm_duty, pwm_duty2, pulse_total_ticks, period_ticks, &pulse1_ticks, &pulse2_ticks);
		// 根据输出周期和脉冲的理论值，调整死区去头等，写入寄存器
		tim1_apply_output(period_ticks, pulse1_ticks, pulse2_ticks);
		test_pwm_apply();

		if (printf_output_en && (TimOut10mS[TTMR_DLY] >= 100U))
	        {
	        TimOut10mS[TTMR_DLY] = 0U;
			
	        printf("cur1=%u cur2=%u period=%u pulse1=%u pulse2=%u\r\n",pwm_duty,pwm_duty2,period_ticks,pulse1_ticks,pulse2_ticks);
	        }
		f_cal_ok = 2;
		}

			
	input_timeout_check();

    if (input_get(1U, &duty))
        {		
        pwm_duty_tag = limit_u16(duty, PWM_SCALE);
        if (printf_input_en)
            {
            printf("IN1 duty=%u\r\n", pwm_duty_tag/120);
            }

		#if WAKE_UP_CHX
		if (pwm_duty_tag > 0)
			{
			if (uc_ch1_en==0)
				{
				//	实际是uc_ch1_en 和 uc_ch2_en 同时为0时才执行
				if (uc_ch2_en==0)
					{
					chx_add_wakeUp();
					}
				// 放在后边逻辑更清晰
				uc_ch1_en = 1;
				}
			}
		else
			{
			uc_ch1_en = 0;
			}
		#endif
		
		uc_sel_ch = 2U;
		Capture_switch(uc_sel_ch);
        }

    if (input_get(2U, &duty))
        {
        pwm_duty2_tag = limit_u16(duty, PWM_SCALE);
        if (printf_input_en)
            {
            printf("IN2 duty=%u\r\n", pwm_duty2_tag/120);
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
		else
			{
			uc_ch2_en = 0;
			}
		#endif
		
		uc_sel_ch = 1U;
		Capture_switch(uc_sel_ch);
        }
}


#define CONST_INPUT_MAX		11693UL		// 12000-307=11693, 防止把死区给推没了，造成重叠；
void pwm_update_isr(void)
{
//    uint16_t out1;
//    uint16_t out2;
//    uint16_t period;

	if (f_cal_ok == 2)	
		{
		tim1_apply_output(period_ticks, pulse1_ticks, pulse2_ticks);
		test_pwm_apply();
		f_cal_ok = 0;
		}
	
	if ((pwm_duty == pwm_duty_tag) && (pwm_duty2 == pwm_duty2_tag))
		{
		return;
		}
	
    if (pwm_duty < pwm_duty_tag)
        {
        pwm_duty++;
        }
    else if (pwm_duty > pwm_duty_tag)
        {
        pwm_duty--;
        }

    if (pwm_duty2 < pwm_duty2_tag)
        {
        pwm_duty2++;
        }
    else if (pwm_duty2 > pwm_duty2_tag)
        {
        pwm_duty2--;
        }

	if (uc_modecfg == MODE_CCT)
		{
		brightness = pwm_duty;
		}
	else
		{
		brightness = (uint32_t)pwm_duty + pwm_duty2;

		// davidd add 20260415
		// brightness 必须小于 ui_duty_in_max=12000UL, 输入的pwm周期不允许超过97.5%，因为要考虑死区的间隔，超过了，就是重叠
		if (brightness >= CONST_INPUT_MAX)		// 12000-307=11693
			{
			float f_delt=0.00L;

			f_delt = (float)1.00L * CONST_INPUT_MAX/brightness;
			brightness = CONST_INPUT_MAX;
			pwm_duty = (uint16_t)(f_delt * pwm_duty);
			pwm_duty2 = (uint16_t)(f_delt * pwm_duty2);
			}
		}
	// 计算搬到主循环里
	f_cal_ok = 1;
	
    //pwm_output_apply(out1, out2, period);
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
    if ((mode == MODE_CCT) || (mode == MODE_DIRECT))
        {
        uc_modecfg = mode;
        }
}

void app_set_manual_level(uint8_t ch,uint8_t level)
{
    if (level > 100U)
        {
        level = 100U;
        }

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
}
void user_serv(void)
{
    WDG_ReloadCounter;
    app_task();

    WDG_ReloadCounter;
    tcp_hand();
}
