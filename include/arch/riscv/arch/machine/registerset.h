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
    X0 = 0, /* Hardwired to zero */

    X1 = 1,
    LR = 1,

    X2 = 2,
    X3 = 3,
    X4 = 4,
    X5 = 5,
    X6 = 6,
    X7 = 7,
    X8 = 8,
    X9 = 9,
    X10 = 10,
    X11 = 11,
    X12 = 12,
    X13 = 13,
   
    X14 = 14,
    SP  = 14,
    
    X15 = 15,
    X16 = 16,
    X17 = 17,
    
    /* a0-a7 function arguments */
    X18 = 18,
    capRegister = 18,
    badgeRegister = 18,
    
    X19 = 19,
    msgInfoRegister = 19,
      
    X20 = 20,
    X21 = 21,
    X22 = 22,
    X23 = 23,
    X24 = 24,
    X25 = 25,

    X26 = 26,
    X27 = 27,
    X28 = 28,
    X29 = 29,
    X30 = 30,
    X31 = 31,
    
    /* TODO: Add supervison registers here */
   
   // LR_svc = 15,
   /* Status, Config registers */
   
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
}

#endif /* __ASSEMBLER__ */

#endif /* !__ARCH_MACHINE_REGISTERSET_H */
