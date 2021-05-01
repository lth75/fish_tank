# CLFLAGS = -g -I. -c
CLFLAGS = -g -I. -c -DX86_SIM
LDFLAGS = -lc -lm

all:arduino_sim arduino_trigger

arduino_sim:arduino_sim.o
	gcc  -o arduino_sim arduino_sim.o $(LDFLAGS)

arduino_trigger:arduino_trigger.o 
	g++ -o arduino_trigger arduino_trigger.o $(LDFLAGS)

.cpp.o:
	g++ $(CLFLAGS) -o $@ $< 

.c.o :
	gcc $(CLFLAGS) -o $@ $< 


clean:
	rm -f *.o
	rm -f arduino_trigger arduino_sim
