// --- Start of Android platform fix --

/* Despite the fact that our kernel headers define sigset_t explicitly
 * as a 32-bit integer, the kernel system call really expects a 64-bit
 * bitmap for the signal set, or more exactly an array of two-32-bit
 * values (see $KERNEL/arch/$ARCH/include/asm/signal.h for details).
 *
 * Unfortunately, we cannot fix the sigset_t definition without breaking
 * the C library ABI, so perform a little runtime translation here.
 */
typedef union {  
    sigset_t   bionic;
    uint32_t   kernel[2];
} kernel_sigset_t;

int pthread_sigmask_android16(int how, const sigset_t *set, sigset_t *oset)  
{
    int ret, old_errno = errno;

    /* We must convert *set into a kernel_sigset_t */
    kernel_sigset_t  in_set, *in_set_ptr;
    kernel_sigset_t  out_set;

    in_set.kernel[0]  = in_set.kernel[1]  =  0;
    out_set.kernel[0] = out_set.kernel[1] = 0;

    /* 'in_set_ptr' is the second parameter to __rt_sigprocmask. It must be NULL
     * if 'set' is NULL to ensure correct semantics (which in this case would
     * be to ignore 'how' and return the current signal set into 'oset'.
      */

    if (set == NULL) {
        in_set_ptr = NULL;
    } else {
        in_set.bionic = *set;
        in_set_ptr = &in_set;
    }

    ret = sigprocmask(how, (const sigset_t *)in_set_ptr, (const sigset_t *)&out_set);
     if (ret < 0)
         ret = errno;

    if (oset)
        *oset = out_set.bionic;

     errno = old_errno;
     return ret;
}

// --- End of Android platform fix --

// first try to call pthread_sigmask, in case of failure try again with the API 16 fix
int pthread_sigmask_patched(int how, const sigset_t *set, sigset_t *oset) {  
  int ret = pthread_sigmask(how, set, oset);
  if (ret == EINVAL) {
    return pthread_sigmask_android16(how, set, oset);
  }
}
