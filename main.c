/***************************************************************************//**
 * @file
 * @brief Implements the Thermometer (GATT Server) Role of the Health
 * Thermometer Profile, which enables a Collector device to connect and
 * interact with a Thermometer.  The device acts as a connection
 * Peripheral. The sample application is based on Micrium OS RTOS.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                            INCLUDE FILES
 *********************************************************************************************************
 *********************************************************************************************************
 */

#include "bsp/siliconlabs/generic/include/bsp_os.h"

#include <cpu/include/cpu.h>
#include <common/include/common.h>
#include <kernel/include/os.h>

#include <common/include/lib_def.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>
#include <common/include/rtos_prio.h>
#include  <common/include/platform_mgr.h>
#include "em_letimer.h"
#include "sleep.h"
#include <stdio.h>

#include "rtos_bluetooth.h"
//Bluetooth API definition
#include "rtos_gecko.h"
//GATT DB
#include "gatt_db.h"

/* Board Headers */
#include "init_mcu.h"
#include "init_board.h"
#include "init_app.h"
#include "ble-configuration.h"
#include "board_features.h"
#include "hal-config-app-common.h"
#include "gpiointerrupt.h"
#include "em_core.h"
#include "em_cmu.h"
#include "em_device.h"
#include "em_emu.h"
#include "em_chip.h"
#include "em_gpio.h"
#include "em_timer.h"
#include "em_rtc.h"
#include "em_rtcc.h"
#include "sl_sleeptimer.h"
#include "efr32mg21_letimer.h"


#include "em_device.h"
#include "em_chip.h"
#include "bsp.h"
#include "bsphalconfig.h"
#include "bspconfig.h"


#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "bsp.h"
/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             LOCAL DEFINES
 *********************************************************************************************************
 *********************************************************************************************************
 */

#define  EX_MAIN_START_TASK_PRIO            21u
#define  EX_MAIN_START_TASK_STK_SIZE        512u

#define  APP_CFG_TASK_INIT_PRIO      6u
#define  APP_CFG_TASK_INIT_STK_SIZE  (1200 / sizeof(CPU_STK))

#define  APP_CFG_TASK_REDHIGHPRIO_PRIO      7u
#define  APP_CFG_TASK_REDHIGHPRIO_STK_SIZE  (1200 / sizeof(CPU_STK))

#define  APP_CFG_TASK_GREENLOWPRIO_PRIO      8u
#define  APP_CFG_TASK_GREENLOWPRIO_STK_SIZE  (1200 / sizeof(CPU_STK))



static  OS_TCB   App_TaskInitTCB;
static  CPU_STK  App_TaskInitStk[APP_CFG_TASK_INIT_STK_SIZE];

static  OS_TCB   App_TaskRedHighPrioTCB;
static  CPU_STK  App_TaskRedHighPrioStk[APP_CFG_TASK_INIT_STK_SIZE];

static  OS_TCB   App_TaskGreenLowPrioTCB;
static  CPU_STK  App_TaskGreenLowPrioStk[APP_CFG_TASK_INIT_STK_SIZE];

// MTM: Added configs for all Micrium OS tasks
// Timer Task Configuration
#if (OS_CFG_TMR_EN == DEF_ENABLED)
#define  TIMER_TASK_PRIO            4u
#define  TIMER_TASK_STK_SIZE        256u
#define  TIMER_TASK_CFG             .TmrTaskCfg = \
{                                                 \
    .StkBasePtr = &TimerTaskStk[0],               \
    .StkSize    = TIMER_TASK_STK_SIZE,            \
    .Prio       = TIMER_TASK_PRIO,                \
    .RateHz     = 10u                             \
},
#else
#define  TIMER_TASK_CFG
#endif

// ISR Configuration
#define  ISR_STK_SIZE               256u
#define  ISR_CFG                        .ISR = \
{                                              \
    .StkBasePtr  = (CPU_STK*) &ISRStk[0],      \
    .StkSize     = (ISR_STK_SIZE)              \
},

/* Define RTOS_DEBUG_MODE=DEF_ENABLED at the project level,
 * for enabling debug information for Micrium Probe.*/
