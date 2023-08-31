#!/usr/bin/python3

import numpy as np
import matplotlib.pyplot as plt

# read data from file
data = np.loadtxt('example_signal.txt', delimiter='\t', skiprows=1)

# extract time and signal
t = data[:,0]
raw_signal = data[:,1:3]
filtered_signal_1 = data[:,3:5]
filtered_signal_2 = data[:,5:7]

# plot raw and filtered signals in two subfigures
plt.figure()
plt.subplot(2,1,1)
plt.plot(t, raw_signal[:,0], label='raw signal')
plt.plot(t, filtered_signal_1[:,0], label='filtered signal 1')
plt.plot(t, filtered_signal_2[:,0], label='filtered signal 2')
plt.title('Example signal and filter')
plt.legend()
plt.subplot(2,1,2)
plt.plot(t, raw_signal[:,1], label='raw signal')
plt.plot(t, filtered_signal_1[:,1], label='filtered signal 1')
plt.plot(t, filtered_signal_2[:,1], label='filtered signal 2')
plt.xlabel('time [s]')
plt.legend()
plt.show()

