# Airline Check-In Simulation

Troy Dwyer

## Overview
A multithreaded airline check-in simulation in C using the POSIX pthread library. 
The program uses concurrent clerks and customers and measures real-time waiting.

## Included files
- ACS.c
- queue.c/queue.h
- customer.c/customer.h
- Makefile

## Input

Input files should be formatted as such:

3  
1:0,0,5  
2:1,1,3  
3:0,2,4

The first line is the number of customers in the simulation.

The subsequent lines are formatted as such:

Customer ID:Class,Arrival Time,Service Time

Class: 0 = Economy, 1 = Business.  
Arrival and service time are in tenths of a second.

## Compilation and Execution

Step 1: Use 'make' in a Linux environment in the same directory to compile the project. This will   
        create an ACS executable  
Step 2: Run ./ACS <input file name> (Input file in same directory)



