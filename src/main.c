/*********************************************************************************
 * The MIT License (MIT)                                                         *
 * <p/>                                                                          *
 * Copyright (c) 2016 Bertrand Martel                                            *
 * <p/>                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a copy  *
 * of this software and associated documentation files (the "Software"), to deal *
 * in the Software without restriction, including without limitation the rights  *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell     *
 * copies of the Software, and to permit persons to whom the Software is         *
 * furnished to do so, subject to the following conditions:                      *
 * <p/>                                                                          *
 * The above copyright notice and this permission notice shall be included in    *
 * all copies or substantial portions of the Software.                           *
 * <p/>                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR    *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,      *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE   *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER        *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN     *
 * THE SOFTWARE.                                                                 *
 *********************************************************************************/

/*******************************************************************************/
/**********************NORDIC SEMICONDUCTOR NOTICE******************************/
/*******************************************************************************
 * Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.               *
 *                                                                             *
 * The information contained herein is property of Nordic Semiconductor ASA.   *
 * Terms and conditions of usage are described in detail in NORDIC             *
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.                          *
 *                                                                             *
 * Licensees are granted free, non-transferable use of the information. NO     *
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from     *
 * the file.                                                                   *
 *******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include "app_timer.h"
#include "bsp.h"
#include "app_gpiote.h"
#include "nrf_adc.h"
#include "adafruit1_8_oled_library.h"
#include "ssd1306.h"
#include "app_util_platform.h"
//#include "SEGGER_RTT.h"


#define E(x,y) x = y,
enum BUTTON_STATE_ENUM {
#include "button_state.h"
};

//#define E(x,y) #x,
//static const char *BUTTON_STATE_STRING_ENUM[] = {
//#include "button_state.h"
//};

/*
 * This example uses only one instance of the SPI master.
 * Please make sure that only one instance of the SPI master is enabled in config file.
 */
#define APP_GPIOTE_MAX_USERS            1    /**< Maximum number of users of the GPIOTE handler. */
#define APP_TIMER_PRESCALER             0    /**< Value of the RTC1 PRESCALER register. */
#define ADC_SAMPLING_INTERVAL                APP_TIMER_TICKS(100, APP_TIMER_PRESCALER) /**< Sampling rate for the ADC */

#define BUTTON_PIN 4

#define APP_TIMER_OP_QUEUE_SIZE  2  

//SD1306_CONFIG_VDD_PIN      2
#define SSD1306_CONFIG_CLK_PIN      1 
#define SSD1306_CONFIG_MOSI_PIN     2 
#define SSD1306_CONFIG_CS_PIN       29 
#define SSD1306_CONFIG_DC_PIN       0
#define SSD1306_CONFIG_RST_PIN      30

//JJ MISO?

int teller=0;
static volatile uint8_t color_index = 0;
static volatile uint8_t button_state = BUTTON_NONE;

APP_TIMER_DEF(m_adc_sampling_timer_id);

static const uint16_t color_sequence[] = {
	8,
	ST7735_BLUE,
	ST7735_RED,
	ST7735_GREEN,
	ST7735_CYAN,
	ST7735_MAGENTA,
	ST7735_YELLOW,
	ST7735_WHITE,
	ST7735_BLACK
};

static volatile bool button_state_change = false;

/**@brief Function for initializing the GPIOTE handler module.
*/
static void gpiote_init(void)
{
	APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
}

// ADC timer handler to start ADC sampling
static void adc_sampling_timeout_handler(void * p_context)
{
	NRF_ADC->EVENTS_END  = 0;
	NRF_ADC->TASKS_START = 1;            //Start ADC sampling
}

/**@brief Function for starting application timers.
*/
static void application_timers_start(void)
{
	//ADC timer start
	uint32_t err_code = app_timer_start(m_adc_sampling_timer_id, ADC_SAMPLING_INTERVAL, NULL);
	APP_ERROR_CHECK(err_code);
}

/*static void increment_color() {
  if (color_index == color_sequence[0]) {
  color_index = 0;
  }
  else {
  color_index++;
  }
  }

  static void decrement_color() {
  if (color_index != 0) {
  color_index--;
  }
  else {
  color_index = color_sequence[0];
  }
  }
  */
/**@brief Function to make the ADC start a battery level conversion.
*/
static void adc_init(void)
{
	NRF_ADC->CONFIG = (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_Pos) /*!< Analog external reference inputs disabled. */
		| (ADC_CONFIG_PSEL_AnalogInput5 << ADC_CONFIG_PSEL_Pos)
		| (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos)   /*!< Use internal 1.2V bandgap voltage as reference for conversion. */
		| (ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) /*!< Analog input specified by PSEL with 1/3 prescaling used as input for the conversion. */
		| (ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos);  /*!< 10bit ADC resolution. */
	// enable ADC
	NRF_ADC->ENABLE = 1; /* Bit 0 : ADC enable. */

	NRF_ADC->INTENSET = ADC_INTENSET_END_Msk;
	NVIC_SetPriority(ADC_IRQn, 1);
	NVIC_EnableIRQ(ADC_IRQn);
}

