/*
 * Lock.h
 *
 *  Created on: 2009-12-26
 *      Author: Administrator
 */

#ifndef LOCK_H_
#define LOCK_H_

#include <pthread.h>
#include <sys/types.h>

namespace forum
{

	class Lock{
		public:
			Lock();
			virtual ~Lock();
			int lock();
			/**
			 * @brief 超时加锁
			 * @retval 超时时间内无法获得锁，返回非零且状态码, 成功返回0
			 **/
			int timed_lock(uint millisecond);
			void unlock();
		private:
			pthread_mutex_t _mutex;
	};


	class AutoLock{
		public:
			AutoLock(pthread_mutex_t * &lock){
				pthread_mutex_lock(lock);
				_lock = lock;
			}
			~AutoLock(){
				pthread_mutex_unlock(_lock);
			}
		private:
			pthread_mutex_t * _lock;
	};

}

#endif /* LOCK_H_ */
