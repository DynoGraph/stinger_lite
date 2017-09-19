#include <stdio.h>
#include <stinger_core/stinger.h>
#include <stinger_core/stinger_internal.h>
#include <stinger_core/core_util.h>
#include <stinger_alg/bfs.h>
#include <stinger_alg/betweenness.h>
#include <stinger_alg/clustering.h>
#include <stinger_alg/static_components.h>
#include <stinger_alg/kcore.h>
#include <stinger_alg/pagerank.h>

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
delete_edges_older_than(struct stinger *S, int64_t threshold)
{
    STINGER_RAW_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S)
    {
        hooks_traverse_edges(1);
        if (STINGER_EDGE_TIME_RECENT < threshold) {
            // Delete the edge
            update_edge_data_and_direction (S, current_eb__, i__, ~STINGER_EDGE_DEST, 0, 0, STINGER_EDGE_DIRECTION, EDGE_WEIGHT_SET);
        }
    }
    STINGER_RAW_FORALL_EDGES_OF_ALL_TYPES_END();
}

void
insert_batch_serial(struct stinger *S, struct dynograph_edge_batch batch)
{
    const int64_t type = 0;
    const bool directed = batch.directed;

    for (int64_t i = 0; i < batch.num_edges; ++i)
    {
        const struct dynograph_edge *e = &batch.edges[i];
        if (directed)
        {
            stinger_incr_edge     (S, type, e->src, e->dst, e->weight, e->timestamp);
        } else { // undirected
            stinger_incr_edge_pair(S, type, e->src, e->dst, e->weight, e->timestamp);
        }
        hooks_traverse_edges(1);
    }
}

void
insert_batch(struct stinger *S, struct dynograph_edge_batch batch)
{
    // Insert the edges in parallel
    const int64_t type = 0;
    const bool directed = batch.directed;

    #pragma cilk grainsize = 1
    stinger_parallel_for (int64_t i = 0; i < batch.num_edges; ++i)
    {
        const struct dynograph_edge *e = &batch.edges[i];
        if (directed)
        {
            stinger_incr_edge     (S, type, e->src, e->dst, e->weight, e->timestamp);
        } else { // undirected
            stinger_incr_edge_pair(S, type, e->src, e->dst, e->weight, e->timestamp);
        }
        hooks_traverse_edges(1);
    }
}

void
do_remote_insert(void* hint, struct stinger *S, struct dynograph_edge e)
{
    (void)hint; // Unused, just a hint to make us migrate to the right nodelet
    const int64_t type = 0;
    const bool directed = true;
    if (directed)
    {
        stinger_incr_edge     (S, type, e.src, e.dst, e.weight, e.timestamp);
    } else { // undirected
        stinger_incr_edge_pair(S, type, e.src, e.dst, e.weight, e.timestamp);
    }
}


// Chop off half of batch <a> and give to batch <b>
void split_batch(struct dynograph_edge_batch *a, struct dynograph_edge_batch* b)
{
    assert(a != NULL && b != NULL);
    // Give half the edges to batch b
    b->num_edges = a->num_edges / 2;
    // Take them away from batch a
    a->num_edges -= b->num_edges;
    // Set b's pointer
    b->edges = a->edges + a->num_edges;
}

void
insert_batch_with_remote_spawn(struct stinger *S, struct dynograph_edge_batch batch)
{
    MAP_STING(S);
    if (batch.num_edges > 1)
    {
        // Recursively split batch in half and spawn thread to handle other half
        struct dynograph_edge_batch other_batch;
        split_batch(&batch, &other_batch);
        if (other_batch.num_edges > 0) {
            cilk_spawn insert_batch_with_remote_spawn(S, other_batch);
        }
        insert_batch_with_remote_spawn(S, batch);

    } else if (batch.num_edges == 1) {
        // Base case: do the work with one edge
        struct dynograph_edge *edge = batch.edges;
        void * hint = stinger_vertices_vertex_get(vertices, edge->src);
        do_remote_insert(hint, S, *edge);
    } else {
        assert(batch.num_edges != 0);
    }
}


