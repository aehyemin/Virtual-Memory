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
#include "threads/thread.h"


static unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED);
static bool page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux UNUSED) ;

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
	list_init(&frame_table);
	lock_init(&frame_table_lock);
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	//list_init(&frame_table);
	//start = list_begin(&frame_table);

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
//새로운 페이지를 할당하고 초기화함
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
        struct page *p = (struct page *)malloc(sizeof(struct page));
        
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
err:
    return false;
}


/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
    struct page *page = NULL;
    /* TODO: Fill this function. */
    page = malloc(sizeof(struct page));
    struct hash_elem *e;

    // va에 해당하는 hash_elem 찾기
    page->va = va;
    e = hash_find(&spt, &page->hash_elem);

    // 있으면 e에 해당하는 페이지 반환
    return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

// struct hash_elem *
// hash_find (struct hash *h, struct hash_elem *e) {
// 	return find_elem (h, find_bucket (h, e), e);
// }

/* Insert PAGE into spt with validation. */
/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
                     struct page *page UNUSED)
{
    /* TODO: Fill this function. */
    return hash_insert(&spt, &page->hash_elem) == NULL ? true : false; // 존재하지 않을 경우에만 삽입
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	hash_delete(&spt->spt_hash, &page->hash_elem);
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
 //프레임을 swap out하고, 확보된 프레임을 반환
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	//swap_out(vicitm->page);
	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
 //물리 메모리에서 프레임을 할당하는 역할.
 //물리 메모리가 가득 차서 더 이상 할당할 수 없는경우, 페이지를 evict(현재 메모리 페이지를 디스크로 내보내고
 //메모리를 확보)
static struct frame *
vm_get_frame(void)
{
    struct frame *frame = NULL;
    /* TODO: Fill this function. */
    void *kva = palloc_get_page(PAL_USER); // user pool에서 새로운 physical page를 가져온다.

    if (kva == NULL)   // page 할당 실패 -> 나중에 swap_out 처리
        PANIC("todo"); // OS를 중지시키고, 소스 파일명, 라인 번호, 함수명 등의 정보와 함께 사용자 지정 메시지를 출력

    frame = malloc(sizeof(struct frame)); // 프레임 할당
    frame->kva = kva;                      // 프레임 멤버 초기화

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
    vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), 1);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}
/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
                         bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
    struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
    struct page *page = NULL;
    if (addr == NULL)
        return false;

    if (is_kernel_vaddr(addr))
        return false;

    if (not_present) // 접근한 메모리의 physical page가 존재하지 않은 경우
    {
        /* TODO: Validate the fault */
        page = spt_find_page(spt, addr);
        if (page == NULL)
            return false;
        if (write == 1 && page->writable == 0) // write 불가능한 페이지에 write 요청한 경우
            return false;
        return vm_do_claim_page(page);
    }
    return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
//가상 주소 va에 해당하는 페이지를 확보
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	//va 에 해당하는 page(virtual page)를 가져온다

	/* Find VA from spt and return page. On error, return NULL. */
// struct page *
// spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED)
    page = spt_find_page (&thread_current()->spt, va);

	if (page == NULL) {
		return false;
	}

	//해당 페이지로 vm_do _claim _page 호출
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
//주어진 페이지를 실제 물리 메모리에 할당하고,이를 물리적 프레임과 매핑하는 작업 수행
//새 프레임을 가져와 페이지와 매핑
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	//가상 주소와 물리 주소를 매핑
	struct thread *cur = thread_current();
	pml4_set_page(cur->pml4, page->va, frame->kva, page->writable);
	//pml4_set_page (uint64_t *pml4, void *upage, void *kpage, bool rw)
	//pml4, 사용자 가상 페이지, 커널 가상 페이지, 읽기쓰기 권환

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
// 보조 테이블 초기화함
/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
    hash_init(spt, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}


//하혜민
/* Returns a hash value for page p. */
//hash_entry를 사용해서 struct page 로 변환한다
//p->va(가상주소) 필드를 해싱하여 해시값을 계산한다.
//hash_bytes 함수는 특정 메모리 주소의 데이터를 바이트로 해싱한다.
static unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
struct page* page = hash_entry(p_,struct page,hash_elem);
	return hash_bytes(&page->va,sizeof(page->va)); 
}

static bool
page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux UNUSED) {
	struct page* a = hash_entry(a_,struct page,hash_elem);
	struct page* b = hash_entry(b_,struct page,hash_elem);
	return a->va < b->va;
}
