## Filter example

This example generates a 2d signal where the first dimension is a superposition of 2 sine waves of frequency 1 and 5 Hz respectively, and the second one is a superposition of 2 sine waves of frequency 1 and 10 Hz respectively. It then implements 2 Butterworth filters of cutoff frequency 2.5 and 6 Hz and generates a file with the taw and filtered signals that can be plotted with the provided python script.


You can plot the signal with the provided script
```
cd examples/02-filters
python plot_signals.py
```

The plot will look like that: 

![](../examples/02-filters.png?raw=true)