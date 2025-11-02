# scripts to run log_to_csv.py
%.html: %.md
	pandoc --from markdown-markdown_in_html_blocks+raw_html \
		--standalone $< > $@

%.csv: %.html
	python log_to_csv.py $<
