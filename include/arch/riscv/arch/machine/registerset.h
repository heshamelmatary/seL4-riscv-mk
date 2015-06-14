#ifndef __ARCH_MACHINE_REGISTERSET_H
#define __ARCH_MACHINE_REGISTERSET_H

#include "hardware.h"

/* Offsets within the user context, these need to match the order in
 * register_t below */
#define PT_LR_svc           (1 * 4)

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <util.h>
#include <arch/types.h>

 /* FIXME: For now just dump all registers */
enum _register {

  x1 = 1, ra = 1, LR = 1,

  x2 = 2, SP = 2, sp = 2,
  x3,
  x4,

  x5, t0 = 5,
  x6, t1 = 6,
  x7, t2 = 7,
  x8, s0 = 8,
  x9, s1 = 9,

  /* x10-x17 > a0-a7 */
  x10 = 10, a0 = 10, capRegister = 10, badgeRegister = 10,
  x11 = 11,  a1 = 11, msgInfoRegister = 11,
  x12 = 12, a2 = 12,
  x13 = 13, a3 = 13,
  x14 = 14, a4 = 14,
  x15 = 15, a5 = 15,
  x16 = 16, a6 = 16,
  x17 = 17, a7 = 17,

  x18 = 18, s2 = 18,
  x19 = 19, s3 = 19,
  x20 = 20, s4 = 20,
  x21 = 21, s5 = 21,
  x22 = 22, s6 = 22,
  x23 = 23, s7 = 23,
  x24 = 24, s8 = 24,
  x25 = 25, s9 = 25,
  x26 = 26, s10 = 26,
  x27 = 27, s11 = 27,

  x28 = 28, t3 = 28,
  x29 = 29, t4 = 29,
  x30 = 30, t5 = 30,
  x31 = 31, t6 = 31,

  n_contextRegisters
};

typedef uint32_t register_t;

/* FIXME */
enum messageSizes {
    n_msgRegisters = 4,
    n_frameRegisters = 10,
    n_gpRegisters = 7,
    n_exceptionMessage = 3,
    n_syscallMessage = 12
};

extern const register_t msgRegisters[] VISIBLE;
extern const register_t frameRegisters[] VISIBLE;
extern const register_t gpRegisters[] VISIBLE;
extern const register_t exceptionMessage[] VISIBLE;
extern const register_t syscallMessage[] VISIBLE;

/* TODO: Optimize the number of context registers */
struct user_context {
    word_t registers[n_contextRegisters];
};
typedef struct user_context user_context_t;

static inline void Arch_initContext(user_context_t* context)
{
    /*TODO: Status registers, config registers config */
}

static inline word_t CONST
sanitiseRegister(register_t reg, word_t v)
{
  /* FIXME */
	return v;
}

#endif /* __ASSEMBLER__ */

#endif /* !__ARCH_MACHINE_REGISTERSET_H */