void record_graph_size(struct stinger *S)
{
    int64_t max_active_nv = stinger_max_active_vertex(S);
    int64_t num_edges = stinger_edges_up_to(S, max_active_nv);
    hooks_set_attr_i64("num_vertices", max_active_nv);
    hooks_set_attr_i64("num_edges", num_edges);
    // if (stinger_consistency_check(S, S->max_nv))
    // {
    //     dynograph_error("Consistency check failed");
    // }
}

void record_graph_distribution(struct stinger *S)
{
    #if defined(__le64__)

    dynograph_message("Recording graph distribution");
    int64_t nv = stinger_max_active_vertex(S) + 1;
    int64_t * num_local_edgeblocks = xcalloc(sizeof(int64_t), nv);
    int64_t * num_edgeblocks = xcalloc(sizeof(int64_t), nv);
    int64_t num_edgeblock_migrations = 0;

    // For each edge block...
    MAP_STING(S);

    // For each active vertex...
    for (int64_t source = 0; source < nv; ++source)
    {
        struct stinger_vertex * vertex = stinger_vertices_vertex_get(vertices, source);
        struct stinger_eb * last_eb = NULL;
        // For each edge block...
        for (
            struct stinger_eb * eb = stinger_ebpool_get_eb(S, (eb_index_t)stinger_vertex_edges_get(vertices, source));
            eb != NULL;
            eb = stinger_next_eb(S, eb)
        ) {
            assert(eb->vertexID == source);
            // Count total # of edge blocks
            num_edgeblocks[source] += 1;
            // Count # of edge blocks that are on the same node as their source vertex
            if (pointers_are_on_same_nodelet(vertex, eb))
            {
                num_local_edgeblocks[source] += 1;
            }

            // Count # of times the previous edge block was on the same nodelet
            if (last_eb != NULL && !pointers_are_on_same_nodelet(last_eb, eb))
            {
                num_edgeblock_migrations += 1;
            }
            last_eb = eb;

        }

    }

    // TODO - calculate more statistics here.
    int64_t total_num_local_edgeblocks = 0;
    int64_t total_num_edgeblocks = 0;

    for (int64_t v = 0; v < nv; ++v)
    {
        total_num_edgeblocks += num_edgeblocks[v];
        total_num_local_edgeblocks += num_local_edgeblocks[v];
    }
    double percent_local_edges = 100 * (double)total_num_local_edgeblocks / (double)total_num_edgeblocks;

    hooks_set_attr_u64("total_num_edgeblocks", total_num_edgeblocks);
    hooks_set_attr_u64("total_num_local_edgeblocks", total_num_local_edgeblocks);
    hooks_set_attr_u64("num_edgeblock_migrations", num_edgeblock_migrations);
    hooks_set_attr_f64("percent_local_edges", percent_local_edges);
    xfree(num_local_edgeblocks);
    xfree(num_edgeblocks);

    size_t elements_per_block = 1 << ebpool->pool.log2_elements_per_block;
    for (size_t i = 0; i < NODELETS(); ++i)
    {
        size_t n = ebpool->pool.block_tail[i] - (i * elements_per_block);
        double percent_eb_storage_used = 100 * (double) n / (double) elements_per_block;
        char buf[256];
        sprintf(buf, "percent_eb_storage_used[%i]", (int)i);
        hooks_set_attr_f64(buf, percent_eb_storage_used);
    }

    #endif
}

struct alg
{
    const char *name;
    int64_t data_per_vertex;
    // Number of optional args?
} algs[] = {
    {"all", 4}, // Must be = max(data_per_vertex) of all other algorithms
    {"bfs", 4},
    {"bfs-do", 4},
    {"bc", 2},
    {"clustering", 1},
    {"cc", 1},
    {"kcore", 2},
    {"pagerank", 2}
};

