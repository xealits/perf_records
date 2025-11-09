---
title: A study a log
date: November 1, 2025
---

<style>
  var.perf_records_parameter::before {
    content: attr(data-name) ": ";
    font-size: 0.7em;
  }
</style>

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

Metrics with a basic config without the app counter:

<div class="perf_records">
<details>
  <summary>
  <var class="perf_records_parameter" data-name="analysisID"><data>1</data></var>
  <div class="perf_records_conditions">
  <var class="perf_records_parameter" data-name="n_thr"><data>2</data></var>
  </div>
  </summary>
  <div class="perf_records_nest">
    <details>
      <summary>
      <var class="perf_records_parameter" data-name="thrMain"><data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var class="perf_records_parameter" data-name="cpu-cycles"><data>17388</data></var>
        <var class="perf_records_parameter" data-name="cache-references"><data>467</data></var>
        <var class="perf_records_parameter" data-name="topdown-fe-bound"><data>45176</data></var>
        <var class="perf_records_parameter" data-name="topdown-be-bound"><data>7289</data></var>
        <var class="perf_records_parameter" data-name="topdown-retiring"><data>5788</data></var>
      </div>
    </details>
    <details>
      <summary>
      <var class="perf_records_parameter" data-name="thr1"><data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var class="perf_records_parameter" data-name="cpu-cycles"><data>43535</data></var>
        <var class="perf_records_parameter" data-name="topdown-fe-bound"><data>102301</data></var>
        <var class="perf_records_parameter" data-name="topdown-retiring"><data>14288</data></var>
      </div>
    </details>
  </div>
</details>
</div>

Some outlier:

<div class="perf_records">
<details>
  <summary>
  <var class="perf_records_parameter" data-name="analysisID"><data>1</data></var>
  <div class="perf_records_conditions">
  <var class="perf_records_parameter" data-name="n_thr"><data>2</data></var>
  <var class="perf_records_parameter" data-name="instr"><data>0</data></var>
  </div>
  </summary>
  <div class="perf_records_nest">
    <details>
      <summary>
      <var class="perf_records_parameter" data-name="thrMain"><data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var class="perf_records_parameter" data-name="cpu-cycles"><data>14134</data></var>
        <var class="perf_records_parameter" data-name="cache-references"><data>444</data></var>
        <var class="perf_records_parameter" data-name="topdown-fe-bound"><data>34446</data></var>
        <var class="perf_records_parameter" data-name="topdown-be-bound"><data>7746</data></var>
        <var class="perf_records_parameter" data-name="topdown-retiring"><data>6253</data></var>
      </div>
    </details>
    <details>
      <summary>
      <var class="perf_records_parameter" data-name="thr1"><data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var class="perf_records_parameter" data-name="cpu-cycles"><data>46825</data></var>
        <var class="perf_records_parameter" data-name="topdown-fe-bound"><data>116091</data></var>
        <var class="perf_records_parameter" data-name="topdown-retiring"><data>14513</data></var>
      </div>
    </details>
  </div>
</details>
</div>

## Added app counters

Metrics with a basic config with the app counter (more `retiring`):

<div class="perf_records">
<details>
  <summary>
  <var class="perf_records_parameter" data-name="analysisID"><data>1</data></var>
  <div class="perf_records_conditions">
  <var class="perf_records_parameter" data-name="n_thr"><data>2</data></var>
  <var class="perf_records_parameter" data-name="instr"><data>1</data></var>
  </div>
  </summary>
  <div class="perf_records_nest">
    <var class="perf_records_parameter" data-name="app_count"><data>5</data></var>
    <details>
      <summary>
      <var class="perf_records_parameter" data-name="thrMain"><data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var class="perf_records_parameter" data-name="cpu-cycles"><data>14364</data></var>
        <var class="perf_records_parameter" data-name="cache-references"><data>452</data></var>
        <var class="perf_records_parameter" data-name="topdown-fe-bound"><data>34453</data></var>
        <var class="perf_records_parameter" data-name="topdown-be-bound"><data>7679</data></var>
        <var class="perf_records_parameter" data-name="topdown-retiring"><data>5803</data></var>
      </div>
    </details>
    <details>
      <summary>
      <var class="perf_records_parameter" data-name="thr1"><data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var class="perf_records_parameter" data-name="cpu-cycles"><data>45302</data></var>
        <var class="perf_records_parameter" data-name="topdown-fe-bound"><data>111648</data></var>
        <var class="perf_records_parameter" data-name="topdown-retiring"><data>14318</data></var>
      </div>
    </details>
  </div>
</details>
</div>

Then let's change the config and try this:

<div class="perf_records">
<details>
  <summary>
  <var class="perf_records_parameter" data-name="analysisID"><data>1</data></var>
  <div class="perf_records_conditions">
  <var class="perf_records_parameter" data-name="n_thr"><data>2</data></var>
  <var class="perf_records_parameter" data-name="instr"><data>1</data></var>
  </div>
  </summary>
  <div class="perf_records_nest">
    <var class="perf_records_parameter" data-name="app_count"><data>5</data></var>
    <details>
      <summary>
      <var class="perf_records_parameter" data-name="thrMain"><data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var class="perf_records_parameter" data-name="cpu-cycles"><data>18647</data></var>
        <var class="perf_records_parameter" data-name="cache-references"><data>437</data></var>
        <var class="perf_records_parameter" data-name="topdown-fe-bound"><data>45943</data></var>
        <var class="perf_records_parameter" data-name="topdown-be-bound"><data>12697</data></var>
        <var class="perf_records_parameter" data-name="topdown-retiring"><data>6028</data></var>
      </div>
    </details>
    <details>
      <summary>
      <var class="perf_records_parameter" data-name="thr1"><data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var class="perf_records_parameter" data-name="cpu-cycles"><data>45889</data></var>
        <var class="perf_records_parameter" data-name="topdown-fe-bound"><data>113369</data></var>
        <var class="perf_records_parameter" data-name="topdown-retiring"><data>14254</data></var>
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
>>> example_s = "..."
>>> soup = BeautifulSoup(example_s, 'html.parser')
>>> soup.select_one("var[data-name=analysisID] > data").contents
['1']
```

And parse it to CSV:
```bash
$ python log_to_csv.py log_example.html
$ cat log_example.csv
thrMain.cpu-cycles,app_count,thr1.cpu-cycles,thrMain.topdown-fe-bound,thr1.topdown-fe-bound,instr,n_thr,thrMain,thrMain.cache-references,thrMain.topdown-be-bound,thrMain.topdown-retiring,thr1.topdown-retiring,thr1
17388,,43535,45176,102301,,2,NA,467,7289,5788,14288,NA
14134,,46825,34446,116091,0,2,NA,444,7746,6253,14513,NA
14364,5,45302,34453,111648,1,2,NA,452,7679,5803,14318,NA
18647,5,45889,45943,113369,1,2,NA,437,12697,6028,14254,NA
```
