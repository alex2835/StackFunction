#pragma once

#include <type_traits>
#include <array>

template <typename Signature, size_t StorageSize = 128>
class StackFunction;

template <typename RetType, typename... Args, size_t StorageSize>
class StackFunction<RetType(Args...), StorageSize>
{
	using FunctionPtrType = RetType(*)(Args...);
	using CollableType    = RetType(*)(void* object, FunctionPtrType func, Args&&... args);
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
	StackFunction(FUNC&& object)
	{
		typedef typename std::remove_reference<FUNC>::type UnrefFunctionType;
		static_assert(sizeof(UnrefFunctionType) < StorageSize, "functional object doesn't fit into internal storage");
		static_assert(std::is_move_constructible<UnrefFunctionType>::value, "Type should be movable");


		mCollable = [](void* object, FunctionPtrType /*func*/, Args&&... args) -> RetType
		{
			return static_cast<UnrefFunctionType*>(object)->operator()(std::forward<Args>(args)...);
		};

		mAllocFunc = [](void *storage, void *object)
		{
			UnrefFunctionType* functional_object = reinterpret_cast<UnrefFunctionType*>(object);
			::new(storage) UnrefFunctionType(std::move(*functional_object));

			static_cast<UnrefFunctionType*>(storage)->operator()(4);
		};

		mDealocFunc = [](void* storage)
		{
			static_cast<UnrefFunctionType*>(storage)->~UnrefFunctionType();
		};

		mAllocFunc(mStorage, &object);
	}

	/**
	 * @brief StackFunction Constructor from function pointer
	 */
	template <typename RET, typename... PARAMS>
	StackFunction(RET(*func)(PARAMS...))
	{
		mFunctionPtr = func;
		mCollable = [](void* /*object*/, FunctionPtrType func, Args&&... args) -> RetType
		{
			return static_cast<RET(*)(PARAMS...)>(func)(std::forward<Args>(args)...);
		};
	}
	StackFunction(StackFunction&& other)
	{
		MoveFromOther(other);
	}

	StackFunction& operator=(StackFunction&& other)
	{
		MoveFromOther(other);
		return *this;
	}

	~StackFunction()
	{
		if (mDealocFunc)
			mDealocFunc(mStorage);
	}

	RetType operator()(Args&&... args)
	{
		if (!mCollable)
			throw std::runtime_error("call of empty functor");
		return mCollable(mStorage, mFunctionPtr, std::forward<Args>(args)...);
	}

private:
	void MoveFromOther(StackFunction& other)
	{
		if (this == &other)
			return;

		if (mDealocFunc)
			mDealocFunc(mStorage);

		if (other.mAllocFunc)
		{
			mAllocFunc = other.mAllocFunc;
			mAllocFunc(mStorage, other.mStorage);
			mCollable = other.mCollable;
			mDealocFunc = other.mDealocFunc;
		}
		else
			mFunctionPtr = other.mFunctionPtr;

		other.mCollable    = nullptr;
		other.mFunctionPtr = nullptr;
		other.mDealocFunc  = nullptr;
		other.mAllocFunc   = nullptr;
	}

private:
	union
	{
		FunctionPtrType mFunctionPtr;
		char mStorage[StorageSize];
	};
	CollableType mCollable = nullptr;
	AllocType mAllocFunc   = nullptr;
	DealocType mDealocFunc = nullptr;
};