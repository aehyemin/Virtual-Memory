/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "userprog/process.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	list_init(&frame_table);
	lock_init(&frame_table_lock);
}

unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED);
bool page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
// 커널이 새로운 page 생성에 대한 요청을 받을 때, page 구조체를 생성하고 vm_type에 따른 적절한 초기화 함수를 설정한다.
// Uninitialized page에 대한 Meta-data를 설정한다.
// 즉, anonymous page와 같은 경우, lazy-loading 시 필요한 데이터를 설정하는 것이라고 볼 수 있다.
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check whether the upage is already occupied or not. */
	/* User virtual address 가 쓰이고 있는지를 확인한다. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initializer according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		
		// 1)새로운 page를 생성한다.
		struct page *p = (struct page*)malloc(sizeof(struct page));
		// 2)page의 종류에 따라 처리할 함수를 선언한다.
		bool (*page_initializer)(struct page*, enum vm_type, void*);
	
		switch(VM_TYPE(type))
		{
			case VM_ANON:
				page_initializer = anon_initializer;
				break;			
			
			case VM_FILE:
				page_initializer = file_backed_initializer;
				break;
		}

		// 3)선언된 page의 종류에 맞는 uninitialized page의 data로 초기화한다.
		// e.g) page fault가 났을 시, anonymous page를 생성하고 lazy_load 함수로 처리하는 정보 등에 대한 meta data를 설정한다.
		uninit_new(p, upage, init, type, aux, page_initializer);

		p->writable = writable;

		// 4)생성한 page를 spt에 추가한다.
		return spt_insert_page(spt,p);

	}
err:
	return false;
}


/* Find VA from spt and return page. On error, return NULL. */
// Supplementary page table 에서, virtual address를 갖고 있는 page를 반환한다.
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function. */
	page = (struct page*)malloc(sizeof(struct page));
	struct hash_elem *e;

	// Supplementary page table의 virtual address에서, hash elem을 찾는 코드이다.
	// Virtual address를 갖고 있는 page를 찾고, 그 page에서 hash elem을 찾는 코드이다.
	// 해당되는 Virtual address를 갖고 있는 page의 시작 주소를 알기 위해서 rounddown을 해줘야 한다.
	page->va = pg_round_down(va);
	e = hash_find(&spt->spt_hash, &page->hash_elem);
	free(page);

	// Supplementary page table 내 virtual address에서 찾은 hash elem의 page를 반환한다.
	return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
// Supplementary page table에 page를 삽입한다.
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/* TODO: Fill this function. */
	// 이때 virtual address가 supplementary page table에 존재하는지 확인하고, 존재하지 않을 경우에만 supplementary page table에 page를 삽입한다. 
	return hash_insert(&spt->spt_hash, &page -> hash_elem) == NULL ? true : false;

}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	hash_delete(&spt->spt_hash, &page->hash_elem);
	vm_dealloc_page(page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void)
{
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */
	struct thread *curr = thread_current();

	lock_acquire(&frame_table_lock);
	struct list_elem *start = list_begin(&frame_table);
	for (start; start != list_end(&frame_table); start = list_next(start))
	{
		victim = list_entry(start, struct frame, frame_elem);
		if (victim->page == NULL) // frame에 할당된 페이지가 없는 경우 (page가 destroy된 경우 )
		{
			lock_release(&frame_table_lock);
			return victim;
		}
		if (pml4_is_accessed(curr->pml4, victim->page->va))
			pml4_set_accessed(curr->pml4, victim->page->va, 0);
		else
		{
			lock_release(&frame_table_lock);
			return victim;
		}
	}
	lock_release(&frame_table_lock);
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{
	struct frame *victim = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */
	if (victim->page)
		swap_out(victim->page);
	return victim;
}

/* Main memory 내 각 Frame 정보를 갖고 있는 Frame Table을 구성한다. */
// palloc_get_page를 이용해서, 새로운 Frame을 가져온다.
// Main memory에서 page로 가득 차 있다면 Disk의 swap space로 eviction을 통해서, main memory내 공간을 창출한다.

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame(void)
{
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	// User 에게 할당된 physical memory내 공간을 palloc_get_page를 통해서 할당한다.
	// User memory 영역에서 가져온 physical memory 영역을 Kernel Virtual Address에 할당하는 것은, Memory allocation 등 해당되는 user을 위한 physical memory 영역에 대한 정보를 Kernel 이 알아야 하기 때문이다.
	void *kva = palloc_get_page(PAL_USER); // user pool에서 새로운 physical page를 가져온다.

	// User memory 영역에서 가져온 physical memory 공간을 Kernel Virtual Address 변수에 할당하지 못하면 할당 실패 메시지를 띄우고, physical memory 공간이 다 차 있기 때문에 frame을 Disk 공간의 swap space로 swap out 한다.
	if (kva == NULL) // page 할당 실패
	{
		struct frame *victim = vm_evict_frame();
		victim->page = NULL;
		return victim;
	}

	// Frame을 위한 공간을 할당하고, Frame의 Kernel Virtual Address 값을 주어진 값으로 초기화한다.
	frame = (struct frame *)malloc(sizeof(struct frame)); // 프레임 할당
	frame->kva = kva;									  // 프레임 멤버 초기화
	frame->page = NULL;

	lock_acquire(&frame_table_lock);
	list_push_back(&frame_table, &frame->frame_elem);
	lock_release(&frame_table_lock);
	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED)
{
	// todo: 스택 크기를 증가시키기 위해 anon page를 하나 이상 할당하여 주어진 주소(addr)가 더 이상 예외 주소(faulted address)가 되지 않도록 합니다.
	// todo: 할당할 때 addr을 PGSIZE로 내림하여 처리
	vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), 1);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

