/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/vaddr.h"
#include "lib/kernel/bitmap.h"

struct bitmap *swap_table;
const size_t SECTORS_PER_PAGE = (PGSIZE + DISK_SECTOR_SIZE - 1) / DISK_SECTOR_SIZE;

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
	// anonymous page subsystem 초기화
	// 함수에서 anonymous page와 관련된 모든 것 세팅 가능

	// swap_disk = NULL;
	swap_disk = disk_get(1, 1);

	// swap disk 에서 사용 가능한 영역과 사용된 영역을 관리할 자료구조 필요
	// swap area는 PGSIZE 단위로 관리될 것

    size_t swap_page_size = disk_size(swap_disk) / SECTORS_PER_PAGE;
    swap_table = bitmap_create(swap_page_size);
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;

	// 처음에 `page->operations` 에 있는 anonymous page를 위한 handler set up

	// anonymous page의 initializer 함수
	// anon_page 에 스왑을 위한 정보 추가

}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;

	// disk에서 데이터 내용 읽음으로써 swap disk에 있는 anonymous page swap in
	// 데이터 위치는 페이지가 swap out 될 때 page 구조체에 저장된 swap disk

	// int page_no = anon_page->swap_index;

    // if (bitmap_test(swap_table, page_no) == false) {
    //     return false;
    // }

    // for (int i = 0; i < SECTORS_PER_PAGE; ++i) {
    //     disk_read(swap_disk, page_no * SECTORS_PER_PAGE + i, kva + DISK_SECTOR_SIZE * i);
    // }

    // bitmap_set(swap_table, page_no, false);
    
    // return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;

	// 먼저, swap table을 사용하는 disk에서 가용 swap slot 찾고
	int page_no = bitmap_scan(swap_table, 0, 1, false);
    if (page_no == BITMAP_ERROR) {
        return false;
    }
	
	// disk로 메모리에서 내용을 복사함으로써 swap disk로 anonymous page swap out
    for (int i = 0; i < SECTORS_PER_PAGE; ++i) {
		// 데이터 페이지를 슬롯으로 복사
        disk_write(swap_disk, page_no * SECTORS_PER_PAGE + i, page->va + DISK_SECTOR_SIZE * i);
    }

    bitmap_set(swap_table, page_no, true);

	// 데이터 위치는 페이지 구조체에 저장되어야 함
    anon_page->swap_no = page_no;
	// 더이상 디스크에 가용 슬롯이 없다면 kernel panic

    pml4_clear_page(thread_current()->pml4, page->va);

    return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	//anonymous page 가 보유하던 리소스 free
	// 명시적으로 페이지 free할 필요 없음 -> 호출자가 할 것
}
