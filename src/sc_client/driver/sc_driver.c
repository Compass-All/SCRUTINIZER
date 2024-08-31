#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cma.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/arm-smccc.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/highmem.h>
#include <asm/pgtable.h>
#include <asm/pgtable-hwdef.h>
#include <linux/kallsyms.h>
#include <linux/dma-map-ops.h>

# define  U_(_x)    (_x##U)
# define   U(_x)    U_(_x)
// # define  UL(_x)    (_x##UL)
// # define ULL(_x)    (_x##ULL)
# define   L(_x)    (_x##L)
# define  LL(_x)    (_x##LL)

#define DEV_NAME		"SC"
#define SC_AUTH_TEST _IOW('m', 1, unsigned int)
#define SC_AUTH   U(0x80000FF0)
#define SC_REGISTER	_IOW('m', 2, unsigned int)
#define SC_REGISTER_INFO   U(0x80000FFA)
#define SC_MEMORY_DUMP	_IOW('m', 3, unsigned int)
#define SC_INTROSPECTION U(0x80000FF6)
#define SC_PA_INTROSPECTION U(0x80000FF8)
#define SC_FULL_MEMORY_DUMP	_IOW('m', 4, unsigned int)
#define SC_FULL_INTROSPECTION U(0x80000FF8)
#define SC_REGISTER_SAVE	_IOW('m', 5, unsigned int)
#define SC_ETE_ON	_IOW('m', 6, unsigned int)
#define SC_ETE_TRACE    U(0x80000FF3)
#define SC_ETE_OFF	_IOW('m', 7, unsigned int)
#define SC_DISABLE_TRACE    U(0x80000FF4)
#define SC_SET_WATCHPOINT	_IOW('m', 8, unsigned int)
#define SC_WATCHPOINT    U(0x80000FF1)
#define SC_SET_BREAKPOINT	_IOW('m', 9, unsigned int)
#define SC_BREAKPOINT    U(0x80000FF2)




/*  manager information */
struct SC_manager {
	struct miscdevice misc;
	struct mutex lock;
	struct list_head head;
};

/* memory device */
static struct SC_manager *manager;

typedef unsigned long u_register_t;
typedef struct register_info {
   u_register_t x0;
   u_register_t x1;
   u_register_t x2;
   u_register_t x3;
   u_register_t x4;
   u_register_t x5;
   u_register_t x6;
   u_register_t x7;
   u_register_t x8;
   u_register_t x9;
   u_register_t x10;
   u_register_t x11;
   u_register_t x12;
   u_register_t x13;
   u_register_t x14;
   u_register_t x15;
   u_register_t x16;
   u_register_t x17;
   u_register_t x18;
   u_register_t x19;
   u_register_t x20;
   u_register_t x21;
   u_register_t x22;
   u_register_t x23;
   u_register_t x24;
   u_register_t x25;
   u_register_t x26;
   u_register_t x27;
   u_register_t x28;
   u_register_t x29;
   u_register_t x30;
   u_register_t scr_el3;
   u_register_t elr_el3;
   u_register_t esr_el3;
   u_register_t far_el3;
   u_register_t sctlr_el3;
   u_register_t ttbr0_el3;
   u_register_t gpccr_el3;
   u_register_t gptbr_el3;
   u_register_t sctlr_el1;
   u_register_t sp_el1;
   u_register_t esr_el1; 
   u_register_t ttbr0_el1;
   u_register_t ttbr1_el1;
   u_register_t vbar_el1; 
   u_register_t spsr_el1;
   u_register_t hcr_el2;   
   u_register_t sctlr_el2;
   u_register_t sp_el2;
   u_register_t esr_el2;
   u_register_t elr_el2;
   u_register_t ttbr0_el2;
   u_register_t vsttbr_el2;
   u_register_t vttbr_el2;
   u_register_t vbar_el2;
   u_register_t spsr_el2;
} sc_reg_info_t;

#define REGISTER_INFO_LENGTH 0x1000
#define TRBE_BASE_ADDRESS 0xF8000000
#define TRBE_LIMIT_ADDRESS 0xFA000000
#define TRBE_SIZE 0x2000000

typedef struct memory_acquisition_info {
    unsigned long addr;
    unsigned long size;
    unsigned long el;
    unsigned long watchpoint_addr;
    unsigned long breakpoint_addr;
    int pa_access;
    unsigned long reg_share_buf;
} sc_mem_info_t;

typedef struct instruction_info {
    unsigned long start_addr;
    unsigned long end_addr;
    unsigned long el;
    unsigned long enable_inst_trace;
    unsigned long ins_share_buf_phys;
} sc_ins_info_t;

//shared buff
unsigned int pool_size_order;
unsigned long nr_pages;
struct page *page;
sc_reg_info_t *reg_info_buf = NULL;
unsigned long s_reg_virt = 0;
unsigned long s_reg_phys = 0;

sc_ins_info_t ins_info_buf;
unsigned long s_ins_virt = 0;
unsigned long s_ins_phys = 0;

sc_mem_info_t mem_info;

