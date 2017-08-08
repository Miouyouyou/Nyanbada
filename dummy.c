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

/* Should return 0 on success and a negative errno on failure. */
static int myy_vpu_probe(struct platform_device * pdev)
{
	printk(KERN_INFO "Hello MEOW !\n");
	print_platform_device(pdev);
	print_platform_device_node(pdev);
	return 0;
}

/* Should return 0 on success and a negative errno on failure. */
static int myy_vpu_remove(struct platform_device * pdev)
{
	printk(KERN_INFO "Nooooo !\n");
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
