CLFLAGS = -g -I. -c
LDFLAGS = -lc -lm

all:cloud_cgi

cloud_cgi:cJSON.o cloud_cgi.o
	gcc  -o cloud_cgi cJSON.o cloud_cgi.o $(LDFLAGS)

#cloud_cgi.o: cloud_cgi.cpp
.cpp.o:
	g++ $(CLFLAGS) -o $@ $< 

.c.o :
# cJSON.o: cJSON.c
	gcc $(CLFLAGS) -o $@ $< 


clean:
	rm *.o
	rm cloud_cgi 
