all: 
	g++ -Wall -Wextra -Wshadow -DB_64 -O3 -I../../include bddqueen.cc ../../lib/BDD64.a -o bddqueen

32: 
	g++ -Wall -Wextra -Wshadow -O3 -I../../include bddqueen.cc ../../lib/BDD32.a -o bddqueen

clean:
	rm -f *.o *.exe *~

