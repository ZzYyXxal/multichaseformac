#ifdef __APPLE__

#include "random_r.h"

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#define	TYPE_0		0
#define	BREAK_0		8
#define	DEG_0		0
#define	SEP_0		0

/* x**7 + x**3 + 1.  */
#define	TYPE_1		1
#define	BREAK_1		32
#define	DEG_1		7
#define	SEP_1		3

/* x**15 + x + 1.  */
#define	TYPE_2		2
#define	BREAK_2		64
#define	DEG_2		15
#define	SEP_2		1

/* x**31 + x**3 + 1.  */
#define	TYPE_3		3
#define	BREAK_3		128
#define	DEG_3		31
#define	SEP_3		3

/* x**63 + x + 1.  */
#define	TYPE_4		4
#define	BREAK_4		256
#define	DEG_4		63
#define	SEP_4		1


/* Array versions of the above information to make code run faster.
   Relies on fact that TYPE_i == i.  */

#define	MAX_TYPES	5	/* Max number of types above.  */

struct random_poly_info
{
  int seps[MAX_TYPES];
  int degrees[MAX_TYPES];
};

static const struct random_poly_info random_poly_info =
{
  { SEP_0, SEP_1, SEP_2, SEP_3, SEP_4 },
  { DEG_0, DEG_1, DEG_2, DEG_3, DEG_4 }
};

/* If we are using the trivial TYPE_0 R.N.G., just do the old linear
   congruential bit.  Otherwise, we do our fancy trinomial stuff, which is the
   same in all the other cases due to all the global variables that have been
   set up.  The basic operation is to add the number at the rear pointer into
   the one at the front pointer.  Then both pointers are advanced to the next
   location cyclically in the table.  The value returned is the sum generated,
   reduced to 31 bits by throwing away the "least random" low bit.
   Note: The code takes advantage of the fact that both the front and
   rear pointers can't wrap on the same call by not testing the rear
   pointer if the front one has wrapped.  Returns a 31-bit random number.  */

int
__random_r (struct random_data *buf, int32_t *result)
{
  int32_t *state;

  if (buf == NULL || result == NULL)
    goto fail;

  state = buf->state;

  if (buf->rand_type == TYPE_0)
    {
      int32_t val = ((state[0] * 1103515245U) + 12345U) & 0x7fffffff;
      state[0] = val;
      *result = val;
    }
  else
    {
      int32_t *fptr = buf->fptr;
      int32_t *rptr = buf->rptr;
      int32_t *end_ptr = buf->end_ptr;
      uint32_t val;

      val = *fptr += (uint32_t) *rptr;
      /* Chucking least random bit.  */
      *result = val >> 1;
      ++fptr;
      if (fptr >= end_ptr)
	{
	  fptr = state;
	  ++rptr;
	}
      else
	{
	  ++rptr;
	  if (rptr >= end_ptr)
	    rptr = state;
	}
      buf->fptr = fptr;
      buf->rptr = rptr;
    }
  return 0;

 fail:
  printf("failed random\n");
  return -1;
}

int
random_r (struct random_data *buf, int32_t *result)
{
    return __random_r (buf, result);
}


/* Initialize the random number generator based on the given seed.  If the
   type is the trivial no-state-information type, just remember the seed.
   Otherwise, initializes state[] based on the given "seed" via a linear
   congruential generator.  Then, the pointers are set to known locations
   that are exactly rand_sep places apart.  Lastly, it cycles the state
   information a given number of times to get rid of any initial dependencies
   introduced by the L.C.R.N.G.  Note that the initialization of randtbl[]
   for default usage relies on values produced by this routine.  */
int
__srandom_r (unsigned int seed, struct random_data *buf)
{
  int type;
  int32_t *state;
  long int i;
  int32_t word;
  int32_t *dst;
  int kc;

  if (buf == NULL)
    goto fail;
  type = buf->rand_type;
  if ((unsigned int) type >= MAX_TYPES)
    goto fail;

  state = buf->state;
  /* We must make sure the seed is not 0.  Take arbitrarily 1 in this case.  */
  if (seed == 0)
    seed = 1;
  state[0] = seed;
  if (type == TYPE_0)
    goto done;

  dst = state;
  word = seed;
  kc = buf->rand_deg;
  for (i = 1; i < kc; ++i)
    {
      /* This does:
	   state[i] = (16807 * state[i - 1]) % 2147483647;
	 but avoids overflowing 31 bits.  */
      long int hi = word / 127773;
      long int lo = word % 127773;
      word = 16807 * lo - 2836 * hi;
      if (word < 0)
	word += 2147483647;
      *++dst = word;
    }

  buf->fptr = &state[buf->rand_sep];
  buf->rptr = &state[0];
  kc *= 10;
  while (--kc >= 0)
    {
      int32_t discard;
      (void) __random_r (buf, &discard);
    }

 done:
  return 0;

 fail:
  printf("failed random\n");
  return -1;
}

