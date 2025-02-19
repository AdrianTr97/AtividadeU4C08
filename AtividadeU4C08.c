#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"  
#include "hardware/pwm.h"  
#include "lib/ssd1306.h"
#include "lib/font.h"

//#define I2C_PORT i2c0
//#define I2C_SDA 8
//#define I2C_SCL 9
//#define VRX_PIN 26   
//#define VRY_PIN 27   
//#define SW_PIN 22
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define JOYSTICK_X_PIN 26  // GPIO para eixo X
#define JOYSTICK_Y_PIN 27  // GPIO para eixo Y
#define JOYSTICK_PB 22 // GPIO para botão do Joystick
#define Botao_A 5 // GPIO para botão A
#define LEDG_PIN 11  //GPIO led verde
#define LEDB_PIN 13  //GPIO led azul
#define LEDR_PIN 12  //GPIO led vermelho

// Função para inicializar o PWM
uint pwm_init_gpio(uint gpio, uint wrap) {
    // Define o pino GPIO como saída de PWM
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    
    // Obtém o número do "slice" de PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    
    // Define o "wrap" do PWM, que define o valor máximo para o contador
    // Isso afeta a resolução do PWM (quanto maior, maior a precisão do duty cycle)
    pwm_set_wrap(slice_num, wrap);
    
    // Habilita o PWM no slice
    pwm_set_enabled(slice_num, true);  
    return slice_num;  // Retorna o número do slice para uso posterior
}

void gpio_config(){
    // Inicializa o pino do LED verde
    gpio_init(LEDB_PIN);            // Inicializa o pino do LED verde
    gpio_set_dir(LEDB_PIN, GPIO_OUT);  // Configura o pino como saída
    gpio_put(LEDB_PIN, false);      // Inicializa o LED verde apagado (false)
}

int main(){
    // Inicializa a comunicação serial para permitir o uso de printf
    // Isso permite enviar dados via USB para depuração no console
    stdio_init_all();
    
    gpio_config();
    bool led_estado = false;  // Flag para controlar o estado do LED (false = desligado, true = ligado), para o led verde

    // Inicializa o PWM para o LED no pino GP12
    uint pwm_wrap = 4096;  // Define o valor máximo para o contador do PWM (influencia o duty cycle)
    uint pwm_slice = pwm_init_gpio(LEDG_PIN, pwm_wrap);  // Inicializa o PWM no pino LED_PIN e retorna o número do slice, green eixo y, para cima
    uint pwm_slice = pwm_init_gpio(LEDR_PIN, pwm_wrap);  // Inicializa o PWM no pino LED_PIN e retorna o número do slice, red eixo x, para esquerda

    uint32_t last_print_time = 0; // Variável para armazenar o tempo da última impressão na serial
    
    gpio_init(JOYSTICK_PB);
    gpio_set_dir(JOYSTICK_PB, GPIO_IN);
    gpio_pull_up(JOYSTICK_PB); 
  
    gpio_init(Botao_A);
    gpio_set_dir(Botao_A, GPIO_IN);
    gpio_pull_up(Botao_A);
  
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);
  
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_t ssd; // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display
  
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
  
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);  
    
    uint16_t adc_value_x;
    uint16_t adc_value_y;  
    //char str_x[5];  // Buffer para armazenar a string
    //char str_y[5];  // Buffer para armazenar a string  
    
    bool cor = true;
    while (true)
    {
      adc_select_input(0); // Seleciona o ADC para eixo X. O pino 26 (VRX) como entrada analógica
      adc_value_x = adc_read();
      uint16_t vrx_value = adc_read();  // Lê o valor analógico do eixo X, retornando um valor entre 0 e 4095
      adc_select_input(1); // Seleciona o ADC para eixo Y. O pino 27 como entrada analógica
      adc_value_y = adc_read();    
      uint16_t vry_value = adc_read();  // Lê o valor analógico do eixo X, retornando um valor entre 0 e 4095
      //sprintf(str_x, "%d", adc_value_x);  // Converte o inteiro em string
      //sprintf(str_y, "%d", adc_value_y);  // Converte o inteiro em string

      // Controle do LED com PWM baseado no valor do ADC0 (VRX)
      pwm_set_gpio_level(LEDR_PIN, vrx_value);  // Ajusta o duty cycle do LED proporcional ao valor lido de VRX

      // Calcula o duty cycle em porcentagem para impressão
      float duty_cycle = (vrx_value / 4095.0) * 100;  // Converte o valor lido do ADC em uma porcentagem do duty cycle

      // Imprime os valores lidos e o duty cycle proporcional na comunicação serial a cada 1 segundo
      uint32_t current_time = to_ms_since_boot(get_absolute_time());  // Obtém o tempo atual desde o boot do sistema
      if (current_time - last_print_time >= 1000) {  // Verifica se passou 1 segundo desde a última impressão
          printf("VRX: %u\n", vrx_value);  // Imprime o valor lido de VRX no console serial
          printf("Duty Cycle LED: %.2f%%\n", duty_cycle);  // Imprime o duty cycle calculado em porcentagem
          last_print_time = current_time;  // Atualiza o tempo da última impressão
      }

      // Introduz um atraso de 100 milissegundos antes de repetir a leitura
      sleep_ms(100);  // Pausa o programa por 100ms para evitar leituras e impressões muito rápidas


      //cor = !cor;
      // Atualiza o conteúdo do display com animações
      ssd1306_fill(&ssd, !cor); // Limpa o display
      ssd1306_rect(&ssd, 1, 1, 126, 64, cor, !cor); // Desenha o retângulo externo 
      //ssd1306_line(&ssd, 3, 25, 123, 25, cor); // Desenha uma linha
      //ssd1306_line(&ssd, 3, 37, 123, 37, cor); // Desenha uma linha   
      //ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6); // Desenha uma string
      //ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16); // Desenha uma string
      //ssd1306_draw_string(&ssd, "ADC   JOYSTICK", 10, 28); // Desenha uma string 
      //ssd1306_draw_string(&ssd, "X    Y    PB", 20, 41); // Desenha uma string    
      //ssd1306_line(&ssd, 44, 37, 44, 60, cor); // Desenha uma linha vertical         
      //ssd1306_draw_string(&ssd, str_x, 8, 52); // Desenha uma string     
      //ssd1306_line(&ssd, 84, 37, 84, 60, cor); // Desenha uma linha vertical      
      //ssd1306_draw_string(&ssd, str_y, 49, 52); // Desenha uma string   
      //ssd1306_rect(&ssd, 52, 90, 8, 8, cor, !gpio_get(JOYSTICK_PB)); // Desenha um retângulo, preenche o retangulo na verdade 
      if (gpio_get(JOYSTICK_PB) == 0) {
        // Desenha apenas o contorno do novo retângulo quando o joystick for pressionado
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); 
        // Se o joystick foi pressionado, alterna o estado do LED verde
        led_estado = !led_estado;  // Alterna o estado (ligado/desligado)
        
        // Atualiza o estado do LED verde com base no flag
        gpio_put(LEDB_PIN, led_estado); // Liga ou desliga o LED azul conforme o estado 
      }
      ssd1306_rect(&ssd, 52, 102, 8, 8, cor, !gpio_get(Botao_A)); // Desenha um retângulo    
      //ssd1306_rect(&ssd, 52, 114, 8, 8, cor, !cor); // Desenha o 3 retângulo aberto que nao serve para nada      
      ssd1306_send_data(&ssd); // Atualiza o display
  
  
      sleep_ms(100);
    }
  }
