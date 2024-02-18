#include "async_event_handler.h"

namespace async_el{

AsyncEventHandler::AsyncEventHandler() :
		semaphore_(0) {
	thread_status_ = 0;
	thread_signal_ = 0;
	thread_ = nullptr;
	errcode_ = 0;
	handlerfunc_ = nullptr;
	event_param_table_mem_ = nullptr;
	event_param_table_mem_capacity_ = 0;
	event_queue_ = nullptr;
	event_queue_capacity_ = 0;
	event_queue_level_ = 0;
	first_empty_index_ = 0;
	next_to_execute_index_ = 0;
	event_queue_enable_ = false;
}

void AsyncEventHandler::handler_bind(handlerfunc_t func) {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	handlerfunc_ = func;
}

void AsyncEventHandler::handler_unbind() {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	handlerfunc_ = nullptr;
}

void AsyncEventHandler::thread_bind(std::thread *thr) {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	thread_ = thr;
}

void AsyncEventHandler::thread_unbind() {
	if (errcode_ != NoError)
		return;
	thread_stop_join(); //lock inside
	std::unique_lock<std::mutex> lk(access_mutex_);
	thread_ = nullptr;
}

void AsyncEventHandler::thread_start() {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	if (thread_ == nullptr)
		errcode_ = InvalidThreadObject;
	if (thread_status_ != 0) {
		lk.unlock();
		thread_stop_join();
	}
	*thread_ = std::thread(&AsyncEventHandler::threadfunc, this);
}

int AsyncEventHandler::thread_ready() {
	return (int) (thread_status_ == true);
}

void AsyncEventHandler::thread_stop_detach() {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	thread_signal_ = 1;
	if (thread_status_ != 0 || thread_->joinable()) {
		lk.unlock();
		thread_->detach();
		semaphore_.release();
	}

}
void AsyncEventHandler::thread_stop_join() {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	if (thread_status_ != 0 || thread_->joinable()) {
		thread_signal_ = 1;
		lk.unlock();
		semaphore_.release();
		thread_->join();
	}
}
int AsyncEventHandler::error() {
	std::unique_lock<std::mutex> lk(access_mutex_);
	int retval = errcode_;
	errcode_ = 0;
	return retval;
}
void AsyncEventHandler::event_bind_param_table_memory(void *memory,
		int bytelen) {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	event_param_table_mem_ = memory;
	int capacity_counter = 0;
	char *memory_ptr = (char*) memory;
	char *memory_end_ptr = &((char*) (memory))[bytelen];
	if((unsigned int)bytelen < sizeof(handler_params)) {
		event_param_table_mem_capacity_ = 0;
		return;
	}
	while ((char*)(&((handler_params*) (memory_ptr))[1]) < memory_end_ptr) {
		memory_ptr = (char*) (&((handler_params*) (memory_ptr))[1]);
		capacity_counter++;
	}
	capacity_counter++;
	event_param_table_mem_capacity_ = capacity_counter;
	return;
}
int AsyncEventHandler::event_capacity() {
	return event_param_table_mem_capacity_;
}
bool AsyncEventHandler::event_bind(int event, void *arg0, void *arg1, int arg2,
		int arg3) {
	if (errcode_ != NoError)
		return false;
	std::unique_lock<std::mutex> lk(access_mutex_);
	if (event_param_table_mem_ == nullptr) {
			errcode_ = InvalidParamTableObject;
			return false;
	}
	if (event_id_out_of_bounds(event)) {
		errcode_ = EventOutOfBounds;
		return false; //out of bounds
	}
	if (event < 0)
		event = event_param_table_mem_capacity_ + event; //if negative, wrap around from the end
	((handler_params*) (event_param_table_mem_))[event].enable_ = 0;
	((handler_params*) (event_param_table_mem_))[event].arg0 = arg0;
	((handler_params*) (event_param_table_mem_))[event].arg1 = arg1;
	((handler_params*) (event_param_table_mem_))[event].arg2 = arg2;
	((handler_params*) (event_param_table_mem_))[event].arg3 = arg3;

	return true;
}

void AsyncEventHandler::event_unbind(int event) {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	if (event_param_table_mem_ == nullptr) {
				errcode_ = InvalidParamTableObject;
				return;
	}
	if (event_id_out_of_bounds(event)) {
		errcode_ = EventOutOfBounds;
		return; //out of bounds
	}
	if (event < 0)
		event = event_param_table_mem_capacity_ + event; //if negative, wrap around from the end
	((handler_params*) (event_param_table_mem_))[event].enable_ = 0;
	((handler_params*) (event_param_table_mem_))[event].arg0 = nullptr;
	((handler_params*) (event_param_table_mem_))[event].arg1 = nullptr;
	((handler_params*) (event_param_table_mem_))[event].arg2 = 0;
	((handler_params*) (event_param_table_mem_))[event].arg3 = 0;

	//if you removed the event, but it was already in the event queue,
	//it will still be in the queue, but disabled, so it will just skip
	//the handler

	return;
}

void AsyncEventHandler::event_enable(int event) {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	if (event_param_table_mem_ == nullptr) {
			errcode_ = InvalidParamTableObject;
			return;
	}
	if (event_id_out_of_bounds(event)) {
		errcode_ = EventOutOfBounds;
		return; //out of bounds
	}
	if (event < 0)
		event = event_param_table_mem_capacity_ + event; //if negative, wrap around from the end
	((handler_params*) (event_param_table_mem_))[event].enable_ = 1;

}
void AsyncEventHandler::event_disable(int event) {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	if (event_param_table_mem_ == nullptr) {
				errcode_ = InvalidParamTableObject;
				return;
		}
	if (event_id_out_of_bounds(event)) {
		errcode_ = EventOutOfBounds;
		return; //out of bounds
	}
	if (event < 0)
		event = event_param_table_mem_capacity_ + event; //if negative, wrap around from the end
	((handler_params*) (event_param_table_mem_))[event].enable_ = 0;

}

bool AsyncEventHandler::event_is_enabled(int event) {
	if (errcode_ != NoError)
		return false;
	std::unique_lock<std::mutex> lk(access_mutex_);
	if (event_id_out_of_bounds(event)) {
		errcode_ = EventOutOfBounds;
		return false; //out of bounds
	}
	if (event_param_table_mem_ == nullptr) {
				errcode_ = InvalidParamTableObject;
				return false;
	}
	if (event < 0)
		event = event_param_table_mem_capacity_ + event;
	return (((handler_params*) (event_param_table_mem_))[event].enable_ != 0);
}

void AsyncEventHandler::event_queue_bind_memory(int *event_queue,
		int event_queue_elem_count) {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	event_queue_ = event_queue;
	event_queue_capacity_ = event_queue_elem_count;
	event_queue_level_ = 0;
	first_empty_index_ = 0;
	next_to_execute_index_ = 0;
}

int AsyncEventHandler::event_queue_capacity(){
	return event_queue_capacity_;
}
void AsyncEventHandler::event_queue_clear() {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	event_queue_level_ = 0;
	first_empty_index_ = 0;
	next_to_execute_index_ = 0;
}
void AsyncEventHandler::event_queue_reset() {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	event_queue_ = nullptr;
	event_queue_capacity_ = 0;
	event_queue_level_ = 0;
	first_empty_index_ = 0;
	next_to_execute_index_ = 0;
	event_queue_enable_ = false;
}

void AsyncEventHandler::event_queue_enable() {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	event_queue_enable_ = true;
	lk.unlock();
	semaphore_.release();
}
void AsyncEventHandler::event_queue_disable() {
	if (errcode_ != NoError)
		return;
	std::unique_lock<std::mutex> lk(access_mutex_);
	event_queue_enable_ = false;
}

bool AsyncEventHandler::event_queue_is_enabled() {
	return event_queue_enable_;
}

bool AsyncEventHandler::event_trigger(int event) {
	if (errcode_ != NoError)
		return false;
	std::unique_lock<std::mutex> lk(access_mutex_);

	if (event_queue_ == nullptr) {
			errcode_ = InvalidEventQueueObject;
			return false;
		}
	if (event_param_table_mem_ == nullptr) {
			errcode_ = InvalidParamTableObject;
			return false;
	}

	if (event_id_out_of_bounds(event)) {
		errcode_ = EventOutOfBounds;
		return false; //out of bounds
	}

	if (event_queue_level_ == event_queue_capacity_) {
		errcode_ = EventQueueFull;
		return false; //out of bounds or queue is full
	}



	if (event < 0)
		event = event_param_table_mem_capacity_ + event; //if negative, wrap around from the end

	bool event_enabled =
			(((handler_params*) (event_param_table_mem_))[event].enable_ != 0);
	if (event_enabled) {
		event_queue_[first_empty_index_] = event;
		first_empty_index_++;
		first_empty_index_ %= event_queue_capacity_;
		event_queue_level_++;
	} else {
		errcode_ = EventTriggerDisabled;
		return false;
	}

	if (event_queue_enable_) {
		lk.unlock();
		semaphore_.release();
	}
	return true;
}

void AsyncEventHandler::threadfunc() {
	std::unique_lock<std::mutex> lk(access_mutex_);
	thread_status_ = 1;
	lk.unlock();
	while (1) {
		while (1) {
			lk.lock();
			if (thread_signal_ != 0)
				break;
			if (errcode_ != NoError)
				goto event_polling_end;
			if (event_queue_enable_ == false) {
				goto event_polling_end;
			}
			if (event_queue_level_ > 0)
				break;
			event_polling_end: lk.unlock();
			semaphore_.acquire(); //sleep until signaled
		}
		//mutex is still locked
		int event; //so goto doesn't cry
		handlerfunc_t hndlr;
		int hndlr_en;
		void *arg0;
		void *arg1;
		int arg2;
		int arg3;
		//process signals
		switch (thread_signal_) {
		case (1):
			thread_signal_ = 0;
			goto thread_exit;
		default:
			thread_signal_ = 0;
			break;
		}

		if (event_queue_ == nullptr) {
			errcode_ = InvalidEventQueueObject;
			goto event_loop_end;
		}
		if (event_param_table_mem_ == nullptr) {
			errcode_ = InvalidParamTableObject;
			goto event_loop_end;
		}

		//copy current params onto stack so we can have unlocked mutex during handler execution
		event = event_queue_[next_to_execute_index_];
		hndlr = handlerfunc_;
		hndlr_en = ((handler_params*) (event_param_table_mem_))[event].enable_;
		arg0 = ((handler_params*) (event_param_table_mem_))[event].arg0;
		arg1 = ((handler_params*) (event_param_table_mem_))[event].arg1;
		arg2 = ((handler_params*) (event_param_table_mem_))[event].arg2;
		arg3 = ((handler_params*) (event_param_table_mem_))[event].arg3;
		next_to_execute_index_++;
		next_to_execute_index_ %= event_queue_capacity_;
		event_queue_level_--;
		lk.unlock();

		if (hndlr_en == 1) {
			if (hndlr == nullptr) {
				this->event_queue_disable();
				errcode_ = InvalidHandlerObject;
				goto event_loop_end;
			}
			hndlr(arg0, arg1, arg2, arg3);
		}

		event_loop_end: ;

	}
	thread_exit: thread_status_ = 0;
}

bool AsyncEventHandler::event_id_out_of_bounds(int event) {
	//mutex is already taken, internal function
	if (errcode_ != NoError)
		goto end;
	if ((event >= event_param_table_mem_capacity_)
			|| (-event >= event_param_table_mem_capacity_)) {
		errcode_ = EventOutOfBounds;
		return true;
	}
	end: return false;
}

}
