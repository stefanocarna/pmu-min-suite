#include <linux/proc_fs.h>

#include "breader_api.h"
#include "../client/client_module.h"

#define DATA_BUFFER_SIZE (PAGE_SIZE << DATA_BUFFER_ORDER)
#define DATA_BUFFER_SIZE_U64 (DATA_BUFFER_SIZE / sizeof(u64))
#define DATA_BUFFER_LEN (DATA_BUFFER_SIZE / sizeof(u64))

extern u64 *data_buffer;

int init_raw_file(void);
int init_seq_file(void);
int init_mmap_file(void);

void fini_mmap_file(void);
void fini_raw_file(void);
void fini_seq_file(void);
