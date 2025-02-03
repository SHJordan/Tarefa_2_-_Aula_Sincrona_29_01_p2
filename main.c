#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define BOTAO_PIN 5
#define LED_AZUL 11
#define LED_VERMELHO 12
#define LED_VERDE 13

typedef enum
{
    PASSO_DOIS,
    PASSO_UM,
    DESLIGAR
} passo_t;

volatile bool sequencia_ativa = false;

// Callback do alarme
int64_t passo_callback(alarm_id_t id, void *user_data)
{
    passo_t passo = (passo_t)user_data;
    switch (passo)
    {
    case PASSO_DOIS:
        gpio_put(LED_AZUL, 0);
        add_alarm_in_ms(3000, passo_callback, (void *)PASSO_UM, true); // Alarmes encadeados
        break;
    case PASSO_UM:
        gpio_put(LED_VERMELHO, 0);
        add_alarm_in_ms(3000, passo_callback, (void *)DESLIGAR, true);
        break;
    case DESLIGAR:
        gpio_put(LED_VERDE, 0);
        sequencia_ativa = false;
        break;
    }
    return 0; // Retorno ignorado para alarmes não repetitivos
}

// Debounce do botão
int64_t debounce_callback(alarm_id_t id, void *user_data)
{
    uint gpio = (uint)user_data;
    if (!gpio_get(gpio) && !sequencia_ativa)
    {
        sequencia_ativa = true;
        gpio_put(LED_AZUL, 1);
        gpio_put(LED_VERMELHO, 1);
        gpio_put(LED_VERDE, 1);
        add_alarm_in_ms(3000, passo_callback, (void *)PASSO_DOIS, true);
    }
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL, true);
    return 0;
}

// Handler de interrupção do botão (GPIO)
void botao_pressionado(uint gpio, uint32_t eventos)
{
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL, false);
    add_alarm_in_ms(50, debounce_callback, (void *)gpio, true); // Debounce de 50ms
}

int main()
{
    stdio_init_all();

    // Configuração dos LEDs
    gpio_init(LED_AZUL);
    gpio_init(LED_VERMELHO);
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_set_dir(LED_VERDE, GPIO_OUT);

    // Configuração do botão
    gpio_init(BOTAO_PIN);
    gpio_set_dir(BOTAO_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_PIN);
    gpio_set_irq_enabled_with_callback(BOTAO_PIN, GPIO_IRQ_EDGE_FALL, true, &botao_pressionado);

    while (true)
    {
        // Loop principal sem bloqueios
        tight_loop_contents(); // CPU entra em modo de espera
    }
}