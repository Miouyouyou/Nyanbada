/* Dummy driver used for testing IOMMUs, DMABUF and all that stuff,
 * using the Rockchip VPU as a concrete example to perform these tests !
 */

/* If you got a dumb editor that does not support parsing defines from
 * a .h file, but support defining special macro values, then define
 * DUMBY_THE_EDITOR to 1 */
#ifdef DUMBY_THE_EDITOR
#include <generated/autoconf.h>
#endif

// THIS_MODULE and others things
#include <linux/module.h>

// platform_driver, platform_device, module_platform_driver
// Also include mod_devicetable.h
// of_device_id
#include <linux/platform_device.h>

// of_match_ptr
#include <linux/of.h>

// of_find_device_by_node
#include <linux/of_platform.h>

// iommu_present
#include <linux/iommu.h>

// dma_buf related functions
#include <linux/dma-buf.h>
#include "analysis_helpers.h"

// cdev_init and help us generate the /dev entries
#include <linux/cdev.h>

#include <linux/types.h>

#include "include/myy_ioctl.h"

struct myy_driver_data;
struct static_vpu_metadata;

// -- DATA STRUCTURES

/* Our driver's private data... Turns out that we need a lot of
 * informations due to the /dev structure creation...
 * I'm still wondering if all these things are *really* needed.
 * The device being ALREADY created, with ID and all...
 * Hmm...
 */
struct myy_driver_data {
	struct iommu_domain * __restrict iommu_domain;
	struct cdev * __restrict cdev;
	dev_t device_id;
	struct class * __restrict cls;
	struct device * __restrict sub_dev;
};

/* Not used at the moment */
struct static_vpu_metadata {
	char const * __restrict const dev_name;
};

static const struct static_vpu_metadata vpu_service_metadata = {
	.dev_name = "vpu-service"
};

static const struct static_vpu_metadata hevc_service_metadata = {
	.dev_name = "hevc-service"
};

static const struct of_device_id myy_vpu_dt_ids[] = {
	{
		.compatible = "rockchip,vpu_service",
		.data = &vpu_service_metadata
	},
	{
		.compatible = "rockchip,hevc_service",
		.data = &hevc_service_metadata
	},
	{}
};

// --- IOCTL
static long myy_dev_ioctl
(struct file * const filp,
 unsigned int const cmd, unsigned long const arg)
{
	int ret = 0;

	printk(KERN_INFO "IOCTL on %pD - %u - %lu!\n", filp, cmd, arg);

	switch (cmd) {
		case MYY_IOCTL_HELLO:
			printk("Meow !\n");
			ret = 1;
			break;
		case MYY_IOCTL_TEST_FD_PASSING:
			printk("So that %d is my FD ?\n", (__s32) arg);
			ret = (__s32) arg;
			break;
		case MYY_IOCTL_TEST_DMA_FD_PASSING: {
			int const dma_fd = (__s32) arg;
			struct dma_buf * __restrict const buf =
				dma_buf_get(dma_fd);
			char const * __restrict const owner = 
				buf->owner ? buf->owner->name : "(Unknown)";
			printk(
				"So... Got a DMA Buffer of %zu bytes from owner %s\n",
				buf->size, owner
			);
			ret = buf->size;
			dma_buf_put(buf);
		}
		break;
	}
	return ret;
}

static int myy_dev_open(struct inode * inode, struct file * filp)
{
	printk(KERN_INFO "Opening %pD !\n", filp);
	return nonseekable_open(inode, filp);
}

static int myy_dev_release(struct inode * inode, struct file * filp)
{
	printk(KERN_INFO "Release %pD !\n", filp);
	return 0;
}

static const struct file_operations myy_dev_file_ops = {
	.unlocked_ioctl = myy_dev_ioctl,
	.open           = myy_dev_open,
	.release        = myy_dev_release,
	.owner          = THIS_MODULE,
};

// --- DEVICE PROBING AND INIT

/* The current objectives are :
 * [x] Get a DMA buffer using PRIME from a user application
 * [x] Send the file descriptor of this DMA Buffer to the driver,
 *     using simple IOCTL
 * [x] Get the DMA Buffer from this FD
 * - Attach it to the VPU device
 * - Map it
 * - Write something in it
 * - Unmap it
 * - Detach it
 * - Check from the user space app if the data were written correctly
 *   in the DMA buffer
 * 
 * Special IOMMU operations might be required for that to work.
 */

