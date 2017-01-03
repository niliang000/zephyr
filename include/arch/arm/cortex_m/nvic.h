/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Nvic.c - ARM CORTEX-M Series Nested Vector Interrupt Controller
 *
 * Provide an interface to the Nested Vectored Interrupt Controller found on
 * ARM Cortex-M processors.
 *
 * The API does not account for all possible usages of the NVIC, only the
 * functionalities needed by the kernel.
 *
 * The same effect can be achieved by directly writing in the registers of the
 * NVIC, with the layout available from scs.h, using the __scs.nvic data
 * structure (or hardcoded values), but these APIs are less error-prone,
 * especially for registers with multiple instances to account for potentially
 * 240 interrupt lines. If access to a missing functionality is needed, this is
 * the way to implement it.
 *
 * Supports up to 240 IRQs and 256 priority levels.
 */

#ifndef _NVIC_H_
#define _NVIC_H_

#include <arch/arm/cortex_m/scs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* for assembler, only works with constants */
#define _EXC_PRIO(pri) (((pri) << (8 - CONFIG_NUM_IRQ_PRIO_BITS)) & 0xff)

#ifdef CONFIG_ZERO_LATENCY_IRQS
#define _ZERO_LATENCY_IRQS_RESERVED_PRIO 1
#else
#define _ZERO_LATENCY_IRQS_RESERVED_PRIO 0
#endif

#if defined(CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS) || \
	defined(CONFIG_CPU_CORTEX_M_HAS_BASEPRI)
#define _EXCEPTION_RESERVED_PRIO 1
#else
#define _EXCEPTION_RESERVED_PRIO 0
#endif

#define _IRQ_PRIO_OFFSET \
	(_ZERO_LATENCY_IRQS_RESERVED_PRIO + \
	 _EXCEPTION_RESERVED_PRIO)

#define _EXC_IRQ_DEFAULT_PRIO _EXC_PRIO(_IRQ_PRIO_OFFSET)

#define _EXC_SVC_PRIO 0
#define _EXC_FAULT_PRIO 0

/* no exc #0 */
#define _EXC_RESET 1
#define _EXC_NMI 2
#define _EXC_HARD_FAULT 3
#define _EXC_MPU_FAULT 4
#define _EXC_BUS_FAULT 5
#define _EXC_USAGE_FAULT 6
/* 7-10 reserved */
#define _EXC_SVC 11
#define _EXC_DEBUG 12
/* 13 reserved */
#define _EXC_PENDSV 14
#define _EXC_SYSTICK 15
/* 16+ IRQs */

#define _NUM_EXC 16

#define NUM_IRQS_PER_REG 32
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)

#if !defined(_ASMLANGUAGE)

#include <stdint.h>
#include <misc/__assert.h>

/**
 *
 * @brief Enable an IRQ
 *
 * Enable IRQ #@a irq, which is equivalent to exception #@a irq+16
 *
 * @param irq IRQ number
 *
 * @return N/A
 */

static inline void _NvicIrqEnable(unsigned int irq)
{
	__scs.nvic.iser[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Find out if an IRQ is enabled
 *
 * Find out if IRQ #@a irq is enabled.
 *
 * @param irq IRQ number
 * @return 1 if IRQ is enabled, 0 otherwise
 */

static inline int _NvicIsIrqEnabled(unsigned int irq)
{
	return __scs.nvic.iser[REG_FROM_IRQ(irq)] & (1 << BIT_FROM_IRQ(irq));
}

/**
 *
 * @brief Disable an IRQ
 *
 * Disable IRQ #@a irq, which is equivalent to exception #@a irq+16
 * @param irq IRQ number
 * @return N/A
 */

static inline void _NvicIrqDisable(unsigned int irq)
{
	__scs.nvic.icer[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Pend an IRQ
 *
 * Pend IRQ #@a irq, which is equivalent to exception #@a irq+16. CPU will handle
 * the IRQ when interrupts are enabled and/or returning from a higher priority
 * interrupt.
 * @param irq IRQ number
 *
 * @return N/A
 */

static inline void _NvicIrqPend(unsigned int irq)
{
	__scs.nvic.ispr[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Find out if an IRQ is pending
 *
 * Find out if IRQ #@a irq is pending
 *
 * @param irq IRQ number
 * @return 1 if IRQ is pending, 0 otherwise
 */

static inline int _NvicIsIrqPending(unsigned int irq)
{
	return __scs.nvic.ispr[REG_FROM_IRQ(irq)] & (1 << BIT_FROM_IRQ(irq));
}

/**
 *
 * @brief Unpend an IRQ
 *
 * Unpend IRQ #@a irq, which is equivalent to exception #@a irq+16. The previously
 * pending interrupt will be ignored when either unlocking interrupts or
 * returning from a higher priority exception.
 *
 * @param irq IRQ number
 * @return N/A
 */

static inline void _NvicIrqUnpend(unsigned int irq)
{
	__scs.nvic.icpr[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Set priority of an IRQ
 *
 * Set priority of IRQ #@a irq to @a prio. There are 256 priority levels.
 *
 * @param irq IRQ number
 * @param prio Priority
 * @return N/A
 */

static inline void _NvicIrqPrioSet(unsigned int irq, uint8_t prio)
{
#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
	volatile uint32_t * const ipr = &__scs.nvic.ipr[_PRIO_IP_IDX(irq)];
	*ipr = ((*ipr & ~((uint32_t)0xff << _PRIO_BIT_SHIFT(irq))) |
		((uint32_t)prio << _PRIO_BIT_SHIFT(irq)));
#else
	__scs.nvic.ipr[irq] = prio;
#endif /* CONFIG_CPU_CORTEX_M0_M0PLUS */
}

/**
 *
 * @brief Get priority of an IRQ
 *
 * Get priority of IRQ #@a irq.
 *
 * @param irq IRQ number
 *
 * @return the priority level of the IRQ
 */

static inline uint8_t _NvicIrqPrioGet(unsigned int irq)
{
#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
	return (__scs.nvic.ipr[_PRIO_IP_IDX(irq)] >> _PRIO_BIT_SHIFT(irq));
#else
	return __scs.nvic.ipr[irq];
#endif /* CONFIG_CPU_CORTEX_M0_M0PLUS */
}

#if !defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
/**
 *
 * @brief Trigger an interrupt via software
 *
 * Trigger interrupt #@a irq. The CPU will handle the IRQ when interrupts are
 * enabled and/or returning from a higher priority interrupt.
 *
 * @param irq IRQ number
 * @return N/A
 */

static inline void _NvicSwInterruptTrigger(unsigned int irq)
{
#if defined(CONFIG_SOC_TI_LM3S6965_QEMU)
	/* the QEMU does not simulate the STIR register: this is a workaround */
	_NvicIrqPend(irq);
#else
	__scs.stir = irq;
#endif
}
#endif /* CONFIG_CPU_CORTEX_M0_M0PLUS */

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _NVIC_H_ */