#if (RTOS_DEBUG_MODE == DEF_ENABLED)
#define STAT_TASK_CFG          .StatTaskCfg = \
{                                             \
    .StkBasePtr = DEF_NULL,                   \
    .StkSize    = 256u,                       \
    .Prio       = KERNEL_STAT_TASK_PRIO_DFLT, \
    .RateHz     = 10u                         \
},

#define  OS_INIT_CFG_APP            { \
    ISR_CFG                           \
    TIMER_TASK_CFG                    \
    STAT_TASK_CFG                     \
    .MsgPoolSize     = 0u,            \
    .TaskStkLimit    = 0u,            \
    .MemSeg          = DEF_NULL,      \
    .TickRate        = 1000u          \
}
#else
#define  OS_INIT_CFG_APP            { \
    ISR_CFG                           \
    TIMER_TASK_CFG                    \
    .MsgPoolSize     = 0u,            \
    .TaskStkLimit    = 0u,            \
    .MemSeg          = DEF_NULL,      \
    .TickRate        = 1000u          \
}
#endif

#define  COMMON_INIT_CFG_APP        { \
    .CommonMemSegPtr = DEF_NULL       \
}

#define  PLATFORM_MGR_INIT_CFG_APP  { \
    .PoolBlkQtyInit = 0u,             \
    .PoolBlkQtyMax  = 0u              \
}

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                        LOCAL GLOBAL VARIABLES
 *********************************************************************************************************
 *********************************************************************************************************
 */

// Timer Task
#if (OS_CFG_TMR_EN == DEF_ENABLED)
static  CPU_STK  TimerTaskStk[TIMER_TASK_STK_SIZE];
#endif

// ISR Stack
static  CPU_STK  ISRStk[ISR_STK_SIZE];

const   OS_INIT_CFG             OS_InitCfg          = OS_INIT_CFG_APP;
const   COMMON_INIT_CFG         Common_InitCfg      = COMMON_INIT_CFG_APP;
const   PLATFORM_MGR_INIT_CFG   PlatformMgr_InitCfg = PLATFORM_MGR_INIT_CFG_APP;


/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************
 *********************************************************************************************************
 */


/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                          GLOBAL FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *                                                main()
 *
 * Description : This is the standard entry point for C applications. It is assumed that your code will
 *               call main() once you have performed all necessary initialization.
 *
 * Argument(s) : None.
 *
 * Return(s)   : None.
 *
 * Note(s)     : None.
 *********************************************************************************************************
 */


static void setup_leds(void);
static inline uint8_t get_leds(void);
static void set_leds(uint8_t control_byte);
/* Button event handler */
static void button_callback(const uint8_t pin);
/* Definitions of external signals */
static  void     initTask      (void *p_arg);
static  void     redHighPrioTask      (void *p_arg);
static  void     greenLowPrioTask      (void *p_arg);

#define BUTTON_PRESSED (1 << 0)
#define BUTTON_RELEASED (1 << 1)

OS_FLAG_GRP  App_Flags;
#define  APP_FLAG_A         (1u << 1)

/* 13761 Hz -> 14Mhz (clock frequency) / 1024 (prescaler)
Setting TOP to 27342 results in an overflow each 2 seconds */

#define TOP 27342

/* Defines for the RTC */
#define LFXO_FREQUENCY              32768
#define WAKEUP_INTERVAL_MS          1000
#define RTC_COUNT_BETWEEN_WAKEUP    ((LFXO_FREQUENCY * WAKEUP_INTERVAL_MS) / 1000)

/*#define SL_SLEEPTIMER_PERIPHERAL_DEFAULT 0
#define SL_SLEEPTIMER_PERIPHERAL_RTCC    1
#define SL_SLEEPTIMER_PERIPHERAL_RTC     2
#define SL_SLEEPTIMER_PERIPHERAL  SL_SLEEPTIMER_PERIPHERAL_DEFAULT
#define SL_SLEEPTIMER_WALLCLOCK_CONFIG  0*/
//#define SL_SLEEPTIMER_FREQ_DIVIDER  1000

// Desired frequency in Hz
#define OUT_FREQ 1000

// Desired repeat count
#define REPEAT_COUNT 10

