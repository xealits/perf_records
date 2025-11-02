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
  <var>analysisID <data>1</data></var>
  <div class="perf_records_conditions">
  <var>n_thr <data>2</data></var>
  </div>
  </summary>
  <div class="perf_records_nest">
    <details>
      <summary>
      <var>thrMain <data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var>cpu-cycles <data>17622</data></var>
        <var>cache-references <data>512</data></var>
        <var>topdown-fe-bound <data>40872</data></var>
        <var>topdown-be-bound <data>9065</data></var>
        <var>topdown-retiring <data>5786</data></var>
      </div>
    </details>
  </div>
</details>
</div>

Then let's change the config and try this:

<div class="perf_records">
<details>
  <summary>
  <var>analysisID <data>1</data></var>
  <div class="perf_records_conditions">
  <var>n_thr <data>2</data></var>
  </div>
  </summary>
  <div class="perf_records_nest">
    <details>
      <summary>
      <var>thrMain <data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var>cpu-cycles <data>18899</data></var>
        <var>cache-references <data>535</data></var>
        <var>topdown-fe-bound <data>45006</data></var>
        <var>topdown-be-bound <data>6723</data></var>
        <var>topdown-retiring <data>5786</data></var>
      </div>
    </details>
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

And parse it to CSV:
```bash
$ python log_to_csv.py example.html 
$ cat example.csv 
thrMain.topdown-be-bound,thrMain,thrMain.cache-references,thrMain.topdown-retiring,n_thr,thrMain.cpu-cycles,thrMain.topdown-fe-bound
9065,NA,512,5786,2,17622,40872
6723,NA,535,5786,2,18899,45006
```
