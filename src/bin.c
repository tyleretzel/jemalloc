#include "jemalloc/internal/jemalloc_preamble.h"
#include "jemalloc/internal/jemalloc_internal_includes.h"

#include "jemalloc/internal/assert.h"
#include "jemalloc/internal/bin.h"
#include "jemalloc/internal/sc.h"
#include "jemalloc/internal/witness.h"

bin_info_t bin_infos[SC_NBINS];

static
void bin_dump_info(bin_info_t *bin_info) {
	malloc_printf("reg_size: %zu\n", bin_info->reg_size);
	malloc_printf("slab_size: %zu\n", bin_info->slab_size);
	malloc_printf("nregs: %d\n", bin_info->nregs);
	malloc_printf("extent_class: %u\n", bin_info->extent_class);
	malloc_printf("\n");
}

void
bin_infos_init(sc_data_t *sc_data, bin_info_t bin_infos[SC_NBINS]) {
	for (unsigned i = 0; i < SC_NBINS; i++) {
		bin_info_t *bin_info = &bin_infos[i];
		sc_t *sc = &sc_data->sc[i];
		bin_info->reg_size = ((size_t)1U << sc->lg_base)
		    + ((size_t)sc->ndelta << sc->lg_delta);
		bin_info->slab_size = (sc->pgs << LG_PAGE);
		bin_info->nregs =
		    (uint32_t)(bin_info->slab_size / bin_info->reg_size);
		bitmap_info_t bitmap_info = BITMAP_INFO_INITIALIZER(
		    bin_info->nregs);
		bin_info->bitmap_info = bitmap_info;

		if (bin_info->nregs > SLAB_SMALL_REGS) {
			bin_info->extent_class = extent_class_large;
		} else {
			bin_info->extent_class = extent_class_small;
		}
		bin_dump_info(bin_info);
	}
}

void
bin_boot(sc_data_t *sc_data) {
	assert(sc_data->initialized);
	bin_infos_init(sc_data, bin_infos);
}

bool
bin_init(bin_t *bin) {
	if (malloc_mutex_init(&bin->lock, "bin", WITNESS_RANK_BIN,
	    malloc_mutex_rank_exclusive)) {
		return true;
	}
	bin->slabcur = NULL;
	extent_heap_new(&bin->slabs_nonfull);
	extent_list_init(&bin->slabs_full);
	if (config_stats) {
		memset(&bin->stats, 0, sizeof(bin_stats_t));
	}
	return false;
}

void
bin_prefork(tsdn_t *tsdn, bin_t *bin) {
	malloc_mutex_prefork(tsdn, &bin->lock);
}

void
bin_postfork_parent(tsdn_t *tsdn, bin_t *bin) {
	malloc_mutex_postfork_parent(tsdn, &bin->lock);
}

void
bin_postfork_child(tsdn_t *tsdn, bin_t *bin) {
	malloc_mutex_postfork_child(tsdn, &bin->lock);
}