void TIMER0_IRQHandler(void)
{
  RTOS_ERR err;
  // Acknowledge the interrupt
  uint32_t flags = TIMER_IntGet(TIMER0);
  TIMER_IntClear(TIMER0, flags);
 gecko_external_signal(BUTTON_RELEASED);
          /* Set Application Flag A.                    */
 //         flags = OSFlagPost(&App_Flags,        /*   Pointer to user-allocated event flag.    */
  //        		APP_FLAG_A,            /*   Application Flag A bitmask.              */
  //				OS_OPT_POST_FLAG_SET,  /*   Set the flag.                            */
  //				&err);
      }

void LETIMER0_IRQHandler(void)
{
  RTOS_ERR err;
  // Acknowledge the interrupt
  uint32_t flags = LETIMER_IntGet(LETIMER0);
  LETIMER_IntClear(LETIMER0, flags);
 gecko_external_signal(BUTTON_RELEASED);
          /* Set Application Flag A.                    */
          flags = OSFlagPost(&App_Flags,        /*   Pointer to user-allocated event flag.    */
          		APP_FLAG_A,            /*   Application Flag A bitmask.              */
  				OS_OPT_POST_FLAG_SET,  /*   Set the flag.                            */
  				&err);
      }


void my_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  //Code executed when the timer expire.
	RTOS_ERR err;
	 gecko_external_signal(BUTTON_RELEASED);
	 OS_FLAGS  flags;
	          /* Set Application Flag A.                    */
//	          flags = OSFlagPost(&App_Flags,        /*   Pointer to user-allocated event flag.    */
//	          		APP_FLAG_A,            /*   Application Flag A bitmask.              */
//	  				OS_OPT_POST_FLAG_SET,  /*   Set the flag.                            */
//	  				&err);
}
  // Update CCVB to alter duty cycle starting next period

/**************************************************************************//**
 * @brief RTC Handler
 * Interrupt Service Routine for Real Time Counter
 *****************************************************************************/



int main(void)
{
  RTOS_ERR  err;

  // Initialize device
  initMcu();
  // Initialize board
  initBoard();
  // Initialize application
  initApp();
  initVcomEnable();
  CMU_ClockEnable(cmuClock_PRS, true);

  //system already initialized by enter_DefaultMode_from_RESET
  //BSP_SystemInit();                                           /* Initialize System.                                   */

  // MTM: Not needed anymore
  //OS_ConfigureTickTask(&tickTaskCfg);

  OSInit(&err);                                                 /* Initialize the Kernel.                               */
                                                                /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  OSTaskCreate(&App_TaskInitTCB,                         /* Create the Start Task.                               */
                 "Thermometer Task",
				 initTask,
                 0,
				 APP_CFG_TASK_INIT_PRIO,
                 &App_TaskInitStk[0],
                 (APP_CFG_TASK_INIT_STK_SIZE / 10u),
                 APP_CFG_TASK_INIT_STK_SIZE,
                 0,
                 0,
                 0,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &err);

  /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  OSStart(&err);                                                /* Start the kernel.                                    */
                                                                /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  return (1);
}


/*
 *********************************************************************************************************
 *                                          Ex_MainStartTask()
 *
 * Description : This is the task that will be called by the Startup when all services are initializes
 *               successfully.
 *
 * Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
 *
 * Return(s)   : None.
 *
 * Notes       : None.
 *********************************************************************************************************
 */

static struct time_date timestamp_date;
static uint32_t timestamp1;

void timestamp_set_builddate(void)
{
	timestamp_date.hour = 19;           //
	timestamp_date.min = 59;	        // Example Time
	timestamp_date.sec = 45;	        //
	timestamp_date.month_day = 23;      //
	timestamp_date.year = 2020;         //
	timestamp_date.month = MONTH_APRIL; //

	sl_status_t sl_timer_status = sl_sleeptimer_build_datetime(&timestamp_date,2020,MONTH_MAY,27,19,42,10,0);
    sl_timer_status = sl_sleeptimer_convert_date_to_time(&timestamp_date,&timestamp1);
	sl_timer_status = sl_sleeptimer_set_time(timestamp1);

	if(sl_timer_status != SL_STATUS_OK)
	{
		printf("set datetime not possible\r\n");
	}

    else if(sl_timer_status == SL_STATUS_OK)
    {
        printf("set datetime done\r\n");
    }
}

