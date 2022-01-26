#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <cstring>
#include <chrono>

#include <libpmem2.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "spin_barrier.h"

int main(int argc, char *argv[]) {
    // test params
	int is_random = 0;
	int thread_num = 8;
	long long int wr_size = 1024*1024*1024; // 1GiB
	long long int block_size = 512; // 64Byte
    int operation = 0;
    std::string dax_path = "/dev/dax0.0";

    std::vector<std::string> args(argv, argv + argc);
    dax_path = args[1];
    is_random = std::stoi(args[2]);
    operation = std::stoi(args[3]);
    thread_num = std::stoi(args[4]);
    wr_size = std::stoll(args[5]);
    block_size = std::stoll(args[6]);

    std::cout << is_random << "," << operation <<  "," << thread_num;
    std::cout << "," << wr_size << "," << block_size << ",";

    int thread_wr_size = wr_size / thread_num;
	int fd;
	struct pmem2_config *cfg;
	struct pmem2_map *map;
	struct pmem2_source *src;

	if((fd = open(dax_path.c_str(), O_RDWR)) < 0) {
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

	if(pmem2_config_set_required_store_granularity(cfg, PMEM2_GRANULARITY_CACHE_LINE)){
		pmem2_perror("pmem2_config_set_required_store_granularity");
		std::exit(1);
	}

	if(pmem2_map_new(&map, cfg ,src)){
		pmem2_perror("pmem2_map_new");
		std::exit(1);
	}

	char *map_addr = static_cast<char*>(pmem2_map_get_address(map));
	pmem2_memcpy_fn memcpy_fn = pmem2_get_memcpy_fn(map);

	pmem2_persist_fn persist_fn = pmem2_get_persist_fn(map);
    // cpu can reorder the flush
    pmem2_flush_fn flush_fn = pmem2_get_flush_fn(map);
    // make sure to finish all of flush work before this function has been finished
    pmem2_drain_fn drain_fn = pmem2_get_drain_fn(map);

    pmem2_memmove_fn memmove_fn = pmem2_memmove_fn(map);
    pmem2_memset_fn memset_fn = pmem2_get_memset_fn(map);

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
		for(auto& s : str) {
			s = generate_random_char();
		}
		return str;
	};
	// random data
	std::string random_string = generate_random_string(1024*1024);

	spin_barrier barrier(thread_num);
	std::vector<std::thread> workers;
	auto start = std::chrono::system_clock::now();

	workers.reserve(thread_num);

	for (int thread_id = 0; thread_id < thread_num; ++thread_id) {
		workers.emplace_back(std::thread([&,thread_id]() {
			std::vector<int> index(thread_wr_size / block_size);
			char random_data[1024 * 1024];
			std::strcpy(random_data, random_string.c_str());

			std::iota(index.begin(), index.end(), 0);
			if (is_random) {
				std::random_device seed;
				std::mt19937 engine(seed());
				std::shuffle(index.begin(), index.end(), engine);
			}

			char *local_base_addr = map_addr + (thread_wr_size * thread_id);

			barrier.wait(thread_id);
			if (thread_id == 0) {
				start = std::chrono::system_clock::now();
			}

			switch (operation) {
				case 0:
					for (const auto &i: index) {
						// force to use ntstore
						memcpy_fn(
								local_base_addr + block_size * i, // dest
								random_data + (block_size * i) % (1024 * 1024), //src
								block_size, // size
								PMEM2_F_MEM_NONTEMPORAL //flag
						);
					}
					break;
				case 1:
					for (const auto &i: index) {
						// force to use store/flush each epoch
						memcpy_fn(
								local_base_addr + block_size * i, // dest
								random_data + (block_size * i) % (1024 * 1024), //src
								block_size, // size
								PMEM2_F_MEM_TEMPORAL //flag
						);
					}
					break;
				case 2:
					for (const auto &i: index) {
						// force to store and flush
						memcpy_fn(
								local_base_addr + block_size * i, // dest
								random_data + (block_size * i) % (1024 * 1024), // src
								block_size,
								PMEM2_F_MEM_NOFLUSH
						);
						flush_fn(local_base_addr + block_size * i, block_size);
						drain_fn();
					}
				case 3:
					for (const auto &i: index) {
						// normal write (use ntstore larger than 512Byte instead of store)
						memcpy_fn(
								local_base_addr + block_size * i, // dest
								random_data + (block_size * i) % (1024 * 1024), // src
								block_size,
								0
						);
					}
					break;
				case 4:
					for (const auto &i: index) {
						// normal write (use ntstore larger than 512Byte)
						memcpy(
								random_data + (block_size * i) % (1024 * 1024),
								local_base_addr + block_size * i,
								block_size
						);
					}
					break;
			}
			}));
		}

		for(auto& w : workers) {
			w.join();
		}
		auto end = std::chrono::system_clock::now();

	std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

	pmem2_map_delete(&map);
	pmem2_config_delete(&cfg);
	close(fd);
	pmem2_source_delete(&src);
}
