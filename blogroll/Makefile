SHELL := bash -o pipefail

CC := tcc
DEFINES := -D COMPILER="$(shell $(CC) -v 2>&1 | tail -n1)" 	\
					 -D GENERATED_DATE="$(shell date +"%h %d, %Y")" 	\
					 -D GENERATED_TIME="$(shell date +"%T")"

define newline


endef

.PHONY: entries sources clean test

blogroll.html: template.html entries sources
	$(CC) $(DEFINES) -E -P -xc $< > $@

entries: generate list.txt
	@echo -n "" > $@
	$(foreach feed,$(shell xargs -a list.txt), \
		curl -s $(feed) | ./generate | sort -r >> $@ $(newline) \
	)
	sort -ro $@ $@

sources: list.txt
	cut -d'/' -f3 $< | xargs -I'*' echo "SOURCE(*)" > $@

generate: generate.c
	$(CC) -o $@ $<

test: generate list.txt
	@echo -n "" > entries
	tail -n 1 list.txt | xargs curl -s | ./generate | sort -r >> entries
	@echo -n "" > sources
	tail -n 1 list.txt | cut -d'/' -f3 | xargs -I'*' echo "SOURCE(*)" >> sources
	$(CC) $(DEFINES) -E -P -xc template.html > blogroll.html

clean: generate entries blogroll.html
	rm $^
