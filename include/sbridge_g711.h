#define SBRIDGE_ALAW_AMI_MASK 0x55

static __inline__ int sbridge_top_bit(unsigned int bits)
{
    int res;
    
#if defined(__i386__)  ||  defined(__x86_64__)
    __asm__ (" xorl %[res],%[res];\n"
             " decl %[res];\n"
             " bsrl %[bits],%[res]\n"
             : [res] "=&r" (res)
             : [bits] "rm" (bits));
    return res;
#elif defined(__ppc__)  ||   defined(__powerpc__)
    __asm__ ("cntlzw %[res],%[bits];\n"
             : [res] "=&r" (res)
             : [bits] "r" (bits));
    return 31 - res;
#else
    if (bits == 0)
        return -1;
    res = 0;
    if (bits & 0xFFFF0000)
    {
        bits &= 0xFFFF0000;
        res += 16;
    }
    if (bits & 0xFF00FF00)
    {
        bits &= 0xFF00FF00;
        res += 8;
    }
    if (bits & 0xF0F0F0F0)
    {
        bits &= 0xF0F0F0F0;
        res += 4;
    }
    if (bits & 0xCCCCCCCC)
    {
        bits &= 0xCCCCCCCC;
        res += 2;
    }
    if (bits & 0xAAAAAAAA)
    {
        bits &= 0xAAAAAAAA;
        res += 1;
    }
    return res;
#endif
}

static __inline__ int16_t sbridge_alaw_to_linear(uint8_t alaw)
{
    int i;
    int seg;

    alaw ^= SBRIDGE_ALAW_AMI_MASK;
    i = ((alaw & 0x0F) << 4);
    seg = (((int) alaw & 0x70) >> 4);
    if (seg)
        i = (i + 0x108) << (seg - 1);
    else
        i += 8;
    return (int16_t) ((alaw & 0x80)  ?  i  :  -i);
}

static __inline__ uint8_t sbridge_linear_to_alaw(int linear)
{
    int mask;
    int seg;
    
    if (linear >= 0)
    {
        /* Sign (bit 7) bit = 1 */
        mask = SBRIDGE_ALAW_AMI_MASK | 0x80;
    }
    else
    {
        /* Sign (bit 7) bit = 0 */
        mask = SBRIDGE_ALAW_AMI_MASK;
        linear = -linear - 1;
    }

    /* Convert the scaled magnitude to segment number. */
    seg = sbridge_top_bit(linear | 0xFF) - 7;
    if (seg >= 8)
    {
        if (linear >= 0)
        {
            /* Out of range. Return maximum value. */
            return (uint8_t) (0x7F ^ mask);
        }
        /* We must be just a tiny step below zero */
        return (uint8_t) (0x00 ^ mask);
    }
    /* Combine the sign, segment, and quantization bits. */
    return (uint8_t) (((seg << 4) | ((linear >> ((seg)  ?  (seg + 3)  :  4)) & 0x0F)) ^ mask);
}

