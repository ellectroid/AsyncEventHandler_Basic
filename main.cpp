#include <iostream>
#include <thread>
#include <chrono>
#include "async_event_handler.h"

void event_handler_function(void *arg0, void *arg1, int arg2, int arg3) {
	switch (arg2) {
	case (0):
		std::cout
				<< "asyncEventHandlerEvent: event 0. This is a dedicated event zero message."
				<< std::endl;
		break;
	case (1):
		std::cout
				<< "asyncEventHandlerEvent: event 1. This is a dedicated event one message."
				<< std::endl;
		break;
	default:
		std::cout << "asyncEventHandlerEvent: event " << int(arg2) << "."
				<< std::endl;
		break;
	}
}

int main() {

	std::cout << "====================================" << std::endl;
	std::cout << std::endl;

	//Step 0: allocate memory wherever and however you wish
	std::cout << "Config: allocating memory" << std::endl;
	//alignas(8) char event_handler_param_table[1024] = { 0 }; //ALIGNMENT! creating arbitrary memory buffer for events; note alignment, internal structures hold pointers
	//void** event_handler_param_table_mem_auto_aligned[128] = {0}; //declaring an array of pointers will result in correct alignment
	async_el::AsyncEventHandler::handler_params event_handler_param_table[16]; //specifying length explicitly
	int event_handler_event_queue[32] = { 0 }; //create event queue just as an array of event numbers (used as a ring buffer)
	std::thread my_event_handler_thread; //thread object can be anywhere

	//Step 1: create object
	std::cout << "Config: creating object" << std::endl;
	async_el::AsyncEventHandler my_event_handler; //creating object
	int error = my_event_handler.error(); //returns error code, clears error code in the object
	//most functions will do nothing if error code in the object is set
	//such as all bind functions

	//Step 2: bind memory
	//will not work if error code in the object is set
	//they themselves do not set error code
	std::cout << "Config: binding memory" << std::endl;
	my_event_handler.event_bind_param_table_memory((void*)event_handler_param_table,
			sizeof(event_handler_param_table)); //
	my_event_handler.event_queue_bind_memory(event_handler_event_queue,
			sizeof(event_handler_event_queue)
					/ sizeof(event_handler_event_queue[0]));
	my_event_handler.handler_bind(event_handler_function);
	my_event_handler.thread_bind(&my_event_handler_thread);

	std::cout << "Config: total number of events: " << int(my_event_handler.event_capacity()) << std::endl;
	std::cout << "Config: event queue capacity: " << int(my_event_handler.event_queue_capacity()) << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	//Step 3: start/restart the thread
	//will not work if error code in the object is set
	//sets error code
	std::cout << "Config: starting thread" << std::endl;
	my_event_handler.thread_start();
	if ((error = my_event_handler.error()) != 0) {
				//error handling
				//error code in the object is cleared
				std::cout << "asyncEventHandlerError: error " << int(error)
						<< std::endl;

	}

	//Step 4: add events
	//will not work if error code in the object is set
	//sets error code
	//note how I'm passing event ID into the handler using arg2
	//it is not strictly necessary
	std::cout << "Config: binding events" << std::endl;
	my_event_handler.event_bind(0, (void*) 0x06, (void*) 0x07, 0, 9);
	my_event_handler.event_bind(1, 0, 0, 1, 0);
	my_event_handler.event_bind(2, 0, 0, 2, 0);
	my_event_handler.event_bind(3, 0, 0, 3, 0);
	my_event_handler.event_bind(-1, 0, 0, -1, 0);
	my_event_handler.event_bind(-2, 0, 0, -2, 0);
	my_event_handler.event_bind(-3, 0, 0, -3, 0); //place into event my_event_handler.event_capacity() - 3
	std::cout << "Config: binding out of bounds event" << std::endl;
	my_event_handler.event_bind(INT_MAX, 0, 0, -3, 0); //bad event code, will set error code
	if ((error = my_event_handler.error()) != 0) {
			//error handling
			//error code in the object is cleared
			std::cout << "asyncEventHandlerError: error " << int(error)
					<< std::endl;

	}

	//Step 5: enable events so they're not ignored
	//will not work if error code in the object is set
	//sets error code
	std::cout << "Config: enabling individual events" << std::endl;
	my_event_handler.event_enable(0);
	my_event_handler.event_enable(1);
	my_event_handler.event_enable(2);
	//my_event_handler.event_enable(3); //didn't enable event 3
	my_event_handler.event_enable(-1);
	my_event_handler.event_enable(-2);
	my_event_handler.event_enable(-3);
	if ((error = my_event_handler.error()) != 0) {
		//error handling
		//error code in the object is cleared
		std::cout << "asyncEventHandlerError: error " << int(error)
				<< std::endl;

	}

	//Step 6: enable queue so events are executed
	//will not work if error code in the object is set
	//does not set error code
	std::cout << "Config: enabling events globally" << std::endl;
	//my_event_handler.event_queue_disable(); //default state; accepts new events into the queue, but handler thread just sleeps
	my_event_handler.event_queue_enable(); //accepts & executes events

	//Step 7: waiting until thread is ready (optional)
	std::cout << "Config: waiting until thread is ready (optional)"
			<< std::endl;
	while (!my_event_handler.thread_ready())
		; //indicates if handler thread is running

	std::cout << "Config: configuration finished" << std::endl;
	std::cout << "====================================" << std::endl;
	std::cout << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	//CONFIGURATION FINISHED

	//TEST

	std::cout << "Test: triggering events" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//will not work if error code in the object is set
	//sets error code
	my_event_handler.event_trigger(0);
	my_event_handler.event_trigger(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::cout << "Test: triggering disabled events" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	my_event_handler.event_trigger(3); //event not previously enabled
	if ((error = my_event_handler.error()) != 0) {
		//error handling
		//error code in the object is cleared
		std::cout << "asyncEventHandlerError: error " << int(error)
				<< std::endl;

	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::cout << "Test: triggering events with negative indices" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	my_event_handler.event_trigger(-1);
	my_event_handler.event_trigger(-2);
	my_event_handler.event_trigger(my_event_handler.event_capacity() - 2); //same as trigger(-2)
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::cout << "Test: disabling global interrupt trigger, triggering events"
			<< std::endl;
	my_event_handler.event_queue_disable();
	my_event_handler.event_trigger(0);
	my_event_handler.event_trigger(1);
	my_event_handler.event_trigger(2);
	my_event_handler.event_trigger(1);
	my_event_handler.event_trigger(-2);
	my_event_handler.event_trigger(0);

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	std::cout
			<< "Test: enabling global interrupt trigger, events must be processed"
			<< std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	my_event_handler.event_queue_enable();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::cout << "Test: misconfiguring to see error flags at work "
			<< std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	my_event_handler.event_trigger(0);
	my_event_handler.event_trigger(1);
	my_event_handler.event_trigger(2);
	my_event_handler.event_trigger(1);
	my_event_handler.event_trigger(-2);
	my_event_handler.event_trigger(0);
	my_event_handler.event_queue_reset();
	my_event_handler.event_trigger(0);
	if ((error = my_event_handler.error()) != 0) {
		//error handling
		//error code in the object is cleared
		std::cout << "asyncEventHandlerError: error " << int(error)
				<< std::endl;

	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	std::cout << "Test: stopping the thread" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	my_event_handler.thread_stop_detach();
	std::cout << "Test: thread exited" << std::endl;
	return 0;
}
