# my C shell - a shell I implemented in C

The project is a tiny shell implemented in C that supports job control. It parses user input and evaluates the command. It has the following built-in commands: 

-	jobs: print the job-list
-	fg <job>: send a program in the background to run in the foreground
-	bg <job>: send a program to run in the background
-	<cmd> &: run a program in the background
-	quit: exit shell

The project involves knowledge of process control and signalling. The tricky part is in masking the right signals at the right time and reverting to previous conditions appropriately. 

