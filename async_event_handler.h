/*
 * async_event_handler.h
 *
 *  Created on: Feb 17, 2024
 *      Author: user
 */

#ifndef ASYNC_EVENT_HANDLER_H_
#define ASYNC_EVENT_HANDLER_H_

#include <mutex>
#include <thread>
#include <semaphore>

namespace async_el{

class AsyncEventHandler{

public:
	enum ErrCode{
			NoError = 0,
			InvalidThreadObject = -1,
			InvalidEventQueueObject = -2,
			InvalidParamTableObject = -3,
			InvalidHandlerObject = -4,
			EventOutOfBounds = -5,
			EventQueueFull = -6,
			EventTriggerDisabled = -7,
	};
	typedef struct arg_list{int enable_; void* arg0; void* arg1; int arg2; int arg3;} handler_params;
private:
	std::mutex access_mutex_;
	std::thread* thread_; //pointer!
	std::counting_semaphore<32767> semaphore_;
	int thread_status_;
	int thread_signal_;
	int errcode_;

	typedef void (*handlerfunc_t)(void*, void*, int, int);
	handlerfunc_t handlerfunc_;
	void* event_param_table_mem_;
	int event_param_table_mem_capacity_;

	int* event_queue_;
	int event_queue_capacity_;
	int event_queue_level_;
	int first_empty_index_;
	int next_to_execute_index_;
	bool event_queue_enable_;
public:
	AsyncEventHandler();
	void handler_bind(handlerfunc_t func);
	void handler_unbind();
	void thread_bind(std::thread* thr);
	void thread_unbind();
	void thread_start();
	int thread_ready();
	void thread_stop_detach();
	void thread_stop_join();
	int error();
	void event_bind_param_table_memory(void* memory, int bytelen);
	int event_capacity();
	bool event_bind(int event, void* arg0, void* arg1, int arg2, int arg3);
	void event_unbind(int event);
	void event_enable(int event);
	void event_disable(int event);
	bool event_is_enabled(int event);
	void event_queue_bind_memory(int* event_queue, int event_queue_elem_count);
	int event_queue_capacity();
	void event_queue_clear();
	void event_queue_reset();
	void event_queue_enable();
	void event_queue_disable();
	bool event_queue_is_enabled();
	bool event_trigger(int event);
private:
	void threadfunc();
	bool event_id_out_of_bounds(int event);

};


}

#endif /* ASYNC_EVENT_HANDLER_H_ */
