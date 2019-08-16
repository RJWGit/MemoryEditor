## General Info
A basic memory editor written in C++ that can access memory of a foreign process and read/write to it. The user inputs the name of the window to get a handle to the foreign process and then inputs the value they’re trying to find from within that process’s memory. After finding the value, the user can overwrite the value with their own
## How it works
Uses the windows API to gain access into the virtual memory space of the foreign program and finds the page state of the memory to check if it has been written to. If it has, then read it, else it is ignored. Potential memory addresses are saved that match the user’s desired value so they then can rescan from the new potential matches and see if any changes have occurred. Once the desired memory address has been found the user can then overwrite the content of that memory address with their own desired value. 
##Compiling 
The code should be compiled to match the architecture of the application. For example, if running an x64 application, the code must be compiled to x64 for it work. 

