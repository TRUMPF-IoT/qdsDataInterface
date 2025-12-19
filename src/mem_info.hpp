#include <stdio.h>

#ifdef __linux__
#include <malloc.h>
#endif

void print_heap_stats() {
#ifdef __linux__
    struct mallinfo2 mi = mallinfo2();
    printf("=== Memory Stats ===\n");
    printf("Heap Arena Size:    %zu bytes (%.2f MB)\n", (size_t)mi.arena, (double)mi.arena / (1024.0 * 1024.0));
    printf("Total Allocated:    %zu bytes (%.2f MB)\n", (size_t)mi.uordblks, (double)mi.uordblks / (1024.0 * 1024.0));
    printf("Total Free:         %zu bytes (%.2f MB)\n", (size_t)mi.fordblks, (double)mi.fordblks / (1024.0 * 1024.0));
    printf("Used Percentage:    %.1f%%\n", (double)mi.uordblks / (double)mi.arena * 100.0);
    printf("====================\n");
#elif defined(_WIN32) || defined(_WIN64)
    // Windows: Memory statistics via _mallinfo is deprecated and less detailed
    // Using GetProcessMemoryInfo would require linking psapi.lib
    printf("=== Memory Stats ===\n");
    printf("Memory statistics not available on Windows (requires platform-specific implementation)\n");
    printf("====================\n");
#else
    printf("=== Memory Stats ===\n");
    printf("Memory statistics not available on this platform\n");
    printf("====================\n");
#endif
}