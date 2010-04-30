all : tty_term_rw

tty_term_rw : tty_term_rw.cpp
	@echo "Compiling $< ..."
	@g++ -Wall -o $@ $<
	@echo "Copying $@ ..."
	@cp $@ ..

clean :
	@-rm -f *~  tty_term_rw

