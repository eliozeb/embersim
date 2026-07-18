// mock_tim.c
#include "mock_hal.h"
#include "mock_tim.h"
#include "ember_regs.h"
#include "ember_sim_kernel.h"
#include "trace_log.h"
#include "ember_sim_kernel.h"
#include <string.h>
#include <stdio.h>

// ---- Register indices ----
enum { IDX_CR1, IDX_SR, IDX_CNT, IDX_PSC, IDX_ARR, IDX_DIER, IDX_CCR1, IDX_CCR2, IDX_CCR3, IDX_CCR4, REG_COUNT };
#define TIM_DIER_UIE  (1 << 0)

static EmberRegister tim_regs[REG_COUNT];
static EmberRegMap   tim_map;
static EmberPeripheral tim_peripheral;
static bool          pwm_active = false;  // simplified for now

// Forward declarations
static void tim_tick(EmberPeripheral *p, uint64_t now_us);
static void tim_handle_event(EmberPeripheral *p, const KernelEvent *ev);
static void tim_init_fn(EmberPeripheral *p);

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim);

uint32_t mock_tim_get_sr(uintptr_t base) {
    return ember_reg_read(&tim_map, IDX_SR);
}

void mock_tim_init(void) {
    // Called by test before using HAL; we'll init registers here
    memset(tim_regs, 0, sizeof(tim_regs));
    // define metadata
    tim_regs[IDX_CR1].name = "CR1"; tim_regs[IDX_CR1].writable_mask = 0x0001;
    tim_regs[IDX_SR].name  = "SR";  tim_regs[IDX_SR].writable_mask  = 0x001F;
    tim_regs[IDX_CNT].name = "CNT"; tim_regs[IDX_CNT].writable_mask = 0xFFFF;
    tim_regs[IDX_PSC].name = "PSC"; tim_regs[IDX_PSC].writable_mask = 0xFFFF;
    tim_regs[IDX_ARR].name = "ARR"; tim_regs[IDX_ARR].writable_mask = 0xFFFF;

    tim_regs[IDX_DIER].name = "DIER"; tim_regs[IDX_DIER].writable_mask = 0xFFFF;  // all bits writable for simplicity

    ember_regs_init(&tim_map, "TIM2", 0x40000400, tim_regs, REG_COUNT);
    // configure peripheral
    tim_peripheral.name         = "TIM2";
    tim_peripheral.base_address = 0x40000400;
    tim_peripheral.irq_number   = 28; // TIM2_IRQn
    tim_peripheral.state        = NULL;
    tim_peripheral.init         = tim_init_fn;
    tim_peripheral.tick         = tim_tick;
    tim_peripheral.handle_event = tim_handle_event;
}

static void tim_init_fn(EmberPeripheral *p) {
    (void)p;
    // Already initialized in mock_tim_init; nothing extra needed.
}

HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim) {
    // ensure peripheral registered
    if (tim_peripheral.base_address == 0) mock_tim_init();
    kernel_register_peripheral(&tim_peripheral);
    trace_log("HAL_TIM_Base_Init", "\"registered\"");
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *htim) {
    ember_reg_set_bits(&tim_map, IDX_CR1, 0x0001, "start", 0);
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_Base_Start", payload);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef *htim) {
    ember_reg_clear_bits(&tim_map, IDX_CR1, 0x0001, "stop", 0);
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_Base_Stop", payload);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim) {
    // enable update interrupt
    ember_reg_set_bits(&tim_map, IDX_CR1, 0x0001, "start IT", 0);
    // also set DIER.UIE (we'll add DIER later; for now we just rely on kernel to fire on overflow)
    // We'll just start the counter; the tick will generate update events and NVIC will trigger.
    // In real implementation we'd set DIER bit in a proper register.
    ember_reg_set_bits(&tim_map, IDX_DIER, TIM_DIER_UIE, "UIE enable", 0);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *htim) {
    ember_reg_clear_bits(&tim_map, IDX_DIER, TIM_DIER_UIE, "UIE disable", 0);
    return HAL_TIM_Base_Stop(htim);
}

HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel) {
    pwm_active = true;
    return HAL_TIM_Base_Start(htim);
}

HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel) {
    pwm_active = false;
    return HAL_TIM_Base_Stop(htim);
}

HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel) {
    // input capture – we'll keep simple for now
    return HAL_TIM_Base_Start(htim);
}

// Public API for tests to set period etc.
void mock_tim_set_period_ticks(uintptr_t base, uint32_t us) {
    ember_reg_write(&tim_map, IDX_ARR, us, "set period", 0);
}

void mock_tim_set_pwm_duty(uintptr_t base, uint32_t channel, uint32_t duty) {
    if (channel <= 4) ember_reg_write(&tim_map, IDX_CCR1 + channel - 1, duty, "set duty", 0);
}

void mock_tim_inject_capture(uintptr_t base, uint32_t channel, uint32_t value) {
    if (channel <= 4) {
        ember_reg_write(&tim_map, IDX_CCR1 + channel - 1, value, "capture inject", 0);
        ember_reg_set_bits(&tim_map, IDX_SR, 0x0002 << (channel-1), "capture", 0);
    }
}

static void tim_tick(EmberPeripheral *p, uint64_t now_us) {
    uint32_t cr1 = ember_reg_read(&tim_map, IDX_CR1);
    if (!(cr1 & 1)) return; // CEN
    uint32_t cnt = ember_reg_read(&tim_map, IDX_CNT);
    uint32_t arr = ember_reg_read(&tim_map, IDX_ARR);
    cnt++;
    if (cnt >= arr) {
        cnt = 0;
        ember_reg_write(&tim_map, IDX_CNT, cnt, "overflow", 0);
        ember_reg_set_bits(&tim_map, IDX_SR, 0x0001, "UIF set", 0);
        // schedule a timer update event
        kernel_schedule_event(0, KERN_EVT_TIM_UPDATE, p->base_address, 0, 0);
    } else {
        ember_reg_write(&tim_map, IDX_CNT, cnt, "increment", 0);
    }
}

static void tim_handle_event(EmberPeripheral *p, const KernelEvent *ev) {
    if (ev->type == KERN_EVT_TIM_UPDATE) {
        uint32_t sr = ember_reg_read(&tim_map, IDX_SR);
        uint32_t dier = ember_reg_read(&tim_map, IDX_DIER);
        if ((sr & 0x0001) && (dier & TIM_DIER_UIE)) {
            // The NVIC set and IRQ handler would be called in a full NVIC model;
            // for now we directly invoke the HAL IRQ handler.
            extern void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim);
            TIM_HandleTypeDef htim = { .Instance = p->base_address, .ErrorCode = 0 };
            HAL_TIM_IRQHandler(&htim);
        }
    }
}

// IRQ handler – calls callbacks
void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim) {
    char details[128];
    snprintf(details, sizeof(details), "{\"instance\":\"TIM2\",\"address\":\"0x%08X\"}", (uint32_t)htim->Instance);
    trace_software_event("TIM", "irq_handler", details);

    uint32_t sr = ember_reg_read(&tim_map, IDX_SR);
    if (sr & 0x0001) { // UIF
        ember_reg_clear_bits(&tim_map, IDX_SR, 0x0001, "HAL cleared UIF", 0);
        HAL_TIM_PeriodElapsedCallback(htim);
        if (pwm_active) {
            HAL_TIM_PWM_PulseFinishedCallback(htim);
        }
    }
    // handle CCx flags (capture/compare)
    for (int ch = 0; ch < 4; ch++) {
        if (sr & (0x0002 << ch)) {
            ember_reg_clear_bits(&tim_map, IDX_SR, 0x0002 << ch, "HAL cleared CCxIF", 0);
            if (ch == 3 && pwm_active) {
                // actually PWM finished is already called on UIF, fine.
            } else {
                HAL_TIM_IC_CaptureCallback(htim);
            }
        }
    }
}

// weak default callbacks (overridden by test)
__attribute__((weak)) void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {}
__attribute__((weak)) void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {}
__attribute__((weak)) void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {}