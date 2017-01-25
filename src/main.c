#include <stdio.h>
#include <stinger_core/stinger.h>
#include <stinger_core/stinger_internal.h>
#include <dynograph_util.h>
#include <hooks_c.h>

// Figure out how many edge blocks we can allocate to fill STINGER_MAX_MEMSIZE
// Assumes we need just enough room for nv vertices and puts the rest into edge blocks
// Basically implements calculate_stinger_size() in reverse
struct stinger_config_t
generate_stinger_config(int64_t nv) {

    // Start with size we will try to fill
    // Scaled by 75% because that's what stinger_new_full does
    //uint64_t sz = ((uint64_t)stinger_max_memsize() * 3)/4;
    uint64_t sz = 1024 * 1024; // FIXME pick memory size

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

    return config;
}

void
delete_edges_older_than(struct stinger *S, int64_t threshold)
{
    // FIXME
}

void
insert_batch(struct stinger *S, struct dynograph_edge_batch batch)
{
    // Insert the edges in parallel
    const int64_t type = 0;
    const bool directed = batch.directed;

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

void record_graph_size(struct stinger *S)
{
    int64_t max_active_nv = stinger_max_active_vertex(S);
    int64_t num_edges = stinger_edges_up_to(S, max_active_nv);
    hooks_set_attr_i64("num_vertices", max_active_nv);
    hooks_set_attr_i64("num_edges", num_edges);
}

void update_alg(struct stinger *S, const char *alg_name, int64_t* sources, int64_t num_sources)
{
    // FIXME
}

int main(int argc, char *argv[])
{
    struct dynograph_args args;
    dynograph_args_parse(argc, argv, &args);

    dynograph_message("Loading dataset...");
    // FIXME replace num_batches with batch_size
    struct dynograph_dataset* dataset = dynograph_load_dataset(args.input_path, 10);

    for (int64_t trial = 0; trial < args.num_trials; trial++)
    {
        hooks_set_attr_i64("trial", trial);

        dynograph_message("Initializing stinger...");
        const int64_t nv = 50; // FIXME get nv from dataset
        struct stinger_config_t config = generate_stinger_config(dataset->max_vertex_id);
        struct stinger *S = stinger_new_full(&config);

        // Run the algorithm(s) after each inserted batch
        int64_t epoch = 0;
        for (int64_t batch_id = 0; batch_id < dataset->num_batches; ++batch_id)
        {
            hooks_set_attr_i64("batch", batch_id);
            hooks_set_attr_i64("epoch", epoch);

            struct dynograph_edge_batch batch = dynograph_get_batch(dataset, batch_id);

            int64_t threshold = dynograph_get_timestamp_for_window(dataset, batch_id);

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
            insert_batch(S, batch);
            hooks_region_end();

            // FIXME implement epoch calculation
            //if (dataset.enableAlgsForBatch(batch_id))
            {
                record_graph_size(S);

                // FIXME implement multiple algs
                //for (std::string alg_name : args.alg_names)
                {
                    int64_t num_sources = 1;
                    //int64_t source = pick_sources_for_alg(S, alg_name);
                    int64_t source = 3; // FIXME pick source
                    hooks_set_attr_i64("source_vertex", source);
                    const char* alg_name = args.alg_names[0];
                    dynograph_message("Running %s", alg_name);
                    hooks_region_begin(alg_name);
                    update_alg(S, alg_name, &source, num_sources);
                    hooks_region_end();
                }
                epoch += 1;
                assert(epoch <= args.num_epochs);
            }

        }
        assert(epoch == args.num_epochs);
        dynograph_message("Shutting down stinger...");
        stinger_free(S);
    }

    return 0;
}