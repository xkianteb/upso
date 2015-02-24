echo "set yrange [$2:$3]
set xrange [$4:$5]
unset key
unset border
unset xtics
unset ytics
set term png
set output \"$1.png\"
plot \"$1.txt\" using 1:2 with points pointtype 7
" | gnuplot

