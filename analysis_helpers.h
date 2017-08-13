#ifndef __MYY_ANALYSIS_HELPERS_H
#define __MYY_ANALYSIS_HELPERS_H 1

#ifdef DUMBY_THE_EDITOR
#include <generated/autoconf.h>
#endif
#include <linux/platform_device.h>
#include <linux/iommu.h>

static inline const char * __restrict myy_bool_str
(bool value)
{
	return value ? "true" : "false";
}

static void print_platform_device
(struct platform_device const * __restrict const pdev)
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
(struct device_node const * __restrict const dev_node)
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
(struct platform_device const * __restrict const pdev)
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

	if (dev->iommu_group) {
		dev_info(dev, "IOMMU GROUP            : %p\n", dev->iommu_group);
	}
}

static void print_platform_device_device_node
(struct platform_device const * __restrict const pdev)
{
	struct device_node const * __restrict const dev_node =
		pdev->dev.of_node;
	dev_info(&pdev->dev,
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

static char const * __restrict iommu_domain_type_name
(unsigned int type)
{
	char const * __restrict type_name = "Unknown";
	
	switch(type) {
		case IOMMU_DOMAIN_BLOCKED:
			type_name = "Blocked (All DMA is blocked)";
			break;
		case IOMMU_DOMAIN_IDENTITY:
			type_name = "Identity (DMA addresses are system physical addresses)";
			break;
		case IOMMU_DOMAIN_UNMANAGED:
			type_name = "Unmanaged (DMA mappings managed by IOMMU-API user)";
			break;
		case IOMMU_DOMAIN_DMA:
			type_name = "DMA (Internally used for DMA-API implementations)";
			break;
		default:
			type_name = "Pure garbage (Your IOMMU driver is broken !)";
			break;
	}
	
	return type_name;
}

static void print_iommu_domain
(struct iommu_domain const * __restrict const domain)
{
	printk(KERN_INFO
		"Type                       : %s\n"
		"Operations struct address  : %p\n"
		"Accepted Page sizes bitmap : %lx\n"
		"Fault handler address      : %p\n"
		"Fault handler data address : %p\n"
		"Geometry :\n"
		"  Aperture Start           : %u\n"
		"  Aperture End             : %u\n"
		"IOVA Cookie address        : %p\n",
		iommu_domain_type_name(domain->type),
		domain->ops,
		domain->pgsize_bitmap,
		domain->handler,
		domain->handler_token,
		domain->geometry.aperture_start,
		domain->geometry.aperture_end,
		domain->iova_cookie
	);
}


#endif
