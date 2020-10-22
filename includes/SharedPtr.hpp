#pragma once

template <typename T>
class SharedPtr
{
	private:
		size_t	*counter;
		T		*obj;
	public:

		SharedPtr()
			: counter(nullptr), obj(nullptr) {}

		void	set(T *obj)
		{
			if (this->obj == obj)
				return;
			if (counter)
			{
				(*counter)--;
				if (*counter == 0)
				{
					delete counter;
					delete this->obj;
				}
			}
			counter = new size_t(1);
			this->obj = obj;
		}

		T*	get() const
		{
			return obj;
		}

		SharedPtr(T* obj)
			: counter(new size_t(1)), obj(obj) {}
		
		SharedPtr(const SharedPtr& o)
			: counter(nullptr), obj(nullptr)
		{
			*this = o;	
		}

		T* operator->()
		{
			return obj;
		}

		T& operator*()
		{
			return *obj;
		}

		SharedPtr&	operator=(const SharedPtr& o)
		{
			if (this == &o)
				return *this;
			if (counter)
			{
				--(*counter);
				if (*counter == 0)
				{
					delete counter;
					delete obj;
				}
			}
			counter = o.counter;
			if (counter)
				(*counter)++;
			obj = o.obj;
			
			return *this;
		}

		size_t use_count()
		{
			return *counter;
		}

		~SharedPtr()
		{
			if (counter)
			{
				--(*counter);
				if (*counter == 0)
				{
					delete counter;
					delete obj;
				}
			}
		}
};