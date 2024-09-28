/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"

unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED);

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
}

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
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux UNUSED) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	printf("유페이지 %p\n", upage);

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		// printf("spt_find_page is NULL\n");
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		// 주어진 type으로 초기화되지 않은 page 생성
		struct page *page = (struct page *)malloc(sizeof(struct page));

		// uninit page 의 swap_in handler 는 자동으로 type에 따라 페이지 초기화하고 주어진 AUX 로 INIT 호출

		// `vm.h` 에 정의된 `VM_TYPE` 매크로 사용
		bool (*initializer)(struct page *, enum vm_type, void *kva);
		initializer = NULL;

		switch(VM_TYPE(type)) {
			case VM_ANON:
				initializer = &anon_initializer;
				// printf("type is VM_ANON\n");
				break;
			case VM_FILE:
				initializer = &file_backed_initializer;
				break;
		}
		uninit_new(page, upage, init, type, aux, initializer);

		/* TODO: Insert the page into the spt. */
		// 페이지 구조체를 가지면 프로세스의 supplementary page table에 page 삽입
		return spt_insert_page(spt, page);
		// page fault handler는 call chain을 따르고 마지막에 swap_in을 호출하면 `uninit_intialize` 에 도달
		// 완성된 구현을 주지만 수정해도 됨
	}

	printf("vm_alloc_page_with_initializer :: spt_find_page is not NULL\n");
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = (struct page *)malloc(sizeof(struct page));
	/* TODO: Fill this function. */
	// 주어진 supplemental page table에서 va 가 일치하는 struct page 찾음
	struct hash_elem *e;

	// page->va = va;
	page->va = pg_round_down(va);

	printf("spt_find_page :: spt_hash :: %p\n", &spt->spt_hash);
	printf("spt_find_page :: page :: %p\n", page);
	e = hash_find(&spt->spt_hash, &page->elem);
	printf("spt_find_page :: hash entry :: %p\n", hash_entry(e, struct page, elem));
	free(page);

	return e != NULL ? hash_entry(e, struct page, elem): NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	// 가상 주소가 주어진 supplemental page table에 존재하지 않는다는 것을 확인해야 함
	// 주어진 supplemental page table에 `strcut page` 삽입
	if (!hash_insert(&spt->spt_hash, &page->elem)) {
		// printf("spt insert page success\n");
		succ = true;
	}
    
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = (struct frame *)malloc(sizeof(struct frame));
	/* TODO: Fill this function. */
	// user pool에서 새로운 물리 페이지 받기 → `palloc_get_page` 호출
	if(!(frame->kva = palloc_get_page(PAL_USER))) {
		// 지금은 page 할당이 실패했을 때 swap out을 처리할 필요 없음
		// 그냥 그런 케이스를 지금은 `PANIC(”todo”)` 로 표시
		PANIC("todo");
	}
	// user pool에서 성공적으로 페이지를 받으면, 프레임도 할당하고 그 멤버도 초기화한 후, 그것을 return
	frame->page = NULL;

	// `vm_get_frame` 을 구현한 후, 너는 모든 user space pages(PALLOC_USER)를 이 함수를 통해 할당해야 함
	
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	// 1. supplemental page table에서 fault 된 페이지 찾기.
	// 만약 메모리 참조가 유효하다면 supplemental page table entry를 사용하여 페이지(file system 또는 swap slot 혹은 all-zero page)에 들어갈 데이터 찾기.
    // user 프로세스가 접근하려는 주소에 데이터가 없거나 페이지가 커널 가상 메모리 내에 있거나
	if (is_kernel_vaddr(addr)) {
		// printf("is kernel vaddr\n");
        return false;
	}
	// printf("addr: %p\n", addr);
	// read-only page에 write 하려는 접근을 하면 해당 엑세스는 유효하지 않음.
	// 어떤 유효하지 않은 접근은 process terminate 하고 모든 리소스 free
    // 2. page를 저장하기 위해 frame 얻기.
	// 3. file system 이나 swap에서 데이터를 읽어오거나 0으로 초기화하는 등의 방식으로 frame에 데이터 fetch.
	// 4. fault 하는 가상 주소의 page table entry를 물리 페이지로 지정. `threads/mmu.c` 에 있는 함수 사용.

	// printf("vm_claim_page\n");
	return vm_claim_page (addr);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	// `va` 에 페이지 할당
