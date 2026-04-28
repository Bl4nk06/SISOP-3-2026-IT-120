all: orion eternal

orion: orion.c arena.h
	gcc orion.c -o orion -lpthread

eternal: eternal.c arena.h
	gcc eternal.c -o eternal -lpthread

clear_ipc:
	ipcrm -M 0x1234
	ipcrm -Q 0x5678

clean:
	rm -f orion eternal players.db