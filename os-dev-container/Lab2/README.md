# Lab Overview
## Banking System Problem  
Consider a simple banking system that maintains bank accounts for its users. Every bank account has a balance (represented by an integer variable amount). The bank allows deposits and withdrawals from these bank accounts, represented by the two functions: `deposit` and `withdraw`. These functions are passed an integer value that is to be deposited or withdrawn from the bank account. Assume that a husband and wife share a bank account. The husband only withdraws from the account and the wife only deposits into 
the account using the withdraw and deposit functions respectively. Race condition is possible when the shared data (amount) is accessed by these two functions concurrently. In this lab you are to write a C program that provides a critical section solution to the Banking System Problem using mutex locks provided by the POSIX `Pthreads` API. See the `Lab 2.pdf` for more details.

# Makefile

 Run make to compile part 1 or part 2 accordingly:

 ```bash
$ make lab2part1
$ ./lab2part1 100 50
 ```

 ```bash
$ make lab2part2
$ ./lab1part2 100 
 ```