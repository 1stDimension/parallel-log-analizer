ProgramName=log-analizer
MPICC?=mpicc

DebugFlags=-std=c11 -Wall -g3 -pedantic
ReleseFlags=-Ofast -std=c11 -g0 -Wno-everything

Objects=hashmap.o mpiUtils.o logEntry.o

release: CompileFlags=$(ReleseFlags)
release: $(Objects)
	$(MPICC) $(CompileFlags) -o $(ProgramName) $(Objects) main.c

rebuild: clean release

debug: CompileFlags=$(DebugFlags) -D DEBUG
debug: $(Objects)
	$(MPICC) $(CompileFlags) -o $(ProgramName) $(Objects) main.c

clean:
	rm -rf *.dSYM
	rm -f *.o

hashmap.o : ./libs/c_hashmap/hashmap.c ./libs/c_hashmap/hashmap.h
	$(MPICC) $(CompileFlags) -c $< -o $@

%.o : %.c %.h
	$(MPICC) $(CompileFlags) -c $< -o $@
