#include <malloc.h>
#include <stdio.h>

void print_heap_stats() {
    struct mallinfo2 mi = mallinfo2();
    printf("=== Memory Stats ===\n");
    printf("Heap Arena Size:    %zu bytes (%.2f MB)\n", (size_t)mi.arena, (double)mi.arena / (1024.0 * 1024.0));
    printf("Total Allocated:    %zu bytes (%.2f MB)\n", (size_t)mi.uordblks, (double)mi.uordblks / (1024.0 * 1024.0));
    printf("Total Free:         %zu bytes (%.2f MB)\n", (size_t)mi.fordblks, (double)mi.fordblks / (1024.0 * 1024.0));
    printf("Used Percentage:    %.1f%%\n", (double)mi.uordblks / (double)mi.arena * 100.0);
    printf("====================\n");
}