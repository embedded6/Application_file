/**
 * @file    main.c  (APPLICATION)
 * @brief   S32K342 Application — runs after bootloader jumps here
 *
 * Confirmed from Application.map:
 *   App flash start  : 0x00500000 (boot_header here)
 *   App VTABLE       : 0x00501000 (bootloader jumps to this)
 *   App Reset Handler: 0x00501420
 *   App Stack top    : 0x20408000
 *   App code (main)  : 0x005015E8
 *
 * HOW TO KNOW APP IS RUNNING:
 *   Bootloader = LED blinks slow (1 second ON/OFF)
 *   Application = LED blinks FAST (100ms ON/OFF)
 *
 *   So when you power ON:
 *     Step 1: LED blinks slow → bootloader running
 *     Step 2: LED switches to fast blink → jump happened, app running ✅
 *     If LED stays slow forever → jump not happening ❌
 */

#include "Mcal.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "Pit_Ip.h"
#include "Clock_Ip.h"
#include "Clock_Ip_Cfg.h"
#include "IntCtrl_Ip.h"
#include "IntCtrl_Ip_Cfg.h"
#include "Siul2_Port_Ip.h"
#include "Siul2_Port_Ip_Cfg.h"
#include "Siul2_Dio_Ip.h"
#include "Siul2_Dio_Ip_Cfg.h"
#include "S32K342.h"
#include <stdint.h>

/* ============================================================
 * LED = PTC14, active LOW (same pin as bootloader)
 * ============================================================ */
#define LED_PORT                PTC_L_HALF
#define LED_PIN                 ((Siul2_Dio_Ip_PinsChannelType)(14U))

/* ============================================================
 * PIT
 * ============================================================ */
#define PIT_INST_0              0U
#define CH_0                    0U

/*
 * BOOTLOADER uses 40000000 ticks = 1 second blink
 * APPLICATION uses 20000000 ticks = 500ms blink
 * This makes the difference visible on board
 */
#define PIT_PERIOD_APP          20000000U   /* 500ms at 40MHz */

/* ============================================================
 * Globals
 * ============================================================ */
volatile uint8_t g_appToggleLed = 0U;

/* ============================================================
 * LED helpers — PTC14 active LOW
 * ============================================================ */
static void App_LedOn(void)
{
    Siul2_Dio_Ip_WritePin(LED_PORT, LED_PIN, 0U);   /* LOW = ON */
}

static void App_LedToggle(void)
{
    Siul2_Dio_Ip_TogglePins(LED_PORT,
                            (Siul2_Dio_Ip_PinsChannelType)(1U << LED_PIN));
}

/* ============================================================
 * PIT ISR — fires every 500ms in application
 * Bootloader fires every 1000ms
 * DIFFERENT period = you can see the difference on board
 * ============================================================ */
void PitNotification(void)
{
    g_appToggleLed = 1U;
}

/* ============================================================
 * main — APPLICATION entry point
 *
 * When bootloader jumps here:
 *   - VTOR already set to 0x00501000 by bootloader
 *   - MSP already set to 0x20408000 by bootloader
 *   - All interrupts disabled by bootloader
 *   - LED is OFF (bootloader turned it off before jump)
 *
 * PROOF that jump worked:
 *   LED will blink FAST (500ms) instead of slow (1000ms)
 *   You will clearly see the blink speed change on board
 * ============================================================ */
int main(void)
{
    Clock_Ip_StatusType clockStatus;

    /* --------------------------------------------------------
     * PROOF STEP — LED ON solid for 2 seconds
     * This is the FIRST thing app does
     * If you see LED turn ON solid after bootloader blinks
     * → Jump worked ✅
     * -------------------------------------------------------- */
    App_LedOn();   /* LED ON solid = "I am the application!" */
    for (volatile uint32_t i = 0U; i < 8000000U; i++)
    {
        __asm volatile ("nop");   /* ~2 second delay at 40MHz */
    }
    /* After 2 seconds, LED will start fast blinking below */

    /* 1. Clock init */
    clockStatus = Clock_Ip_Init(&Clock_Ip_aClockConfig[0]);
    if (clockStatus != CLOCK_IP_SUCCESS)
    {
        /* Clock failed — LED stays ON solid */
        App_LedOn();
        while (1) { __asm volatile ("nop"); }
    }

    /* 2. Pin init */
    Siul2_Port_Ip_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);

    /* 3. Interrupt controller init */
    IntCtrl_Ip_ConfigIrqRouting(&intRouteConfig);
    IntCtrl_Ip_EnableIrq(PIT0_IRQn);

    /* 4. PIT init — 500ms period (DIFFERENT from bootloader 1000ms) */
    Pit_Ip_Init(PIT_INST_0, &PIT_0_InitConfig_PB);
    Pit_Ip_InitChannel(PIT_INST_0, PIT_0_CH_0);
    Pit_Ip_EnableChannelInterrupt(PIT_INST_0, CH_0);
    Pit_Ip_StartChannel(PIT_INST_0, CH_0, PIT_PERIOD_APP);   /* 500ms */

    /* 5. Enable interrupts */
    __asm volatile (" cpsie i" ::: "memory");

    /* 6. Application main loop
     *    LED blinks every 500ms
     *    Bootloader blinks every 1000ms
     *    You can clearly see the speed difference on board
     */
    while (1)
    {
        if (g_appToggleLed == 1U)
        {
            App_LedToggle();       /* Fast blink = app is running */
            g_appToggleLed = 0U;
        }

        /* Add your application logic here */
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
