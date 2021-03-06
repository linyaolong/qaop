#include<cstdio>
#include<mutex>
#include "../qaop.h"
#include<iostream>
namespace util {
	class Item {
		friend class Queue;
		Item* next;
		public:
		Item() : next(0){}
	};
	class Queue {
			Item* first;
			Item* last;
		public:
			Queue() : first(0), last(0) {}
			void enqueue( Item* item ) {
				printf( "  > Queue::enqueue()\n" );
				if( last ) {
					last->next = item;
					last = item;
				} else
					last = first = item;
				printf( "  < Queue::enqueue()\n" );
			}
			Item* dequeue() {
				printf("  > Queue::dequeue()\n");
				Item* res = first;
				if( first == last )
					first = last = 0;
				else
					first = first->next;
				printf("  < Queue::dequeue()\n");
				return res;
			}
	}; // class Queue
} // namespace util

namespace util {

template < typename _Queue, typename _Item >
struct jointpoint_ : public _Queue::this_t {
typedef jointpoint_ this_t;
typedef typename _Queue::this_t base_t;
typedef typename _Queue::fulltype_t fulltype_t;

jointpoint_() {}

virtual void enqueue (_Item * item) {
	printf("invoke enqueue\n");
	qaop::invoke(dynamic_cast<fulltype_t *>(this), &jointpoint_::enqueue, item);
};

virtual _Item *  dequeue () {
	printf("invoke dequeue\n");
	return qaop::invoke(dynamic_cast<fulltype_t *>(this), &jointpoint_::dequeue);
}

};

template <typename _Class>
using Jointpoint_= jointpoint_ <_Class, util::Item>;

template < typename _Queue, typename _Item >
struct count_ : public _Queue::this_t {
typedef count_ this_t;
typedef typename _Queue::this_t base_t;
typedef typename _Queue::fulltype_t fulltype_t;

int counter;
count_():counter(0) {}

std::function<int()> count_advice_add (std::function<int()> &f) {
	return [=](){
		f();
		this->counter++;
		return 0;
	};
}

std::function<int()> count_advice_remove (std::function<int()> &f) {
	return [=](){
		f();
		if(counter > 0)
			this->counter--;
		return 0;
	};
}
	

int count() const { return  counter;}

};

template <typename _Class>
using Count_= count_ <_Class, util::Item>;

template < typename _Queue, typename _Item >
struct lock_ : public _Queue::this_t {
	typedef lock_ this_t;
	typedef typename _Queue::this_t base_t;
	typedef typename _Queue::fulltype_t fulltype_t;
	std::mutex lock;

std::function<int()>  lock_advice (std::function<int()> & f) {
		return [=]() {
			lock.lock();
			try {
				f();
			} catch (...) {
				lock.unlock();
				throw;
			}
			lock.unlock();
			return 0;
		};
}

};

template <typename _Class>
using Lock_= lock_ <_Class, util::Item>;

}


/* start config .h*/
typedef qaop::Decorate<util::Queue>::with<util::Jointpoint_,util::Count_, util::Lock_>::type JointLockCountQueue;


int print(JointLockCountQueue * self){
	printf("Action called! %x \n", self);
	return 0;
}

int print_a(JointLockCountQueue * self, util::Item * *ppi) {
	printf("Action called! Item %x, this %x \n", *ppi, self);
	return 0;
}

int print2(JointLockCountQueue * self, util::Item * pi) {
	printf("Action called! Item %x, this %x \n", pi, self);
	return 0;
}

int enqueue_w (JointLockCountQueue * self, util::Item * pi) {
	self->Queue::enqueue(pi);
	return 0;
}
int dequeue_w (JointLockCountQueue * self, util::Item ** pi) {
	*pi = self->Queue::dequeue();
	return 0;
}

