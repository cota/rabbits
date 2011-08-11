all:

cscope: cleanscope
	find . -name '*.cpp' > cscope.files
	find . -name '*.c' >> cscope.files
	find . -name '*.h' >> cscope.files
	cscope -b -q -k

cleanscope:
	rm -f cscope.*

.PHONY: all cscope cleanscope
