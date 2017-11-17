#include "my_malloc/protection.h"

#include <signal.h>

#include "my_malloc/mymalloc.h"

static void gthread_segv_trap(int signum, siginfo_t* info, ucontext_t* uap) {
  assert(signum == SIGSEGV);

  gthread_task_t* current = gthread_task_current();
  Node* page = whichPage(info->si_addr);
  if (branch_unexpected(page == NULL)) {
    fprintf(stderr, "segmentation fault by task %p at address %p\n", current,
            info->si_addr);
    fflush(stderr);
    abort();
  }
  if (branch_unexpected(page->thread != current)) {
    fprintf(stderr,
            "segmentation fault by task %p accessing memory owned by task %p "
            "at address %p\n",
            current, page->thread, info->si_addr);
    fflush(stderr);
    abort();
  }

  // check size of page is a multiple of `page_size`
  assert(((uintptr_t)page->page_end_addr - (uintptr_t)page->page_start_addr) %
             page_size ==
         0);
  // check page start address is aligned to `page_size` boundary
  assert((uintptr_t)page->page_start_addr % page_size == 0);

  mprotect(page->page_start_addr,
           (uintptr_t)page->page_end_addr - (uintptr_t)page->page_start_addr,
           PROT_READ | PROT_WRITE);
}

void gthread_malloc_protection_init() {
  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;
  if (!sigaction(SIGSEGV, &sa, NULL)) {
    fprintf(stderr, "warning: memory protection will not work as expected\n");
  }
}
