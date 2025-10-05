# Tokenizer performance comparison loop vs goto

```bash
$ poop "./aven-ctokenize-loop include/aven/c.h" "./aven-ctokenize-goto include/aven/c.h"
Benchmark 1 (1146 runs): ./aven-ctokenize-loop include/aven/c.h
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          4.32ms ±  512us    3.86ms … 9.57ms        102 ( 9%)        0%
  peak_rss           1.65MB ± 82.3KB    1.49MB … 1.80MB        146 (13%)        0%
  cpu_cycles         11.4M  ±  481K     11.0M  … 14.1M         133 (12%)        0%
  instructions       26.1M  ±  206      26.1M  … 26.1M         390 (34%)        0%
  cache_references   8.83K  ± 4.16K     4.33K  … 45.3K         108 ( 9%)        0%
  cache_misses        893   ±  475       595   … 13.3K          89 ( 8%)        0%
  branch_misses       120K  ± 14.8K      107K  …  200K         138 (12%)        0%
Benchmark 2 (1284 runs): ./aven-ctokenize-goto include/aven/c.h
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          3.86ms ±  536us    3.22ms … 8.66ms         12 ( 1%)        ⚡- 10.7% ±  1.0%
  peak_rss           1.65MB ± 82.8KB    1.49MB … 1.80MB        172 (13%)          +  0.1% ±  0.4%
  cpu_cycles         8.78M  ± 70.8K     8.67M  … 9.25M          67 ( 5%)        ⚡- 23.1% ±  0.2%
  instructions       17.3M  ±  204      17.3M  … 17.3M         443 (35%)        ⚡- 34.0% ±  0.0%
  cache_references   8.71K  ± 4.50K     4.22K  … 50.9K         120 ( 9%)          -  1.4% ±  3.9%
  cache_misses        925   ±  800       552   … 26.6K          35 ( 3%)          +  3.7% ±  5.9%
  branch_misses       103K  ± 2.77K     99.0K  …  118K         117 ( 9%)        ⚡- 14.1% ±  0.7%
$ poop "./aven-ctokenize-loop ../raylib/src/rcore.c" "./aven-ctokenize-goto ../raylib/src/rcore.c"
Benchmark 1 (1452 runs): ./aven-ctokenize-loop ../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          3.40ms ±  384us    2.25ms … 6.36ms        172 (12%)        0%
  peak_rss           1.25MB ±    0      1.25MB … 1.25MB          0 ( 0%)        0%
  cpu_cycles         6.12M  ±  155K     5.90M  … 6.80M           2 ( 0%)        0%
  instructions       12.5M  ±  201      12.5M  … 12.5M         468 (32%)        0%
  cache_references   4.16K  ± 1.87K     2.76K  … 19.7K         126 ( 9%)        0%
  cache_misses        532   ±  159       371   … 2.66K         111 ( 8%)        0%
  branch_misses      78.8K  ± 4.64K     72.2K  … 93.8K           0 ( 0%)        0%
Benchmark 2 (1749 runs): ./aven-ctokenize-goto ../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          2.82ms ±  304us    1.82ms … 4.17ms        427 (24%)        ⚡- 17.1% ±  0.7%
  peak_rss           1.29MB ±    0      1.29MB … 1.29MB          0 ( 0%)        💩+  2.9% ±  0.0%
  cpu_cycles         4.61M  ± 34.3K     4.57M  … 4.99M         146 ( 8%)        ⚡- 24.6% ±  0.1%
  instructions       8.54M  ±  208      8.54M  … 8.54M           0 ( 0%)        ⚡- 31.8% ±  0.0%
  cache_references   4.08K  ± 2.04K     2.75K  … 23.7K         139 ( 8%)          -  2.0% ±  3.3%
  cache_misses        524   ±  260       356   … 7.27K         125 ( 7%)          -  1.6% ±  2.9%
  branch_misses      71.6K  ± 1.09K     68.9K  … 78.3K          97 ( 6%)        ⚡-  9.1% ±  0.3%
$ poop "./aven-cfmt-loop include/aven/c.h" "./aven-cfmt-goto include/aven/c.h"
Benchmark 1 (530 runs): ./aven-cfmt-loop include/aven/c.h
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          9.38ms ±  576us    8.81ms … 14.2ms         34 ( 6%)        0%
  peak_rss           3.17MB ± 46.6KB    2.99MB … 3.25MB          6 ( 1%)        0%
  cpu_cycles         27.0M  ±  654K     26.4M  … 31.7M          79 (15%)        0%
  instructions       75.8M  ±  211      75.8M  … 75.8M           0 ( 0%)        0%
  cache_references   54.3K  ± 9.08K     30.8K  …  106K          13 ( 2%)        0%
  cache_misses       4.24K  ± 1.51K     2.10K  … 30.7K          46 ( 9%)        0%
  branch_misses       199K  ± 18.1K      181K  …  310K          81 (15%)        0%
Benchmark 2 (581 runs): ./aven-cfmt-goto include/aven/c.h
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          8.58ms ±  440us    8.18ms … 11.4ms         46 ( 8%)        ⚡-  8.6% ±  0.6%
  peak_rss           3.16MB ± 41.9KB    3.03MB … 3.22MB          0 ( 0%)          -  0.4% ±  0.2%
  cpu_cycles         24.4M  ±  517K     24.1M  … 29.4M          24 ( 4%)        ⚡-  9.8% ±  0.3%
  instructions       66.3M  ±  200      66.3M  … 66.3M         194 (33%)        ⚡- 12.5% ±  0.0%
  cache_references   54.8K  ± 8.90K     32.1K  …  133K          31 ( 5%)          +  0.9% ±  1.9%
  cache_misses       4.29K  ± 2.92K     2.47K  … 65.5K          32 ( 6%)          +  1.2% ±  6.5%
  branch_misses       173K  ±  947       171K  …  179K          17 ( 3%)        ⚡- 12.8% ±  0.7%
$ poop "./aven-cfmt-loop --columns 128 ../raylib/src/rcore.c" "./aven-cfmt-goto --columns 128 ../raylib/src/rcore.c"
Benchmark 1 (1065 runs): ./aven-cfmt-loop --columns 128 ../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          4.65ms ±  644us    4.29ms … 12.5ms        131 (12%)        0%
  peak_rss           1.86MB ± 44.5KB    1.70MB … 1.94MB         14 ( 1%)        0%
  cpu_cycles         12.5M  ±  409K     12.3M  … 16.3M         128 (12%)        0%
  instructions       31.9M  ±  206      31.9M  … 31.9M         354 (33%)        0%
  cache_references   11.8K  ± 11.7K     5.68K  … 90.3K          92 ( 9%)        0%
  cache_misses       2.54K  ± 6.43K     1.02K  … 48.1K          62 ( 6%)        0%
  branch_misses       124K  ± 3.28K      117K  …  143K         122 (11%)        0%
Benchmark 2 (1187 runs): ./aven-cfmt-goto --columns 128 ../raylib/src/rcore.c
  measurement          mean ± σ            min … max           outliers         delta
  wall_time          4.18ms ±  437us    3.89ms … 8.34ms        165 (14%)        ⚡- 10.1% ±  1.0%
  peak_rss           1.85MB ± 43.3KB    1.73MB … 1.91MB          0 ( 0%)          -  0.8% ±  0.2%
  cpu_cycles         11.1M  ±  178K     11.0M  … 12.8M          65 ( 5%)        ⚡- 11.0% ±  0.2%
  instructions       27.5M  ±  207      27.5M  … 27.5M           0 ( 0%)        ⚡- 13.8% ±  0.0%
  cache_references   10.2K  ± 4.78K     5.62K  … 41.4K         113 (10%)        ⚡- 13.2% ±  6.1%
  cache_misses       1.30K  ±  680       829   … 17.4K          64 ( 5%)        ⚡- 48.9% ± 14.5%
  branch_misses       118K  ± 1.04K      113K  …  124K          52 ( 4%)        ⚡-  4.8% ±  0.2%
```
