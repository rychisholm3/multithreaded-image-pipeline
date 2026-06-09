# Multithreaded Image Pipeline

A image-processing pipeline written in C that distributes work across multiple threads running in parallel.

---

## What it does

Generates synthetic test images (gradients, noise, circles, checkerboards) and runs each one through a chain of filters — things like grayscale conversion, Gaussian blur, and Sobel edge detection. Instead of processing images one at a time, it hands them out to a pool of worker threads so multiple images get processed simultaneously.

**Example output on 4 threads:**
```
Total worker time : 173 ms
Wall-clock time   :  64 ms
Parallelism gain  : 2.72x
```
The same work that would take 173 ms single-threaded finishes in 64 ms because 4 threads run at once.

---

## Key concepts demonstrated

**Thread Pool**
A fixed set of worker threads created once at startup. They sleep when there's nothing to do and wake up the moment a new job arrives. This is more efficient than spawning a new thread for every task.

**Mutex (Mutual Exclusion Lock)**
When multiple threads try to update shared data (like a job counter) at the same time, a mutex ensures only one thread writes at a time — preventing corrupted results.

**Condition Variables**
Used alongside mutexes so sleeping threads are woken up *only* when there's actual work waiting, rather than checking in a loop and wasting CPU.

**Race Condition Prevention**
Without synchronization, two threads reading and writing the same memory simultaneously produces unpredictable bugs. Every shared resource in this project is protected so results are always correct regardless of thread timing.

---

## Filters implemented

| Filter | What it does |
|---|---|
| Grayscale | Converts RGB to a single brightness value using human eye weighting |
| Gaussian Blur | Smooths an image by averaging neighboring pixels with a bell-curve weight |
| Sobel Edge Detection | Highlights edges by detecting sharp changes in brightness |
| Brightness | Shifts all pixel values up or down |
| Invert | Flips every pixel to its opposite color |

---

## Build & Run

Requires [MSYS2](https://www.msys2.org/) with `mingw-w64-ucrt-x86_64-gcc`.

```bash
make run          # build and run with 4 threads
./build/pipeline.exe 1   # single-threaded (compare the time!)
./build/pipeline.exe 8   # 8 threads
```

Output images are written to the `output/` folder as `.ppm` files (viewable in most image viewers).

---

## Project structure

```
src/
├── main.c          — entry point; defines the 10 pipeline jobs
├── image.h/c       — PPM image format I/O and synthetic image generators
├── filters.h/c     — all image processing filters
├── thread_pool.h/c — worker thread pool with mutex-protected job queue
└── pipeline.h/c    — job dispatcher and shared statistics tracking
```

---

**Language:** C (C11) · **Concurrency:** POSIX Threads (pthreads) · **Topic:** Operating Systems / Systems Programming
