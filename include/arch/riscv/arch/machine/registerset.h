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

  x2,
  x3,
  x4,
  x5,
  x6,
  x7,
  x8,
  x9,
  x10,
  x11,
  x12,
  x13,
  x14 = 14, SP = 14, sp = 14,
  x15,

  /* v0-v1 -> x16-x17 return values */
  x16 = 16, v0 = 16,
  x17 = 17, v1 = 17,

  /* a0-a7 -> x18-x25 function arguments */
  x18 = 18, a0 = 18, capRegister = 18, badgeRegister = 18,
  x19 = 19, a1 = 19, msgInfoRegister = 19,
  x20 = 20, a2 = 20,
  x21 = 21, a3 = 21,
  x22 = 22, a4 = 22,
  x23 = 23, a5 = 23,
  x24 = 24, a6 = 24,
  x25 = 25, a7 = 25,

  /* x26-x30 -> t0-t4 Temporaries */
  x26 = 26, t0 = 26,
  x27 = 27, t1 = 27,
  x28 = 28, t2 = 28,
  x29 = 29, t3 = 29,
  x30 = 30, t4 = 30,

  x31 = 31, gp = 31,
   n_contextRegisters
};

typedef uint32_t register_t;

/* FIXME */
enum messageSizes {
    n_msgRegisters = 4,
    n_frameRegisters = 10,
    n_gpRegisters = 7,
    n_exceptionMessage = 3,
    n_syscallMessage = 12,
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
  /* TODO */
	return 0;
}

#endif /* __ASSEMBLER__ */

#endif /* !__ARCH_MACHINE_REGISTERSET_H */
