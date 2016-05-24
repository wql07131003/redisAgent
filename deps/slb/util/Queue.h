/*
 * Queue.h
 *
 *  Created on: 2010-4-8
 *      Author: Administrator
 */

#ifndef QUEUE_H_
#define QUEUE_H_
#include <typeinfo>
#include <bsl/ResourcePool.h>

namespace forum
{
#ifdef DEBUG
	#define CHECK_POOL(str,debug_rp)  do{ \
		bsl::xnofreepool *debug_pool = static_cast<bsl::xnofreepool *>(&((debug_rp)->get_mempool())); \
		UB_LOG_DEBUG(str"Debug_mempool: pool[%p],buf_size[%lu],buf_left[%lu],buf_free[%lu]", \
		debug_pool->_buffer, debug_pool->_bufcap, debug_pool->_bufleft, debug_pool->_free); \
	}while(0)
#else
	#define CHECK_POOL(str,debug_rp) (void)debug_rp
#endif

	//数组实现循环队列, 每个结点保存T类型数据及状态
	template <class T>
	class Queue
	{
	public:
		/**
		 * @brief  构造函数,使用资源池上分配结点
		**/
		Queue(int queueSize = 100, bsl::ResourcePool * rp)
		{
			try {
				//使用前面extern出来的pool，这里可以分析空间的使用情况
				//extern bsl::xnofreepool * debug_pool;
				//UB_LOG_WARNING("=================%d", debug_pool->_bufleft);
				_queue = rp->create_array<node_t>(queueSize);
				memset(_queue, 0, sizeof (node_t) * queueSize);
				_queueSize = queueSize;
				_nextPos = 0;
			} catch(bsl::Exception& e) {
				_queueSize = 0;
				_nextPos = 0;
				UB_LOG_WARNING("Queue bsl::excepiton queue:%d err:%s %lu", queueSize, e.what(), sizeof(node_t));
			} catch(...) {
				_queueSize = 0;
				_nextPos = 0;
				UB_LOG_WARNING("Queue excepiton queue:%d rp:%p", queueSize, rp);
			}
		}

		/**
		 * @brief 使用resourcePool管理无需回收资源
		**/
		virtual ~Queue()
		{
			_queue = NULL;
			_nextPos = 0;
			_queueSize = 0;
		}

		/**
		 * @brief 创建的队列大小
		**/
		int size() const
		{
			return _queueSize;
		}

		/**
		 * @brief 入队，循环队列队尾插入 一个T类型数据, 队列满时出队到old中
		 * @param value 待插入数据; old 若队列满，通过old出队头数据
		 * @retval true 队列满,队头数据通过old返回(若old非空)
		**/
		bool insert(T value, T* old = NULL)
		{

			//确保_nextPos合法
			node_t& nextNode = _queue[_nextPos];

			if (old && nextNode.valid){
				*old = nextNode.value;
			}
			bool ret = nextNode.valid;

			nextNode.value = value;
			nextNode.valid = true;

			//循环队列
			_nextPos = (_nextPos + 1) % _queueSize;
			return ret;
		}

		/**
		 * @brief 获取前index+1次插入的数据
		 * @param index(>0) 指明前index+1次插入的数据
		 * @retval true 数据存在，前index+1次的数据通过value返回(若value非空)
		**/
		bool getLast(int index, T* value)
		{
			int lastPos = _nextPos - 1 - index;
			while (lastPos < 0){
				lastPos += _queueSize;
			}
			lastPos %= _queueSize;

			node_t& lastNode = _queue[lastPos];
			if (value && lastNode.valid){
				*value = lastNode.value;
			}

			return lastNode.valid;
		}

		/**
		 * @brief dump接口，用于debug，暂时只实现有限的类型
		 */
	    void dump()
	    {
	    	char buf[5*1024] = {0};
	    	char * str = buf;
	    	int len;
	    	int buf_left = sizeof(buf);

	        int nextPos = _nextPos;
			//Queue队尾(前一次插入的数据) 从后向前遍历
	        nextPos = (nextPos + _queueSize -1) % _queueSize;
	        while (nextPos != _nextPos) {
	            node_t& currNode = _queue[nextPos];
	            if (!currNode.valid) {
	            	break;
	            }
	            if (buf_left <= 0) {
	            	break;
	            }
	            const char * type = typeid(currNode.value).name();
	            switch (type[0]) {
	            case 'i':
	            	len = snprintf(str, buf_left, "%d,", currNode.value);
	            	break;
	            case 'j':
	            	len = snprintf(str, buf_left, "%u,", currNode.value);
	            	break;
	            }
	            str = str + len;
	            buf_left = buf_left - len;
				if(buf_left <= 0){
					str[sizeof(buf)-1]='\0';
					break;
				}
	            //printf("%s\n", typeid(currNode.value).name());
				//前一个
	            nextPos = (nextPos + _queueSize -1) % _queueSize;
	        }
	        UB_LOG_TRACE("Queue::dump[%s]", buf);
	    }

	private:
		struct node_t		  /**< Queue结点  */
		{
			T value;
			bool valid;
		};

		node_t* _queue;		  /**< 队列首地址  */
		int _queueSize;		  /**< 创建的队列大小  */
		int _nextPos;		  /**< 下一结点位置  */
	};

}

#endif /* QUEUE_H_ */
