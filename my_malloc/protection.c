#include "my_malloc/protection.h"

#include <signal.h>
#include <sys/mman.h>

#include "my_malloc/log.h"
#include "my_malloc/mymalloc.h"

static int restore_signal(int signum) {
  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = SIG_DFL;
  return sigaction(signum, &sa, NULL);
}

static void gthread_segv_trap(int signum, siginfo_t* info, void* uctx) {
#ifdef __APPLE__
  assert(signum == SIGSEGV || signum == SIGBUS);
#else
  assert(signum == SIGSEGV);
#endif

  gthread_task_t* current = gthread_task_current();
  Node* page = whichPage(info->si_addr);
  if (branch_unexpected(page == NULL)) {
    fprintf(stderr, "segmentation fault by task %p at address %p\n", current,
            info->si_addr);
    fflush(stderr);
    // attempt to let the original signal to crash the process
    if (restore_signal(signum)) {
      abort();
    } else {
      return;
    }
  }
  if (branch_unexpected(page->thread != current)) {
    fprintf(stderr,
            "segmentation fault by task %p accessing memory owned by task %p "
            "at address %p\n",
            current, page->thread, info->si_addr);
    fflush(stderr);
    // attempt to let the original signal to crash the process
    if (restore_signal(signum)) {
      abort();
    } else {
      return;
    }
  }

  // check size of page is a multiple of `page_size`
  assert(((uintptr_t)page->page_end_addr - (uintptr_t)page->page_start_addr) %
             page_size ==
         0);
  // check page start address is aligned to `page_size` boundary
  assert((uintptr_t)page->page_start_addr % page_size == 0);

  gthread_malloc_unprotect_region(
      page->page_start_addr,
      (uintptr_t)page->page_end_addr - (uintptr_t)page->page_start_addr);
}

void gthread_malloc_protection_init() {
  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  // block all signals (delaying timers)
  sigfillset(&sa.sa_mask);
  sa.sa_sigaction = gthread_segv_trap;
  if (sigaction(SIGSEGV, &sa, NULL) != 0) {
    fprintf(stderr, "warning: memory protection will not work as expected\n");
  }
#ifdef __APPLE__
  if (sigaction(SIGBUS, &sa, NULL) != 0) {
    fprintf(stderr, "warning: memory protection will not work as expected\n");
  }
#endif
}

void gthread_malloc_protect_region(void* region, size_t size) {
  mprotect(region, size, PROT_NONE);
}

void gthread_malloc_unprotect_region(void* region, size_t size) {
  mprotect(region, size, PROT_READ | PROT_WRITE);
}