/* Return true on success */
/**
 *not_present; // True: not-present page, false: writing r/o page.
 *write;	  // True: access was write, false: access was read.
 *user;		  // True: access by user, false: access by kernel.
 */
// SPT를 참조하여 특정 Page가 대응되는 Frame이 없을 경우, 해결하도록 하는 함수인 vm_try_handle_fault를 수정한다. 
// Page fault가 발생할 때, 제어권을 전달 받는 함수이다. 
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;

	if(addr == NULL){
		return false;
	}
	
	if(is_kernel_vaddr(addr)){
		return false;
	}

	// Page에 할당할 Frame이 존재하지 않을 경우, not_present = true를 전달 받는다.
	// SPT 내 Page 자체가 존재하지 않거나 Read-only page에 Write 요청을 할 경우, Frame을 할당할 수 없기 때문에 해당 예외에 대한 false 처리를 해준다.
	// 그리고 SPT 내 해당 주소에 주어진 Page가 있는지 확인해서 존재한다면, 해당 Page 에 Frame 할당을 요청하는 vm_do_claim_page 함수를 호출한다. 	

	// Page에 할당할 Frame이 존재하지만 read-only page에 write을 한 경우이기 때문에, not_present = false를 전달 받는다.
	if(not_present){
		/* TODO: Validate the fault */
		/* TODO: Your code goes here */
		page = spt_find_page(spt, addr);
		if(page == NULL){
			return false;
		}

		if(write ==1 && page->writable == 0){
			return false;
		}
		return vm_do_claim_page(page);
	}
	return false;

}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page)
{
	destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
// Virtual address 로 page를 찾고, vm_do_claim_page를 호출하는 함수이다.
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	// 현재 Thread의 supplementary page table에서의 virtual address를 통해서 page를 찾는 함수이다.
	page = spt_find_page(&thread_current()->spt, va);	
	if (page == NULL){
		return false;
	}

	// Virtual address로 page를 찾았다면, 해당 page와 frame을 대응시키는 vm_do_claim_page를 실행시킨다.
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
// Page 와 Frame을 대응 시켜주고, 현재 Thread의 page table에 해당 관계를 기록한다.
// 그리고 User Memory 내 가용 가능한 공간이 없어서 해당 Frame 공간이 Disk의 swap space로 swap out 됐다면,
// Disk의 swap space에서 다시 User memory로 가져오는 함수이다. 
static bool
vm_do_claim_page (struct page *page) {
	// User memory 내 physical memory에서 공간을 할당 받아온다. 
	struct frame *frame = vm_get_frame ();

	/* Set links */
	// Virtual address 와 Physical address 간 대응 관계를 만드는 변수를 선언해준다.
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	struct thread * curr = thread_current();
	// 현재 Thread의 page table에, 현재 page의 user virtual address와 frame 내의 kernel virtual address를 대응시켜주는 함수이다. 
	// 최종적으로는 Thread의 page table에, page의 user virtual address와 frame 내 kernel virtual address가 갖고 있는 physical address를 대응시키는 것이 목적이다. 
	pml4_set_page(curr->pml4, page->va, frame->kva, page->writable);
	
	return swap_in (page, frame->kva); // swap out 될 경우에는, disk에서부터 해당 page를 가져온다.
}									   // 애초에 uninitialized 될 경우에는, uninit_initialize 함수가 실행돼서 page를 frame에 적재하는 과정을 거친다. 	

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	// Hash table 형식으로 새로운 Supplemental page table을 생성한다.
	// Hash table 을 생성하는 함수로, page_hash 함수를 통해서 virtual address를 hashed index로 만들어주고 page_less 함수를 통해서 해당 hashed index와 같은지 비교한다. 
	hash_init(spt, page_hash, page_less, NULL);

}

