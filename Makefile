out: blogroll/blogroll.html
	mkdir -p $@
	cp $^ $@

blogroll/blogroll.html: blogroll
	cd $< && $(MAKE)
