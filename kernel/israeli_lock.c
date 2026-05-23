#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define NISRAELI 15
#define ISRAELI_MAX_WAITERS 16

struct israeli_lock {
  uint active;
  uint locked;

  int pid;
  int gid;
  int favoritism;

  struct spinlock lk;

  struct proc *queue[ISRAELI_MAX_WAITERS];
  int qsize;
};

static struct israeli_lock ilocks[NISRAELI];

void
israeliinit(void)
{
  for(int i = 0; i < NISRAELI; i++){
    ilocks[i].active = 0;
    ilocks[i].locked = 0;
    ilocks[i].pid = -1;
    ilocks[i].gid = -1;
    ilocks[i].favoritism = 0;
    ilocks[i].qsize = 0;
    initlock(&ilocks[i].lk, "israeli_lock");
  }
}

static void
remove_from_queue(struct israeli_lock *l, int index)
{
  for(int i = index; i < l->qsize - 1; i++)
    l->queue[i] = l->queue[i + 1];

  l->queue[l->qsize - 1] = 0;
  l->qsize--;
}

static int
choose_next(struct israeli_lock *l, int releasing_gid)
{
  int same_gid_index = -1;

  for(int i = 0; i < l->qsize; i++){
    if(l->queue[i] && l->queue[i]->gid == releasing_gid){
      same_gid_index = i;
      break;
    }
  }

  if(same_gid_index != -1){
    if((lcg_rand() % 100) < l->favoritism)
      return same_gid_index;
  }

  return 0;
}

int
israeli_create(int favoritism)
{
  if(favoritism < 0 || favoritism > 100)
    return -1;

  for(int i = 0; i < NISRAELI; i++){
    acquire(&ilocks[i].lk);

    if(ilocks[i].active == 0){
      ilocks[i].active = 1;
      ilocks[i].locked = 0;
      ilocks[i].pid = -1;
      ilocks[i].gid = -1;
      ilocks[i].favoritism = favoritism;
      ilocks[i].qsize = 0;

      for(int j = 0; j < ISRAELI_MAX_WAITERS; j++)
        ilocks[i].queue[j] = 0;

      release(&ilocks[i].lk);
      return i;
    }

    release(&ilocks[i].lk);
  }

  return -1;
}

int
israeli_acquire(int lock_id)
{
  struct proc *p = myproc();

  if(lock_id < 0 || lock_id >= NISRAELI)
    return -1;

  struct israeli_lock *l = &ilocks[lock_id];

  acquire(&l->lk);

  if(l->active == 0){
    release(&l->lk);
    return -1;
  }

  if(l->locked == 0 && l->qsize == 0){
    l->locked = 1;
    l->pid = p->pid;
    l->gid = p->gid;
    release(&l->lk);
    return 0;
  }

  if(l->qsize >= ISRAELI_MAX_WAITERS){
    release(&l->lk);
    return -1;
  }

  l->queue[l->qsize++] = p;

  while(l->active && l->pid != p->pid)
    sleep(p, &l->lk);

  if(l->active == 0){
    release(&l->lk);
    return -1;
  }

  release(&l->lk);
  return 0;
}

int
israeli_release(int lock_id)
{
  struct proc *p = myproc();

  if(lock_id < 0 || lock_id >= NISRAELI)
    return -1;

  struct israeli_lock *l = &ilocks[lock_id];

  acquire(&l->lk);

  if(l->active == 0 || l->locked == 0 || l->pid != p->pid){
    release(&l->lk);
    return -1;
  }

  int releasing_gid = l->gid;

  if(l->qsize == 0){
    l->locked = 0;
    l->pid = -1;
    l->gid = -1;
    release(&l->lk);
    return 0;
  }

  int chosen_index = choose_next(l, releasing_gid); 
  struct proc *next = l->queue[chosen_index];

  remove_from_queue(l, chosen_index);

  l->locked = 1;
  l->pid = next->pid;
  l->gid = next->gid;

  wakeup(next);

  release(&l->lk);
  return 0;
}

int
israeli_destroy(int lock_id)
{
  if(lock_id < 0 || lock_id >= NISRAELI)
    return -1;

  struct israeli_lock *l = &ilocks[lock_id];

  acquire(&l->lk);

  if(l->active == 0 || l->locked || l->qsize > 0){
    release(&l->lk);
    return -1;
  }

  l->active = 0;
  l->favoritism = 0;
  l->pid = -1;
  l->gid = -1;

  release(&l->lk);
  return 0;
}