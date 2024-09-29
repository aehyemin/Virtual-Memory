/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/file.h"
#include "vm/inspect.h"
#include "list.h"
#include "include/threads/vaddr.h"
#include "lib/kernel/hash.h"
#include "include/threads/mmu.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

void hash_page_destroy(struct hash_elem *e, void *aux);
bool page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);
unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED);


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
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
                                    vm_initializer *init, void *aux)
{
    ASSERT(VM_TYPE(type) != VM_UNINIT)

    struct supplemental_page_table *spt = &thread_current()->spt;

    /* Check wheter the upage is already occupied or not. */
    // upage가 이미 사용 중인지 확인합니다.
    if (spt_find_page(spt, upage) == NULL)
    {
        /* TODO: Create the page, fetch the initialier according to the VM type,
         * TODO: and then create "uninit" page struct by calling uninit_new. You
         * TODO: should modify the field after calling the uninit_new. */
         
        // 1) 페이지를 생성하고,
        struct page *p = malloc(sizeof (struct page));
        
        // 2) type에 따라 초기화 함수를 가져와서
        bool (*page_initializer)(struct page *, enum vm_type, void *);

        switch (VM_TYPE(type))
        {
        case VM_ANON:
            page_initializer = anon_initializer;
            break;
        case VM_FILE:
            page_initializer = file_backed_initializer;
            break;
        }

        // 3) "uninit" 타입의 페이지로 초기화한다.
        uninit_new(p, upage, init, type, aux, page_initializer);

        // 필드 수정은 uninit_new를 호출한 이후에 해야 한다.
        p->writable = writable;

        // 4) 생성한 페이지를 SPT에 추가한다.
        return spt_insert_page(spt, p);
    }
	return true;
err:
    return false;
}


/* Find VA from spt and return page. On error, return NULL. */
/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page(struct supplemental_page_table *spt, void *va) {
    struct hash_elem *e;
	struct page p;

    // va에 해당하는 hash_elem 찾기
    p.va = va;
    
    // 페이지 주소를 이용하여 hash_elem 검색
    e = hash_find(&spt->spt_hash, &p.hash_elem); // rounded_va 사용

    // 찾은 페이지가 있으면 반환
    return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}


/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
                     struct page *page UNUSED)
{
    /* TODO: Fill this function. */
    return hash_insert(&spt->spt_hash, &page->hash_elem) == NULL ? true : false; // 존재하지 않을 경우에만 삽입
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
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	void *kva = palloc_get_page(PAL_USER);
	
	if(kva == NULL) PANIC("todo");
   frame = malloc(sizeof (struct frame));
   frame->kva = kva;                             // 프레임의 kva(멤버) 초기화
   frame->page = NULL;

   // lock_acquire(&frame_table_lock);
   // list_push_back(&frame_table, &frame->frame_elem);
   // lock_release(&frame_table_lock);
   ASSERT(frame != NULL);
   ASSERT(frame->page == NULL);
   return frame;
}

/* Growing the stack. */
/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED)
{
    // todo: 스택 크기를 증가시키기 위해 anon page를 하나 이상 할당하여 주어진 주소(addr)가 더 이상 예외 주소(faulted address)가 되지 않도록 합니다.
    // todo: 할당할 때 addr을 PGSIZE로 내림하여 처리
   // vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), 1);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}


/* Return true on success */
/* Return true on success */
/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	page = spt_find_page(spt, addr);
	if(page == NULL) return false;

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool vm_claim_page(void *va UNUSED)
{
    struct page *page = NULL;
    /* TODO: Fill this function */
    // spt에서 va에 해당하는 page 찾기
    page = spt_find_page(&thread_current()->spt, va);
    // if (page == NULL)
    //     return false;
    return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
    struct frame *frame = vm_get_frame();

    /* Set links */
    frame->page = page;
    page->frame = frame;

    /* TODO: Insert page table entry to map page's VA to frame's PA. */
    // 가상 주소와 물리 주소를 매핑
    struct thread *current = thread_current();
    pml4_set_page(current->pml4, page->va, frame->kva, page->writable);

    return swap_in(page, frame->kva); // uninit_initialize
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
    hash_init(&spt->spt_hash, page_hash, page_less, NULL);
}
/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
                                  struct supplemental_page_table *src UNUSED)
{
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

        /* 2) type이 uninit이 아니면 */
        if (!vm_alloc_page(type, upage, writable)) // uninit page 생성 & 초기화
            // init이랑 aux는 Lazy Loading에 필요함
            // 지금 만드는 페이지는 기다리지 않고 바로 내용을 넣어줄 것이므로 필요 없음
            return false;

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
void supplemental_page_table_kill (struct supplemental_page_table *spt) {

	hash_clear(&spt->spt_hash, hash_page_destroy);

}


unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED)
{
    const struct page *p = hash_entry(p_, struct page, hash_elem);
    return hash_bytes(&p->va, sizeof p->va);
}


void hash_page_destroy(struct hash_elem *e, void *aux)
{
	struct page *page = hash_entry(e, struct page, hash_elem);
	destroy(page);
	free(page);
}


bool page_less(const struct hash_elem *a_,
               const struct hash_elem *b_, void *aux UNUSED)
{
    const struct page *a = hash_entry(a_, struct page, hash_elem);
    const struct page *b = hash_entry(b_, struct page, hash_elem);

    return a->va < b->va;
}
