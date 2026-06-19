#include "driver/gpio.h"


#define GPIO_AQUECIMENTO    GPIO_NUM_18
#define GPIO_REFRIGERACAO   GPIO_NUM_19

typedef enum
{
    DESLIGADO = 0,
    AQUECENDO,
    REFRIGERANDO
} acionamento_sistema_t;

void gpioInit(void);
void atualiza_saidas(acionamento_sistema_t estado);
