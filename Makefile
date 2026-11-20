define EXAMPLE_LOG_SESSION_HEADER :=
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

Using Intel perfmon description of events, and RAW PMU events in `perf_event`:
```
./dump_perfmon.py GenuineIntel-6-BE known_events_gracemont > known_events_gracemont.hpp
```


# testing an idea 1

Metrics with a basic config without the app counter:
endef

define EXAMPLE_LOG_SESSION_PANDOC =

# Render HTML with Pandoc

Add the `--from` option to make Pandoc correctly treat custom tags as
not a part of markdown:
```bash
$ pandoc --from markdown-markdown_in_html_blocks+raw_html \
    --standalone log_example.md  > log_example.html
```

HTML can be parsed on its own:
```Python
>>> from bs4 import BeautifulSoup
>>> example_s = "..."
>>> soup = BeautifulSoup(example_s, 'html.parser')
>>> soup.select_one("var[data-name=analysisID] > data").contents
['1']
```
endef

build/example:
	cmake --build build --target example

log_example.md: build/example
	$(file > $@,$(EXAMPLE_LOG_SESSION_HEADER))
	build/example >> $@

%.html: %.md
	# the pandoc-csv2table filter is installed via Haskell's cabal:
	# cabal install pandoc-csv2table
	# https://hackage.haskell.org/package/pandoc-csv2table
	# install cabal from get-ghcup.haskell.org, not the distro package
	pandoc --from markdown-markdown_in_html_blocks+raw_html \
		--citeproc \
		--filter pandoc-csv2table \
		--standalone $< > $@

%.csv: %.html
	python log_to_csv.py $<

log_example_final.md: log_example.md log_example.csv
	cp log_example.md log_example_final.md
	$(file >> $@,$(EXAMPLE_LOG_SESSION_PANDOC))
	#
	echo "And parse it to CSV:" >> log_example_final.md
	#echo '![The data from the whole session](./log_example.csv)' >> $@
	# include the content to keep it in case of rendering failure
	echo '```{.table caption="The data of the whole session" header="yes"}' >> $@
	cat log_example.csv >> $@
	echo '```' >> $@
