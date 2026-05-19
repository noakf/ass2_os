#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

static uint lcg_state = 1;
static struct spinlock lcg_lock;

void
lcginit(void)
{
  initlock(&lcg_lock, "lcg");
}

void
lcg_srand(uint seed)
{
  acquire(&lcg_lock);
  lcg_state = seed;
  release(&lcg_lock);
}

uint
lcg_rand(void)
{
  uint next;

  acquire(&lcg_lock);
  lcg_state = (1664525 * lcg_state + 1013904223);
  next = lcg_state;
  release(&lcg_lock);

  return next;
}