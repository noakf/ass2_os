#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TEAMS 3
#define RUNNERS_PER_TEAM 5
#define TARGET_SCORE 30

void
run_race(int favoritism)
{
  int lock_id = israeli_create(favoritism);

  if(lock_id < 0){
    printf("failed to create israeli lock\n");
    exit(1);
  }

  if(race_init(TEAMS, TARGET_SCORE) < 0){
    printf("failed to init race\n");
    israeli_destroy(lock_id);
    exit(1);
  }

  printf("\n=== Relay race with favoritism %d ===\n", favoritism);

  for(int i = 0; i < RUNNERS_PER_TEAM; i++){
    for(int team = 0; team < TEAMS; team++){
      int pid = fork();

      if(pid < 0){
        printf("fork failed\n");
        exit(1);
      }

      if(pid == 0){
        setgid(team);

        while(!race_is_finished()){
          if(israeli_acquire(lock_id) == 0){
            if(!race_is_finished()){
              int score = race_inc_score(getgid());

              printf("Runner %d (Team %d) acquired the baton\n", getpid(), getgid());
              printf("Team %d score = %d\n", getgid(), score);
            }

            israeli_release(lock_id);
          }

          pause(2);
        }

        exit(0);
      }
    }
  }

  for(int i = 0; i < TEAMS * RUNNERS_PER_TEAM; i++)
    wait(0);

  printf("\nFinal scores for favoritism %d:\n", favoritism);

  for(int team = 0; team < TEAMS; team++)
    printf("Team %d final score = %d\n", team, race_get_score(team));

  printf("Winner: Team %d\n", race_winner());

  israeli_destroy(lock_id);
}

int
main(void)
{
  run_race(0);
  run_race(50);
  run_race(100);

  exit(0);
}