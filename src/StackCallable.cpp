
#include "StackCallable.h"
#include <tuple>
#include <type_traits>
#include <vector>

template <typename Signature, size_t StorageSize = 128>
class StackFunction;

template <typename RetType, typename... Args, size_t StorageSize>
class StackFunction<RetType(Args...), StorageSize>
{
	using FunctionPtrType = RetType(*)(Args...);
	using CollableType    = RetType(*)(void* object_ptr, FunctionPtrType free_func_ptr, Args... args);
	using AllocType       = void (*)(void* storage_ptr, void* object_ptr);
	using DealocType      = void (*)(void* storage_ptr);

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

        mCollable = [](void* object_ptr, FunctionPtrType, Args... args) -> RetType
        {
            return static_cast<UnrefFunctionType*>(object_ptr)->operator()(args...);
        };

        mAllocFunc = [](void* storage_ptr, void* object_ptr)
        {
			auto x_object = static_cast<UnrefFunctionType*>(object_ptr);
			new(storage_ptr) UnrefFunctionType(std::move(*x_object));
        };

        mDealocFunc = [](void* storage_ptr)
        {
            static_cast<UnrefFunctionType*>(storage_ptr)->~UnrefFunctionType();
        };

        mAllocFunc(&mStorage, &object);
    }

	/**
	 * @brief StackFunction Constructor from function pointer
	 */
    template <typename RET, typename... PARAMS>
    StackFunction(RET(*func_ptr)(PARAMS...))
    {
        mFunctionPtr = func_ptr;
		mCollable = [](void*, FunctionPtrType f_ptr, Args... args) -> RetType
        {
            return static_cast<RET(*)(PARAMS...)>(f_ptr)(args...);
        };
    }

	StackFunction(StackFunction&& o)
	{
		moveFromOther(o);
	}

	StackFunction& operator=(StackFunction&& o)
	{
		moveFromOther(o);
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
	void moveFromOther(StackFunction& o)
	{
		if (this == &o) 
            return;

		if (mAllocFunc)
		{
			mAllocFunc(&mStorage, nullptr);
			mAllocFunc = nullptr;
		}
		else
		{
			mFunctionPtr = nullptr;
		}

		mCollable = o.mCollable;
		o.mCollable = nullptr;

		if (o.mAllocFunc)
		{
			mAllocFunc = o.mAllocFunc;
			mAllocFunc(&mStorage, &o.mStorage);
		}
		else
		{
            mFunctionPtr = o.mFunctionPtr;
		}
	}

private:
	FunctionPtrType mFunctionPtr = nullptr;

	std::aligned_storage<StorageSize, alignof(size_t)> mStorage;
	CollableType mCollable = nullptr;
	AllocType  mAllocFunc  = nullptr;
	DealocType mDealocFunc = nullptr;
};



int main(int argc, char** argv)
{
	StackFunction<int(int), sizeof(int)> function;


	return 0;
}
