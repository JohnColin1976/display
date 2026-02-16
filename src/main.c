#include "sam3xa.h"
#include "init.h"
#include "uart.h"


// Функция обработчика прерывания с номером ID_TC0
void TC0_Handler(void) __attribute__((used));
void TC0_Handler(void) {
    TcChannel *tc = &TC0->TC_CHANNEL[0];
    uint32_t sr = tc->TC_SR;           // ACK
}


int main(void) {
    // Cнять защиту PMC
    PMC->PMC_WPMR = 0x504D4300;

    // Cнять защиту TC0
    TC0->TC_WPMR = 0x54430000;

    // Контроллер прерываний внутри ядра
    NVIC_DisableIRQ(TC0_IRQn);
    NVIC_ClearPendingIRQ(TC0_IRQn);
    NVIC_SetPriority(TC0_IRQn, 0);   // высокий приоритет
    NVIC_EnableIRQ(TC0_IRQn);

    setup();

    while (1) {
        loop();
    }
}
