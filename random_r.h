#ifdef __APPLE__

#include <stdio.h>
struct random_data
  {
    int32_t *fptr;		/* Front pointer.  */
    int32_t *rptr;		/* Rear pointer.  */
    int32_t *state;		/* Array of state values.  */
    int rand_type;		/* Type of random number generator.  */
    int rand_deg;		/* Degree of random number generator.  */
    int rand_sep;		/* Distance between front and rear.  */
    int32_t *end_ptr;		/* Pointer behind state table.  */
  };

int random_r (struct random_data *__restrict __buf,
		     int32_t *__restrict __result);

int srandom_r (unsigned int __seed, struct random_data *__buf);

int initstate_r (unsigned int __seed, char *__restrict __statebuf,
			size_t __statelen,
			struct random_data *__restrict __buf);

int setstate_r (char *__restrict __statebuf,
		       struct random_data *__restrict __buf); 
#endif