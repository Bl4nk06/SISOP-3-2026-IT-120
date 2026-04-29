all: orion eternal

orion: orion.c arena.h
	gcc orion.c -o orion -lpthread

eternal: eternal.c arena.h
	gcc eternal.c -o eternal -lpthread

# Membersihkan Shared Memory dan Message Queue berdasarkan Key di arena.h
clear_ipc:
	ipcrm -M 0x9988 || true
	ipcrm -Q 0x8899 || true
	rm -f players.db
	@echo "IPC Cleared and Database Deleted."

clean:
	rm -f orion eternal players.db
	@echo "Binaries and Database cleaned."