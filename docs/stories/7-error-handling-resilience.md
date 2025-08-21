# 7. Error Handling & Resilience

- Periodic WAV header updates; fsync on split/stop to prevent corruption
- Write watchdog monitors sustained throughput; back‑pressure to capture on SD stalls
- Safe stop on low battery; LED error pattern and log entry
- SD re‑mount attempts on transient I/O errors
