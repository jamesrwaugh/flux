/*
 Copyright (c) 2014 TehPwns
 Original Flux Lua library Copyright (c) 2014 rxi

 This software is provided 'as-is', without any express or implied warranty.
 In no event will the authors be held liable for any damages arising from the use
 of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it freely,
 subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not claim that
    you wrote the original software. If you use this software in a product, an
    acknowledgment in the product documentation would be appreciated but is not required.

 2. Altered source versions must be plainly marked as such, and must not be misrepresented as
    being the original software.

 3. This notice may not be removed or altered from any source distribution.

contact: p00n3dj002@yahoo.com
*/
#ifndef FLUX_CPP_CPP
#define FLUX_CPP_CPP

#include <unordered_map>
#include <math.h>
#include <algorithm>
#include "flux.hpp"

#ifndef M_PI
 #define M_PI    3.14159265358979323846
 #define M_PI_2  1.57079632679489661923
#endif

namespace flux
{
namespace impl
{
	typedef double (*TweenFunc)(double t);
	typedef double (*TweenModidyFunc)(TweenFunc& fn, double t);

	static double TweenFunc_Linear(double t) {
		return t;
	}
	static double TweenFunc_Quad(double t) {
		return t * t;
	}
	static double TweenFunc_Cubic(double t) {
		return t * t * t;
	}
	static double TweenFunc_Quart(double t) {
		return t * t * t * t;
	}
	static double TweenFunc_Quint(double t) {
		return t * t * t * t * t;
	}
	static double TweenFunc_Expo(double t) {
		return pow(2, 10*(t - 1));
	}
	static double TweenFunc_Sine(double t) {
		return -cos(t * (M_PI_2)) + 1;
	}
	static double TweenFunc_Circ(double t) {
		return -sqrt(1 - (t * t)) + 1;
	}
	static double TweenFunc_Back(double t) {
		return t * t * (2.7 * t - 1.7);
	}
	static double TweenFunc_Elastic(double t) {
		return -pow(2, 10 * (t - 1)) * sin((t - 1.075) * (2*M_PI) / .3);
	}

	static double TweenModidyFunc_easeIn(TweenFunc& fn, double t) {
		return fn(t);
	}
	static double TweenModidyFunc_easeOut(TweenFunc& fn, double t) {
		t = 1 - t;
		return 1 - fn(t);
	}
	static double TweenModidyFunc_easeInOut(TweenFunc& fn, double t) {
		t *= 2;
		if(t < 1) {
			return 0.5f * fn(t);
		} else {
			t = 2 - t;
			return 0.5f * (1 - fn(t)) + 0.5f;
		}
	}

	static TweenFunc easingTable[] = {
		TweenFunc_Linear, 	TweenFunc_Quad,
		TweenFunc_Cubic,	TweenFunc_Quart,
		TweenFunc_Quint,	TweenFunc_Expo,
		TweenFunc_Sine,		TweenFunc_Circ,
		TweenFunc_Back,		TweenFunc_Elastic,
	};

	static TweenModidyFunc modifyTable[] = {
		TweenModidyFunc_easeIn,		TweenModidyFunc_easeOut,
		TweenModidyFunc_easeInOut
	};

	/********************************************************************/
	/** unordered_map for string-easing translation **/
	/********************************************************************/

	static const std::unordered_map<const char*, easing> stringToEasingMap = {
		{"linear", easing::linear},
		{"quadin", easing::quadin},  {"quadout", easing::quadout},  {"quadinout", easing::quadinout},
		{"cubicin", easing::cubicin},{"cubicout", easing::cubicout},{"cubicinout", easing::cubicinout},
		{"quartin", easing::quartin},{"quartout", easing::quartout},{"quartinout", easing::quartinout},
		{"quintin", easing::quintin},{"quintout", easing::quintout},{"quintinout", easing::quintinout},
		{"expoin", easing::expoin},  {"expoout", easing::expoout},  {"expoinout", easing::expoinout},
		{"sinein", easing::sinein},  {"sineout", easing::sineout},  {"sineinout", easing::sineinout},
		{"circin", easing::circin},  {"circout", easing::circout},  {"circinout", easing::circinout},
		{"backin", easing::backin},  {"backout", easing::backout},  {"backinout", easing::backinout},
		{"elasticin", easing::elasticin}, {"elasticout", easing::elasticout}, {"elasticinout", easing::elasticinout}
	};

