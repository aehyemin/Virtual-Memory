/* uninit.c: Implementation of uninitialized page.
 *
 * All of the pages are born as uninit page. When the first page fault occurs,
 * the handler chain calls uninit_initialize (page->operations.swap_in).
 * The uninit_initialize function transmutes the page into the specific page
 * object (anon, file, page_cache), by initializing the page object,and calls
 * initialization callback that passed from vm_alloc_page_with_initializer
 * function.
 * */
/* 페이지는 처음에 초기화되지 않은 상태로 태어나며,
 페이지 폴트가 발생하면 해당 페이지가 실제로 필요해졌다는 의미.
 페이지가 실제로 사용될때까지 메모리에 내용을 로드하지 않다가 
 페이지 폴트가 발생하면 uninit_initialize() 함수를 통해 필요한 페이지로 변환되고, 메모리에 적제됨
 메모리 낭비 줄임

 uninit_new 초기화되지 않은 페이지 생성, 초기화될 수 있도록 필요한 데이터 설정
 uninit_initialize 페이지에 실제 메모리 공간 할당, 초기화 작업 수행
*/

#include "vm/vm.h"
#include "vm/uninit.h"

static bool uninit_initialize (struct page *page, void *kva);
static void uninit_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations uninit_ops = {
	.swap_in = uninit_initialize,//페이지폴트 발생 시, 페이지 초기화
	.swap_out = NULL,//아직 구현되지 않았으며, 초기화되지 않은 페이지는 스왑 아웃되지 않도록 설정
	.destroy = uninit_destroy,//
	.type = VM_UNINIT,
};

/* DO NOT MODIFY this function */
void //초기화되지 않은 페이지 새롭게 생성
uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *)) {
	ASSERT (page != NULL);

	*page = (struct page) {
		.operations = &uninit_ops,
		.va = va,
		.frame = NULL, /* no frame for now */
		.uninit = (struct uninit_page) {
			.init = init,
			.type = type,
			.aux = aux,
			.page_initializer = initializer,
		}
	};
}

/* Initalize the page on first fault */
//uninit_new()에서 설정한 page_initializer를 호출하여, 
//페이지를 특정 유형(예: 익명 페이지, 파일 페이지)으로 변환하고 메모리에 적재
//페이지에 실제 메모리공간 할당, 초기화 수행
static bool
uninit_initialize (struct page *page, void *kva) {
	struct uninit_page *uninit = &page->uninit;

	/* Fetch first, page_initialize may overwrite the values */
	vm_initializer *init = uninit->init;
	void *aux = uninit->aux;

	/* TODO: You may need to fix this function. */
	return uninit->page_initializer (page, uninit->type, kva) &&
		(init ? init (page, aux) : true);
}

/* Free the resources hold by uninit_page. Although most of pages are transmuted
 * to other page objects, it is possible to have uninit pages when the process
 * exit, which are never referenced during the execution.
 * PAGE will be freed by the caller. */
static void
uninit_destroy (struct page *page) {
	struct uninit_page *uninit UNUSED = &page->uninit;
	/* TODO: Fill this function.
	 * TODO: If you don't have anything to do, just return. */
}
