/***************************************************************************//**
 * @file
 * @brief SLEEPTIMER configuration.
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

// <<< Use Configuration Wizard in Context Menu >>>

#ifndef SLEEPTIMER_CONFIG_H
#define SLEEPTIMER_CONFIG_H

#define SL_SLEEPTIMER_PERIPHERAL_DEFAULT 0
#define SL_SLEEPTIMER_PERIPHERAL_RTCC    1
#define SL_SLEEPTIMER_PERIPHERAL_RTC     2

// <o SL_SLEEPTIMER_PERIPHERAL> Timer Peripheral Used by Sleeptimer
//   <SL_SLEEPTIMER_PERIPHERAL_DEFAULT=> Default (auto select)
//   <SL_SLEEPTIMER_PERIPHERAL_RTCC=> RTCC
//   <SL_SLEEPTIMER_PERIPHERAL_RTC=> RTC
// <i> Selection of the Timer Peripheral Used by the Sleeptimer
#define SL_SLEEPTIMER_PERIPHERAL  SL_SLEEPTIMER_PERIPHERAL_RTCC

// <q SL_SLEEPTIMER_WALLCLOCK_CONFIG> Enable wallclock functionality
// <i> Enable or disable wallclock functionalities (get_time, get_date, etc).
// <i> Default: 0
#define SL_SLEEPTIMER_WALLCLOCK_CONFIG  1

// <o SL_SLEEPTIMER_FREQ_DIVIDER> Timer frequency divider
// <i> Default: 1
#define SL_SLEEPTIMER_FREQ_DIVIDER  32

#endif /* SLEEPTIMER_CONFIG_H */

// <<< end of configuration section >>>
