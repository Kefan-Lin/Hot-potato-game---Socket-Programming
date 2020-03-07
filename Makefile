TARGETS=player ringmaster

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

player: player.cpp potato.h
	g++ -g -o $@ $<

ringmaster: ringmaster.cpp potato.h
	g++ -g -o $@ $<