#ifdef VM
	if (!(page = spt_find_page(&thread_current()->spt, va))) {
		return false;
	}
#endif

	// 처음에 페이지를 받고 페이지로 `vm_do_claim_page` 를 호출함
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	// `vm_get_frame` 호출로 프레임을 받음
	struct frame *frame = vm_get_frame ();

	// page에 물리 프레임 할당을 의미
	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	// 그 다음, MMU를 set up
	// 즉, page table에 가상 주소를 물리 주소로 매핑 추가
	struct thread *t = thread_current();

	if (pml4_get_page(t->pml4, page->va) == NULL && pml4_set_page(t->pml4, page->va, frame->kva, true)) {
		// printf("kfjwo\n");
		return swap_in(page, frame->kva);
	}

	// 반환 값은 작업이 successful했는지 아닌지 나타내야 함
	return false;
}

unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED) {
	const struct page *p = hash_entry(p_, struct page, elem);
	return hash_bytes(&p->va, sizeof(p->va));
}

static bool hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
	const struct page *p_a = hash_entry(a, struct page, elem);
	const struct page *p_b = hash_entry(b, struct page, elem);
	return p_a->va < p_b->va;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->spt_hash, page_hash, hash_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	// src 에서 dst 로 supplemental page table 복사
	struct hash_iterator i;
    hash_first (&i, &src->spt_hash);
	// 자식이 부모의 실행 문맥을 상속해야 할 때 사용
	// src의 supplemental page table에 있는 각 페이지에 대해 반복
    while (hash_next (&i)) {
        struct page *parent_page = hash_entry (hash_cur (&i), struct page, elem);
        enum vm_type type = page_get_type(parent_page);
        void *upage = parent_page->va;
        vm_initializer *init = parent_page->uninit.init;
        void* aux = parent_page->uninit.aux;

		// 스택 페이지이면
        if (type & VM_MARKER_0) {
			void *stack_bottom = (void *)(((uint8_t *)USER_STACK) - PGSIZE);
			if (vm_alloc_page(type, stack_bottom, 1)) {
				if (!vm_claim_page(stack_bottom)) {
					return false;
				}
			}
        }
		// uninit page 할당 필요하고 즉시 그들을 claim
        else if(type == VM_UNINIT) {
            if(!vm_alloc_page_with_initializer(type, upage, 1, init, aux))
                return false;
			vm_claim_page(upage);
        }
        else {
            if(!vm_alloc_page(type, upage, 1))
                return false;
            if(!vm_claim_page(upage))
                return false;
        }

		// dst의 supplemental page table에 정확한 항목의 복사
        if (parent_page->frame) {
			struct page* child_page = spt_find_page(dst, upage);
            memcpy(child_page->frame->kva, parent_page->frame->kva, PGSIZE);
        }
    }

    return true;
}

void spt_destructor(struct hash_elem *e, void* aux) {
    const struct page *p = hash_entry(e, struct page, elem);
    free(p);
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// supplemental page table에 의해 열린 모든 리소스 free

	// `process exit()` 할 때 호출
	// 페이지 항목을 반복하고 table에 페이지들에 대해 `destroy(page)` 호출
	// struct hash_iterator i;
    // hash_first (&i, &spt->spt_hash);
	// while (hash_next (&i)) {
    //     struct page *page = hash_entry (hash_cur (&i), struct page, elem);
	// 	destroy(page);
	// 	// free()
	// }
	// 실제 page table (pml4)과 함수의 물리 메모리 걱정할 필요 없음
	// 호출자가 supplemental page table clean up한 후에 clean
	hash_destroy(&spt->spt_hash, spt_destructor);
}
