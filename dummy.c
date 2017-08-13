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

struct myy_driver_data {
	struct iommu_domain * __restrict iommu_domain;
};

/* The current objectives are :
 * - Get a DMA buffer using PRIME from a user application
 * - Send the file descriptor of this DMA Buffer to the driver, using
 *   simple IOCTL
 * - Get the DMA Buffer from this FD
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
	 * releasing the driver. */
	struct myy_driver_data * __restrict const data =
		devm_kzalloc(&pdev->dev, sizeof(struct iommu *), GFP_KERNEL);

	data->iommu_domain = NULL;
	platform_set_drvdata(pdev, data);

	return 0;
}

/* Should return 0 on success and a negative errno on failure. */
static int myy_vpu_remove(struct platform_device * pdev)
{
	struct myy_driver_data * __restrict const data =
		(struct myy_driver_data * __restrict) platform_get_drvdata(pdev);

	if (data->iommu_domain != NULL) {
		dev_info(
			&pdev->dev, 
			"IOMMU Domain was allocated. Deallocating now.\n"
		);
		pdev->dev.bus->iommu_ops->domain_free(data->iommu_domain);
		data->iommu_domain = NULL;
	}
	printk(KERN_INFO "Okay, I'll go, I'll go... !\n");

	// data was allocated through devm_kzalloc and will be deallocated
	// automatically.

	return 0;
}

static void myy_vpu_shutdown(struct platform_device *pdev)
{
	printk(KERN_INFO "Shutting down...\n");
}

static const struct of_device_id myy_vpu_dt_ids[] = {
	{
		.compatible = "rockchip,vpu_service",
	},
	{
		.compatible = "rockchip,hevc_service",
	},
	{}
};

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
