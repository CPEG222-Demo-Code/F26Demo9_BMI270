#include "stm32f4xx.h"

#define NUM_LEDS          4
#define BITS_PER_LED      24
#define RESET_SLOTS       50
#define PWM_0_VAL         36  // ~0.40 us High Pulse at 90 MHz TIM2 clock
#define PWM_1_VAL         72  // ~0.80 us High Pulse at 90 MHz TIM2 clock

#define BUFFER_LEN        ((NUM_LEDS * BITS_PER_LED) + RESET_SLOTS)

// 32-bit entries: TIM2 CCR4 is 32-bit and APB bridges duplicate halfword writes
uint32_t dma_pwm_buffer[BUFFER_LEN];

// Convert RGB to GRB Bit Stream inside DMA Buffer
void NeoPixel_SetColor(uint8_t index, uint8_t red, uint8_t green, uint8_t blue) {
    if (index >= NUM_LEDS) return;

    // WS2812B expects GRB order MSB first
    uint32_t grb = ((uint32_t)green << 16) | ((uint32_t)red << 8) | blue;
    uint32_t start_idx = index * BITS_PER_LED;

    for (int i = 0; i < 24; i++) {
        // Extract MSB to LSB
        if (grb & (1U << (23 - i))) {
            dma_pwm_buffer[start_idx + i] = PWM_1_VAL;
        } else {
            dma_pwm_buffer[start_idx + i] = PWM_0_VAL;
        }
    }
}

void NeoPixel_Init_PB11(void) {
    // 1. Enable Clocks for GPIOB, TIM2, and DMA1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // An off pixel is still transmitted as 24 zero bits, not as a zero compare value.
    for (uint32_t i = 0; i < NUM_LEDS * BITS_PER_LED; i++) {
        dma_pwm_buffer[i] = PWM_0_VAL;
    }

    // 2. Configure PB11 as TIM2_CH4 Alternate Function (AF1)
    GPIOB->MODER   &= ~(3U << (11 * 2));
    GPIOB->MODER   |=  (2U << (11 * 2)); // AF Mode (10)
    GPIOB->OSPEEDR |=  (3U << (11 * 2)); // High speed for clean WS2812 edges

    // PB11 is in AFR High Register (AFR[1]) at position (11 - 8) * 4 = 12
    GPIOB->AFR[1]  &= ~(0xFU << ((11 - 8) * 4));
    GPIOB->AFR[1]  |=  (0x1U << ((11 - 8) * 4)); // AF1 for TIM2

    // 3. Configure TIM2 Channel 4 in PWM Mode 1
    TIM2->PSC  = 0;                   // Full 90 MHz APB1 timer clock
    TIM2->ARR  = 111;                 // 800 kHz PWM frequency (1.25 us period)
    TIM2->CCR4 = 0;
    
    // Configure Channel 4 PWM Mode 1 & enable output
    TIM2->CCMR2 &= ~TIM_CCMR2_OC4M;
    TIM2->CCMR2 |=  (6U << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE; // PWM Mode 1 + Preload enable
    TIM2->CCER  |=  TIM_CCER_CC4E;                               // Enable Channel 4 Output
    TIM2->DIER  &= ~TIM_DIER_CC4DE;                              // DMA is paced by the update event (enabled in Show)

    // 4. Configure DMA1 Stream 7 Channel 3 (TIM2_UP Request)
    DMA1_Stream7->CR &= ~DMA_SxCR_EN; // Disable before configuration
    while (DMA1_Stream7->CR & DMA_SxCR_EN);

    DMA1_Stream7->PAR  = (uint32_t)&(TIM2->CCR4); // Target TIM2 Compare Register 4
    DMA1_Stream7->M0AR = (uint32_t)dma_pwm_buffer;
    DMA1_Stream7->NDTR = BUFFER_LEN;

    // Channel 3, Memory-to-Peripheral, 32-bit data size, Memory increment, Normal mode
    DMA1_Stream7->CR   = (3U << DMA_SxCR_CHSEL_Pos) |
                         DMA_SxCR_DIR_0 |
                         DMA_SxCR_MINC |
                         DMA_SxCR_MSIZE_1 |
                         DMA_SxCR_PSIZE_1;
}

void NeoPixel_Show_PB11(void) {
    // Stop DMA and Timer
    TIM2->CR1 &= ~TIM_CR1_CEN;
    DMA1_Stream7->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream7->CR & DMA_SxCR_EN);

    // Force CCR4 to 0 (output low) without raising a DMA request
    TIM2->DIER &= ~TIM_DIER_UDE;
    TIM2->CCR4 = 0;
    TIM2->EGR  = TIM_EGR_UG;
    TIM2->SR   = 0;

    // Reset DMA transfer counter and clear all stream 7 flags
    DMA1_Stream7->M0AR = (uint32_t)dma_pwm_buffer;
    DMA1_Stream7->NDTR = BUFFER_LEN;
    DMA1->HIFCR = DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 |
                  DMA_HIFCR_CDMEIF7 | DMA_HIFCR_CFEIF7;

    // Each update event (one per bit period) makes DMA write the next compare value
    DMA1_Stream7->CR |= DMA_SxCR_EN;
    TIM2->CNT = 0;
    TIM2->DIER |= TIM_DIER_UDE;
    TIM2->CR1 |= TIM_CR1_CEN;
}