struct alg *
get_alg(const char *name)
{
    struct alg *b = NULL;
    if (!strcmp(name, "")) { return NULL; }
    for (int i = 0; i < sizeof(algs) / sizeof(algs[0]); ++i)
    {
        if (!strcmp(algs[i].name, name))
        {
            b = &algs[i];
            break;
        }
    }
    if (b == NULL)
    {
        dynograph_message("Alg '%s' not implemented!", name);
        dynograph_die();
    }
    return b;
}

struct vertex_degree {
    int64_t vertex_id;
    int64_t degree;
};

int compare_vertex_degree(const void * lhs, const void * rhs)
{
    const struct vertex_degree a = *(struct vertex_degree *)lhs;
    const struct vertex_degree b = *(struct vertex_degree *)rhs;

    // Sort by degree descending...
    if (a.degree > b.degree) return -1;
    if (a.degree < b.degree) return 1;

    // ...then by vertex_id ascending
    if (a.vertex_id < b.vertex_id) return -1;
    if (a.vertex_id > b.vertex_id) return 1;

    return 0;
}

int64_t highest_degree_vertex(const struct stinger *S)
{
    int64_t nv = stinger_max_nv(S);
    if (nv == 0) { return 0; }
    struct vertex_degree *vertex_degrees = xcalloc(sizeof(struct vertex_degree), nv);
    stinger_parallel_for(int64_t v = 0; v < nv; ++v)
    {
        vertex_degrees[v].vertex_id = v;
        vertex_degrees[v].degree = stinger_outdegree_get(S, v);
    }
    qsort(vertex_degrees, nv, sizeof(struct vertex_degree), compare_vertex_degree);
    int64_t vertex_id = vertex_degrees[0].vertex_id;
    free(vertex_degrees);
    return vertex_id;
}

void run_alg(stinger_t * S, const char *alg_name, int64_t num_vertices, void *alg_data, int64_t source_vertex)
{
    int64_t max_nv = stinger_max_nv(S);
    if (false) {}
    else if (!strcmp(alg_name, "bfs"))
    {
        int64_t * marks = (int64_t*)alg_data + 0 * max_nv;
        int64_t * queue = (int64_t*)alg_data + 1 * max_nv;
        int64_t * Qhead = (int64_t*)alg_data + 2 * max_nv;
        int64_t * level = (int64_t*)alg_data + 3 * max_nv;
        int64_t levels = parallel_breadth_first_search (S, num_vertices, source_vertex, marks, queue, Qhead, level);
        if (levels < 5)
        {
            dynograph_message("WARNING: Breadth-first search was only %ld levels. Consider choosing a different source vertex.", levels);
        }
    }
    else if (!strcmp(alg_name, "bfs-do"))
    {
        int64_t * marks = (int64_t*)alg_data + 0 * max_nv;
        int64_t * queue = (int64_t*)alg_data + 1 * max_nv;
        int64_t * Qhead = (int64_t*)alg_data + 2 * max_nv;
        int64_t * level = (int64_t*)alg_data + 3 * max_nv;
        int64_t levels = direction_optimizing_parallel_breadth_first_search (S, num_vertices, source_vertex, marks, queue, Qhead, level);
        if (levels < 5)
        {
            dynograph_message("WARNING: Breadth-first search was only %ld levels. Consider choosing a different source vertex.", levels);
        }
    }
    else if (!strcmp(alg_name, "bc"))
    {
        double *bc =            (double*) alg_data + 0 * max_nv;
        int64_t *found_count =  (int64_t*)alg_data + 1 * max_nv;
        int64_t num_samples = 128; // FIXME Pick samples from highest degree
        sample_search(S, num_vertices, num_samples, bc, found_count);
    }
    else if (!strcmp(alg_name, "clustering"))
    {
        int64_t *num_triangles = (int64_t*) alg_data + 0 * max_nv;
        count_all_triangles(S, num_triangles);
    }
    else if (!strcmp(alg_name, "cc"))
    {
        int64_t *component_map = (int64_t*) alg_data + 0 * max_nv;
        parallel_shiloach_vishkin_components(S, num_vertices, component_map);
    }
    else if (!strcmp(alg_name, "kcore"))
    {
        int64_t *labels = (int64_t*) alg_data + 0 * max_nv;
        int64_t *counts = (int64_t*) alg_data + 1 * max_nv;
        int64_t k = 0;
        kcore_find(S, labels, counts, num_vertices, &k);
    }
    else if (!strcmp(alg_name, "pagerank"))
    {
        double * pagerank_scores =      (double*)alg_data + 0 * max_nv;
        double * pagerank_scores_tmp =  (double*)alg_data + 1 * max_nv;
        page_rank_directed(S, num_vertices, pagerank_scores, pagerank_scores_tmp, 1e-8, 0.85, 100);
    }
    else
    {
        dynograph_error("Algorithm %s not implemented!", alg_name);
    }
}

