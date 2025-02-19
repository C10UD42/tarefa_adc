#include <stdio.h>  
#include <stdlib.h>           
#include "pico/stdlib.h"       
#include "hardware/adc.h"      
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
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

bool func_pwm = true;
bool cor = true;

bool debouncing(){
    //implementar o debouncing
    static uint32_t last_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time < 200) {
        return false;
    }
    last_time = current_time;
    return true;

}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    if (!debouncing()) 
        return;
    if(gpio == button) {
        func_pwm = !func_pwm;
    }
    
    if(gpio == SW_PIN) {
        // Alterna o estado do LED verde (pino 11)
        gpio_put(LED_PIN_GREEN, !gpio_get(LED_PIN_GREEN));
        cor = !cor;
        printf("Alterado led verde\n");
    }
}

int main() {
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

    
    stdio_init_all();
    adc_init();
    adc_gpio_init(VRX_PIN); 
    adc_gpio_init(VRY_PIN); 
    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN); 
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_put(LED_PIN_GREEN, false); 
    gpio_set_function(LED_PIN_BLUE, GPIO_FUNC_PWM); //habilitar o pino GPIO como PWM
    gpio_set_function(LED_PIN_RED, GPIO_FUNC_PWM); //habilitar o pino GPIO como PWM
    uint slice1 = pwm_gpio_to_slice_num(LED_PIN_BLUE);
    uint slice2 = pwm_gpio_to_slice_num(LED_PIN_RED);
    pwm_set_clkdiv(slice1, 125.0); //define o divisor de clock do PWM para 125
    pwm_set_clkdiv(slice2, 125.0); //define o divisor de clock do PWM para 125
    pwm_set_wrap(slice1, 19999); //define o valor de wrap para a frequência de 50 Hz (aproximadamente 20ms)
    pwm_set_wrap(slice2, 19999); //define o valor de wrap para a frequência de 50 Hz (aproximadamente 20ms)
    pwm_set_enabled(slice1, true);
    pwm_set_enabled(slice2, true);
    gpio_init(button);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button); 
    gpio_set_irq_enabled_with_callback(button, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    

    while (true) {
        adc_select_input(0); 
        uint16_t vrx_value = adc_read(); 
        adc_select_input(1); 
        uint16_t vry_value = adc_read(); 
        //cor = !cor;
        // Atualiza o conteúdo do display com animações
        ssd1306_fill(&ssd, !cor); // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
        printf("x:%d\ny:%d\n", vrx_value, vry_value);
        if(vry_value < 2047) { 
            ssd1306_draw_string(&ssd, "0", 60, 48); // Quadrado de baixo
            uint16_t duty_cycle = ((2047 - vry_value) * 19999) / 2047;
            if(func_pwm){
                pwm_set_chan_level(slice1, pwm_gpio_to_channel(LED_PIN_BLUE), duty_cycle);
            }
        }
        if(vry_value > 2047){
            ssd1306_draw_string(&ssd, "0", 60, 8);//Quadrado de cima
            uint16_t duty_cycle = ((vry_value - 2047) * 19999) / 2047;
            if(func_pwm){
                pwm_set_chan_level(slice1, pwm_gpio_to_channel(LED_PIN_BLUE), duty_cycle);
            }
        }
        if(vry_value == 2047){
            pwm_set_chan_level(slice1, pwm_gpio_to_channel(LED_PIN_BLUE), 0);
        }
        if(vrx_value < 2047){
            ssd1306_draw_string(&ssd, "0", 80, 28);//Quadrado a direita
            uint16_t duty_cycle = ((2047 - vrx_value) * 19999) / 2047;
            if(func_pwm){
                pwm_set_chan_level(slice2, pwm_gpio_to_channel(LED_PIN_RED), duty_cycle);
            }
        }
        if(vrx_value > 2047){
            ssd1306_draw_string(&ssd, "0", 40, 28);//Quadrado a esquerda
            uint16_t duty_cycle = ((vrx_value - 2047) * 19999) / 2047;
            if(func_pwm){
                pwm_set_chan_level(slice2, pwm_gpio_to_channel(LED_PIN_RED), duty_cycle);
            }
        }
        if(vrx_value == 2047){
            pwm_set_chan_level(slice2, pwm_gpio_to_channel(LED_PIN_RED), 0);
        }
        if(vrx_value == 2047 && vry_value == 2047){
            ssd1306_draw_string(&ssd, "0", 60, 28); //Quadrado centralizado
        }
        ssd1306_send_data(&ssd); // Atualiza o display
        sleep_ms(250);
    }
    return 0;
}
