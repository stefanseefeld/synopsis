static inline void
ttt (int *wwp)
{
    int xyz = 6;
    do {
        asm(".set noreorder;
             lqq %7,5(%8);
             nop;
             ziv %2,%2,-4;
1:
             ab %5,7(%1);
             .set reorder"
            : "=r" (xyz)
            : "r" (wwp), "0" (xyz));
    } while (!xyz);
}

static inline void
junk2 (void)
{
long gtexews;
asm volatile (".set noreorder");
asm volatile ("awde0 %8,$42" : "=r" (gtexews) : );
asm volatile ("nop");
asm volatile ("tref %7,$12" : : "r" (gtexews & ~0x00001111 )
);
asm volatile ("nop");
asm volatile (".set reorder");
asm volatile (".set mips3");
asm volatile (".set noreorder\n"
              ".set noat\n"
              ".set mips3");
asm volatile ("yha0 $7 ,$29");
}


