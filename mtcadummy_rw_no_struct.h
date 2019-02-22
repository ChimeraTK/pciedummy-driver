#ifndef MTCA_DUMMY_RW_NO_STRUCT_H
#define MTCA_DUMMY_RW_NO_STRUCT_H

#include <linux/fs.h>
#include <linux/kernel.h>

ssize_t mtcaDummy_read_no_struct(struct file* filp, char __user* buf, size_t count, loff_t* f_pos);

ssize_t mtcaDummy_write_no_struct(struct file* filp, const char __user* buf, size_t count, loff_t* f_pos);

#endif /* MTCA_DUMMY_RW_NO_STRUCT_H */