	//Internal tweens held by the flux namespace
	static flux::group internalGroup;


	/********************************************************************/
	/** Implementation of TweenList update **/
	/********************************************************************/

	template<typename T>
	bool TweenList<T>::update(double deltaTime)
	{
		for(auto it = mTweens.begin(); it != mTweens.end(); ++it)
		{
			bool is_finished = it->update(deltaTime);
			if(is_finished) {
				auto remove_item = it; ++it;
				mTweens.erase(remove_item);
			}
		}
		return mTweens.empty();
	}

} //namespace impl

	/********************************************************************/
	/** Tween implementation **/
	/********************************************************************/

	template<typename T>
	tween<T>::tween() : parent(nullptr)
	{
	}

	template<typename T>
	void tween<T>::initialize()
	{
		//Initialize the tween.
		vars.reserve(my_initPtrs.size());
		auto it_p = my_initPtrs.begin();
		auto it_v = my_initVals.begin();

		for(; it_p != my_initPtrs.end(); ++it_p, ++it_v)
		{
			T start = *(*it_p); //Dereferencing original pointer to tweened variable
			T end   = *it_v;
			T diff  = end - start;

			vars.emplace_back(start, diff, *it_p);
		}
	}

	template<typename T>
	tween<T>& tween<T>::ease(easing type)
	{
		this->easeFuncIndex = (char)type / 10; 	//Integer division
		this->modFuncIndex = (char)type % 10;
		return *this;
	}

	template<typename T>
	tween<T>& tween<T>::ease(const char* type)
	{
		auto it = impl::stringToEasingMap.find(type);
		if(it != impl::stringToEasingMap.end()) {
			return ease(it->second);
		} else {
			return ease(easing::quadout);	// Invalid input string - return default?
		}
	}

	template<typename T>
	tween<T>& tween<T>::onstart(callbackFn fn)
	{
		callbacks_onstart.push_front(fn);
		return *this;
	}

	template<typename T>
	tween<T>& tween<T>::onupdate(callbackFn fn)
	{
		callbacks_onupdate.push_front(fn);
		return *this;
	}

	template<typename T>
	tween<T>& tween<T>::oncomplete(callbackFn fn)
	{
		callbacks_oncomplete.push_front(fn);
		return *this;
	}

	template<typename T>
	tween<T>& tween<T>::delay(float sec)
	{
		/* += to account for possible delay beforehand
		 * from "after" or whatever
		 */
		this->start_delay += sec;
		return *this;
	}

	template<typename T>
	template<typename T1, typename T2>
	tween<T1>& tween<T>::after(float seconds, T1* ptr, T2 val)
	{
		return after(seconds, {ptr}, {(T1)val});
	}

	template<typename T>
	template<typename T1, typename T2>
	tween<T1>& tween<T>::after(float seconds, std::initializer_list<T1*> ptrs, std::initializer_list<T2> vals)
	{
		return parent->to(seconds, ptrs, vals).delay(start_delay + ((rate != 0) ? (1 / rate) : 0));
	}

	template<typename T>
	template<typename T2>
	tween<T>& tween<T>::after(float seconds, std::initializer_list<T2> vals)
	{
		/* So, with these functions that don't specify pointers, because they are meant to be
		 * on the same variables, what we want to do is first create a tween with pointers to nothing, 
		 * then assign the new tween's pointers to that of this one's.
		 */
		auto& newTween = this->after(seconds, {(T*)(nullptr)}, vals);
		newTween.my_initPtrs = this->my_initPtrs;
		return newTween;
	}

	template<typename T>
	template<typename T2>
	tween<T>& tween<T>::after(float seconds, T2 val)
	{
		auto& newTween = this->after(seconds, {(T*)(nullptr)}, {(T)val});
		newTween.my_initPtrs = this->my_initPtrs;
		return newTween;
	}

