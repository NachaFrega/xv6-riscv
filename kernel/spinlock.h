#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "types.h"

// Mutual exclusion lock.
struct spinlock {
  uint locked;        // Indica si el lock está activo.

  // Para depuración:
  char *name;         // Nombre del lock.
  struct cpu *cpu;    // La CPU que tiene el lock actualmente.
};

// Declaraciones de funciones para manipular spinlocks
void initlock(struct spinlock *lock, char *name);
void acquire(struct spinlock *lock);
void release(struct spinlock *lock);
int holding(struct spinlock *lock);

#endif // SPINLOCK_H
