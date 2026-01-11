# measurements.py
import subprocess
import matplotlib.pyplot as plt

from pprint import pprint
from collections import defaultdict


generations = 100
sizes = [10, 15, 1000, 2000, 3000, 5000]
nworkers = [4, 8, 12, 16, 20, 24, 28, 32, 64, 128]

# sizes = [10, 15, 1000, 2000]
# nworkers = [4, 8, 12, 16, 20]

# sizes = [10, 15]
# nworkers = [4, 8, 12]

parallel1d_speedups = defaultdict(list)
parallel2d_speedups = defaultdict(list)

base = "bacteria"
serial = "_serial"
parallel1d = "_parallel1d"
parallel2d = "_parallel2d"

graph_data = []


for workers in nworkers:
    simulations = []

    for size in sizes:
        in_file = f"inputs\\{base}{size}.txt"
        simulations.append({
            "proc": subprocess.Popen(["mpiexec", "-n", str(workers + 1), "life_mpi.exe", in_file, str(generations)]),
            "sz": size,
        })
    
    for sim in simulations:
        sim.get("proc").communicate() # blocks until process is done
        size = sim.get("sz")

        serial_time = 0
        parallel1d_time = 0
        parallel2d_time = 0

        with open(f"outputs\\{base}{size}\\{base}{size}{serial}.txt", "r") as f:
            serial_time = float(f.readline())

        with open(f"outputs\\{base}{size}\\{base}{size}{parallel1d}.txt", "r") as f:
            parallel1d_time = float(f.readline())

        with open(f"outputs\\{base}{size}\\{base}{size}{parallel2d}.txt", "r") as f:
            parallel2d_time = float(f.readline())
        
        parallel1d_speedups[size].append(serial_time / parallel1d_time)
        parallel2d_speedups[size].append(serial_time / parallel2d_time)

        print(f"Finished size {size}:\n\t1d: {parallel1d_speedups[size][-1]}\n\t2d: {parallel2d_speedups[size][-1]}")

print(parallel1d_speedups)
print(parallel2d_speedups)
print()

measurements = []

for size in sizes:
    measurements .append({
        "grid_size": size,
        "worker_count": nworkers,
        "speedup_1d": parallel1d_speedups[size],
        "speedup_2d": parallel2d_speedups[size]
    })

pprint(measurements)

for idx, item in enumerate(measurements):
    # 1d
    plt.subplot(2, len(sizes), idx + 1)
    plt.title(f"Grid size: {item.get("grid_size")}")
    if idx == 0:
        plt.xlabel("$worker count$")
        plt.ylabel("$speedup_1d$")
    plt.plot(item.get("worker_count"), item.get("speedup_1d"))

    # 2d
    plt.subplot(2, len(sizes), idx + 1 + len(sizes))
    # plt.title(f"Grid size: {item.get("grid_size")}")
    if idx == 0:
        plt.xlabel("$worker count$")
        plt.ylabel("$speedup_2d$")
    plt.plot(item.get("worker_count"), item.get("speedup_2d"))

plt.show()
