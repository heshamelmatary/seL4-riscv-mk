/*
 * Copyright 2015 Hesham Almatary <heshamelmatary@gmail.com>
 * Author: Hesham Almatary <heshamelmatary@gmail.com>
 */

#ifndef ARCH_BENCHMARK_H
#define ARCH_BENCHMARK_H

#include <config.h>
#include <arch/object/structures.h>

#ifdef CONFIG_BENCHMARK

extern uint64_t ksEntry;
extern uint64_t ksExit;
extern uint32_t ksLogIndex;
extern uint32_t *ksLog;

static inline uint64_t
timestamp(void)
{
}

#endif /* CONFIG_BENCHMARK */
#endif /* ARCH_BENCHMARK_H */
