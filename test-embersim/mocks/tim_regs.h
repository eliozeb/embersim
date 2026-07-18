#ifndef TIM_REGS_H
#define TIM_REGS_H

#include <stdint.h>
#include <stdbool.h>

/* ----- register map for a single timer instance ----- */
typedef struct {
    volatile uint32_t CR1;   /* control register 1 */
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;  /* DMA / interrupt enable register */
    volatile uint32_t SR;    /* status register */
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;   /* counter */
    volatile uint32_t PSC;   /* prescaler */
    volatile uint32_t ARR;   /* auto‑reload register */
    volatile uint32_t RCR;
    volatile uint32_t CCR1;  /* capture / compare channel 1 */
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t BDTR;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} TIM_Registers;

/* ----- register bit definitions (common) ----- */
#define TIM_CR1_CEN   (1 << 0)   /* counter enable */
#define TIM_CR1_UDIS  (1 << 1)   /* update disable */
#define TIM_CR1_URS   (1 << 2)   /* update request source */
#define TIM_CR1_OPM   (1 << 3)   /* one pulse mode */
#define TIM_CR1_ARPE  (1 << 7)   /* auto‑reload preload enable */

#define TIM_DIER_UIE  (1 << 0)   /* update interrupt enable */
#define TIM_DIER_CC1IE (1 << 1)  /* capture/compare 1 interrupt enable */
#define TIM_DIER_CC2IE (1 << 2)
#define TIM_DIER_CC3IE (1 << 3)
#define TIM_DIER_CC4IE (1 << 4)

#define TIM_SR_UIF    (1 << 0)   /* update interrupt flag */
#define TIM_SR_CC1IF  (1 << 1)   /* capture/compare 1 interrupt flag */
#define TIM_SR_CC2IF  (1 << 2)
#define TIM_SR_CC3IF  (1 << 3)
#define TIM_SR_CC4IF  (1 << 4)

/* ----- public API ----- */
void         tim_regs_init(uint32_t base_address);
TIM_Registers* tim_regs_get(uint32_t base_address);
void         tim_regs_tick(uint32_t base_address);
void         tim_regs_check_irq(uint32_t base_address);
#endif