// Page 내 virtual address 를, size 크기의 hash index로 바꿔주는 함수이다.
unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED){
	// Hash elem -> page 형태의 자료구조로 바꿔주는 함수이다.
	const struct page *p = hash_entry(p_, struct page, hash_elem);
	return hash_bytes( &p -> va, sizeof p->va);

}

// 같은 Hash index 로 인해 Hash collision이 발생하지 않도록, Hash index 가 오름차순으로 정렬될 수 있도록 확인한다. 
bool page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED){
	const struct page *a = hash_entry(a_, struct page, hash_elem);
	const struct page *b = hash_entry(b_, struct page, hash_elem);

	return a->va < b->va;
}

/* Copy supplemental page table from src to dst */
// SPT를 복사하는 함수 (자식 프로세스가 부모 프로세스의 실행 컨텍스트를 상속해야 할 때 (즉, fork() 시스템 호출이 사용될 때) 사용)
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
	// TODO: 보조 페이지 테이블을 src에서 dst로 복사합니다.
	// TODO: src의 각 페이지를 순회하고 dst에 해당 entry의 사본을 만듭니다.
	// TODO: uninit page를 할당하고 그것을 즉시 claim해야 합니다.
	struct hash_iterator i;
	hash_first(&i, &src->spt_hash);
	while (hash_next(&i))
	{
		// src_page 정보
		struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
		enum vm_type type = src_page->operations->type;
		void *upage = src_page->va;
		bool writable = src_page->writable;

		/* 1) type이 uninit이면 */
		if (type == VM_UNINIT)
		{ // uninit page 생성 & 초기화
			vm_initializer *init = src_page->uninit.init;
			void *aux = src_page->uninit.aux;
			vm_alloc_page_with_initializer(VM_ANON, upage, writable, init, aux);
			continue;
		}

		/* 2) type이 file이면 */
		if (type == VM_FILE)
		{
			struct lazy_load_arg *file_aux = malloc(sizeof(struct lazy_load_arg));
			file_aux->file = src_page->file.file;
			file_aux->ofs = src_page->file.ofs;
			file_aux->read_bytes = src_page->file.read_bytes;
			file_aux->zero_bytes = src_page->file.zero_bytes;
			if (!vm_alloc_page_with_initializer(type, upage, writable, NULL, file_aux))
				return false;
			struct page *file_page = spt_find_page(dst, upage);
			file_backed_initializer(file_page, type, NULL);
			file_page->frame = src_page->frame;
			pml4_set_page(thread_current()->pml4, file_page->va, src_page->frame->kva, src_page->writable);
			continue;
		}

		/* 3) type이 anon이면 */
		if (!vm_alloc_page(type, upage, writable)) // uninit page 생성 & 초기화
			return false;						   // init이랑 aux는 Lazy Loading에 필요. 지금 만드는 페이지는 기다리지 않고 바로 내용을 넣어줄 것이므로 필요 없음

		// vm_claim_page으로 요청해서 매핑 & 페이지 타입에 맞게 초기화
		if (!vm_claim_page(upage))
			return false;

		// 매핑된 프레임에 내용 로딩
		struct page *dst_page = spt_find_page(dst, upage);
		memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
	}
	return true;
}

/* Free the resource hold by the supplemental page table */
// SPT가 보유하고 있던 모든 리소스를 해제하는 함수 (process_exit(), process_cleanup()에서 호출)
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// todo: 페이지 항목들을 순회하며 테이블 내의 페이지들에 대해 destroy(page)를 호출
	hash_clear(&spt->spt_hash, hash_page_destroy); // 해시 테이블의 모든 요소를 제거

	/** hash_destroy가 아닌 hash_clear를 사용해야 하는 이유
	 * 여기서 hash_destroy 함수를 사용하면 hash가 사용하던 메모리(hash->bucket) 자체도 반환한다.
	 * process가 실행될 때 hash table을 생성한 이후에 process_clean()이 호출되는데,
	 * 이때는 hash table은 남겨두고 안의 요소들만 제거되어야 한다.
	 * 따라서, hash의 요소들만 제거하는 hash_clear를 사용해야 한다.
	 */

	// todo🚨: 모든 수정된 내용을 스토리지에 기록
}

void hash_page_destroy(struct hash_elem *e, void *aux)
{
	struct page *page = hash_entry(e, struct page, hash_elem);
	destroy(page);
	free(page);
}
