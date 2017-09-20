#include <stdio.h>
#include <stinger_core/stinger.h>
#include <stinger_core/stinger_internal.h>
#include <stinger_core/core_util.h>

#if defined(STINGER_USE_DISTRIBUTED_ALLOCATION)
#include <stinger_core/emu_xmalloc.h>
#endif
#include <dynograph_util.h>
#include <hooks_c.h>

#if defined(__le64__)
#include <memoryweb.h>
#else
#define NODELETS() 1
#define BYTES_PER_NODELET() stinger_max_memsize()
#define replicated
#endif

#include "layout.h"

// Figure out how many edge blocks we can allocate to fill STINGER_MAX_MEMSIZE
// Assumes we need just enough room for nv vertices and puts the rest into edge blocks
// Basically implements calculate_stinger_size() in reverse
struct stinger_config_t
generate_stinger_config(int64_t nv) {
    dynograph_message("Stinger will consume %lu MB of RAM",
        calculate_stinger_size(0, 0, 0, 0).size >> 20);
    struct stinger_config_t config;
    return config;
}

void
insert_batch(struct stinger *S, struct dynograph_edge_batch batch)
{
    stinger_parallel_for (int64_t i = 0; i < batch.num_edges; ++i)
    {
    }
}

void
good_insert_batch(struct stinger *S, struct dynograph_edge_batch batch)
{
    struct dynograph_edge_batch batch1;
    batch1.num_edges = 10;
    stinger_parallel_for (int64_t i = 0; i < batch.num_edges; ++i)
    {
    }
}


replicated struct stinger local_S;

int main(int argc, char *argv[])
{
    struct dynograph_args args = {0};
    dynograph_args_parse(argc, argv, &args);

    dynograph_message("Loading dataset...");
    struct dynograph_dataset* dataset = dynograph_load_dataset(&args);

    int64_t batch_id = 0;
    struct dynograph_edge_batch batch = dynograph_get_batch(dataset, batch_id);
    dynograph_message("Inserting batch %lli", batch_id);

    insert_batch(NULL, batch);

    dynograph_free_dataset(dataset);

    return 0;
}