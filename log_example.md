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

Metrics with a basic config without the app counter:

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
        <var>cpu-cycles <data>16953</data></var>
        <var>cache-references <data>441</data></var>
        <var>topdown-fe-bound <data>44681</data></var>
        <var>topdown-be-bound <data>8375</data></var>
        <var>topdown-retiring <data>5786</data></var>
      </div>
    </details>
    <details>
      <summary>
      <var>thr1 <data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var>cpu-cycles <data>45055</data></var>
        <var>topdown-fe-bound <data>111043</data></var>
        <var>topdown-retiring <data>14286</data></var>
      </div>
    </details>
  </div>
</details>
</div>

Some outlier:

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
        <var>cpu-cycles <data>18522</data></var>
        <var>cache-references <data>458</data></var>
        <var>topdown-fe-bound <data>47260</data></var>
        <var>topdown-be-bound <data>11895</data></var>
        <var>topdown-retiring <data>6253</data></var>
      </div>
    </details>
    <var>app_count <data>5</data></var>
    <details>
      <summary>
      <var>thr1 <data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var>cpu-cycles <data>44505</data></var>
        <var>topdown-fe-bound <data>109718</data></var>
        <var>topdown-retiring <data>14318</data></var>
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
  <var>analysisID <data>1</data></var>
  <div class="perf_records_conditions">
  <var>n_thr <data>2</data></var>
  <var>instr <data>1</data></var>
  </div>
  </summary>
  <div class="perf_records_nest">
    <var>app_count <data>5</data></var>
    <details>
      <summary>
      <var>thrMain <data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var>cpu-cycles <data>16758</data></var>
        <var>cache-references <data>453</data></var>
        <var>topdown-fe-bound <data>44161</data></var>
        <var>topdown-be-bound <data>8544</data></var>
        <var>topdown-retiring <data>5803</data></var>
      </div>
    </details>
    <details>
      <summary>
      <var>thr1 <data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var>cpu-cycles <data>45382</data></var>
        <var>topdown-fe-bound <data>109630</data></var>
        <var>topdown-retiring <data>14318</data></var>
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
  <var>instr <data>1</data></var>
  </div>
  </summary>
  <div class="perf_records_nest">
    <var>app_count <data>5</data></var>
    <details>
      <summary>
      <var>thrMain <data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var>cpu-cycles <data>14227</data></var>
        <var>cache-references <data>437</data></var>
        <var>topdown-fe-bound <data>35027</data></var>
        <var>topdown-be-bound <data>7669</data></var>
        <var>topdown-retiring <data>5803</data></var>
      </div>
    </details>
    <details>
      <summary>
      <var>thr1 <data>NA</data></var>
      </summary>
      <div class="perf_records_nest">
        <var>cpu-cycles <data>46176</data></var>
        <var>topdown-fe-bound <data>114118</data></var>
        <var>topdown-retiring <data>14318</data></var>
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
$ python log_to_csv.py log_example.html
$ cat log_example.csv
thr1.topdown-retiring,thr1.cpu-cycles,thr1,thr1.topdown-fe-bound,thrMain,n_thr,thrMain.topdown-retiring,app_count,thrMain.cpu-cycles,thrMain.cache-references,thrMain.topdown-be-bound,thrMain.topdown-fe-bound,instr
14286,45055,NA,111043,NA,2,5786,,16953,441,8375,44681,
14318,44505,NA,109718,NA,2,6253,5,18522,458,11895,47260,
14318,45382,NA,109630,NA,2,5803,5,16758,453,8544,44161,1
14318,46176,NA,114118,NA,2,5803,5,14227,437,7669,35027,1
```