#define OPTIMIZATION_FLAGS_MAX_NUM 32
#define OPTIMIZATION_FLAGS_BUF_LEN 256
static struct optimization_flag {
    char key [OPTIMIZATION_FLAGS_BUF_LEN];
    char value [OPTIMIZATION_FLAGS_BUF_LEN];
} optimization_flags[OPTIMIZATION_FLAGS_MAX_NUM];

bool optimization_flag_test(const char* key, const char* value)
{
    for (size_t i = 0; i < OPTIMIZATION_FLAGS_MAX_NUM; ++i)
    {
        const struct optimization_flag *entry = &optimization_flags[i];
        // Search for matching key
        if (!strcmp(key, entry->key))
        {
            // Return matching value
            return !strcmp(value, entry->value);
        }
    }
    // Return false if key not present
    return false;
}

void optimization_flag_set(const char* key, const char* value)
{
    for (size_t i = 0; i < OPTIMIZATION_FLAGS_MAX_NUM; ++i)
    {
        struct optimization_flag *entry = &optimization_flags[i];
        // Search for empty row or matching key
        if (entry->key == NULL || !strcmp(key, entry->key)) {
            // Overwrite value, being careful with max length
            strncpy(entry->value, value, OPTIMIZATION_FLAGS_BUF_LEN-1);
            entry->value[OPTIMIZATION_FLAGS_BUF_LEN-1] = '\0';
            return;
        }
    }
    // Couldn't find an available entry
    assert(false && "Ran out of room in optimization_flags table. Try increasing OPTIMIZATION_FLAGS_MAX_NUM");
}

extern char ** environ;

void optimization_flags_init_from_environment()
{
    const size_t MAX_STR_LEN = OPTIMIZATION_FLAGS_BUF_LEN - 1;
    size_t row = 0;
    for (char** p = environ; *p != NULL; ++p)
    {
        // Find the split between key and value
        char* equals_pos = strstr(*p, "=");
        size_t key_len = equals_pos - *p;
        // Test to see if the key starts with "OPTIMIZATION_"
        const char * prefix = "OPTIMIZATION_";
        size_t prefix_len = strlen(prefix);
        if (key_len > prefix_len && !strncmp(*p, prefix, prefix_len))
        {
            struct optimization_flag *entry = &optimization_flags[row];

            // Copy the key into our table (omitting the prefix) and add null terminator
            if (key_len > MAX_STR_LEN) { key_len = MAX_STR_LEN; }
            strncpy(entry->key, *p + prefix_len, key_len - prefix_len);
            entry->key[MAX_STR_LEN] = '\0';
            // Copy the value into our table and add null terminator
            size_t value_len = strlen(equals_pos+1);
            if (value_len > MAX_STR_LEN) { value_len = MAX_STR_LEN; }
            strncpy(entry->value, equals_pos+1, value_len);
            entry->value[MAX_STR_LEN] = '\0';

            printf("Loaded optimization_flag: %s = %s\n", entry->key, entry->value);

            // Move to the next row
            ++row;
            assert(row < OPTIMIZATION_FLAGS_MAX_NUM && "Ran out of room in optimization_flags table. Try increasing OPTIMIZATION_FLAGS_MAX_NUM");
        }
    }
}


