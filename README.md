# Multithreaded Image Pipeline

A C program that processes images in parallel using a thread pool. Instead of handling images one at a time, it distributes jobs across multiple worker threads so they run simultaneously.

## How it works

At startup, a fixed pool of worker threads is created. Each thread sleeps until a job is available, processes it, then picks up the next one. A mutex (lock) protects the job queue and shared stats so threads never interfere with each other.

Running 10 image jobs across 4 threads:
```
Total worker time : 173 ms
Wall-clock time   :  64 ms
Parallelism gain  : 2.72x
```

## Filters

- **Grayscale** — converts RGB to brightness
- **Gaussian Blur** — smooths the image by averaging nearby pixels
- **Sobel Edge Detection** — highlights edges by finding sharp brightness changes
- **Brightness** — shifts pixel values up or down
- **Invert** — flips every pixel to its opposite color

## Build & Run

Requires [MSYS2](https://www.msys2.org/) with `mingw-w64-ucrt-x86_64-gcc`.

```bash
make run                   # 4 threads (default)
./build/pipeline.exe 1     # single-threaded
./build/pipeline.exe 8     # 8 threads
```

Output images are written to `output/` as `.ppm` files.

## Stack

C (C11) · POSIX Threads · Mutex Synchronization
