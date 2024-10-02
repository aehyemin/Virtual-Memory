/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "threads/mmu.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {

}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	// printf("file_backed_initializer :: page : %p\n", page);
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;

	// struct container *c = page->uninit.aux;

	// file_page->file = c->file;
	// file_page->ofs = c->ofs;
	// file_page->page_read_bytes = c->page_read_bytes;
	// file_page->page_zero_bytes = c->page_zero_bytes;
	// printf("file_backed_initializer :: done\n");

	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page = &page->file;

	// 쓰여진 페이지는 file에 쓰여짐
	// 쓰여지지 않은 페이지는 그러면 안됨
	struct container *c = page->uninit.aux;

    if (pml4_is_dirty(thread_current()->pml4, page->va))
    {
		file_write_at(c->file, page->va, c->page_read_bytes, c->ofs);
        // file_write_at(file_page->file, page->va, file_page->page_read_bytes, file_page->ofs);
        pml4_set_dirty(thread_current()->pml4, page->va, 0);
    }
    pml4_clear_page(thread_current()->pml4, page->va);
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	
	// 리눅스에서 만약 `addr` 가 NULL이면, 커널은 매핑하기에 적절한 주소 찾음
	// 따라서, 만약 `addr` 가 0이면, 실패해야 함 → 몇몇 Pintos code는 가상 페이지 0은 매핑되지 않는다고 가정

	// file_reopen 을 파일의 각 매핑에 대해 분리되고 독립적인 참조를 얻기 위해 사용
	struct file *f = file_reopen(file);
    void *start_addr = addr;

	uint32_t read_bytes, zero_bytes;
	// `fd` 로 열린 파일을 `offset` 바이트부터 시작해서 `length` 바이트 만큼
	// 프로세스의 가상주소 공간(`addr`)으로 map
    read_bytes = file_length(f) - offset < length ? file_length(f) - offset : (uint32_t)length;
	// 만약 파일의 길이가 PGSIZE의 배수가 아닌 경우, 마지막 mapped page에 있는 몇몇 바이트는 파일 끝을 넘어 stick out 됨
	// 페이지가 fault in 됐을 때 이러한 바이트를 0으로 set
	// 페이지가 디스크로 쓰여 들어갈 때 버려라
    zero_bytes = read_bytes > 0 ? PGSIZE - read_bytes % PGSIZE : 0;

	// 페이지 정렬
    ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
	// 만약 addr가 페이지 정렬 되어 있지 않으면 실패
	// 매핑된 페이지 범위가 기존에 매핑된 페이지(스택이나 실행파일 로드시 매핑된 페이지 포함)와 겹치면 실패
    // ASSERT(pg_ofs(addr) == 0);
    // ASSERT(offset % PGSIZE == 0);

	int page_count = 0;
	// 전체 파일은 `addr`부터 시작되는 연속된 가상 페이지에 매핑
    while (read_bytes > 0 || zero_bytes > 0)
    {
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct container *aux = malloc(sizeof (struct container));
        aux->file = f;
        aux->page_read_bytes = page_read_bytes;
        aux->page_zero_bytes = page_zero_bytes;
        aux->ofs = offset;
		// page 객체 만들기 위해 `vm_alloc_page_with_initializer` 사용
		// lazy하게 페이지 로드
		if (!vm_alloc_page_with_initializer (VM_FILE, addr,
					writable, lazy_load_segment, aux))
		{
			// printf("load_segment :: vm_alloc_page_with_initializer fail\n");
#ifdef VM
	struct page *p = spt_find_page(&thread_current()->spt, start_addr);
	if (p)
		p->page_count = page_count;
#endif
			free(aux);
			// 실패라면, NULL 반환해야 함
			return NULL;
		}

		page_count += 1;

        /* Advance. */
        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        addr += PGSIZE;
        offset += page_read_bytes;
    }
#ifdef VM
	struct page *p = spt_find_page(&thread_current()->spt, start_addr);
	if (p)
		p->page_count = page_count;
	// 만약 fd로 열린 파일이 0 바이트 길이를 가지고 있으면 `mmap` 호출은 실패
	// `length`가 0일 때도 실패
	else
		return NULL;
#endif

	// 성공적이라면, 이 함수는 file이 매핑된 가상 주소 리턴
    return start_addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {

	// 아직 unmap 되지 않은 같은 프로세스에 의해 이전 mmap 호출이 반환한 가상 주소인 특정 주소 범위 addr 에 대해 unmap
	// 모든 매핑은 process exit 할 때 암시적으로 unmap

#ifdef VM
	struct supplemental_page_table *spt = &thread_current()->spt;
    struct page *p = spt_find_page(spt, addr);

	// 파일을 닫거나 삭제하는 것은 매핑을 unmap하지는 않음
	// 생성된 매핑은 Unix 규칙에 따라 `munmap`이 호출되거나 프로세스가 exit 할 때까지 유효
	// mmap 시스템 콜은 페이지가 공유되는지 private한지 클라이언트가 지정할 수 있는 인자를 가짐 (ex. copy-on-write)

	if (p) {
		int count = p->page_count;

		for (int i = 0; i < count; i++)
		{
			if (p) {
				// 페이지는 프로세스의 가상 페이지 리스트에서 제거됨
				spt_remove_page(spt, p);
			}
			addr += PGSIZE;
			p = spt_find_page(spt, addr);
		}
	}
#endif

}
