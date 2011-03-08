INC = -I.
LIB =
OBJ = hac.o

CFLAGS = $(INC) -I${RABBITS_DIR}/platforms/roger/soft/linux/drivers/rabbits_host_access

hac : $(OBJ)
	$(CC) -o $@ $^ $(LIB)

clean :
	$(RM) $(OBJ)

realclean : clean
	$(RM) hac

