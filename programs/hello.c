// Simple test program that writes to a known memory location
// We'll write to 0x800000 (start of our frame allocator region) as a marker
void _start() {
    volatile int counter = 0;
    volatile int *marker = (volatile int*)0x800000;  // Known memory location
    
    // Initialize marker
    *marker = 0xDEADBEEF;  // Magic number to show we started
    
    for (;;) {
        counter++;
        
        // Update marker periodically to show we're running
        if (counter >= 500000) {
            (*marker)++;
            counter = 0;
            
            // Reset marker to prevent overflow
            if (*marker >= 0xDEADBEEF + 1000) {
                *marker = 0xDEADBEEF;
            }
        }
    }
}