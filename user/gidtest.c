#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  printf("initial gid = %d\n", getgid());

  setgid(7);

  printf("new gid = %d\n", getgid());

  exit(0);
}