	template<typename T>
	bool tween<T>::update(double deltaTime)
	{
		if(start_delay > 0) {
			start_delay -= deltaTime;
		} else {
			if(inited == false) {
				inited = true;
				initialize();

				for(callbackFn& fn : callbacks_onstart) fn();
				callbacks_onstart.clear();
			}

			time = time + (rate * deltaTime);

			double p = time;
			double x = (p >= 1) ? 1 :
				impl::modifyTable[modFuncIndex](impl::easingTable[easeFuncIndex], p);

			for(auto& var : vars)
				*(var.variable) = var.start + (x * var.diff);

			for(callbackFn& fn : callbacks_onupdate) fn();

			if(p >= 1) {
				for(callbackFn& fn : callbacks_oncomplete) fn();
				finished = true;
			}
		}

		return finished;
	}

	template<typename T>
	void tween<T>::stop(void)
	{
		if(this->parent != nullptr)		//Just in case for now
			parent->removeTween<T>(this);
	}

	/********************************************************************/
	/** flux::group implementation **/
	/********************************************************************/

	template<typename T1, typename T2>
	tween<T1>& flux::group::to(float seconds, std::initializer_list<T1*> ptrs, std::initializer_list<T2> vals)
	{
		//Tween initialization. The tween's "constructor" outside of the class.
		tween<T1> New;
		New.parent = this;
		New.inited = false;
		New.finished = false;
		New.rate  = (seconds > 0) ? (1 / seconds) : 0;
		New.time  = (New.rate > 0) ? 0 : 1;
		New.start_delay = 0;
		New.easeFuncIndex = New.modFuncIndex = 1; //Quadout is default tween
		New.my_initPtrs = ptrs;

		/* For now, convert each value one by one
		 */
		for(const T2& entry : vals)
			New.my_initVals.push_back((T1)entry);


		auto tList = this->getTweens<T1>();

		/* Assign some id for possible removing later.
		 * This might give warnings on some compilers,
		 * and honestly isn't very good.
		 */
		New.id = tList->mTweens.size();
		New.id |= ((long)((void*)&New) & 0xFFFF) << 16;

		tList->mTweens.push_back(New);

		return tList->mTweens.back();
	}

	template<typename T1, typename T2>
	tween<T1>& flux::group::to(float seconds, T1* ptr, T2 val)
	{
		return this->to(seconds, {ptr}, {(T1)val});
	}

	template<typename T>
	impl::TweenList<T>* flux::group::getTweens()
	{
		if(mTweensLists.find(typeid(T)) == mTweensLists.end())
			mTweensLists[typeid(T)] = new impl::TweenList<T>();

		return (impl::TweenList<T>*)mTweensLists[typeid(T)];
	}

	inline void flux::group::update(double deltaTime)
	{
		auto it = mTweensLists.begin();

		while(it != mTweensLists.end())
		{
			bool isTypeEmpty = it->second->update(deltaTime);
			it++;
			if(isTypeEmpty) {
				auto remove_item = it;
					--remove_item; 			//Delete node in list before it

				/* The GenericTweenList needs to be explicitly cast or else
				 * the incorrect destructor is called, resulting in a
				 * memory leak. Double should work because the destructor is generic.
				 */
				auto typedTweenList =
					reinterpret_cast<impl::TweenList<double>*>(remove_item->second);
				delete typedTweenList;
				
				this->mTweensLists.erase(remove_item);
			}
		}
	}

	template<typename T>
	void flux::group::removeTween(tween<T>* toRemove)
	{
		auto tList = this->getTweens<T>();

		/* Linear lookup of an element in a list, but as far as I know
		 * tween::stop isn't used often.
		 */
		auto tweenEntry = std::find_if(tList->mTweens.begin(), tList->mTweens.end(),
			[=](const tween<T>& tw){return tw.id == toRemove->id;});

		if(tweenEntry != tList->mTweens.end())
			tList->mTweens.erase(tweenEntry);
	}

	/********************************************************************/
	/** General namespace function implementation **/
	/********************************************************************/

	template<typename T1, typename T2>
	tween<T1>& to(float seconds, std::initializer_list<T1*> ptrs, std::initializer_list<T2> vals)
	{
		return impl::internalGroup.to(seconds, ptrs, vals);
	}

	template<typename T1, typename T2>
	tween<T1>& to(float seconds, T1* ptr, T2 val)
	{
		return flux::to(seconds, {ptr}, {(T1)val});
	}

	inline void update(double deltaTime)
	{
		impl::internalGroup.update(deltaTime);
	}

}	//namespace flux

#endif
