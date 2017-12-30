/*
 * Accu_monitor.h
 *
 *  Created on: 30.12.2017
 *      Author: rober_000
 */

#ifndef COMMON_ACCU_MONITOR_H_
#define COMMON_ACCU_MONITOR_H_


/*! *********************************************************************************
* \brief  InitADC
*
* \remarks	The Analog and Digital conversion. 16Bit and no differential conversion.
* No interrupts, because conversion is triggered in PIT interrupt, because Software trigger
* is used.
********************************************************************************** */
void InitADC(void);
/*! *********************************************************************************
* \brief  InitPIT
*
* \remarks	The Periodic interrupt timer is intialized: Ch0,every couple of second
*
********************************************************************************** */
void InitPIT(void);


#endif /* COMMON_ACCU_MONITOR_H_ */
