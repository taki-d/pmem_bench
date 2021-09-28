//
// Created by taki on 2021/09/27.
//

#include "spin_barrier.h"

spin_barrier::spin_barrier(std::size_t threads) :
	threads_(static_cast<int>(threads)),
	waits_(static_cast<int>(threads)),
	sense_(false),
	my_sense_(threads << 6)
	{
		for (std::size_t i = 0; i <threads; ++i) {
			my_sense_[i << 6] = true;
		}
	}

void spin_barrier::wait(int i) {
	int sense = my_sense_[i << 6];
	if(waits_.fetch_sub(1) == 1) {
		waits_.store(threads_);
		sense_.store(sense != 0, std::memory_order_release);
	} else {
		while (sense != sense_);
	}
	my_sense_[i << 6] = !sense;
}
