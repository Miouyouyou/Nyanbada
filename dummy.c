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

	printk(KERN_INFO "Hello MEOW !\n");
	print_platform_device(pdev);
	print_platform_device_device(pdev);
	print_platform_device_device_node(pdev);

	/* Most, if not every piece of code encountered prefer to use
	
	      if (iommu_present(&platform_bus_type))
	      	iommu_domain_alloc(&platform_bus_type);
	
	   Turns out that platform_bus_type is a global identifier providing
	   the bus platform (type : struct bus_type).
	 */

	/* Trying new IOMMU API ... without much success... */
	/* iommu_request_dm_for_dev
	 * → Cannot work as it does not allocate an IOMMU_DMA_DOMAIN domain,
	 *   which is the only domain understood by Rockchip systems.
	 *   Exynos systems seem to have the same behaviour too.
	 * iommu_group_get_for_dev
	 * → Nice and all, but won't lead us very far.
	 * iommu_get_domain_for_dev
	 * → This HAS to work in order to use the DMA-IOMMU SG mapping
	 *   functions.
	 *   However, it doesn't because the IOMMU group of the device does
	 *   not have any domain set up.
	 *   Now IOMMU Groups are opaque structures...
	 *   Setting up the domain of an IOMMU group seems to be a fucking
	 *   pain in the ass and can easily CRASH the machine when done wrong.
	 */
	if (iommu_present(pdev->dev.bus)) {
		struct iommu_domain * __restrict const iommu_domain =
			pdev->dev.bus->iommu_ops->domain_alloc(IOMMU_DOMAIN_DMA);
			//iommu_domain_alloc(pdev->dev.bus);

		print_iommu_domain(iommu_domain);
		data->iommu_domain = iommu_domain;
	}

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
