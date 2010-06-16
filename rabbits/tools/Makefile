SUBDIRS = xterm_rw ramdac chronograph_src
FLAGS_TO_PASS = 

all:
	@for dir in ${SUBDIRS}; do \
		echo ""; \
		echo [Building $$dir ]; \
		(cd $$dir; $(MAKE) --no-print-directory $(FLAGS_TO_PASS);); \
		if [ ! $$? -eq 0 ]; then \
			exit 1; \
		fi; \
	done

install:
	@for dir in ${SUBDIRS}; do \
		echo ""; \
		echo [Installing $$dir ]; \
		(cd $$dir; $(MAKE) --no-print-directory install;); \
		if [ ! $$? -eq 0 ]; then \
			exit 1; \
		fi; \
	done

clean:
	@for dir in ${SUBDIRS}; do \
		echo ""; \
		echo [Cleaning $$dir ]; \
		(cd $$dir; $(MAKE) --no-print-directory clean;); \
		if [ ! $$? -eq 0 ]; then \
			exit 1; \
		fi; \
	done
	@rm -f *~