/* Should return 0 on success and a negative errno on failure. */
static int myy_vpu_probe(struct platform_device * pdev)
{
	/* The device associated with the platform_device. Identifier
	 * used for convenience. (Dereferencing pdev every time is useless) */
	struct device * __restrict const vpu_dev = &pdev->dev;

	/* The data structure that will be used as driver's private data. */
	struct myy_driver_data * __restrict const driver_data =
		devm_kzalloc(&pdev->dev, sizeof(struct iommu *), GFP_KERNEL);

	/* Will be used to create /dev entries */
	struct cdev * __restrict const cdev = cdev_alloc();
	dev_t vpu_dev_t;
	const char * __restrict const name = pdev->dev.of_node->name;

	/* Used to check various return codes for errors */
	int ret;

	print_platform_device_device_node(pdev);

	/* Setup the private data storage for this device.
	 * 
	 * The data will be available through the device structure until
	 * the driver module gets unloaded.
	 * 
	 * platform_set_drvdata is equivalent to dev_set_drvdata, except that
	 * - dev_set_drvdata takes a device pointer
	 * - platform_set_drvdata takes a platform_device pointer
	 * 
	 * The main difference between kzalloc and devm_kzalloc is that
	 * allocated memory with devm_kzalloc gets automagically freed when
	 * releasing the driver.
	 */
	platform_set_drvdata(pdev, driver_data);


	/* Initialize our /dev entries
	 * 
	 * It seems that creating a sub-device is necessary to get the
	 * - /dev/#{name} entries to appear... Weird
	 * 
	 * alloc_chrdev_region will call MKDEV
	 */
	ret = alloc_chrdev_region(&vpu_dev_t, 0, 1, name);
	if (ret)
		dev_err(vpu_dev, "alloc_chrdev_region returned %d\n", ret);

	cdev_init(cdev, &myy_dev_file_ops);
	cdev->owner = THIS_MODULE;
	ret = cdev_add(cdev, vpu_dev_t, 1);

	driver_data->cls = class_create(THIS_MODULE, name);

	if (IS_ERR(driver_data->cls)) {
		ret = PTR_ERR(driver_data->cls);
		dev_err(vpu_dev, "class_create err:%d\n", ret);
		goto err;
	}

	driver_data->sub_dev = 
		device_create(driver_data->cls, vpu_dev,	vpu_dev_t, NULL, "%s", name);
	
	dev_info(&pdev->dev, "cdev_add returned %d\n", ret);


	/* Set up our driver's private metadata with the valid values. */
	driver_data->iommu_domain = NULL;
	driver_data->cdev         = cdev;
	driver_data->device_id    = vpu_dev_t;
err:
	return ret;
}

/* Should return 0 on success and a negative errno on failure. */
static int myy_vpu_remove(struct platform_device * pdev)
{
	struct myy_driver_data * __restrict const driver_data =
		(struct myy_driver_data * __restrict) platform_get_drvdata(pdev);

	/* Remove the subdevice and /dev entries */
	device_destroy(driver_data->cls, driver_data->device_id);
	class_destroy(driver_data->cls);
	cdev_del(driver_data->cdev);
	unregister_chrdev_region(driver_data->device_id, 1);

	/* Free the IOMMU domain if it was allocated */
	if (driver_data->iommu_domain) {
		dev_info(
			&pdev->dev, 
			"IOMMU Domain was allocated. Deallocating now.\n");
		pdev->dev.bus->iommu_ops->domain_free(driver_data->iommu_domain);
	}

	/* Reset our driver private data.
	 * 
	 * Kind of useless though... the data structure will be blowed up
	 * a few milliseconds later.
	 */
	driver_data->iommu_domain = NULL;
	driver_data->cdev         = NULL;
	driver_data->device_id    = 0;
	driver_data->cls          = NULL;
	driver_data->sub_dev      = NULL;
	printk(KERN_INFO "Okay, I'll go, I'll go... !\n");

	/* driver_data was allocated through devm_kzalloc and will be
	 * deallocated automatically.
	 */

	return 0;
}

static void myy_vpu_shutdown(struct platform_device *pdev)
{
	printk(KERN_INFO "Shutting down...\n");
}

static struct platform_driver myy_vpu_driver = {
	.probe = myy_vpu_probe,
	.remove = myy_vpu_remove,
	.shutdown = myy_vpu_shutdown,
	.driver = {
		.name = "myy-vpu",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(myy_vpu_dt_ids),
	},
};

module_platform_driver(myy_vpu_driver);
MODULE_LICENSE("GPL v2");
