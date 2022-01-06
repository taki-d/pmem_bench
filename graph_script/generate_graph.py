import itertools
import os

import glob
import matplotlib.pyplot as plt
import matplotlib.scale
import numpy as np
import pandas as pd
import re
from itertools import product

def generate_graph(filename, file):
    data = pd.read_csv(filename, sep=',', header=None, names=['is_random', "operation", "thread_num", "wr_size", "block_size", "time"])

    r = data['is_random'].unique()
    op = data['operation'].unique()

    r_l = ['sequential', 'random']
    op_l = ['fdwrite', 'normalwrite', 'read']

    for is_random, operation in list(product(r, op)):
        thread_size_set = np.power(2, np.arange(6))
        block_size_set = np.power(2, np.arange(3,21))

        # graph setting
        plt.ioff()
        plt.rcParams['figure.dpi'] = 300

        fig, ax = plt.subplots(figsize=(8, 6))
        ax.set_ylabel("Band Width [MiB/s]")
        ax.set_xlabel("Block Size [Byte]")
        ax.set_xscale('log')
        ax.set_xticks(block_size_set, ["8", "16", "32", "64", "128", "256", "512", "1K", "2K", "4K", "8K", "16K", "32K", "64K", "128K", "256K", "512K", "1M"])

        for thread in thread_size_set:
            xdata = np.empty(0)
            ydata = np.empty(0)
            for block in block_size_set:
                d = data.query('thread_num == @thread and block_size == @block and is_random == @is_random and operation == @operation')
                if len(d) > 0:
                    mib = pow(2, 30) / d.mean()['time'] * pow(10, 6) / pow(2, 20)
                    ydata = np.append(ydata, mib)
                    xdata = np.append(xdata, block)

            if len(xdata) != 0 and len(ydata) != 0 and len(xdata) == len(ydata):
                ax.plot(xdata, ydata, marker='o', label=f'{thread} thread')

        ax.legend()

        f = f"{filename}_{r_l[is_random]}_{op_l[operation]}"
        file.write(f"{f}\n ![figure]({f.split('/')[-1]}.png)\n")

        fig.savefig(f"{f}.eps", dpi=200)
        fig.savefig(f"{f}.png", dpi=200)


if __name__=="__main__":
    directories = glob.glob("../data/*")
    for directory in directories:
        view = open(f"{directory}/graph.md", 'w')
        for file in glob.glob(f"{directory}/*.data"):
            generate_graph(file, view)
