# CLFLAGS = -g -I. -c
CLFLAGS = -g -I. -c -DX86_SIM
LDFLAGS = -lc -lm

all:rpi_sim rpi_trigger

rpi_sim:cJSON.o rpi_sim.o
	gcc  -o rpi_sim cJSON.o rpi_sim.o $(LDFLAGS)

rpi_trigger:rpi_trigger.o cJSON.o
	g++ -o rpi_trigger cJSON.o rpi_trigger.o $(LDFLAGS)

.cpp.o:
	g++ $(CLFLAGS) -o $@ $< 

.c.o :
	gcc $(CLFLAGS) -o $@ $< 


clean:
	rm -f *.o
	rm -f rpi_trigger rpi_sim
