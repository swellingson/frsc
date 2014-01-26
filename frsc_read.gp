reset

# uncomment below to get a pretty png instead
# set output "frsc_read.png"
# set term pngcairo enhanced 

set xlabel 'time [s]'

# uncomment below to get Stokes
plot 'frsc_read.dat' using 2:25 with lines title '<|X|^2>', \
     'frsc_read.dat' using 2:30 with lines title '<|Y|^2>'

