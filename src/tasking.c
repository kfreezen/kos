#include <tasking.h>
#include <paging.h>
#include <kheap.h>
#include <print.h>

#define TASKING_DEBUG

volatile Task* current_task;
volatile Task* ready_queue;

extern PageDirectory* currentPageDir;
extern PageDirectory* kernelPageDir;
extern void AllocPage();

extern UInt32 read_eip();
UInt32 next_pid = 1;

extern UInt32 new_task_entry; // Not to be referenced as such.  Is a symbol in _idt.s

void InitTasking() {
	// This assumes that the stack has been moved to 0xEFFFFFF0 with 2 pages worth of stack space.
	
	asm volatile("cli");
	
	current_task = ready_queue = (Task*)kalloc(sizeof(Task));
	current_task->id = next_pid++;
	current_task->esp = current_task->ebp = 0;
	current_task->eip = 0;
	current_task->dir = currentPageDir;
	current_task->next = 0;
	
	asm volatile("sti");
}

extern void TaskSwitch();

#if 0
void TaskSwitch() {
	if(!current_task) {
		return;
	}
	
	UInt32 esp, ebp, eip;
	asm volatile("mov %%esp, %0" : "=r"(esp));
	asm volatile("mov %%ebp, %0" : "=r"(ebp));
	
	eip = read_eip();
	
	if(eip == 0x12345) {
		return;
	}
	
	current_task->eip = eip;
	current_task->esp = esp;
	current_task->ebp = ebp;
	
	current_task = current_task->next;
	if(!current_task) { 
		current_task = ready_queue;
	}
	
	eip = current_task->eip;
	esp = current_task->esp;
	ebp = current_task->ebp;
	
	//currentPageDir = current_task->dir;
	
	int i=0;
	for(i; i<1000000; i++);
	
	asm volatile("cli;\
		mov %0, %%ecx;\
		mov %1, %%esp;\
		mov %2, %%ebp;\
		\
		mov $0x12345, %%eax;\
		sti;\
		jmp *%%ecx" : : "c"(eip), "r"(esp), "r"(ebp)/*, "r"(currentPageDir->phys)*/);
}
#endif

extern void TaskStackSetup(UInt32 stack_top, UInt32 eip);

void CreateStack(PageDirectory* dir, void* top, UInt32 size) {
	if(size&0xFFF) {
		size = (size&~0xFFF)+0x1000;
	}
	
	if((UInt32)top&0xFFF) {
		top = (void*) ((UInt32)top&~0xFFF);
	}
	
	void* ptr = top-0x1000;
	while(size!=0) {
		MapAllocatedPageTo(dir, ptr, DIR_OTHER_TASK);
		ptr-=0x1000;
		size-=0x1000;
	}
}

int CreateTask(UInt32 start, PageDirectory* dir) {
	if(dir==NULL) {
		dir = CloneDirectory(currentPageDir, DIR_OTHER_TASK);
	}
	Task* newTask = (Task*)kalloc(sizeof(Task));
	newTask->id = next_pid++;
	newTask->esp = newTask->ebp = 0;
	newTask->eip = start;
	newTask->dir = dir;
	newTask->next = 0;
	
	#ifdef TASKING_DEBUG
	kprintf("CreateTask.waypoint1:eip=%x\n", newTask->eip);
	#endif
	
	// Here we need to set up the stack.
	CreateStack(dir, (void*)0xE0000000, 0x2000);
	
	PageDirectory* saved = currentPageDir;
	SwitchPageDirectory(dir);
	// Set up the stack for return from an interrupt.
	//TaskStackSetup(0xE0000000-(sizeof(void*)<<1), start);
	SwitchPageDirectory(saved);
	
	//newTask->esp = 0xE0000000-8;
	newTask->esp = 0xE0000000-sizeof(void*);
	newTask->eip = (UInt32) start;
	
	Task* tmp = (Task*) ready_queue;
	while(tmp->next) {
		tmp = tmp->next;
	}
	
	tmp->next = newTask;
	
	#ifdef TASKING_DEBUG
	kprintf("newTask=%x\n", newTask);
	#endif
	
	return 0;
}

volatile Task* saved_task;
void DisableTasking() {
	saved_task = current_task;
	current_task = NULL;
}

void ReenableTasking() {
	current_task = saved_task;
}
int GetPID() {
	return current_task->id;
}
