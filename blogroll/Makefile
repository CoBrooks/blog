SHELL := bash -o pipefail

CC := tcc
DEFINES := -D COMPILER="$(shell $(CC) -v 2>&1 | tail -n1)" 	\
					 -D GENERATED_DATE="$(shell date +"%h %d, %Y")" 	\
					 -D GENERATED_TIME="$(shell date +"%T")"

define newline


endef

blogroll.html: template.html entries
	$(CC) $(DEFINES) -E -P -xc $< > $@

.PHONY: entries

entries: generate list.txt
	@echo -n "" > $@
	$(foreach feed,$(shell xargs -a list.txt), \
		curl -s $(feed) | ./generate | sort -r | head -n3 >> $@ $(newline) \
	)
	sort -ro $@ $@

generate: generate.c
	$(CC) -o $@ $<

.PHONY: clean

clean: generate entries blogroll.html
	rm $^
