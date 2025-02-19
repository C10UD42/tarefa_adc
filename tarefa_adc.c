#include <stdio.h>  
#include <stdlib.h>           
#include "pico/stdlib.h"       
#include "hardware/adc.h"      
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "hardware/clocks.h"
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define VRX_PIN 26   
#define VRY_PIN 27   
#define SW_PIN 22    
#define LED_PIN_GREEN 11  
#define LED_PIN_BLUE 12 
#define LED_PIN_RED 13  
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define UART_ID uart0
#define button 5
#define SQUARE_SIZE 8

bool func_pwm = true;
bool cor = true;
int x_square;
int y_square;

bool debouncing_buttonA(){
    static uint32_t last_time_button = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time_button < 200) {
        return false;
    }
    last_time_button = current_time;
    return true;
}

bool debouncing_SW(){
    static uint32_t last_time_SW = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time_SW < 200) {
        return false;
    }
    last_time_SW = current_time;
    return true;
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    if(gpio == button) {
        if (!debouncing_buttonA())
            return;
        func_pwm = !func_pwm;
    }
    
    if(gpio == SW_PIN) {
        if (!debouncing_SW())
            return;
        gpio_put(LED_PIN_GREEN, !gpio_get(LED_PIN_GREEN));
        cor = !cor;
    }
}

void setup_pwm(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice_num, 4095);
    pwm_set_enabled(slice_num, true);
}

int main() {
    stdio_init_all(); 
    sleep_ms(1000);  
    printf("Iniciando configurações...\n");

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;
    
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    adc_init();
    adc_gpio_init(VRX_PIN); 
    adc_gpio_init(VRY_PIN); 

    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN); 

    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_put(LED_PIN_GREEN, false); 

    setup_pwm(LED_PIN_RED);  
    setup_pwm(LED_PIN_BLUE);  

    gpio_init(button);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button); 
    gpio_set_irq_enabled_with_callback(button, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    gpio_set_irq_enabled(SW_PIN, GPIO_IRQ_EDGE_FALL, true);
    
    while (true) {
        adc_select_input(1); 
        uint16_t x_value = adc_read(); 
        adc_select_input(0); 
        uint16_t y_value = adc_read(); 

        ssd1306_fill(&ssd, !cor);
        ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor);
        printf("x:%d\ny:%d\n", x_value, y_value);
        
        if (func_pwm) {
            uint16_t red_intensity = (x_value > 2048) ? (x_value - 2048) : (2048 - x_value);
            uint16_t blue_intensity = (y_value > 2048) ? (y_value - 2048) : (2048 - y_value);
            pwm_set_gpio_level(LED_PIN_RED, red_intensity);
            pwm_set_gpio_level(LED_PIN_BLUE, blue_intensity);
        } else {
            pwm_set_gpio_level(LED_PIN_RED, 0);
            pwm_set_gpio_level(LED_PIN_BLUE, 0);
        }

        uint8_t square_x = (x_value * (WIDTH - SQUARE_SIZE)) / 4095;
        uint8_t square_y = ((4095 - y_value) * (HEIGHT - SQUARE_SIZE)) / 4095;
        
        ssd1306_fill(&ssd, false);
        ssd1306_rect(&ssd, square_y, square_x, SQUARE_SIZE, SQUARE_SIZE, true, true);
        ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, cor, false);  
        ssd1306_send_data(&ssd);
        
        sleep_ms(10);
    }

    return 0;
}
