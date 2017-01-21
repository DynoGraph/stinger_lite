#include <stdio.h>
#include <stinger_core/stinger.h>
#include <dynograph_util.h>

int main(int argc, char *argv[])
{
    dynograph_message("Initializing stinger...");
    struct stinger_config_t config;
    config.nv = 100;
    config.nebs = 100;
    config.netypes = 1;
    config.nvtypes = 1;
    config.memory_size = 16 * 1024 * 1024;
    config.no_resize = 1;
    struct stinger_t *S = stinger_new_full(&config);

    dynograph_message("Inserting edges...");
    stinger_insert_edge(S, 0, 4, 5, 1, 0);
    stinger_insert_edge(S, 0, 4, 6, 1, 0);
    stinger_insert_edge(S, 0, 4, 7, 1, 0);
    dynograph_message("Vertex 4 has %llu neighbors", stinger_outdegree_get(S, 4));

    dynograph_message("Shutting down...");
    stinger_free(S);

    return 0;
}