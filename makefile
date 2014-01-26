
all: frsc frsc_read

frsc: frsc.c ra_aux.c ra_format.c ra_format_defines.h ra_read_jobfile.c ra_guppi_file.c ra_swallow.c ra_analyze.c
	gcc -o frsc frsc.c -lm

frsc_read: frsc_read.c ra_format.c 
	gcc -o frsc_read frsc_read.c

clean:
	rm frsc frsc_read


