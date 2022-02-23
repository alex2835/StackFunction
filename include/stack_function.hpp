#pragma once

#include <type_traits>

template <typename Signature, size_t StorageSize = 128>
class StackFunction;

template <typename RetType, typename... Args, size_t StorageSize>
class StackFunction<RetType(Args...), StorageSize>
{
	using FunctionPtrType = RetType(*)(Args...);
	using CollableType    = RetType(*)(void* object, FunctionPtrType func, Args... args);
	using AllocType       = void (*)(void* storage, void* collable);
	using DealocType      = void (*)(void* storage);

public:
	StackFunction() = default;

    /**
     * @brief StackFunction Constructor from functional object.
     * @param object Functor object will be stored in the internal storage
     * using move constructor. Unmovable objects are prohibited explicitly.
     */
    template <typename FUNC>
    StackFunction(FUNC object)
    {
        typedef typename std::remove_reference<FUNC>::type UnrefFunctionType;
		static_assert(sizeof(UnrefFunctionType) < StorageSize, "functional object doesn't fit into internal storage");
		static_assert(std::is_move_constructible<UnrefFunctionType>::value, "Type should be movable");

		auto* functional_object = reinterpret_cast<UnrefFunctionType*>(&object);
		::new(&mStorage) UnrefFunctionType(std::move(*functional_object));

        mCollable = [](void* object, FunctionPtrType /*func*/, Args... args) -> RetType
        {
            return static_cast<UnrefFunctionType*>(object)->operator()(args...);
        };

        mDealocFunc = [](void* storage)
        {
            static_cast<UnrefFunctionType*>(storage)->~UnrefFunctionType();
        };
    }

	/**
	 * @brief StackFunction Constructor from function pointer
	 */
	template <typename RET, typename... PARAMS>
	StackFunction(RET(*func_ptr)(PARAMS...))
	{
		mFunctionPtr = func_ptr;
		mCollable = [](void* /*object*/, FunctionPtrType func, Args... args) -> RetType
		{
			return static_cast<RET(*)(PARAMS...)>(func)(args...);
		};
	}
	StackFunction(StackFunction&& o)
	{
		MoveFromOther(o);
	}

	StackFunction& operator=(StackFunction&& o)
	{
		MoveFromOther(o);
		return *this;
	}

    ~StackFunction()
    {
        if (mDealocFunc)
			mDealocFunc(&mStorage);
    }

    RetType operator()(Args... args)
    {
        if (!mCollable) throw std::runtime_error("call of empty functor");
		return mCollable(&mStorage, mFunctionPtr, args...);
    }

private:
	void MoveFromOther(StackFunction& o)
	{
		if (this == &o) 
            return;

		if (mDealocFunc)
			mDealocFunc(&mStorage);

		mCollable = o.mCollable;
		mFunctionPtr = o.mFunctionPtr;
		mDealocFunc = o.mDealocFunc;
		if (o.mAllocFunc)
		{
			mAllocFunc = o.mAllocFunc;
			mAllocFunc(&mStorage, &o.mStorage);
		}


		o.mCollable    = nullptr;
		o.mFunctionPtr = nullptr;
		o.mDealocFunc  = nullptr;
		o.mAllocFunc   = nullptr;
	}

private:
	union
	{
		FunctionPtrType mFunctionPtr;
		std::aligned_storage<StorageSize, alignof(size_t)> mStorage;
	};
	CollableType mCollable = nullptr;
	AllocType  mAllocFunc  = nullptr;
	DealocType mDealocFunc = nullptr;
};