static  void  initTask(void *p_arg)
{
  RTOS_ERR  err;
  CORE_DECLARE_IRQ_STATE;

  PP_UNUSED_PARAM(p_arg);                                       /* Prevent compiler warning.                            */

  BSP_OS_Init();                                                /* Initialize the BSP. It is expected that the BSP ...  */
                                                                /* ... will register all the hardware controller to ... */
                                                                /* ... the platform manager at this moment.             */

  /* Initialize GPIO interrupt handler */
   GPIOINT_Init();

   /* Set the pin of Push Button 0 as input with pull-up resistor */
   GPIO_PinModeSet( BSP_BUTTON0_PORT, BSP_BUTTON0_PIN, HAL_GPIO_MODE_INPUT_PULL_FILTER, HAL_GPIO_DOUT_HIGH );

   /* Enable interrupt on Push Button 0 */
   GPIO_ExtIntConfig(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN, BSP_BUTTON0_PIN, true, true, true);

   /* Register callback for Push Button 0 */
   GPIOINT_CallbackRegister(BSP_BUTTON0_PIN, button_callback);
   setup_leds();

   CMU_ClockEnable(cmuClock_TIMER0, true);
   CMU_ClockSelectSet(cmuClock_EM01GRPACLK,cmuSelect_HFXO  );

     TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
     timerCCInit.mode = timerCCModeCompare ;
     TIMER_InitCC(TIMER0, 2, &timerCCInit);

     // Initialize the timer
      TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
      timerInit.prescale = timerPrescale1024;
      timerInit.clkSel= timerClkSelHFPerClk;
      //TIMER_TopSet(TIMER0, 375001);
      //TIMER_Init(TIMER0, &timerInit);

     //Configure compare
     // TIMER_CompareSet(TIMER0, 2, 375000);
     // Enable TIMER0 compare event interrupts to update the duty cycle
      //TIMER_IntClear(TIMER0, TIMER_IF_CC2);
     //TIMER_IntEnable(TIMER0, TIMER_IEN_CC2);
     //NVIC_EnableIRQ(TIMER0_IRQn);

   sl_status_t status;
   sl_sleeptimer_timer_handle_t my_timer;
    uint32_t timer_timeout = 100;
   // CMU_ClockEnable(cmuClock_HFLE, true);
    CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);
    CMU_ClockSelectSet(cmuClock_RTCCCLK, cmuSelect_LFRCO);
    CMU_ClockEnable(cmuClock_RTCC, true);
    		    status = sl_sleeptimer_init();
    		    if(status != SL_STATUS_OK) {
    		      printf("Sleeptimer init error.\r\n");
    		    }
    		    /*status = sl_sleeptimer_start_periodic_timer(&my_timer,
    		                                       timer_timeout,
    		                                       my_timer_callback,
    		                                       (void *)NULL,
    		                                       0,
    		                                       0);*/
    		    timestamp_set_builddate();



  // Create task for Event handler
   OSFlagCreate(&App_Flags,              /*   Pointer to user-allocated event flag.         */
   	             "App Flags",             /*   Name used for debugging.                      */
   	                  0,                      /*   Initial flags, all cleared.                   */
   	                 &err);

   OSTaskCreate(&App_TaskRedHighPrioTCB,                         /* Create the Start Task.                               */
                    "Thermometer Task",
   				 redHighPrioTask,
                    0,
   				 APP_CFG_TASK_REDHIGHPRIO_PRIO,
                    &App_TaskRedHighPrioStk[0],
                    (APP_CFG_TASK_REDHIGHPRIO_STK_SIZE / 10u),
                    APP_CFG_TASK_REDHIGHPRIO_STK_SIZE,
                    0,
                    0,
                    0,
                    (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                    &err);

   OSTaskCreate(&App_TaskGreenLowPrioTCB,                         /* Create the Start Task.                               */
                    "Thermometer Task",
   				 greenLowPrioTask,
                    0,
   				 APP_CFG_TASK_GREENLOWPRIO_PRIO,
                    &App_TaskGreenLowPrioStk[0],
                    (APP_CFG_TASK_GREENLOWPRIO_STK_SIZE / 10u),
                    APP_CFG_TASK_GREENLOWPRIO_STK_SIZE,
                    0,
                    0,
                    0,
                    (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                    &err);


  /* Done starting everyone else so let's exit */
  // MTM: Remove Delete
  //OSTaskDel((OS_TCB *)0, &err);
}

static  void  redHighPrioTask(void *p_arg)
{
	PP_UNUSED_PARAM(p_arg);                                       /* Prevent compiler warning.                            */
	RTOS_ERR err;
	OS_FLAGS  flags;
	bool a=true;
	uint32 timestamp2;
	struct time_date timestamp2_date;
	while(1){
		timestamp2 = sl_sleeptimer_get_time();
		sl_sleeptimer_get_datetime(&timestamp2_date);
		a=!a;
		//flags = OSFlagPend(&App_Flags,                /*   Pointer to user-allocated event flag. */
			//                        APP_FLAG_A,             /*   Flag bitmask to match.                */
			  //                      0,                      /*   Wait for 10000 OS Ticks maximum.        */
			    //                    OS_OPT_PEND_FLAG_SET_ALL |/*   Wait until all flags are set and      */
			      //                  OS_OPT_PEND_BLOCKING     |/*    task will block and                  */
			        //                OS_OPT_PEND_FLAG_CONSUME, /*    function will clear the flags.       */
			          //              DEF_NULL,                 /*   Timestamp is not used.                */
			            //           &err);
			if(a==true && timestamp2_date.sec>timestamp_date.sec ){
				set_leds(BUTTON_PRESSED);
			}
			else{
				set_leds(BUTTON_RELEASED);
			}
	}

}


static  void  greenLowPrioTask(void *p_arg)
{
	PP_UNUSED_PARAM(p_arg);                                       /* Prevent compiler warning.                            */
	//set_leds(BUTTON_RELEASED);

}

/***************************************************************************//**
 * @brief
 *   This is the idle hook.
 *
 * @detail
 *   This will be called by the Micrium OS idle task when there is no other
 *   task ready to run. We just enter the lowest possible energy mode.
 ******************************************************************************/
void SleepAndSyncProtimer();
void OSIdleContextHook(void)
{
  while (1) {
    /* Put MCU in the lowest sleep mode available, usually EM2 */
    SleepAndSyncProtimer();
  }
}


/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */

static void button_callback(const uint8_t pin)
{
	RTOS_ERR  err;
	OS_FLAGS  flags;

  if (pin == BSP_BUTTON0_PIN) {
    /* when input is high, the button was released */
    if (GPIO_PinInGet(BSP_BUTTON0_PORT,BSP_BUTTON0_PIN)) {
        gecko_external_signal(BUTTON_RELEASED);
        /* Set Application Flag A.                    */
        flags = OSFlagPost(&App_Flags,        /*   Pointer to user-allocated event flag.    */
        		APP_FLAG_A,            /*   Application Flag A bitmask.              */
				OS_OPT_POST_FLAG_SET,  /*   Set the flag.                            */
				&err);
    }
    /* when input is low, the button was pressed*/
    else {
        gecko_external_signal(BUTTON_PRESSED);
    }
  }
}

/**
 * @brief setup_leds
 * Configure LED pins as output
 */
static void setup_leds(void) {
  GPIO_PinModeSet(BSP_LED0_PORT, BSP_LED0_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(BSP_LED1_PORT, BSP_LED1_PIN, gpioModePushPull, 0);
}

/**
 * @brief get_leds
 * Get LED statuses as two least significant bits of a return byte.
 * @return uint8_t LED status byte.
 */
static inline uint8_t get_leds(void) {
  return ((GPIO_PinOutGet(BSP_LED1_PORT, BSP_LED1_PIN) << 1) | GPIO_PinOutGet(BSP_LED0_PORT, BSP_LED0_PIN));
}

/**
 * @brief LED control function
 * bit 0 = LED0
 * bit 1 = LED1
 * bits 2-7 = don't care
 */
static void set_leds(uint8_t control_byte) {

  /* LED 0 control */
  if ((control_byte & 0x01) == 1) {
    GPIO_PinOutSet(BSP_LED0_PORT, BSP_LED0_PIN);
  } else {
    GPIO_PinOutClear(BSP_LED0_PORT, BSP_LED0_PIN);
  }

  /* LED 1 control */
  if (((control_byte >> 1) & 0x01) == 1) {
    GPIO_PinOutSet(BSP_LED1_PORT, BSP_LED1_PIN);
  } else {
    GPIO_PinOutClear(BSP_LED1_PORT, BSP_LED1_PIN);
  }
}

