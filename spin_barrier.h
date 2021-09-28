//
// Created by taki on 2021/09/27.
//

#ifndef PMEM_SPIN_BARRIER_H
#define PMEM_SPIN_BARRIER_H

#include <atomic>
#include <vector>

class spin_barrier {
	spin_barrier() = delete;
	spin_barrier(const spin_barrier &) = delete;
	spin_barrier& operator=(const spin_barrier &) = delete;

public:
	spin_barrier(std::size_t threads);
	void wait(int i);

private:
	const int threads_;
	std::atomic<int> waits_;
	std::atomic<bool> sense_;
	std::vector<int> my_sense_;
};


#endif //PMEM_SPIN_BARRIER_H
