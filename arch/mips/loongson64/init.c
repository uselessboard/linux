// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2009 Lemote Inc.
 * Author: Wu Zhangjin, wuzhangjin@gmail.com
 */

#include <linux/irqchip.h>
#include <linux/logic_pio.h>
#include <linux/memblock.h>
#include <asm/bootinfo.h>
#include <asm/traps.h>
#include <asm/smp-ops.h>
#include <asm/cacheflush.h>
#include <asm/fw/fw.h>

#include <loongson.h>

static void __init mips_nmi_setup(void)
{
	void *base;
	extern char except_vec_nmi;

	base = (void *)(CAC_BASE + 0x380);
	memcpy(base, &except_vec_nmi, 0x80);
	flush_icache_range((unsigned long)base, (unsigned long)base + 0x80);
}

void __init prom_init(void)
{
	fw_init_cmdline();
	prom_init_env();

	prom_init_numa_memory();

	/* Hardcode to CPU UART 0 */
	if ((read_c0_prid() & PRID_IMP_MASK) == PRID_IMP_LOONGSON_64R)
		setup_8250_early_printk_port(TO_UNCAC(0x1fe00000), 0, 1024);
	else
		setup_8250_early_printk_port(TO_UNCAC(LOONGSON_REG_BASE + 0x1e0), 0, 1024);

	register_smp_ops(&loongson3_smp_ops);
	board_nmi_handler_setup = mips_nmi_setup;
}

void __init prom_free_prom_memory(void)
{
}

static __init void reserve_pio_range(void)
{
	struct logic_pio_hwaddr *range;

	range = kzalloc(sizeof(*range), GFP_ATOMIC);
	if (!range)
		return;

	range->fwnode = &of_root->fwnode;
	range->size = MMIO_LOWER_RESERVED;
	range->hw_start = LOONGSON_PCIIO_BASE;
	range->flags = LOGIC_PIO_CPU_MMIO;

	if (logic_pio_register_range(range)) {
		pr_err("Failed to reserve PIO range for legacy ISA\n");
		kfree(range);
		return;
	}

	/*
	 * i8259 would access I/O space, so mapping must be done here.
	 * Please remove it when all drivers can be managed by logic_pio.
	 */
	ioremap_page_range(PCI_IO_START, PCI_IO_START + MMIO_LOWER_RESERVED,
				  LOONGSON_PCIIO_BASE,
				  pgprot_device(PAGE_KERNEL));
}

void __init arch_init_irq(void)
{
	reserve_pio_range();
	irqchip_init();
}
