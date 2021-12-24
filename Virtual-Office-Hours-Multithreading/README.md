# Virtual-Office-Hours-Multithreading

This program demonstrates the use of threads to build a robust parallel program free of deadlocks and race conditions.\
This is acheived through proper placement of mutex and semaphores to guard critical regions and avoid conditions that cause deadlocks.\

**Scenario:** A virtual office hour scheduling is implemented for a professor who has 1000 students (each represented by a single thread) sign up for his/her office hours.
**Problem statement(s):**

1. The professor teaches two sections and can only have 3 students at once in the office.
2. Students that arrive after the office is full will wait outside (Note: Students arriving = new thread bring created).
3. If a student waiting outside is from a different section than students present in the office, the waiting student cannot enter the office.
4. To be fair to both classes the professor is only allowed to help 5 consequtive students from the same section (unless no student from the other section is waiting).
5. The professor will take a break after helping 10 students.
6. No student should be waiting outside if he/she meets the requirement to enter. (i.e. No thread starvation)

The program expects an input file containing information for students where each line has 3 numbers. For example:\
1 0 2\
0 0 7\
1 0 5\

The first number refers to the students section (0:A 1:B). The second number specifies the time in seconds between the arrival of the current student and the previous student.
The third number specifies the amount of time(in seconds) the student spends asking questions.

**The program can be compiled in the following way:**\
gcc officehours.c -o officehours -lpthread\
./officehours <testfile.txt>\