static long SC_ioctl(struct file *filp, unsigned int cmd, 
							unsigned long arg)
{
    int rvl;
	struct arm_smccc_res smccc_res;
    switch (cmd) {
	case SC_REGISTER:
		mutex_lock(&manager->lock);
		printk(KERN_INFO "SC_REGISTER.\n");
		memset((void*)s_reg_virt, 0, REGISTER_INFO_LENGTH);        
		arm_smccc_smc(SC_REGISTER_INFO, s_reg_phys, REGISTER_INFO_LENGTH, 0, 0, 0, 0, 0, &smccc_res);
		asm volatile("isb");
	
		if (copy_to_user((void __user *)arg, reg_info_buf, sizeof(sc_reg_info_t))) {
			printk(KERN_ERR "SC_REGISTER: copy_to_user error\n");
			rvl = -EINVAL;
			goto err_to;
		}
		mutex_unlock(&manager->lock);
        return 0;
	case SC_REGISTER_SAVE:
		mutex_lock(&manager->lock);
		printk(KERN_INFO "SC_REGISTER_SAVE.\n");
		if(reg_info_buf)
		{
			if (copy_to_user((void __user *)arg, reg_info_buf, sizeof(sc_reg_info_t))) {
				printk(KERN_ERR "SC_REGISTER_SAVE: copy_to_user error\n");
				rvl = -EINVAL;
				goto err_to;
			}
		}
		mutex_unlock(&manager->lock);
        return 0;

	case SC_AUTH_TEST:
		mutex_lock(&manager->lock);
		unsigned long key;
		if (copy_from_user(&key, (void __user *)arg,
					sizeof(unsigned long))) {
			printk(KERN_ERR "SC_REGISTER: copy_from_user error\n");
			rvl = -EFAULT;
			goto err_user;
		}
		arm_smccc_smc(SC_AUTH, key, 0, 0, 0, 0, 0, 0, &smccc_res);
		asm volatile("isb");
		mutex_unlock(&manager->lock);
		return 0;

	case SC_MEMORY_DUMP:
		mutex_lock(&manager->lock);
		printk(KERN_INFO "SC_MEMORY_DUMP.\n");
	
		if (copy_from_user(&mem_info, (void __user *)arg,
				sizeof(sc_mem_info_t))) {
			printk(KERN_ERR "SC_MEMORY_DUMP: copy_from_user error\n");
			rvl = -EFAULT;
			goto err_user;
		}
		if(!mem_info.pa_access)
		{
			arm_smccc_smc(SC_INTROSPECTION, mem_info.addr, mem_info.size, mem_info.el, 0, 0, 0, 0, &smccc_res);
			asm volatile("isb");
		}
		else
		{
			arm_smccc_smc(SC_PA_INTROSPECTION, mem_info.addr, mem_info.size, 0, 0, 0, 0, 0, &smccc_res);
			asm volatile("isb");
		}
		mutex_unlock(&manager->lock);
		return 0;

	case SC_SET_WATCHPOINT:
		mutex_lock(&manager->lock);
		printk(KERN_INFO "SC_SET_WATCHPOINT.\n");
		if (copy_from_user(&mem_info, (void __user *)arg,
				sizeof(sc_mem_info_t))) {
			printk(KERN_ERR "SC_SET_WATCHPOINT: copy_from_user error\n");
			rvl = -EFAULT;
			goto err_user;
		}
		mem_info.reg_share_buf = s_reg_phys;
		arm_smccc_smc(SC_WATCHPOINT, mem_info.addr, mem_info.size, mem_info.el, mem_info.watchpoint_addr, mem_info.reg_share_buf, mem_info.pa_access, 0, &smccc_res);
		asm volatile("isb");

		mutex_unlock(&manager->lock);
		return 0;

	case SC_SET_BREAKPOINT:
		mutex_lock(&manager->lock);
		printk(KERN_INFO "SC_SET_BREAKPOINT.\n");
		if (copy_from_user(&mem_info, (void __user *)arg,
				sizeof(sc_mem_info_t))) {
			printk(KERN_ERR "SC_SET_BREAKPOINT: copy_from_user error\n");
			rvl = -EFAULT;
			goto err_user;
		}
		mem_info.reg_share_buf = s_reg_phys;
		arm_smccc_smc(SC_BREAKPOINT, mem_info.addr, mem_info.size, mem_info.el, mem_info.breakpoint_addr, mem_info.reg_share_buf, mem_info.pa_access, 0, &smccc_res);
		asm volatile("isb");

		mutex_unlock(&manager->lock);
		return 0;


	case SC_ETE_ON:
		mutex_lock(&manager->lock);
		printk(KERN_INFO "SC_ETE_ON.\n");
		memset(&ins_info_buf, 0, sizeof(sc_ins_info_t));
		if (copy_from_user(&ins_info_buf, (void __user *)arg,
				sizeof(sc_ins_info_t))) {
			printk(KERN_ERR "SC_ETE_ON: copy_from_user error\n");
			rvl = -EFAULT;
			goto err_user;
		}
		arm_smccc_smc(SC_ETE_TRACE, ins_info_buf.start_addr, ins_info_buf.end_addr, ins_info_buf.el, s_ins_phys, 0, 0, 0, &smccc_res);
		asm volatile("isb");
		mutex_unlock(&manager->lock);
		return 0;

	case SC_ETE_OFF:
		mutex_lock(&manager->lock);
		printk(KERN_INFO "SC_ETE_OFF.\n");
		if(s_ins_phys)
		{
			arm_smccc_smc(SC_DISABLE_TRACE, 0, 0, 0, 0, 0, 0, 0, &smccc_res);
			asm volatile("isb");
			ins_info_buf.ins_share_buf_phys = s_ins_phys;
			if (copy_to_user((void __user *)arg, &ins_info_buf, sizeof(sc_ins_info_t))) {
				printk(KERN_ERR "SC_ETE_OFF: copy_to_user error\n");
				rvl = -EINVAL;
				goto err_to;
			}
		}

		mutex_unlock(&manager->lock);
		return 0;

    default:
		printk(KERN_INFO "SC not support command.\n");
		return -EFAULT;
    }

err_to:
	// dma_release_from_contiguous(NULL, page, nr_pages);

err_user:
	mutex_unlock(&manager->lock);
	return rvl;
}


