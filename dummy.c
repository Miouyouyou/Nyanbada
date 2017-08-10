/* Dummy driver used for testing IOMMUs, DMABUF and all that stuff,
 * using the Rockchip VPU as a concrete example to perform these tests !
 */

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

static const char * __restrict myy_bool_str
(bool value)
{
	return value ? "true" : "false";
}

static void print_platform_device
(struct platform_device const * __restrict pdev)
{
	printk(KERN_INFO
		"Name               : %s\n"
		"ID                 : %d\n"
		"ID auto            : %s\n"
		"Device (name)      : %s\n"
		"Num resources      : %u\n"
		"Platform device ID : %s\n",
		pdev->name,
		pdev->id,
		myy_bool_str(pdev->id_auto),
		dev_name(&pdev->dev),
		pdev->num_resources,
		pdev->id_entry->name
	);
}

static void print_device_node_properties
(struct device_node const * __restrict dev_node)
{
	struct property const * __restrict current_property =
		dev_node->properties;
	while(current_property) {
		printk(KERN_INFO
			"-> Property\n"
			"   Name   : %s\n"
			"   Length : %d\n",
			current_property->name,
			current_property->length
		);
		current_property = current_property->next;
	}
}

static void print_platform_device_device
(struct platform_device const * __restrict pdev)
{
	struct device const * __restrict dev = &pdev->dev;

	dev_info(dev, "Parent address         : %p\n", dev->parent           );
	dev_info(dev, "Device private address : %p\n", dev->p                );
	dev_info(dev, "Kobject (name)         : %s\n", dev->kobj.name        );
	dev_info(dev, "Initial name           : %s\n", dev->init_name        );
	if (dev->type)
		dev_info(dev, "Device type (name)     : %s\n", dev->type->name);
	else
		dev_info(dev, "No device type defined\n");
	dev_info(dev, "Mutex (Not shown)\n");

	if (dev->bus) {
		dev_info(dev, "Bus type (name)        : %s\n", dev->bus->name);
		dev_info(dev, "Bus type (dev name)    : %s\n", dev->bus->dev_name);
	}
	else
		dev_info(dev, "No bus defined\n");

	if (dev->driver) {
		dev_info(dev, "Device driver (name)   : %s\n", dev->driver->name);
		if (dev->driver->bus)
			dev_info(dev, "Device driver Bus name : %s\n", dev->driver->bus->name);
		else
			dev_info(dev, "No bus defined on the device driver\n");
	}
	else
		dev_info(dev, "No device driver\n");

	dev_info(dev, "Platform data address  : %p\n", dev->platform_data    );
	dev_info(dev, "Driver data address    : %p\n", dev->driver_data      );

}

static void print_platform_device_node
(struct platform_device const * __restrict pdev)
{
	struct device_node const * __restrict dev_node = pdev->dev.of_node;
	printk(KERN_INFO
		"[Device node of : %s]\n"
		"Name      : %pOFn\n"
		"Type      : %s\n"
		"Phandle   : %pOFp\n"
		"Full name : %pOFf\n",
		dev_name(&pdev->dev),
		dev_node,
		dev_node->type,
		dev_node,
		dev_node
	);
	print_device_node_properties(dev_node);
}

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
	print_platform_device_node(pdev);

	/* Most, if not every piece of code encountered prefer to use
	
	      if (iommu_present(&platform_bus_type))
	      	iommu_domain_alloc(&platform_bus_type);
	
	   Turns out that platform_bus_type is a global identifier providing
	   the bus platform (type : struct bus_type).
	 */
	if (iommu_present(pdev->dev.bus)) {
		struct iommu_domain * __restrict const iommu_domain =
			iommu_domain_alloc(pdev->dev.bus);
		dev_info(&pdev->dev, "IOMMU present on device !\n");
		dev_info(&pdev->dev, "IOMMU Page Size Bitmap : %lu\n",
			iommu_domain->pgsize_bitmap
		);
		dev_info(&pdev->dev, "IOMMU Page Size Bitmap (ops) : %lu\n",
			iommu_domain->ops->pgsize_bitmap
		);
		dev_info(&pdev->dev, "The System page size : %lu\n",
			PAGE_SIZE
		);
		dev_info(&pdev->dev,
			"Does the IOMMU support the system page size : %s\n",
			myy_bool_str(PAGE_SIZE & iommu_domain->ops->pgsize_bitmap)
		);
		
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
		iommu_domain_free(data->iommu_domain);
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
