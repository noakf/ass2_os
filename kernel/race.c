#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define RACE_MAX_TEAMS 8

struct race_state {
  struct spinlock lock;
  int active;
  int teams;
  int target;
  int scores[RACE_MAX_TEAMS];
  int winner;
};

static struct race_state race;

void
raceinit(void)
{
  initlock(&race.lock, "race");
  race.active = 0;
  race.teams = 0;
  race.target = 0;
  race.winner = -1;

  for(int i = 0; i < RACE_MAX_TEAMS; i++)
    race.scores[i] = 0;
}

int
race_init(int teams, int target)
{
  if(teams <= 0 || teams > RACE_MAX_TEAMS || target <= 0)
    return -1;

  acquire(&race.lock);

  race.active = 1;
  race.teams = teams;
  race.target = target;
  race.winner = -1;

  for(int i = 0; i < RACE_MAX_TEAMS; i++)
    race.scores[i] = 0;

  release(&race.lock);
  return 0;
}

int
race_inc_score(int team)
{
  int score;

  acquire(&race.lock);

  if(!race.active || team < 0 || team >= race.teams){
    release(&race.lock);
    return -1;
  }

  if(race.winner != -1){
    score = race.scores[team];
    release(&race.lock);
    return score;
  }

  race.scores[team]++;
  score = race.scores[team];

  if(score >= race.target)
    race.winner = team;

  release(&race.lock);
  return score;
}

int
race_get_score(int team)
{
  int score;

  acquire(&race.lock);

  if(!race.active || team < 0 || team >= race.teams){
    release(&race.lock);
    return -1;
  }

  score = race.scores[team];

  release(&race.lock);
  return score;
}

int
race_is_finished(void)
{
  int finished;

  acquire(&race.lock);
  finished = race.winner != -1;
  release(&race.lock);

  return finished;
}

int
race_winner(void)
{
  int winner;

  acquire(&race.lock);
  winner = race.winner;
  release(&race.lock);

  return winner;
}