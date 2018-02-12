#ifndef _QAOP_H_
#define _QAOP_H_

namespace qaop {

/* Aspect example
template <typename _Base>
struct Aspect1 : public _Base::this_t { //here is the only different part from ordinary 
	typedef typename _Base::fulltype_t  fulltype_t; 
	typedef Aspect1 this_t; 
	Aspect1() {std::cout<<"from aop1    "<<typeid(this).name()<<", fulltype_t is "<<typeid(fulltype_t).name()<<std::endl;}
	fulltype_t somefunc( fulltype_t a, fulltype_t b) {}
	virtual void foo(){std::cout<<"greeting from Aop1::foo"<<std::cout<<std::endl;}
	virtual ~Aspect1() {std::cout<<"from aop1 destructor."<<std::endl;}
};
 *  It is very usefull and important that fulltype_t will keep the same for all apects.
*/ 

/* use it like this
typedef qaop::Decorate<A>::with<Aspect1, Aspect2, Aspect3>::type CombinedAspects;
*/

//forward declarition. aopfy will reconstruct the series of type after iteration, and put it into fulltype_t;
template < typename _AopBase, template<typename> class ...__Aspects>
struct aopfy;

//parameter base wrap.Only if we have a base with the template parameters, we can reconstruct the fulltype.
template <typename _Base, template<typename> class ...__Aspects >
struct aopbase {
	typedef aopfy<aopbase, __Aspects...> reference_t; //will be kept the same during iteration.
	typedef _Base Base_t;
};

//iteration template defination.
template < typename _AopBase, template <typename> class __FirstAspects, template <typename> class ...__RestAspects >
struct aopfy<_AopBase ,__FirstAspects, __RestAspects...> {
	typedef __FirstAspects<typename aopfy<_AopBase, __RestAspects...>::fulltype_t> fulltype_t; 
	typedef typename _AopBase::Base_t this_t;
};

//end of iteration tempalte defination.
template <typename _AopBase>
struct aopfy<_AopBase>{
	typedef typename _AopBase::reference_t fulltype_t; //magic begins from here.
	typedef typename _AopBase::Base_t this_t; //this is why the reconstuction works. becuse aspects only inherate from this_t, not the aopfy<> class. 
};	

//In fact, you can use AOP just by the upper codes
//example: typedef  Aspect1<Aspect2<Aspect3< aopfy<aopbase< A, Aspect1, Aspect2, Aspect3>, Aspect1, Aspect2, Aspect3 > > > >::fulltype_t  Combined;
//however, it seems wired, though it will be more easily to understand how it works.

//Struct Decorat makes the expression clear and easy to use.
template <typename _Base>
struct Decorate{
	//keep the reference during iteration.
	template <template <typename> class ... __Aspects>
	struct remember {
		typedef aopfy< aopbase<_Base, __Aspects...>, __Aspects...> reference_t; 
	};

	template <typename _Remember, template <typename> class ...  __Aspects>
	struct with_imp;

	template <template <typename> class ...  __Aspects>
	struct with {
		typedef typename with_imp<remember< __Aspects...>, __Aspects...>::combined_t type;
	};
	
	template <typename _Remember, template <typename> class __FirstAspect, template <typename> class ...  __RestAspects>
	struct with_imp<_Remember, __FirstAspect, __RestAspects...> {
		typedef  __FirstAspect< typename with_imp< _Remember, __RestAspects...>::combined_t > combined_t;
	};
	
	template <typename _Remember, template <typename> class __LastAspect >
	struct with_imp<_Remember, __LastAspect> {
		typedef __LastAspect<typename _Remember::reference_t> combined_t;
	};	
};

//Pattern 2:  Clone & Take 
//
//Condition: 
//It is following the Protype Design Pattern. Every class has one virtual clone() interface  which returns 
//a new object based on *this 
//
//At the same time, we need use "move" and swap method to swap a rval reference
//guts out. This may reduce time for building new objects. Here the semantic is "take", create with a default parameter and
//swap to rval objects for them to destroy.
//
//the reason we need implement it in virtual methods, instead of 
//new T(std::move(obj));
//because just as the clone cases, it is not always true that T is available,
//sometimes you will only get a ptr or ref to a opaque-type object. 
//one way to get a proper swapable trival object could only be use virtual fuction create_default()
//
//Solution:
//because this point contains the necessay type info, it is easy to implemented the clone mehtod as following: 
//
// virtual base_t * clone() { return self_clone(this); } 
//
// there is no class_name related inside the implemetion, so it can be used in macro, surely.
//
// Need:
// 1. typedef baseclass base_t;
// 2. of course every derived class must have the copy constructor defined.
//
template <typename T>
T * self_clone(T * const self) { return new T(*self); } 
//e.g. virtual base_t * clone() { return self_clone(this); } 

template <typename T>
T * default_create(T * const self) { return new T; } 
//e.g. virtual base_t * create_default() { return default_create(this); } 

//the following clone(T&) and take(T &&) are not suggested to used directly.
//because it is very easy to mix them with the class member func. (not only by 
//us, sometime the compiler may also be made comfused.) 
//on the contrast, clone_by_ptr/take_by_ref will be more easier to be greped.
//
//Remeber each of them return a ptr, this is because they are similar with 
//the "new" allocator. and all of them allocaed mem in fact, and this mem 
//should be released with "delete" on a pointer.
//
template <typename T>
typename std::enable_if<!std::is_constructible<T>::value, typename 
		std::add_pointer<typename std::decay<T>::type>::type>::type 
clone(T & obj) { 
	typedef typename std::decay<T>::type rval_t;
	typedef typename std::add_pointer<rval_t>::type ptr_t;
	return dynamic_cast<ptr_t>(obj.clone()); 
}
template <typename T>
typename std::enable_if<std::is_constructible<T>::value, typename 
		std::add_pointer<typename std::decay<T>::type>::type>::type
clone(T & obj) { 
	typedef typename std::decay<T>::type rval_t;
	return new rval_t(obj); 
}

template <typename T>
typename std::enable_if<!std::is_constructible<T>::value, typename 
		std::add_pointer<typename std::decay<T>::type>::type>::type 
take(T && obj) { 
	typedef typename std::decay<T>::type rval_t;
	typedef typename std::add_pointer<rval_t>::type ptr_t;
	ptr_t temp = dynamic_cast<ptr_t>(obj.create_default());
	swap(*temp, obj);
	return temp;
}
template <typename T>
typename std::enable_if<std::is_constructible<T>::value, typename 
		std::add_pointer<typename std::decay<T>::type>::type>::type
take(T && obj) { 
	typedef typename std::decay<T>::type rval_t;
	typedef typename std::add_pointer<rval_t>::type ptr_t;
	ptr_t temp = new rval_t();
	swap(*temp, obj);
	return temp; 
}

template <typename T>
T * clone_by_ptr(T * const pointer) { return clone(*pointer); }
//e.g. clone_by_ptr([protype passed by pointer]);

template <typename T>
T * clone_by_ref(T & reference) { return clone(reference); }
//e.g. clone_by_ref([protype passed by reference]);

template <typename T>
T * take_by_ptr(T * const pointer) { return take(std::move(*pointer)); }
//e.g. take_by_ptr([rval resource passed by pointer]);

template <typename T>
T * take_by_ref(T & reference) { return take(std::move(reference)); }
//e.g. take_by_ref([rval resourece passed by reference]);
} //end of namespace qaop

#endif //_QAOP_H_ 