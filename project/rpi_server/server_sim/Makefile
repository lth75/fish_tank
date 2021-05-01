# CLFLAGS = -g -I. -c
CLFLAGS = -g -I. -c -DX86_SIM
LDFLAGS = -lc -lm

all:server_sim server_trigger

server_sim:cJSON.o server_sim.o
	gcc  -o server_sim cJSON.o server_sim.o $(LDFLAGS)

server_trigger:server_trigger.o cJSON.o
	g++ -o server_trigger cJSON.o server_trigger.o $(LDFLAGS)

.cpp.o:
	g++ $(CLFLAGS) -o $@ $< 

.c.o :
	gcc $(CLFLAGS) -o $@ $< 


clean:
	rm -f *.o
	rm -f server_trigger server_sim
