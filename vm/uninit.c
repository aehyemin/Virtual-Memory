/* uninit.c: Implementation of uninitialized page.
 *
 * All of the pages are born as uninit page. When the first page fault occurs,
 * the handler chain calls uninit_initialize (page->operations.swap_in).
 * The uninit_initialize function transmutes the page into the specific page
 * object (anon, file, page_cache), by initializing the page object,and calls
 * initialization callback that passed from vm_alloc_page_with_initializer
 * function.
 * */

#include "vm/vm.h"
#include "vm/uninit.h"

static bool uninit_initialize (struct page *page, void *kva);
static void uninit_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations uninit_ops = {
	.swap_in = uninit_initialize,
	.swap_out = NULL,
	.destroy = uninit_destroy,
	.type = VM_UNINIT,
};

/* DO NOT MODIFY this function */
// 새로운 uninitialized page에 필요한 data를 초기화해준다.
void
uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *)) {
	ASSERT (page != NULL);

	*page = (struct page) {
		.operations = &uninit_ops,
		.va = va,
		.frame = NULL, /* no frame for now */
		.uninit = (struct uninit_page) {
			.init = init, // page fault가 났을 시, data를 load하는 방식에 대한 함수를 의미한다.(e.g. lazy_load_segment)
			.type = type, // page의 종류를 의미한다.(e.g. )
			.aux = aux,   // Auxiliary data로 file-backed page의 경우 file과 관련된 정보를 저장한다.
			.page_initializer = initializer,//page의 종류에 맞는 meta-data를 초기화한다.
		}
	};
}

/* Initalize the page on first fault */
// 첫번째 page fault 시, page를 어떻게 초기화할지 설정한다.
static bool
uninit_initialize (struct page *page, void *kva) {
	struct uninit_page *uninit = &page->uninit; // uninit 구조체 내 data를 참조한다.

	/* Fetch first, page_initialize may overwrite the values */
	//vm_initializer는 page fault가 났을 시, page를 어떤 방식으로 load 할 것인지를 정의한 구조체이다. 
	vm_initializer *init = uninit->init;	// page를 어떤 방식으로 frame에 적재할지에 대한 정보를 담는다.(e.g. lazy_load_segment)
	void *aux = uninit->aux;

	/* TODO: You may need to fix this function. */
	// Virtual address 와 Physical address를 대응시켜주고, page를 frame에 적재하는 과정을 거친다.
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
