#include "mdmout.h"

#define SYM_PER_BLOCK 8

#define SMP_PER_SYM 4
#define TMP_BIT_BUF_SIZE (SYM_PER_BLOCK + 1)
#define TMP_SMP_BUF_SIZE (TMP_BIT_BUF_SIZE * SMP_PER_SYM)

#define FLAG_OFF 0
#define FLAG_ON  1

//DataOutFlag values:

#define V32_DATA_OFF 0
#define V32_DATA_START  1
#define V32_DATA_ON 2
#define V32_DATA_IDLE 3
#define V32_DATA_RETRAIN 4

#define BIT_BUF_BLOCK_SIZE	 (SYM_PER_BLOCK>>1)



#define V32_ENABLE_RESYNC	1
#define V32_RESYNC_ENABLED	2
#define V32_ENABLE_RETRAIN  3

#ifdef V32_NO_VITERBI
#define V32_MODEM_FOR_VOICE
#endif