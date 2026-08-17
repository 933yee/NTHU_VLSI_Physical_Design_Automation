# NTHU VLSI Physical Design Automation

Five projects from NTHU VLSI Physical Design Automation, covering an RTL-to-GDS implementation flow and four classic physical-design optimization problems implemented in C++.

## Results

![HW1 grade](https://img.shields.io/badge/HW1-91-green)
![HW2 grade](https://img.shields.io/badge/HW2-96-green)
![HW3 grade](https://img.shields.io/badge/HW3-89-green)
![HW4 grade](https://img.shields.io/badge/HW4-96-green)
![HW5 grade](https://img.shields.io/badge/HW5-83-green)

## Project overview

| Work | Problem | Approach |
| --- | --- | --- |
| [HW1](HW1/) | RTL-to-GDS physical implementation | Cadence Innovus experiments on utilization, clock period, timing optimization, routing, DRC, area, and wirelength |
| [HW2](HW2/) | Two-die netlist partitioning | Fiduccia–Mattheyses-style weighted partitioner with area constraints, custom tie-breaking, multiset gain ordering, and OpenMP variant |
| [HW3](HW3/) | Fixed-outline floorplanning | Sequence-pair representation, constructed legal initial floorplan, and simulated annealing for wirelength minimization |
| [HW4](HW4/) | Analog placement with symmetry | Hierarchical B*-trees, symmetry islands, contour-aware packing, and staged simulated annealing |
| [HW5](HW5/) | Standard-cell legalization | Abacus-inspired clustering, blockage-aware subrows, displacement constraints, localized row search, and swap refinement |

## HW1 — Physical implementation study

The SHA-256 design is taken through floorplanning, placement, clock/timing optimization, routing, and verification in Cadence Innovus. Eighteen configurations compare core utilization and clock period against:

- DRC violations;
- setup slack;
- chip area;
- total routed wirelength.

The selected result uses 58.5% core utilization and a 525 ps clock period, producing zero DRC violations and 0.111 ps reported slack in the course flow. The directory preserves the final SDC and result artifacts.

[Report](HW1/CS6135_HW1_110062222_report.pdf) · [Specification](HW1/CS6135_HW1_spec.pdf)

## HW2 — Weighted two-die partitioning

The partitioner adapts the FM algorithm to weighted nets and cells whose areas differ between dies:

- constructs an area-feasible initial partition using each cell's smaller die area;
- stores movable cells in an ordered multiset because weighted gains exceed a compact bucket-list range;
- simulates a complete pass and commits the prefix with maximum cumulative gain;
- uses net weight, net degree, and cell area as deterministic quality tie-breakers;
- provides both serial and OpenMP-enabled implementations.

## HW3 — Fixed-outline floorplanning

Blocks are first packed into a legal fixed outline with a size-aware row heuristic. Simulated annealing then perturbs the sequence pair to minimize half-perimeter wirelength while rejecting outline violations. The implementation explores dead-space ratios down to the reported legal limits of 0.0786, 0.0620, and 0.0557 on the three public cases.

## HW4 — Symmetry-constrained analog placement

Symmetric device groups are represented as islands with their own internal B*-trees. The implementation:

- optimizes alternative island shapes first;
- preserves the island's non-rectangular contour when packing surrounding blocks;
- uses tree reinsertion as the main perturbation;
- searches combinations of minimum-area island shapes for a strong initial placement;
- performs a final high-level simulated annealing pass.

## HW5 — Placement legalization

The legalizer splits placement rows around fixed blockages, incrementally assigns cells to nearby subrows, and builds Abacus-style clusters. Candidate placement checks enforce site alignment, overlap legality, and maximum displacement. A local swap phase further lowers total displacement after a legal solution is found.

## Build and run

HW2–HW5 include Makefiles and public testcases. A Linux-like environment with GNU Make and g++ is expected.

~~~bash
# Example: HW4
cd HW4/src
make
../bin/hw4 ../testcase/public1.txt ../output/public1.out
~~~

Command formats:

| Work | Executable usage |
| --- | --- |
| HW2 | hw2 input.txt output.txt (or hw2_parallel) |
| HW3 | hw3 input.txt output.txt dead_space_ratio |
| HW4 | hw4 input.txt output.txt |
| HW5 | hw5 input.txt output.txt |

Use make clean in the same source directory to remove that assignment's objects and executable.

## Repository structure

Each homework contains its [specification](HW1/CS6135_HW1_spec.pdf) and submitted report. HW2–HW5 additionally provide source, public testcases, and selected outputs. Generated binaries are intentionally not tracked.

## Notes

- HW1 requires the licensed Cadence/course technology environment and cannot be reproduced with the repository alone.
- The optimization programs were tuned for the assignment formats and runtime limits; they are educational implementations rather than general LEF/DEF production tools.
- Detailed algorithms, experiments, and result plots are preserved in each homework's report PDF.
