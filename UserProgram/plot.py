#!/usr/bin/python

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt



f = open('interrupt_latency_1.csv');
plt.figure()
for line in f:
    data = line.split(',');
    data = [int(x) for x in data];
    plt.plot(list(range(len(data))), data, label='Interrupt latency without load'); 
    
plt.xlabel('Sample');
plt.ylabel('Latency');
plt.savefig('interrupt_latency_without_load.png', dpi=300)
