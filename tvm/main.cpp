
#include <memory>




struct block
{
	block *next;
	int size; 
	bool marked;
	void *data;
};




#define MEM_STACK_SIZE 512
class gc
{
public:
	class mem_ref
	{
		block *block_;

	public:
		mem_ref():block_(0){}
		mem_ref(block *blk):block_(blk){ gc::push(blk);}
		mem_ref(const mem_ref &rhs):block_(rhs.block_)
		{ 
			gc::push(block_); 
		}
		~mem_ref() 
		{ 
			gc::pop(); 
		}

		template <typename ObjT>
		ObjT *ptr() { return (ObjT*)block_->data; }
	};

private:

	block* start_;
	block* stack_[MEM_STACK_SIZE];
	int obj_cnt;

	static gc *instance_;

	void push_front(block *blk)
	{
		if(obj_cnt+1 >= MEM_STACK_SIZE)
			throw "memory stack overflow";
		stack_[obj_cnt++]=blk;
	}

	void pop_front()
	{
		if(obj_cnt < 1)
			throw "memory stack underflow";
		--obj_cnt;
	}

	block* alloc_block(int size)
	{
		int memsize=sizeof(block)+size;
		void *ptr = malloc(28);
		block* next= (block*)ptr;
		next->next=NULL;
		next->data = (char*)ptr+sizeof(block); // move the pointer ahead that many bytes
		next->size=size;
		next->marked=false;
		return next;
	}

	mem_ref gc_alloc(int size)
	{
		block *next =alloc_block(size);
		if(!start_)
		{
			start_=next;
		}
		else
		{
			block *ptr = start_;
			while(ptr->next)
			{
				ptr=ptr->next;
			}
			ptr->next=next;
		}

		return mem_ref(next);	
	}

	void mark_all(bool mk)
	{
		for(int i=0;i<obj_cnt; ++i)
		{
			if(stack_[i]->marked != mk)
				stack_[i]->marked=mk;
		}
	}
	void sweep()
	{
		block *ptr=start_;
		block *last=start_;
		while(ptr)
		{
			if(!ptr->marked)
			{
				block *tmp = ptr;
				ptr = ptr->next;
				free(tmp);
			}
			else
			{
				last=ptr;
				ptr=ptr->next;
			}
		}
		mark_all(false);
	}

public:

	

	gc():obj_cnt(0),start_(0){};

	static void initialize() { instance_ = new gc; }
	
	
	static void push(block *blk) { instance_->push_front(blk); }
	static void pop() { instance_->pop_front(); }

	static mem_ref alloc(int size)
	{
		return instance_->gc_alloc(size);
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
		gc::mem_ref* ref;
	} data;
};

enum instructions
{
	LOAD_VAL= 1,
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

void write_to_reference(gc::mem_ref mem, const char *data, int size)
{
	memcpy(mem.ptr<void>(),data,size);
}

int main()
{
	gc::initialize();
	{
		gc::mem_ref str = gc::alloc(12);

		write_to_reference(str,"HELLO WORLD",12);
	}

	gc::collect();
	
}