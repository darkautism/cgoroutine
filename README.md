# Cgoroutine

Cgoroutine is an concurrency library like goroutine, it's goal is support an lightweight thread and developer friendly api.

**This project still experimental project, all of these api may be change in the feature.**

## Why cgoroutine?

In linux, thread is use clone an new process. So we can not use and delete an thread too quickly like goroutine.

In general case, we always use epoll to handle these. Epoll performance is best, but we have take a lot of times to develope/tune epoll's schedule.

Cgoroutine will not give you true performance, but we are trying to find the balance with performance and develope speed.


## How to build

In linux, just type makein shell.

In windows, you may need install cygwin or mingw.

## Feature

- [x] yield
- [x] next
- [ ] lock
- [ ] channel (golang)
- [ ] Time slice (singal)
 
## Examples

- [yield](yield_and_next.c)