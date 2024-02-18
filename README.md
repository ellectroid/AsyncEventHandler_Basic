# AsyncEventHandler_Basic
Asynchronous single-threaded event handler  

For demonstration purposes only  

AsyncEventHandler implementation  
Dialect: C++20 (uses \<semaphore\>)  

Properties:  
- 1 thread, sleeps when idle  
- 1 externally provided event handler function for all events  
- 1 externally provided event parameter buffer of user-defined size  
- 4 handler function parameters individual to every event  
- 1 externally provided queue of events of user-defined size  
- Binds event ID number to a set of parameters for the handler  
- Events triggered using event number  
- Supports negative event numbers (for example, for error handling events)  
- Activate/deactivate handler execution on per-event and global level  
- Diverse error codes in case something goes wrong  
- Trivially destructible  
- Buffer memory and a thread object are provided externally (doesn't manage object lifetime of anything)  
- You can change internal pointers at runtime if you're into that sort of thing
- It actually seems to work

Includes a test function with detailed explanation and example of setup, use and error handling.
