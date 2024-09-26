/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */
//이 코드는 **익명 페이지(Anonymous Page)**를 위한 구현을 제공합니다.
// 익명 페이지는 디스크와 관련이 없는 메모리 페이지로, 주로 스택, 힙 등의 메모리에 사용됩니다. 
//익명 페이지는 스왑 영역과 상호작용하며, 
//메모리 부족 시 스왑 디스크에 데이터를 저장했다가 다시 로드할 수 있습니다.
//말록으로 할당되는 힙 영역의 메모리나 스택의 메모리가 익명 페이지로 처리

//특정 파일과 연결되지 않은 메모리

#include "vm/vm.h"
#include "devices/disk.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = NULL;
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
