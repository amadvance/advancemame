
#ifndef __alpha__

#define v_readb(addr) (*(volatile uint8_t *) (MMIO_POINTER+(addr)))
#define v_readw(addr) (*(volatile uint16_t *) (MMIO_POINTER+(addr)))
#define v_readl(addr) (*(volatile uint32_t *) (MMIO_POINTER+(addr)))

#define v_writeb(b,addr) (*(volatile uint8_t *) (MMIO_POINTER+(addr)) = (b))
#define v_writew(b,addr) (*(volatile uint16_t *) (MMIO_POINTER+(addr)) = (b))
#define v_writel(b,addr) (*(volatile uint32_t *) (MMIO_POINTER+(addr)) = (b))

#else

#define vip	volatile int *
#define vuip	volatile unsigned int *
#define vulp	volatile unsigned long *

#define mb() \
__asm__ __volatile__("mb": : :"memory")

#define __kernel_extbl(val, shift)					\
  ({ unsigned long __kir;						\
     __asm__("extbl %2,%1,%0" : "=r"(__kir) : "rI"(shift), "r"(val));	\
     __kir; })
#define __kernel_extwl(val, shift)					\
  ({ unsigned long __kir;						\
     __asm__("extwl %2,%1,%0" : "=r"(__kir) : "rI"(shift), "r"(val));	\
     __kir; })

static inline uint8_t v_readb(unsigned long addr)
{
	unsigned long result;

	result = *(vip) ((addr << 5) + SPARSE_MMIO + 0x00);
	return __kernel_extbl(result, addr & 3);
}

static inline uint16_t v_readw(unsigned long addr)
{
	unsigned long result;

	result = *(vip) ((addr << 5) + SPARSE_MMIO + 0x08);
	return __kernel_extwl(result, addr & 3);
}

static inline uint32_t v_readl(unsigned long addr)
{
	return *(vuip) ((addr << 5) + SPARSE_MMIO + 0x18);
}

static inline void v_writeb(uint8_t b, unsigned long addr)
{
	*(vuip) ((addr << 5) + SPARSE_MMIO + 0x00) = b * 0x01010101;
        mb();
}

static inline void v_writew(uint16_t b, unsigned long addr)
{
	*(vuip) ((addr << 5) + SPARSE_MMIO + 0x08) = b * 0x00010001;
        mb();
}

static inline void v_writel(uint32_t b, unsigned long addr)
{
	*(vuip) ((addr << 5) + SPARSE_MMIO + 0x18) = b;
        mb();
}

#endif /* __alpha__ */
