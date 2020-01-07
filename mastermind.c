#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <asm/switch_to.h>		/* cli(), *_flags */
#include <linux/uaccess.h>	/* copy_*_user */

#include "mastermind_ioctl.h"

#define MASTERMIND_MAJOR 0
#define MASTERMIND_NR_DEVS 4
#define MASTERMIND_QUANTUM 4000
#define MASTERMIND_QSET 1000
#define MMIND_MAX_GUESSES 10

static int mastermind_major = MASTERMIND_MAJOR;
int mastermind_minor = 0;
int mastermind_nr_devs = MASTERMIND_NR_DEVS;
int mastermind_quantum = MASTERMIND_QUANTUM;
int mastermind_qset = MASTERMIND_QSET;
static char *mmind_number = "simpdev";
static int mmind_max_guesses = MMIND_MAX_GUESSES;

module_param(mastermind_major, int, S_IRUGO);
module_param(mastermind_minor, int, S_IRUGO);
module_param(mastermind_nr_devs, int, S_IRUGO);
module_param(mastermind_quantum, int, S_IRUGO);
module_param(mastermind_qset, int, S_IRUGO);
module_param(mmind_number,charp,0660);
module_param(mmind_max_guesses, int, S_IRUGO);

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

struct mastermind_dev {
    int quantum;
    int qset;
    unsigned long size;
    struct semaphore sem;
    struct cdev cdev;
    int counter;
};
	
struct mastermind_dev *mastermind_devices;

int mastermind_trim(struct mastermind_dev *dev)
{   int i;
    dev->quantum = mastermind_quantum;
    dev->qset = mastermind_qset;
    dev->size = 0;
    dev->counter = 0;
    return 0;
}


int mastermind_open(struct inode *inode, struct file *filp)
{
    struct mastermind_dev *dev;

    dev = container_of(inode->i_cdev, struct mastermind_dev, cdev);
    filp->private_data = dev;
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
        mastermind_trim(dev);
        up(&dev->sem);
    }
    return 0;
}

int mastermind_release(struct inode *inode, struct file *filp)
{
    return 0;
}

char* local_buffer;
char result_string[4096] ={'\0'}; 
int work = 1;

void delete_all(void){
	work =1;
	int i;
	for(i = 0; i<4096; i++)
	  result_string[i] ='\0'; 	
}
	
	
void calculate(char* local_buffer){
	int visited_node[4] = {0}; 
	int corr = 0;
	int incorr = 0;
	int i =0;
	int counter =0;
	int ii;
	int j;
	int ones, tens, hundreds,thousands;
	ones = work %10;
	tens = (work %100)/10;
	hundreds = (work %1000)/100;
	thousands = (work /1000);
	
	for(ii = 0; ii<4; ii++)
	{
		if(local_buffer[ii] == mmind_number[ii])
		{
			visited_node[ii] = 1;
			corr++;
		}
	}
	
	for (ii = 0; ii <4; ii++){
		for(j = 0; j < 4; j++){
			if(ii != j && visited_node[j] == 0){
				if(local_buffer[ii] == mmind_number[j])
				{
					visited_node[j] = 1;
					incorr++;
				}
			}
		}
	}
	     
	while(result_string[i]!='\0')
		i++;
	while(counter<4)
	{
		result_string[i]=local_buffer[counter];
		counter++;
		i++;
	}

	result_string[i++] = ' ';
	result_string[i++] = corr+'0';
	result_string[i++] = '+';
	result_string[i++] = ' ';
	result_string[i++] = incorr+'0';
	result_string[i++] = '-';
	result_string[i++] = ' ';
	result_string[i++] = thousands +'0';
	result_string[i++] = hundreds +'0';
	result_string[i++] = tens +'0';
	result_string[i++] = ones +'0';
	result_string[i++] = '\n';
	result_string[i++]='\0';
	work +=1;
}

ssize_t mastermind_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos)
{
    struct mastermind_dev *dev = filp->private_data;
    int quantum = dev->quantum;
    int s_pos, q_pos;
    ssize_t retval = 0;
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    if (*f_pos >= dev->size)
        goto out;
    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;

    s_pos = (long) *f_pos / quantum;
    q_pos = (long) *f_pos % quantum;

    if (count > quantum - q_pos)
        count = quantum - q_pos;	
    

	if (copy_to_user(buf, result_string, count)) {
		retval = -EFAULT;
		goto out;
	}
    *f_pos +=count+1000;
    retval = count;

  out:
    up(&dev->sem);
    return retval;
}

ssize_t mastermind_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct mastermind_dev *dev = filp->private_data;
    int quantum = dev->quantum, qset = dev->qset;
    int s_pos, q_pos;
    ssize_t retval = -ENOMEM;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    if (*f_pos >= quantum * qset) {
        retval = 0;
        goto out;
    }
    if(work > mmind_max_guesses){
		 retval = -EDQUOT;
		 goto out;
	 }

    s_pos = (long) *f_pos / quantum;
    q_pos = (long) *f_pos % quantum;

    if (count > quantum - q_pos)
        count = quantum - q_pos;
	local_buffer = kmalloc((16*work) * sizeof(char), GFP_KERNEL);
    if (copy_from_user(local_buffer, buf, count)) {
        retval = -EFAULT;
        goto out;
    }
    calculate(local_buffer);
    *f_pos += count+1000;
    retval = count;

    if (dev->size < *f_pos)
        dev->size = *f_pos;
  out:
    up(&dev->sem);
    return retval;
}

