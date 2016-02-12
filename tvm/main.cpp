#include <memory>
#include <vector>
#include <algorithm>

//http://blogs.msdn.com/b/abhinaba/archive/2009/01/30/back-to-basics-mark-and-sweep-garbage-collection.aspx

struct block
{
	block *next;
	int size;
	bool marked;
	bool free;
	void *data;

	// http://www.codeproject.com/script/Articles/ViewDownloads.aspx?aid=912
	bool contains(void *ptr)
	{
		const char *begin = static_cast<const char*>(data);
		const char *end = begin + size;
		return ptr >= begin && ptr < end;
	}

	void(*deallocator)(void*);
};

class gc
{
	class mem_ref
	{
		block *block_;
	public:
		mem_ref() :block_(0){ gc::instance()->push_ref(this); }
		mem_ref(block *blk) :block_(blk){ gc::instance()->push_ref(this); }
		mem_ref(const mem_ref &rhs)
			:block_(rhs.block_)
		{
			gc::instance()->push_ref(this);
		}
		mem_ref& operator=(mem_ref &rhs)
		{
			if (this == &rhs)
				return *this;

			this->block_ = rhs.block_;
			return *this;
		}

		mem_ref(mem_ref &&rhs)
			:block_(rhs.block_)
		{
			gc::instance()->push_ref(this);
		}

		virtual ~mem_ref()
		{
			gc::instance()->remove_ref(this);
		}

		template <typename ObjT>
		ObjT *ptr() { return (ObjT*)block_->data; }

		block * block() { return block_; }

	};

public:
	
	template<typename T>
	class object 
	{
		gc::mem_ref ptr_;

		friend gc;


		void assign(gc::mem_ref &rhs) 
		{ 
			ptr_ = rhs;
		}

		static void deallocator(void *ptr)
		{
			T *obj = static_cast<T*>(ptr);
			obj->~T();
		}

	public:
		object()
			:ptr_(NULL)
		{
		}

		object(const object &rhs)
			:ptr_(rhs.ptr_)
		{
		}

		T& operator*() { return *ptr_.ptr<T>(); }
	};


private:

	block* start_;
	std::vector<mem_ref *> stack_;

	static gc *instance_;

	static gc* instance() { return instance_;  }

	void push_ref(mem_ref *ref)
	{
		stack_.push_back(ref);
	}

	void remove_ref(mem_ref *ref)
	{
		stack_.erase(std::remove(stack_.begin(), stack_.end(), ref));
	}

	block* find_free_block(int size)
	{
		block *ptr = start_;
		while (ptr)
		{
			if (ptr->free && ptr->size <= size)
				return ptr;
			ptr = ptr->next;
		}
		return NULL;
	}

	block* alloc_block(int size)
	{
		
		int memsize = sizeof(block) + size;
		void *ptr = malloc(memsize);
		block* next = (block*)ptr;
		next->next = NULL;
		next->data = (char*)ptr + sizeof(block); // move the pointer ahead that many bytes
		next->size = size;
		next->marked = false;
		next->free   = false;
		return next;
	}

	mem_ref gc_alloc(int size)
	{
		block *next = find_free_block(size);
		if (next)
			return mem_ref(next);

		next = alloc_block(size);

		if (!start_)
		{
			start_ = next;
		}
		else
		{
			block *ptr = start_;
			while (ptr->next)
			{
				ptr = ptr->next;
			}
			ptr->next = next;
		}

		return mem_ref(next);
	}


	void mark(mem_ref *ref, bool mk)
	{
		if (ref->block()->marked == mk)
			return; // Should have all containing objects marked?

		ref->block()->marked = mk;
		// mark all contained children
		for (auto i = stack_.begin(), i_end = stack_.end(); i != i_end; ++i)
		{
			mem_ref *child = *i;
			if (child == ref) // skip myself
				continue;
			if (ref->block()->contains(child))
				mark(child, mk);
		}
	}

	void mark_all(bool mk)
	{
		for (auto i = stack_.begin(), i_end = stack_.end(); i != i_end; ++i)
		{
			mem_ref *ref = *i;
			// if no other nodes contain this node, it's a root
			bool root = true;
			for (auto j = stack_.begin(); j != i_end; ++j)
			{			
				if (*j == ref)
					continue;

				if ((*j)->block()->contains(ref))
				{
					root = false;
					break;
				}
			}
			if (root)
				mark(ref, mk);
		}
	}

	void deallocate(block *ref)
	{
		// first we deallocate all nested ptrs
		for (auto i = stack_.begin(), i_end = stack_.end(); i != i_end; ++i)
		{
			if (ref == (*i)->block())
				continue;

			if (ref->contains(*i))
				deallocate((*i)->block());
		}
				
		ref->deallocator(ref->data); //dealloc
		//free(ref); //free the memory?? nah lets keep it around
		ref->free = true;
	}

	void sweep()
	{
		block *ptr = start_;
		block *last = start_;
		while (ptr)
		{
			if (!ptr->marked)
			{
				block *tmp = ptr;
				ptr = ptr->next;
				deallocate(tmp);
				if (last)
					last->next = ptr;
			}
			else
			{
				last = ptr;
				ptr = ptr->next;
			}
		}

		//if(alloc_limit)
		//  free_all(start_);

		mark_all(false);
	}

	static mem_ref alloc(int size)
	{
		return instance_->gc_alloc(size);
	}

public:
	
	gc() :start_(0){};

	static void initialize() { instance_ = new gc; }

	template <typename T>
	static gc::object<T> gc_new()
	{
		mem_ref rf = gc::alloc(sizeof(T));
		rf.block()->deallocator = &gc::object<T>::deallocator;
		new (rf.ptr<void>()) T(); // remember to link up the d_tor

		gc::object<T> ptr;
		ptr.assign(rf);
		return ptr;
	}

	template <typename T>
	static gc::object<T> gc_new(const T& val)
	{
		mem_ref rf = gc::alloc(sizeof(T));
		rf.block()->deallocator = &gc::object<T>::deallocator;
		new (rf.ptr<void>()) T(val);

		gc::object<T> ptr;
		ptr.assign(rf);

		return ptr;
	}


	static void collect()
	{
		instance_->mark_all(true);
		instance_->sweep();
	}
	
};


struct stack_obj
{
	union
	{
		char byte;
	} data;
};

enum instructions
{
	LOAD_VAL = 1,
	ADD = 2,
	SUBTRACT = 3,
	PRINT = 4,
	INPUT = 5
};

#define MAX_STACK 256
class tvm
{
	stack_obj stack_[MAX_STACK];
	int obj_cnt;

	void push(stack_obj obj);
	stack_obj pop();

public:
	void execute(char instruction);


};



gc* gc::instance_ = NULL;

#include <iostream>
#include <string>

struct test_child
{
	gc::object<std::string> child; 

	~test_child()
	{
		std::cout << "Destroyed" << std::endl;
	}
};


int main()
{
	gc::initialize();

	{
		gc::object<test_child> tc = gc::gc_new<test_child>();
		(*tc).child = gc::gc_new<std::string>(std::string("THIS IS SPARTA"));

		std::cout << *((*tc).child) << std::endl;
	}

	gc::collect();
	

	//std::cout << *((*tc).child) << std::endl;

}
