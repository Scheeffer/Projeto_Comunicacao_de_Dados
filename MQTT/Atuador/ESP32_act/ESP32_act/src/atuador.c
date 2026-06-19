#include "atuador.h"


static acionamento_sistema_t atuador_atual = DESLIGADO;
void gpioInit(void)
{
    gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_AQUECIMENTO) |
                    (1ULL << GPIO_REFRIGERACAO),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};
}
void atualiza_saidas(acionamento_sistema_t estado)
{
    /* Primeiro desliga tudo */
    gpio_set_level(GPIO_AQUECIMENTO, 0);
    gpio_set_level(GPIO_REFRIGERACAO, 0);

    switch(estado)
    {
        case AQUECENDO:
            gpio_set_level(GPIO_AQUECIMENTO, 1);
            break;

        case REFRIGERANDO:
            gpio_set_level(GPIO_REFRIGERACAO, 1);
            break;

        case DESLIGADO:
        default:
            break;
    }

    atuador_atual = estado;
}