#define QUEUE_SUCCESS   0
#define QUEUE_REJECTED  1

// function prototypes
void execute(void (*somefunction)(void *p), void *p);
int work_submit(void (*somefunction)(void *p), void *p);
void *worker(void *param);
void pool_init(void);
void pool_shutdown(void);
