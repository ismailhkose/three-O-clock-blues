#include <linux/module.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/kobject.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("gmate.amit@gmail.com");
MODULE_VERSION("1.1");
MODULE_DESCRIPTION("Sample module on usage of spinlock");
/* Create kobject and use sysfs to export those objects */

/* Name of the directory entry in debugfs file system */
#define SYSFS_NAME "sysfs_lkm_spinlock"
#define DRV_NAME "sysfs_lkm_spinlock"

/* sysfs sysfs-lkm/id file node read/write buffer */
static const char text[] = "bitprolix";
static int len = sizeof(text);

/* sysfs sysfs-lkm/foo file node read/write buffer */
static char foo_kbuff[PAGE_SIZE];

/* For foo sysfs file node read/write synchronization using spinlock */
static spinlock_t lock;

static ssize_t id_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%s\n", text);
}


static ssize_t id_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	if (memcmp(buf, text, len - 1) != 0) {
		pr_debug("Invalid value\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t jiffies_show(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf)
{
	return sprintf(buf, "current value of jiffies: %lu\n", jiffies);
}

static ssize_t foo_show(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf)
{
	size_t count;

	spin_lock(&lock);
	count = sprintf(buf, "%s\n", foo_kbuff);

	spin_unlock(&lock);
	return count;
}

static ssize_t foo_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	spin_lock(&lock);
	memcpy(foo_kbuff, buf, (strlen(buf)+1));
	
	spin_unlock(&lock);
	return count;
}

static struct kobj_attribute id_attribute =
	__ATTR(id, 0666, id_show, id_store);

static struct kobj_attribute jiffies_attribute =
	__ATTR_RO(jiffies);

static struct kobj_attribute foo_attribute =
	__ATTR(foo, S_IWUSR|S_IRGRP|S_IROTH, foo_show, foo_store);

/* group of attributes */
static struct attribute *attrs[] = {
	&id_attribute.attr,
	&jiffies_attribute.attr,
	&foo_attribute.attr,
	NULL,
};

/* An unnamed attribute group, hence all the attributes will go directly intto
 * the kobject directory.
 */
static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *sysfs_lkm_kobject;

static int __init sysfs_lkm_sysfs_init(void)
{
	int ret;

	pr_debug("%s: Init\n", DRV_NAME);

	sysfs_lkm_kobject = kobject_create_and_add(SYSFS_NAME, kernel_kobj);
	if (!sysfs_lkm_kobject) {
		pr_debug("Failed to create kobject for %s\n", SYSFS_NAME);
		return -ENOMEM;
	}

	/* Files go online as soon they are created */
	spin_lock_init(&lock);

	ret = sysfs_create_group(sysfs_lkm_kobject, &attr_group);
	if (ret) {
		kobject_put(sysfs_lkm_kobject);
		return ret;
	}

	return 0;
}

static void sysfs_lkm_sysfs_exit(void)
{
	kobject_put(sysfs_lkm_kobject);
	pr_debug("%s: Exiting\n", DRV_NAME);
}

module_init(sysfs_lkm_sysfs_init);
module_exit(sysfs_lkm_sysfs_exit);
