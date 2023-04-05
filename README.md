# TCPSocketServer
A multithreaded TCP socket server for a stock exchange

When started the program connects to the specified SQL server and requests customer and stock information, which then gets inserted into the customer map and stock vector.
After the startup function completes, vectors are resized according to the amount of threads. The threads are then created and set to run in a loop waiting for client sockets
to receive, process and send data to. The main thread then indefinitely loops accepting connections and passing the connections on to the worker threads. I programmed it
evenly distribute connections between threads so that one thread is not overloaded with work while the other threads wait, the function that does the distributing is called
SetManager. To shutdown the server the user has to type "Shutdown" into the console, this will set a flag for the worker threads and wait for them to join, when the worker
threads see the flag is set they will end all connections and break out of the loop resulting in joining the main thread. The main thread will then start cleaning up
and update the SQL server with the information.

Notes 

-I did not use any mutexes or semaphores because each thread accesses different objects and elements so there isn't a chance of race conditions.

-The server uses a library to hash the password and compare the hash with the database.

-Each thread can support up to 64 sockets which is equal to the FD_SETSIZE definition.
