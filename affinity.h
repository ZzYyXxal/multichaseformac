#ifdef __APPLE__
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <mach/mach_types.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>

#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"

#define __CPU_SETSIZE	1024
#define CPU_SETSIZE __CPU_SETSIZE

typedef struct cpu_set {
  uint32_t    count;
} cpu_set_t;

static inline void
CPU_ZERO(cpu_set_t *cs) { cs->count = 0; }

static inline void
CPU_SET(int num, cpu_set_t *cs) { cs->count |= (1 << num); }

static inline int
CPU_ISSET(int num, cpu_set_t *cs) { return (cs->count & (1 << num)); }

int sched_getaffinity(pid_t pid, size_t cpu_size, cpu_set_t *cpu_set);

int pthread_setaffinity_np(pthread_t thread, size_t cpu_size,
                           cpu_set_t *cpu_set);

#endif // __APPLE__