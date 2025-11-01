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

; kudos GitHub does highlight assembly syntax

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

<div class="perf_records">
<details>
  <summary>
  <var>testing X</var> <data>NA</data>
  </summary>
  <div class="perf_subrecs">
    <summary>
    <var>cpu-cycles</var> <data>17006</data>
    </summary>
    <summary>
    <var>cache-references</var> <data>517</data>
    </summary>
    <summary>
    <var>topdown-fe-bound</var> <data>40401</data>
    </summary>
    <summary>
    <var>topdown-be-bound</var> <data>8386</data>
    </summary>
    <summary>
    <var>topdown-retiring</var> <data>5786</data>
    </summary>
  </div>
</details>
</div>

Then let's change the config and try this:

<div class="perf_records">
<details>
  <summary>
  <var>testing X</var> <data>NA</data>
  </summary>
  <div class="perf_subrecs">
    <summary>
    <var>cpu-cycles</var> <data>20047</data>
    </summary>
    <summary>
    <var>cache-references</var> <data>438</data>
    </summary>
    <summary>
    <var>topdown-fe-bound</var> <data>47276</data>
    </summary>
    <summary>
    <var>topdown-be-bound</var> <data>10518</data>
    </summary>
    <summary>
    <var>topdown-retiring</var> <data>5786</data>
    </summary>
  </div>
</details>
</div>

And add more text.

When does HTML get the `<p>` back?


# Render HTML with Pandoc

Add the `--from` option to make Pandoc correctly treat custom tags as
not a part of markdown:
```bash
$ pandoc --from markdown-markdown_in_html_blocks+raw_html \
    --standalone example.md  > example.html
```

HTML can be parsed on its own:
```Python
from bs4 import BeautifulSoup
>>> from bs4 import BeautifulSoup
>>> with open("example.html", 'r') as f:
...     example_s = f.read()
...     
>>> 
>>> soup = BeautifulSoup(example_s, 'html.parser')
>>> soup.select(".perf_records")[0].select("summary")[0]
<summary>
<var>testing X</var> <data>NA</data>
</summary>
>>> soup.select(".perf_records")[0].find("summary").find("var").get_text(strip=True)
'testing X'
```
