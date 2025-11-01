---
title: A study a log
date: November 1, 2025
---

Some log testing this code
```cpp

Record<PerfCounter::CounterVal_t> translate_perf_counters(
    RecordString_t rec_name, const PerfCounter::AllCountersData& data) {
  Record<PerfCounter::CounterVal_t> bench(rec_name);
  bench.pid = data.pid;
  bench.cpu = data.cpu;
  for (const auto& rec : data.counters) {
    bench.subrecs.push_back(translate_perf_record(rec.second));
  }
  return bench;
}
```

And looking at (not really) its code:
```assembly
.LVL3:
  .loc 1 17 12 view .LVU10
  .p2align 6
.L6:
.LVL4:
.LBB4:
.LBB5:
  .loc 1 21 7 is_stmt 1 view .LVU11
  movl  (%rax), %edx
  .loc 1 21 20 is_stmt 0 view .LVU12
  movl  %edx, %ecx
  andl  $1771, %ecx
  .loc 1 21 12 view .LVU13
  addl  %ecx, %edx
  movl  %edx, (%rax)
  .loc 1 19 5 is_stmt 1 discriminator 3 view .LVU14
  addq  $4, %rax
.LVL5:
.L5:
  .loc 1 19 23 discriminator 1 view .LVU15
  leaq  AData(%rip), %rdx
  leaq  1024000000(%rdx), %rdx
  cmpq  %rdx, %rax
  jne .L6
```

# testing an idea 1

Metrics with a basic config:

<details>
  <summary>
  testing X <data>NA</data>
  </summary>
    <summary>
    cpu-cycles <data>29309</data>
    </summary>
    <summary>
    cache-references <data>637</data>
    </summary>
    <summary>
    topdown-fe-bound <data>76191</data>
    </summary>
    <summary>
    topdown-be-bound <data>17888</data>
    </summary>
    <summary>
    topdown-retiring <data>5786</data>
    </summary>
</details>


Then let's change the config and try this:

<details>
  <summary>
  testing X <data>NA</data>
  </summary>
    <summary>
    cpu-cycles <data>15874</data>
    </summary>
    <summary>
    cache-references <data>442</data>
    </summary>
    <summary>
    topdown-fe-bound <data>37937</data>
    </summary>
    <summary>
    topdown-be-bound <data>7441</data>
    </summary>
    <summary>
    topdown-retiring <data>5786</data>
    </summary>
</details>

And add more text.

When does HTML get the `<p>` back?


# Render HTML with Pandoc

```bash
$ pandoc --standalone example.md  > example.html
```