static int SC_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long base_phsy_offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long phsy_page;
	int ret;

	phsy_page = base_phsy_offset >> PAGE_SHIFT;
	vma->vm_pgoff = phsy_page;
	vma->vm_flags &= ~VM_IO;
	vma->vm_flags |= (VM_DONTEXPAND|VM_DONTDUMP|VM_READ|VM_WRITE|VM_SHARED);

	/* Remap */
	if (remap_pfn_range(vma, start, phsy_page, size, vma->vm_page_prot)) {
		printk("REMAP: failed\n");
		return -EAGAIN;
	}	

	return 0;
}


/* file operations */
static struct file_operations SC_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= SC_ioctl,
	.mmap		= SC_mmap,
};


static int __init SC_init(void)
{	
	int rvl;

	/* Initialize device */
	manager = kzalloc(sizeof(struct SC_manager), GFP_KERNEL);
	if (!manager) {
		printk(KERN_ERR "Allocate memory failed\n");
		rvl = -ENOMEM;
		goto err_alloc;
	}

	/* Lock: initialize */
	mutex_init(&manager->lock);
	/* Misc: initialize */
	manager->misc.name  = DEV_NAME;
	manager->misc.minor = MISC_DYNAMIC_MINOR;
	manager->misc.fops  = &SC_fops;

	/* list: initialize */
	INIT_LIST_HEAD(&manager->head);


	/* Register Misc device */
	misc_register(&manager->misc);

	if(!reg_info_buf)
	{
		nr_pages = REGISTER_INFO_LENGTH >> PAGE_SHIFT;
		pool_size_order = get_order(REGISTER_INFO_LENGTH);
		page = dma_alloc_from_contiguous(NULL, nr_pages,
				pool_size_order, GFP_KERNEL);
		if (!page) {
			printk(KERN_ERR "SC_REGISTER: DMA allocate error\n");
			rvl = -ENOMEM;
			goto err_alloc;
		}
		s_reg_virt = (dma_addr_t)page_to_virt(page);
		s_reg_phys = (dma_addr_t)page_to_phys(page);
		reg_info_buf = (sc_reg_info_t *)s_reg_virt;
	}

	if(!s_ins_phys)
	{
		nr_pages = TRBE_SIZE >> PAGE_SHIFT;
		pool_size_order = get_order(TRBE_SIZE);
		page = dma_alloc_from_contiguous(NULL, nr_pages,
				pool_size_order, GFP_KERNEL);
		if (!page) {
			printk(KERN_ERR "SC_ETE_ON: DMA allocate error\n");
			rvl = -ENOMEM;
			goto err_alloc;
		}
		s_ins_virt = (dma_addr_t)page_to_virt(page);
		s_ins_phys = (dma_addr_t)page_to_phys(page);
	}
	printk(KERN_INFO "SC driver load.\n");

	return 0;

err_alloc:
	return rvl;
}

/* Module exit entry */
static void __exit SC_exit(void)
{

	misc_deregister(&manager->misc);
	kfree(manager);
	manager = NULL;

	if(s_reg_phys)
	{
		page = phys_to_page(s_reg_phys);
		nr_pages = REGISTER_INFO_LENGTH >> PAGE_SHIFT;
		dma_release_from_contiguous(NULL, page, nr_pages);
		reg_info_buf = NULL;
	}

	if(s_ins_phys)
	{
		page = phys_to_page(s_ins_phys);
		nr_pages = TRBE_SIZE >> PAGE_SHIFT;
		dma_release_from_contiguous(NULL, page, nr_pages);
	}
	printk(KERN_ERR "SC driver unload.\n");
}

module_init(SC_init);
module_exit(SC_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zym");
MODULE_DESCRIPTION("SC Driver");
