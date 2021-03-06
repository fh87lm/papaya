#ifndef LINUX_MM_H
#define LINUX_MM_H

#include<valType.h>
#include<utils.h>
#include<list.h>
#include<mmzone.h>
#include<pmm.h>
#include<linux/sched.h>

//#define i386_endbase 0xa0000

struct mem_seginfo{
	u32 base_low, base_high;
	u32 len_low, len_high;
	u32 type;
};

//void map_pg(u32*dir,int vpg_id,int ppg_id,int us,int rw);
//char* kmalloc_pg(u32 gfp_mask, int order);
void mm_init(void);
void mm_init2(void);
extern u32 gmemsize;

/* when @size bytes required, we */
static inline int size2pages(int size){
	int  least_pages = (size + (PAGE_SIZE - 1)) >> PAGE_SHIFT;	
	/* round it to the power of 2 */
	return ceil2n(least_pages);
}


#pragma pack(push)
#pragma pack(1)
union vm_flags{
	struct{
		unsigned readable: 1;
		unsigned writable: 1;
		unsigned executable: 1;
		unsigned shared: 1;

		unsigned mayread: 1;
		unsigned maywrite: 1;
		unsigned mayexec: 1;
		unsigned mayshare: 1;

		unsigned growsdown: 1;
		unsigned growsup: 1;
		unsigned denywrite: 1;
		unsigned dontcopy: 1;
	};
	unsigned value;	
};
#pragma pack(pop)
//take care	--------------+++++++++++++++++++++------------------
enum{
	VM_READ 	=		1<<0,
	VM_WRITE 	=		1<<1,
	VM_EXEC		=		1<<2,
	VM_SHARED	=		1<<3,

	VM_MAYREAD 	=		1<<4,
	VM_MAYWRITE	=		1<<5,
	VM_MAYEXEC	=		1<<6,
	VM_MAYSHARE	=		1<<7,


	VM_GROWSDOWN=		1<<8,
	VM_GROWSUP  =		1<<9,
	VM_DENYWRITE= 		1<<10,
	VM_DONTCOPY	=		1<<11,

	VM_STACK = VM_READ | VM_WRITE | VM_GROWSDOWN | VM_MAYREAD | VM_MAYWRITE,
};

enum{
	MAP_FIXED	=	0,	//以后再说吧，谁知道穿进来的MAP_xx怎么塞到vma->flags里?
};

struct vm_area;
struct vm_operations{
	void (*open)(struct vm_area *area);
	void (*close)(struct vm_area *area);
	struct page *(*nopage)(struct vm_area *area, u32 address, union pgerr_code errcode);
};

struct vm_area{
	struct mm *mm;
	u32 start;
	u32 end;	//shall be aligned on PAGE_SIZE
	union pte empty_pte;	/*high 20 bits cleared. low 12 bits used as template
							 When adding a page, the kernel 
							sets the flags in the corresponding Page Table entry
							according to this field */
	union vm_flags flags;		/* flags of the region */

	struct vm_area *prev, *next;
	struct vm_operations *ops;
	struct file *file;
	u32 pgoff;			/* offset in PAGE_SIZE units */
};

extern struct slab_head *vm_area_cache;
extern struct slab_head * mm_cache;
void vm_update_pgprot(struct vm_area *vma);
u32 get_unmapped_area(u32 addr, u32 len);
struct vm_area *find_vma(struct mm *mm, unsigned long addr);
void * mmap(ulong addr, u32 len, int vm_flags, int map_flags, struct file *file, u32 offset);

int common_no_page(struct vm_area *vma, u32 err_addr, union pgerr_code errcode);

//看看@start_addr和@end_addr这一段区间是否跟某个vma接壤
static inline struct vm_area *
find_vma_intersection(struct mm *mm, 
					unsigned long start_addr, unsigned long end_addr)
{
	struct vm_area *beneath = find_vma(mm, start_addr);	
	if(beneath && beneath->start < end_addr){
		return beneath;
	}
	return 0;
}

bool __release_address(union pte *pgdir, unsigned long vaddr);
bool __resolve_address(union pte *pgdir, u32 vaddr, u32 pgprot);
bool vm_area_shrink(struct vm_area *vma, unsigned long new_end);
bool vm_area_expand(struct vm_area *vma, unsigned long new_end);
unsigned long k_brk(unsigned long brk);

#define PGDIR_OF_MM(mm) ( (union pte *)__va(mm->cr3.value & PAGE_MASK) )
extern struct slab_head *fs_struct_cache, *files_struct_cache;

static inline struct mm *
get_mm(struct mm *that){
	that->users++;	
	return that;
}

int __release_mm(struct mm *mm);
/* 下面说错了，可以在非上下文环境被释放的。因为页目录，页表什么的，都在内核里。
 * 递减count计数，若为０．则释放mm结构体,以及里面的pgdir。
 * 注意，在调用put_mm之前,你必须调用release_user_space()释放掉用户空间
 * 因此，所put的mm是一个空壳
 * TODO unstable. 希望能写出来一个跟exit通用的接口
 */
static inline void 
put_mm(struct mm *that){
																				 assert(that->vma);
	that->users--;
	if(that->users == 0){
		__release_mm(that);
	}
}



#endif