replicated struct stinger local_S;

int main(int argc, char *argv[])
{
    struct dynograph_args args = {0};
    dynograph_args_parse(argc, argv, &args);

    optimization_flags_init_from_environment();
    hooks_set_active_region(getenv("HOOKS_ACTIVE_REGION"));

    dynograph_message("Loading dataset...");
    struct dynograph_dataset* dataset = dynograph_load_dataset(&args);
    // Look up the algorithm that will be benchmarked
    struct alg *alg = get_alg(args.alg_names);

    for (int64_t trial = 0; trial < args.num_trials; trial++)
    {
        hooks_set_attr_i64("trial", trial);

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

        dynograph_message("Allocating alg data...");
        // Allocate data structures for the algorithm(s)
        void *alg_data = NULL;
        if (alg != NULL) {
            #if defined(STINGER_USE_CONTIGUOUS_ALLOCATION)
               alg_data = xcalloc(sizeof(int64_t) * alg->data_per_vertex, nv);
            #elif defined(STINGER_USE_DISTRIBUTED_ALLOCATION)
               alg_data = xmw_malloc1d(alg->data_per_vertex * nv);
            #endif
        }

        // Run the algorithm(s) after each inserted batch
        int64_t epoch = 0;
        for (int64_t batch_id = 0; batch_id < dataset->num_batches; ++batch_id)
        {
            hooks_set_attr_i64("batch", batch_id);
            hooks_set_attr_i64("epoch", epoch);

            struct dynograph_edge_batch batch = dynograph_get_batch(dataset, batch_id);

            int64_t threshold = dynograph_get_timestamp_for_window(dataset, &batch);

            if (args.window_size != 1.0 && args.sort_mode != SNAPSHOT)
            {
                record_graph_size(S);

                dynograph_message("Deleting edges older than %lli", threshold);
                hooks_region_begin("deletions");
                delete_edges_older_than(S, threshold);
                hooks_region_end();
            }

            record_graph_size(S);

            dynograph_message("Inserting batch %lli", batch_id);
            hooks_region_begin("insertions");

            if (optimization_flag_test("INSERT_MODE", "REMOTE_SPAWN"))
            {
                dynograph_message("Using REMOTE_SPAWN algorithm");
                insert_batch_with_remote_spawn(S, batch);
            } else if (optimization_flag_test("INSERT_MODE", "SERIAL")) {
                dynograph_message("Using SERIAL algorithm");
                insert_batch_with_remote_spawn(S, batch);
            } else {
                dynograph_message("Using default algorithm");
                insert_batch(S, batch);
            }
            hooks_region_end();

            record_graph_distribution(S);

            if (dynograph_enable_algs_for_batch(dataset, batch_id))
            {
                record_graph_size(S);
                const char *alg_name = args.alg_names;
                // FIXME implement multiple algs
                //for (std::string alg_name : args.alg_names)
                if (alg != NULL)
                {
                    // TODO implement multiple sources for BC
                    int64_t num_sources = 1;
                    int64_t source_vertex = highest_degree_vertex(S);
                    hooks_set_attr_i64("source_vertex", source_vertex);
                    dynograph_message("Running %s", alg_name);
                    hooks_region_begin(alg_name);
                    run_alg(S, alg_name, nv, alg_data, source_vertex);
                    hooks_region_end();
                }
                epoch += 1;
                assert(epoch <= args.num_epochs);
            }

        }
        assert(epoch == args.num_epochs);
        dynograph_message("Shutting down stinger...");
        #if defined(__le64__)
        stinger_free(alloced_S);
        #else
        stinger_free(S);
        #endif
    }

    dynograph_free_dataset(dataset);

    return 0;
}