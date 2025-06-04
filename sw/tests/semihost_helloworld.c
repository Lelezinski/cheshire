#include "printf.h"
#include "semihost.h"
#include "util.h"

int main()
{
    semihost_printf("Hello World from Cheshire!\n");
    
    while (1)
    {
        wfi();
    }
    return 0;
}
