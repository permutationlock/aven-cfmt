# Tokenizer performance comparison loop vs goto

```bash
poop "./aven-ctokenize-loop include/aven/c.h" "./aven-ctokenize-goto include/aven/c.h"
Benchmark 1 (762 runs): ./aven-ctokenize-old include/aven/c.h
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          6.53ms ±  828us    5.96ms … 18.0ms         58 ( 8%)        0%
  peak_rss           1.73MB ± 56.8KB    1.54MB … 1.80MB          0 ( 0%)        0%
  cpu_cycles         18.3M  ±  734K     17.7M  … 24.4M          74 (10%)        0%
  instructions       50.9M  ±  199      50.9M  … 50.9M         251 (33%)        0%
  cache_references   19.5K  ± 10.2K     5.92K  … 74.5K          23 ( 3%)        0%
  cache_misses       2.69K  ± 6.28K      862   … 49.9K          43 ( 6%)        0%
  branch_misses       144K  ± 16.9K      130K  …  273K          64 ( 8%)        0%
Benchmark 2 (883 runs): ./aven-ctokenize-new include/aven/c.h
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          5.63ms ±  431us    5.22ms … 8.35ms         73 ( 8%)        ⚡- 13.7% ±  1.0%
  peak_rss           1.73MB ± 55.2KB    1.58MB … 1.80MB          0 ( 0%)          +  0.1% ±  0.3%
  cpu_cycles         15.4M  ± 86.9K     15.3M  … 16.6M          19 ( 2%)        ⚡- 15.4% ±  0.3%
  instructions       41.4M  ±  207      41.4M  … 41.4M           0 ( 0%)        ⚡- 18.6% ±  0.0%
  cache_references   16.2K  ± 7.44K     5.72K  … 69.4K          13 ( 1%)        ⚡- 16.8% ±  4.4%
  cache_misses       1.55K  ± 1.43K      750   … 34.5K          25 ( 3%)        ⚡- 42.4% ± 15.9%
  branch_misses       122K  ±  997       119K  …  127K          34 ( 4%)        ⚡- 15.5% ±  0.8%
```

```bash
poop "./aven-cfmt-loop include/aven/c.h" "./aven-cfmt-goto include/aven/c.h"
Benchmark 1 (502 runs): ./aven-cfmt-old include/aven/c.h
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          9.92ms ±  873us    8.89ms … 16.3ms         30 ( 6%)        0%
  peak_rss           3.17MB ± 42.0KB    3.04MB … 3.25MB          4 ( 1%)        0%
  cpu_cycles         27.2M  ±  691K     26.5M  … 31.3M          49 (10%)        0%
  instructions       75.8M  ±  207      75.8M  … 75.8M           0 ( 0%)        0%
  cache_references   70.3K  ± 15.6K     32.2K  …  127K           7 ( 1%)        0%
  cache_misses       8.38K  ± 6.83K     2.61K  … 49.5K          70 (14%)        0%
  branch_misses       199K  ± 17.7K      182K  …  306K          65 (13%)        0%
Benchmark 2 (551 runs): ./aven-cfmt-new include/aven/c.h
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          9.04ms ±  903us    8.20ms … 15.3ms         56 (10%)        ⚡-  8.8% ±  1.1%
  peak_rss           3.16MB ± 42.2KB    3.04MB … 3.22MB          0 ( 0%)          -  0.5% ±  0.2%
  cpu_cycles         24.6M  ±  602K     24.1M  … 29.8M          58 (11%)        ⚡-  9.8% ±  0.3%
  instructions       66.3M  ±  205      66.3M  … 66.3M         197 (36%)        ⚡- 12.5% ±  0.0%
  cache_references   67.9K  ± 16.6K     32.0K  …  142K          27 ( 5%)          -  3.3% ±  2.8%
  cache_misses       7.99K  ± 7.39K     2.94K  … 64.5K          81 (15%)          -  4.7% ± 10.3%
  branch_misses       174K  ± 1.61K      172K  …  180K          68 (12%)        ⚡- 12.5% ±  0.7%
```