/**@brief Function for initializing the button handler module.
*/
static void buttons_init(void)
{
	nrf_gpio_cfg_sense_input(BUTTON_PIN, BUTTON_PULL, NRF_GPIO_PIN_SENSE_LOW);
}


/**@brief ADC interrupt handler.
 * @details  This function will fetch the conversion result from the ADC, convert the value into
 *           percentage and send it to peer.
 */
void ADC_IRQHandler(void)
{
	NRF_ADC->EVENTS_END = 0;
	uint16_t adc_result = NRF_ADC->RESULT;
	NRF_ADC->TASKS_STOP = 1;

	uint8_t old_state = button_state;

	if (button_state == BUTTON_NONE) {
		if (adc_result < 62) button_state = BUTTON_DOWN;
		else if (adc_result < 310) button_state = BUTTON_RIGHT;
		else if (adc_result < 465) button_state = BUTTON_SELECT;
		else if (adc_result < 620) button_state = BUTTON_UP;
		else if (adc_result < 1020) button_state = BUTTON_LEFT;
	}
	else {
		if (adc_result > 1020) button_state = BUTTON_NONE;
	}
	if (old_state != button_state) {
		//SEGGER_RTT_printf(0, "\x1B[32mbutton state : %s\x1B[0m\n", BUTTON_STATE_STRING_ENUM[button_state]);
		button_state_change = true;
	}
}

/**
 * @brief Function for initializing bsp module.
 */

void bsp_configuration() {

	uint32_t err_code = NRF_SUCCESS;

	NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_LFCLKSTART    = 1;

	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {
		// Do nothing.
	}

	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);

	err_code = app_timer_create(&m_adc_sampling_timer_id,
			APP_TIMER_MODE_REPEATED,
			adc_sampling_timeout_handler);

	err_code = bsp_init(BSP_INIT_LED, APP_TIMER_TICKS(100, APP_TIMER_PRESCALER), NULL);
	APP_ERROR_CHECK(err_code);
}
/*
   void draw_bitmap() {
   if (color_index == 0) {
   draw_bitmap_st7735(0, ST7735_TFTHEIGHT_18, imageLogo, ST7735_TFTWIDTH, ST7735_TFTHEIGHT_18);
   }
   }
   */
/**
 * @brief Function for application main entry. Does not return.
 */

void testdrawchar(void)

{

	ssd1306_clear_display();

	ssd1306_set_textsize(1);

	ssd1306_set_textcolor(WHITE);

	ssd1306_set_cursor(0, 0);



	for (uint8_t i = 0; i < 168; i++) {

		if (i == '\n') continue;

		ssd1306_write(i);

		if ((i > 0) && (i % 21 == 0))

			ssd1306_write('\n');

	}

	ssd1306_display();

}



int main(void) {

	// Setup bsp module.
	bsp_configuration();
	buttons_init();
	gpiote_init();
	adc_init();
	application_timers_start();
	//	ssd1306_power_on();
	//

	ssd1306_init(SSD1306_CONFIG_DC_PIN, SSD1306_CONFIG_RST_PIN, SSD1306_CONFIG_CS_PIN, SSD1306_CONFIG_CLK_PIN, SSD1306_CONFIG_MOSI_PIN);


	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, true);
	ssd1306_display();
	testdrawchar();
	ssd1306_draw_pixel(10,10,WHITE);
	//
	//
	//	tft_setup();

	//	fillScreen(ST7735_WHITE);

	//	color_index = 0;

	//	draw_bitmap_st7735(0, ST7735_TFTHEIGHT_18, imageLogo, ST7735_TFTWIDTH, ST7735_TFTHEIGHT_18);

	for (;;)
	{
	ssd1306_display();
	testdrawchar();
	teller++;
	if (teller > 31) teller=0;
	ssd1306_draw_pixel(teller,teller,WHITE);
		/*		if (button_state_change) {

				button_state_change = false;
		//SEGGER_RTT_printf(0, "\x1B[32mbutton_state_change\x1B[0m\n");

		switch (button_state) {

		case BUTTON_DOWN:
		decrement_color();
		draw_bitmap();
		if (color_index != 0) {
		fillScreen(color_sequence[color_index]);
		}
		break;
		case BUTTON_RIGHT:
		increment_color();
		draw_bitmap();
		if (color_index != 0) {
		fillScreen(color_sequence[color_index]);
		}
		break;
		case BUTTON_UP:
		increment_color();
		draw_bitmap();
		if (color_index != 0) {
		fillScreen(color_sequence[color_index]);
		}
		break;
		case BUTTON_LEFT:
		decrement_color();
		draw_bitmap();
		if (color_index != 0) {
		fillScreen(color_sequence[color_index]);
		}
		break;
		}
		}*/
	}
}
