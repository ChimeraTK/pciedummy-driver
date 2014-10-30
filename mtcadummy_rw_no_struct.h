#ifndef MTCA_DUMMY_RW_NO_STRUCT_H
#define MTCA_DUMMY_RW_NO_STRUCT_H

#include <linux/kernel.h>
#include <linux/fs.h>

ssize_t mtcaDummy_write_no_struct(struct file *filp, const char __user *buf, 
				  size_t count, loff_t *f_pos);


#endif /* MTCA_DUMMY_RW_NO_STRUCT_H */