long mastermind_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int err = 0, tmp;
	int retval = 0;


	if (_IOC_TYPE(cmd) != MASTERMIND_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > MASTERMIND_IOC_MAXNR) return -ENOTTY;


	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {
	  case MASTERMIND_IOCRESET:
		mastermind_quantum = MASTERMIND_QUANTUM;
		mastermind_qset = MASTERMIND_QSET;
		break;

	  case MASTERMIND_IOCSQUANTUM: 
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		retval = __get_user(mastermind_quantum, (int __user *)arg);
		break;
		
		//implemented parts
	   case MMIND_REMAINING:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		return mmind_max_guesses - work + 1;
		break;
		
	   case MMIND_ENDGAME:
		 if (! capable (CAP_SYS_ADMIN))
				return -EPERM;
		 delete_all();
		 retval = 1;
		 break;
		
		case MMIND_NEWGAME:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		param_t parampa;
		copy_from_user(&parampa, (char __user*)arg, sizeof(param_t));
		delete_all();
		int jj;
		for(jj = 0; jj<4; jj++)
			mmind_number[jj] = parampa.buffer[jj];
		break;
	  

	  case MASTERMIND_IOCTQUANTUM:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		mastermind_quantum = arg;
		break;

	  case MASTERMIND_IOCGQUANTUM: /* Get: arg is pointer to result */
		retval = __put_user(mastermind_quantum, (int __user *)arg);
		break;
	 
	 case MASTERMIND_IOCXQUANTUM: /* eXchange: use arg as pointer */
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		tmp = mastermind_quantum;
		retval = __get_user(mastermind_quantum, (int __user *)arg);
		if (retval == 0)
			retval = __put_user(tmp, (int __user *)arg);
		break;
	
	  case MASTERMIND_IOCHQUANTUM: /* sHift: like Tell + Query */
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		tmp = mastermind_quantum;
		mastermind_quantum = arg;
		return tmp;	

	  case MASTERMIND_IOCSQSET:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		retval = __get_user(mastermind_qset, (int __user *)arg);
		break;

	  case MASTERMIND_IOCTQSET:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		mastermind_qset = arg;
		break;

	  case MASTERMIND_IOCGQSET:
		retval = __put_user(mastermind_qset, (int __user *)arg);
		break;

	  case MASTERMIND_IOCQQSET:
		return mastermind_qset;

	  default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	return retval;
}



loff_t mastermind_llseek(struct file *filp, loff_t off, int whence)
{
    struct mastermind_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;

        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;

        case 2: /* SEEK_END */
            newpos = dev->size + off;
            break;

        default: /* can't happen */
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}

struct file_operations mastermind_fops = {
    .owner =    THIS_MODULE,
    .llseek =   mastermind_llseek,
    .read =     mastermind_read,
    .write =    mastermind_write,
    .unlocked_ioctl =    mastermind_ioctl,
    .open =     mastermind_open,
    .release =  mastermind_release,
};


void mastermind_cleanup_module(void)
{
    int i;
    dev_t devno = MKDEV(mastermind_major, mastermind_minor);

    if (mastermind_devices) {
        for (i = 0; i < mastermind_nr_devs; i++) {
            mastermind_trim(mastermind_devices + i);
            cdev_del(&mastermind_devices[i].cdev);
        }
    kfree(mastermind_devices);
    }

    unregister_chrdev_region(devno, mastermind_nr_devs);
}


int mastermind_init_module(void)
{
    int result, i;
    int err;
    dev_t devno = 0;
    struct mastermind_dev *dev;
    if (mastermind_major) {
        devno = MKDEV(mastermind_major, mastermind_minor);
        result = register_chrdev_region(devno, mastermind_nr_devs, "mastermind");
    } else {
        result = alloc_chrdev_region(&devno, mastermind_minor, mastermind_nr_devs,
                                     "mastermind");
        mastermind_major = MAJOR(devno);
    }
    if (result < 0) {
        printk(KERN_WARNING "mastermind: can't get major %d\n", mastermind_major);
        return result;
    }

    mastermind_devices = kmalloc(mastermind_nr_devs * sizeof(struct mastermind_dev),
                            GFP_KERNEL);
    if (!mastermind_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(mastermind_devices, 0, mastermind_nr_devs * sizeof(struct mastermind_dev));

    /* Initialize each device. */
    for (i = 0; i < mastermind_nr_devs; i++) {
        dev = &mastermind_devices[i];
        dev->quantum = mastermind_quantum;
        dev->counter = 0;
        dev->qset = mastermind_qset;
        sema_init(&dev->sem,1);

        devno = MKDEV(mastermind_major, mastermind_minor + i);
        cdev_init(&dev->cdev, &mastermind_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &mastermind_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        if (err)
            printk(KERN_NOTICE "Error %d adding mastermind%d", err, i);
    }

    return 0; /* succeed */

  fail:
    mastermind_cleanup_module();
    return result;
}

module_init(mastermind_init_module);
module_exit(mastermind_cleanup_module);