int
srandom_r (unsigned int seed, struct random_data *buf)
{
    return __srandom_r (seed, buf);
}


/* Initialize the state information in the given array of N bytes for
   future random number generation.  Based on the number of bytes we
   are given, and the break values for the different R.N.G.'s, we choose
   the best (largest) one we can and set things up for it.  srandom is
   then called to initialize the state information.  Note that on return
   from srandom, we set state[-1] to be the type multiplexed with the current
   value of the rear pointer; this is so successive calls to initstate won't
   lose this information and will be able to restart with setstate.
   Note: The first thing we do is save the current state, if any, just like
   setstate so that it doesn't matter when initstate is called.
   Returns 0 on success, non-zero on failure.  */
int
__initstate_r (unsigned int seed, char *arg_state, size_t n,
	       struct random_data *buf)
{
  if (buf == NULL)
    goto fail;

  int32_t *old_state = buf->state;
  if (old_state != NULL)
    {
      int old_type = buf->rand_type;
      if (old_type == TYPE_0)
	old_state[-1] = TYPE_0;
      else
	old_state[-1] = (MAX_TYPES * (buf->rptr - old_state)) + old_type;
    }

  int type;
  if (n >= BREAK_3)
    type = n < BREAK_4 ? TYPE_3 : TYPE_4;
  else if (n < BREAK_1)
    {
      if (n < BREAK_0)
	goto fail;

      type = TYPE_0;
    }
  else
    type = n < BREAK_2 ? TYPE_1 : TYPE_2;

  int degree = random_poly_info.degrees[type];
  int separation = random_poly_info.seps[type];

  buf->rand_type = type;
  buf->rand_sep = separation;
  buf->rand_deg = degree;
  int32_t *state = &((int32_t *) arg_state)[1];	/* First location.  */
  /* Must set END_PTR before srandom.  */
  buf->end_ptr = &state[degree];

  buf->state = state;

  __srandom_r (seed, buf);

  state[-1] = TYPE_0;
  if (type != TYPE_0)
    state[-1] = (buf->rptr - state) * MAX_TYPES + type;

  return 0;

 fail:
  printf("failed random\n");
  return -1;
}

int
initstate_r (unsigned int seed, char *arg_state, size_t n,
	       struct random_data *buf)
{
    return __initstate_r (seed, arg_state, n, buf);
}


/* Restore the state from the given state array.
   Note: It is important that we also remember the locations of the pointers
   in the current state information, and restore the locations of the pointers
   from the old state information.  This is done by multiplexing the pointer
   location into the zeroth word of the state information. Note that due
   to the order in which things are done, it is OK to call setstate with the
   same state as the current state
   Returns 0 on success, non-zero on failure.  */
int
__setstate_r (char *arg_state, struct random_data *buf)
{
  int32_t *new_state = 1 + (int32_t *) arg_state;
  int type;
  int old_type;
  int32_t *old_state;
  int degree;
  int separation;

  if (arg_state == NULL || buf == NULL)
    goto fail;

  old_type = buf->rand_type;
  old_state = buf->state;
  if (old_type == TYPE_0)
    old_state[-1] = TYPE_0;
  else
    old_state[-1] = (MAX_TYPES * (buf->rptr - old_state)) + old_type;

  type = new_state[-1] % MAX_TYPES;
  if (type < TYPE_0 || type > TYPE_4)
    goto fail;

  buf->rand_deg = degree = random_poly_info.degrees[type];
  buf->rand_sep = separation = random_poly_info.seps[type];
  buf->rand_type = type;

  if (type != TYPE_0)
    {
      int rear = new_state[-1] / MAX_TYPES;
      buf->rptr = &new_state[rear];
      buf->fptr = &new_state[(rear + separation) % degree];
    }
  buf->state = new_state;
  /* Set end_ptr too.  */
  buf->end_ptr = &new_state[degree];

  return 0;

 fail:
  printf("failed random\n");
  return -1;
}

int
setstate_r (char *arg_state, struct random_data *buf)
{
    return __setstate_r (arg_state, buf);
}


#endif  // __APPLE__