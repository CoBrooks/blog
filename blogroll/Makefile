SHELL := bash -o pipefail
CC := tcc

define newline


endef

blogroll.html: template.html out.i
	$(CC) -E -P -xc $< > $@

out.i: generate
	$(foreach feed,$(shell xargs -a list.txt), \
		curl -s $(feed) | ./generate | sort -r | head -n3 >> $@ $(newline) \
	)
	sort -ro $@ $@

generate: generate.c
	$(CC) -o $@ $<

.PHONY: clean

clean: generate out.i blogroll.html
	rm $^
