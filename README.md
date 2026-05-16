# qsim: High-Performance Multi-Threaded Quantum Circuit Simulator

`qsim` is a generic quantum circuit simulator for POSIX-compliant environments, engineered entirely in standard C. The system enables the deterministic evolution of $n$-qubit quantum registers and the multi-threaded statistical sampling of the final state via arbitrary quantum gates expressed as dense complex matrices of size $2^n \times 2^n$.

`qsim` rigorously isolates the management of opaque data structures, low-latency linear algebra, and the concurrent orchestration runtime, maximizing computational throughput on multi-core architectures.

---

## 1. System Architecture and Modularity

The software adopts a decoupled and modular architecture, divided into the following functional components:

* **`main.c` (Control Flow Orchestrator)**: Manages the application lifecycle, decodes the command-line interface (CLI) via POSIX standards, and coordinates the simulator's state transitions (I/O, allocation, unitary computation, parallel reduction, sampling, and deallocation).
* **`engine.c / engine.h` (Concurrency Runtime)**: Contains the multi-threaded computing engine. It leverages threads operating on a binary tree model for circuit matrix reduction and handles isolated and concurrent probabilistic sampling.
* **`parser.c / parser.h` (Lexical Analysis & Sanitizer)**: Performs lexical analysis and input stream compression, validating the structural syntax of configuration files (`.q`) and executing deterministic tokenization.
* **`qmath.c / qmath.h` (Linear Algebra Kernel)**: A kernel optimized for computational linear algebra on complex numbers. It provides high-efficiency primitives for matrix-matrix and matrix-vector products.
* **`types.c / types.h` (Data Structures & Memory Isolation)**: Implements opaque ADTs (Abstract Data Types) for vectors (`Vector`) and matrices (`Matrix`). It guarantees full data encapsulation and prevents memory fragmentation through explicit allocation and destruction interfaces.

---

## 2. Software Design and Concurrency Optimizations

### 2.1 Unitary Evolution via Parallel Binary Reduction Tree
To avoid computation serialization (sequential linear multiplication of gates), the engine implements a parallel binary tree reduction within the `mult_engine` method. 

Threads reduce the search space by combining pairs of adjacent matrices in parallel at each level of the tree (e.g., $M_{\text{new}} = M_{2k+1} \times M_{2k}$). 
* **Scaling Efficiency:** This approach reduces the time complexity of circuit composition to a logarithmic level, $O(\log N)$.
* **Numerical Error Invariance:** By reducing the depth of the multiplication chain, it minimizes the accumulation of rounding errors (floating-point drift) typical of finite-precision complex arithmetic.

### 2.2 Isolated and Thread-Safe Pseudo-Random Sampling
In the statistical measurement phase (`measure`), `meas_engine` linearly partitions the sampling workload (shots) across the allocated logical cores. 

Each worker thread executes an independent setup leveraging the preprocessed discrete algorithm from the GNU Scientific Library (`gsl_ran_discrete_preproc`) based on the probability density derived from the final state vector ($P_j = | \alpha_j |^2$).
* **State Isolation**: To eliminate lock contention and guarantee maximum statistical entropy without cross-thread correlation, each worker allocates its own random number generator (`gsl_rng`), dynamically initialized using the unique address of the native thread descriptor (`pthread_self()`) as a seed.
* **Consolidation of Results**: Local occurrence vectors are consolidated into the global destination vector through an ultra-reduced critical section protected by a low-contention POSIX mutex (`my_lock`).

---

## 3. Parsing Strategy and Rigorous Lexical Analysis

The system implements a strict whitespace-insensitivity policy. All whitespace characters (`\t`, `\n`, ` `) are eliminated upfront via `remove_all_whitespace` to uniform the input stream.

### 3.1 The Token Boundary Erasure Problem
The total removal of whitespaces eliminates the natural delimiters between quantum gate names. Consequently, an intrinsic grammatical ambiguity arises if the user defines identifiers that are prefixes of one another (e.g., defining distinct gates named `X`, `Y`, and `XY` transforms both the sequence `X Y` and the sequence `XY` into the continuous complex string `XY`).

### 3.2 Solution: Maximal Munch Algorithm
To guarantee deterministic and unambiguous parsing without violating the compression specification, the lexer implements the **Maximal Munch (Longest Match)** strategy. During sequential circuit scanning, the analyzer does not stop at the first match found in the definitions dictionary; instead, it examines the entire active ruleset to select the valid token possessing the **longest character length**. This ensures that a string containing the gate name `G10` is correctly interpreted as such, rather than being erroneously fragmented into gates `G1` and `0`.

**System Assumption / Operational Constraint:**
It is assumed as a contractual specification that the end user does not introduce quantum gate names into the circuit file that create combinatoric prefix collisions when positioned consecutively (e.g., avoiding using gates named `X` and `Y` in immediate sequence if a gate named `XY` exists), thereby safeguarding the mathematical integrity of the simulation.

---

## 4. Command Line Interface (CLI)

The application exposes a standard POSIX CLI based on operational flags managed via the `getopt` utility:

| Flag | Argument | Description | Requirement |
| :--- | :--- | :--- | :--- |
| `-s` | `string (path)` | Absolute or relative path to the `.q` file containing the initial quantum state definition. | **Required** |
| `-c` | `string (path)` | Absolute or relative path to the `.q` file containing the matrix definitions and the execution circuit. | **Required** |
| `-t` | `int` | Number of POSIX threads to allocate in the concurrent thread pool. (Default: `1`). | Optional |
| `-h` | *None* | Displays the inline help menu with detailed command syntax and terminates execution. | Optional |

---

## 5. Build System Automation (Makefile)

The project includes a `Makefile` designed to optimize the compilation and linking process. 

### Available Targets

* `make` (or `make all` / `make qsim`): Main target. Executes incremental compilation of all source files (`.c`) into their respective object files (`.o`) and generates the optimized binary executable, statically linked to external libraries.
* `make clean`: Maintenance target. Safely removes all intermediate object files (`main.o`, `engine.o`, `parser.o`, `qmath.o`, `types.o`) and the final `qsim` executable to clean the workspace and force a full recompilation (rebuild).

### Internal Compilation Flags
* `-g`: Retains debugging symbols useful for analysis using `lldb` or `gdb`.
* `-O2`: Enables second-level compiler optimizations to maximize code execution speed and CPU register efficiency.
* `-pthread`: Enables native compiler support for the University-provided POSIX Threads library during both compilation and linking.
* `-lgsl -lgslcblas -lm`: Links the executable to the GNU Scientific Library, the accelerated BLAS core, and the standard system math library.

---

## 6. System Requirements and Deployment

### Hardware & Software Dependencies
* Compiler: `gcc` (v9+) or `clang` (v11+).
* Concurrency Library: POSIX `pthreads`.
* Scientific Libraries: `GSL (GNU Scientific Library)` and `CBLAS`.

### Dependency Installation on macOS
Dependencies for the host architecture can be quickly distributed via the Homebrew package manager:
```bash
brew install gsl