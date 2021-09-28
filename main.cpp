#include <iostream>
#include <thread>
#include <vector>

#include <libpmem2.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "spin_barrier.h"

int main() {
	int fd;
	struct pmem2_config *cfg;
	struct pmem2_map *map;
	struct pmem2_source *src;
	pmem2_persist_fn persist;

	if((fd = open("/home/taki/CLionProjects/pmem_bench/pmem_test", O_RDWR)) < 0) {
		perror("open");
	}

	if(pmem2_config_new(&cfg)){
		pmem2_perror("pmem2_config_new");
		std::exit(1);
	}

	if(pmem2_source_from_fd(&src, fd)) {
		pmem2_perror("pmem2_source_from_fd");
		std::exit(1);
	}

	if(pmem2_config_set_required_store_granularity(cfg, PMEM2_GRANULARITY_PAGE)){
		pmem2_perror("pmem2_config_set_required_store_granularity");
		std::exit(1);
	}

	if(pmem2_map_new(&map, cfg ,src)){
		pmem2_perror("pmem2_map_new");
		std::exit(1);
	}

	char *map_addr = static_cast<char*>(pmem2_map_get_address(map));
	pmem2_memcpy_fn memmcpy_fn = pmem2_get_memcpy_fn(map);
	pmem2_persist_fn persist_fn = pmem2_get_persist_fn(map);

	auto generate_random_string = [](int length)->std::string {
		auto generate_random_char = []() -> char {
			const char charset[] =
					"0123456789"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"abcdefghijk";
			const size_t max_index = (sizeof(charset) - 1);
			return charset[rand() % max_index];
		};

		std::string str(length, 0);
		for(auto s : str) {
			s = generate_random_char();
		}
		return str;
	};

	const int thread_num = 3;
	spin_barrier spinBarrier(thread_num);
	std::vector<std::thread> workers;

	for (int thread_id = 0; thread_id < thread_num; ++thread_id) {
		workers.emplace_back(std::thread([&,thread_id](){

			spinBarrier.wait(thread_id);
		}));
	}

	for(auto& w : workers) {
		w.join();
	}


	pmem2_map_delete(&map);
	pmem2_config_delete(&cfg);
	close(fd);
	pmem2_source_delete(&src);
}
