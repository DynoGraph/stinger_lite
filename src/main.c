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

    // Start with size we will try to fill
    // Scaled by 75% because that's what stinger_new_full does
    // HACK scaling by 25% just so we don't bump into any memory limits
    size_t sz = ((uint64_t)stinger_max_memsize() * 1)/(4*NODELETS());
    dynograph_message("Actual memory size: %lu x %lu MB", NODELETS(), BYTES_PER_NODELET() >> 20);
    dynograph_message("Detected memory size as %lu MB", stinger_max_memsize() >> 20, sz);

    // Subtract storage for vertices
    sz -= stinger_vertices_size(nv);
    sz -= stinger_physmap_size(nv);

    // Assume just one etype and vtype
    int64_t netypes = 1;
    int64_t nvtypes = 1;
    sz -= stinger_names_size(netypes);
    sz -= stinger_names_size(nvtypes);

    // Leave room for the edge block tracking structures
    sz -= sizeof(struct stinger_ebpool);
    sz -= sizeof(struct stinger_etype_array);

    // Finally, calculate how much room is left for the edge blocks themselves
    int64_t nebs = sz / (sizeof(struct stinger_eb) + sizeof(eb_index_t));

    nebs = 16 * 1024;

    struct stinger_config_t config = {
            nv,
            nebs,
            netypes,
            nvtypes,
            0, //size_t memory_size;
            0, //uint8_t no_map_none_etype;
            0, //uint8_t no_map_none_vtype;
            1, //uint8_t no_resize;
    };

    dynograph_message("Configuring stinger storage for %lu vertices and %lu edges",
        config.nv, config.nebs * STINGER_EDGEBLOCKSIZE);
    dynograph_message("Stinger will consume %lu MB of RAM",
        calculate_stinger_size(nv, nebs, netypes, nvtypes).size >> 20);

    return config;
}

void
insert_batch(struct stinger *S, struct dynograph_edge_batch batch)
{
    // #pragma cilk grainsize = 1
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

    int64_t nv = dataset->max_vertex_id + 1;
    struct stinger_config_t config = generate_stinger_config(nv);

    dynograph_message("Initializing stinger...");
    struct stinger *S = stinger_new_full(&config);

    #if defined(__le64__)
    dynograph_message("Replicating stinger struct...");
    /*
        The stinger struct contains the pointers to each distributed data structure.
        All threads will need to access it, and it won't change after allocation.
        So we replicate it to each nodelet to avoid unnecessary migrations
    */
    // For each nodelet...
    for (size_t i = 0; i < NODELETS(); ++i)
    {
        // Get a pointer to the storage reserved for the local copy of the stinger struct
        struct stinger *local_ptr = mw_get_nth(&local_S, i);
        // Fill it with valid pointers
        memcpy(local_ptr, S, sizeof(struct stinger));
    }
    // Keep track of the original allocation so we can free it
    struct stinger *alloced_S = S;
    // From now on always use the local copy of S
    S = &local_S;
    #endif

    // Run the algorithm(s) after each inserted batch
    int64_t epoch = 0;
    for (int64_t batch_id = 0; batch_id < dataset->num_batches; ++batch_id)
    {
        struct dynograph_edge_batch batch = dynograph_get_batch(dataset, batch_id);
        dynograph_message("Inserting batch %lli", batch_id);

        insert_batch(S, batch);
    }

    dynograph_message("Shutting down stinger...");
    #if defined(__le64__)
    stinger_free(alloced_S);
    #else
    stinger_free(S);
    #endif

    dynograph_free_dataset(dataset);

    return 0;
}