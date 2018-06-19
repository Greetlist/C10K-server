.PHONY all:
	gcc util.c ipcunix.c main.c -lpthread
