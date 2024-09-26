/* file.c: Implementation of memory backed file object (mmaped object). */
//파일을 기반으로 한 메모리 매핑 객체(메모리 매핑 파일)관리하는 구현
//페이지 스왑인,아웃. 페이지제거, mmap, munmap 작업

#include "vm/vm.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,//파일에서 페이지를 읽어 메모리에 로드
	.swap_out = file_backed_swap_out,//메모리에서 페이지를 파일로 기록
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {//파일 기반 페이지 초기화
}

//file backed 는 파일 시스템에 의해 백업되는 페이지 - 파일 매핑 페이지
/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
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
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
//파일의 내용을 메모리에 매핑
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
}

/* Do the munmap */

void
do_munmap (void *addr) {
}