std::function<int()>  lock_advice_w (JointLockCountQueue * self, std::function<int()> & f) {
	return self->lock_advice(f);
}
std::function<int()>  count_advice_add_w (JointLockCountQueue * self, std::function<int()> & f) {
	std::cout<<"advice add wrap"<<std::endl;
	return self->count_advice_add(f);
}
std::function<int()>  count_advice_remove_w (JointLockCountQueue * self, std::function<int()> & f) {
	std::cout<<"advice remove wrap"<<std::endl;
	return self->count_advice_remove(f);
}

	
	typedef  qaop::stub<JointLockCountQueue> Q;
	typedef  qaop::waven<JointLockCountQueue> L;
	template <typename _Callable>
	using action = qaop::action<JointLockCountQueue, _Callable>;

void initializtion() {
	using namespace util;

	static action<decltype(Q::_(&Queue::dequeue))> m{print};
	//static action<decltype(Q::_r(&Queue::dequeue))> m_a{print_a};
	static action<std::function<decltype(print_a)> > m_a{print_a};
//	static action<decltype(Q::_(&Queue::enqueue))> n{print2};
	static action<decltype(Q::_(&Queue::enqueue))> n{Q::_(&Queue::enqueue, []()->int{std::cout<<"try this!"<<std::endl; return 0;})};
	
	static action<decltype(Q::_r(&Queue::dequeue))> d{Q::wrap_r(&Queue::dequeue), nullptr};
	static action<decltype(Q::_(&Queue::enqueue))> e{enqueue_w,nullptr};

	//static actioe, std::function<decltype(Queue::enqueue)> > e{Queue::enqueue,nullptr};
	//static action< std::function<decltype(enqueue_w)> > u{enqueue_w, &lock_advice_w};
	static action< std::function<decltype(enqueue_w)> > u{enqueue_w, &lock_advice_w};
	static action< std::function<decltype(dequeue_w)> > v{dequeue_w, &lock_advice_w};
	//static action< std::function<decltype(enqueue_w)> > a{enqueue_w, &count_advice_add_w};
	static action< std::function<decltype(enqueue_w)> > a{enqueue_w, Q::wrap(&JointLockCountQueue::count_advice_add)};
	static action< std::function<decltype(dequeue_w)> > r{dequeue_w, &count_advice_remove_w};

	
	L::insitu(&JointLockCountQueue::jointpoint_::dequeue, &d);
	L::insitu(&JointLockCountQueue::jointpoint_::enqueue, &e);
	
	L::before(&JointLockCountQueue::jointpoint_::enqueue, &n);
	L::after (&JointLockCountQueue::jointpoint_::enqueue, &n);

	L::insitu(&JointLockCountQueue::jointpoint_::dequeue, &r);;
	L::insitu(&JointLockCountQueue::jointpoint_::enqueue, &a);;
	L::insitu(&JointLockCountQueue::jointpoint_::enqueue, &u);;
	L::insitu(&JointLockCountQueue::jointpoint_::dequeue, &v);;

	L::before(&JointLockCountQueue::jointpoint_::dequeue, &m);
	L::after (&JointLockCountQueue::jointpoint_::dequeue, &m_a);
}
static struct A {A(){initializtion();}} a;


int main(int argc, char **argv) {
	using namespace util;

	/* version 1 Begin
	Queue q; // version 1 End */
	
	/* verion 2 Begin
	typedef qaop::Decorate<Queue>::with<Count_>::type CountQueue;
	CountQueue q; //version 2 End */
	
	/* verion 3 Begin
	typedef qaop::Decorate<Queue>::with<Count_,Lock_>::type LockCountQueue;
	LockCountQueue q; //version 3 End */
	
	//* verion 4 Begin
	typedef qaop::Decorate<Queue>::with<Jointpoint_, Count_, Lock_>::type JointLockCountQueue;
	JointLockCountQueue q,p; //version 3 End */
	
	Item I1, I2, I3;
	q.enqueue(&I1);
	printf( "count: %d \n",q.count());
	q.enqueue(&I2);
	printf( "count: %d \n",q.count());
	q.enqueue(&I3);
	printf( "count: %d \n",q.count());
	q.dequeue();
	printf( "count: %d \n",q.count());
	q.dequeue();
	printf( "count: %d \n",q.count());
	q.dequeue();
	printf( "count: %d \n",q.count());
	p.enqueue(&I1);
	p.dequeue();
	return 0;
};


