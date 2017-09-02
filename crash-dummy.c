#ifdef DUMBY_THE_EDITOR /* Used for Kdevelop */
#include <generated/autoconf.h>
#endif

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>

/* Crashes when trying to access pdev->resources[0].

[10731.890314] Name               : ff9a0000.vpu-service
[10731.890314] ID                 : -1
[10731.890314] ID auto            : false
[10731.890314] Device (name)      : ff9a0000.vpu-service
[10731.890314] Num resources      : 3
[10731.890314] Resources address  : eea33480
[10731.890314] Platform device ID : (null)
[10731.922220] Printing resources of device
[10731.922223] [0/2] Resource
[10731.930041]   Address : eea33480

 */

#define MHZ (1000*1000)

static inline const char * __restrict myy_bool_str
(bool value)
{
	return value ? "true" : "false";
}

/* Get the clocks
 * Activate them
 * See if that works now */

static void print_resources
(struct resource * resources, uint const n_resources)
{
	uint r;
	uint r_max = n_resources - 1;
	for (r = 0; r < n_resources; r++) {
		struct resource * current_res =	&resources[r];
		printk("[%d/%d] Resource\n", r, r_max);
		printk("  Address : %p\n", current_res);
		printk("  Start : %pa\n", &current_res->start);
		printk("  End   : %pa\n", &current_res->end);
		printk("  Name  : %s\n",  current_res->name);
		printk("  Flags : %lx\n", current_res->flags);
		printk("  Desc  : %lx\n", current_res->desc);
	}
}

/* The freezing part */
static int myy_vpu_probe(struct platform_device * pdev)
{
	//struct device * __restrict const dev = &pdev->dev;
	//struct device_node *np = pdev->dev.of_node;
	struct resource * res;

	/*struct clk * __restrict const aclk = devm_clk_get(dev, "aclk_vcodec");
	struct clk * __restrict const hclk = devm_clk_get(dev, "hclk_vcodec");
	struct clk * __restrict const pd_video = devm_clk_get(dev, "pd_video");

	if (IS_ERR(aclk)) {
		dev_err(dev, "failed on clk_get aclk_vcodec\n");
		return -1;
	}

	if (IS_ERR(hclk)) {
		dev_err(dev, "failed on clk_get hclk_vcodec\n");
		return -1;
	}

	if (IS_ERR(pd_video))
		dev_err(dev, "No PD_VIDEO Clock\n");

	dev_info(dev, "Clocks set\n");*/

	printk("Hello\n");
	print_resources(pdev->resource, pdev->num_resources);

	/*if (of_property_read_bool(np, "reg")) {*/
		res =	platform_get_resource(pdev, IORESOURCE_MEM, 0);
		printk("Freeze !\n");
		printk("Nope >:C\n");
		printk("  Start : %x, End : %x\n", res->start, res->end);
		printk("  pdev->resource[0].flags = %lx\n", pdev->resource[0].flags);
	/*}*/

	return 0;
}

/* Should return 0 on success and a negative errno on failure. */
static int myy_vpu_remove(struct platform_device * pdev)
{
	return 0;
}

static void myy_vpu_shutdown(struct platform_device * pdev)
{
	printk(KERN_INFO "Shutting down...\n");
}

static const struct of_device_id myy_vpu_dt_ids[] = {
	{
		.compatible = "rockchip,vpu_service",
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

/* Associated DTS : rk3288.dtsi and rk3288-miqi.dts

/ {

	...

	vpu_service: vpu-service@ff9a0000 {
		compatible = "rockchip,vpu_service";
		reg = <0xff9a0000 0x800>;
		interrupts = 
			<GIC_SPI  9 IRQ_TYPE_LEVEL_HIGH>,
			<GIC_SPI 10 IRQ_TYPE_LEVEL_HIGH>;
		interrupt-names = "irq_enc", "irq_dec";
		clocks = <&cru ACLK_VCODEC>, <&cru HCLK_VCODEC>;
		clock-names = "aclk_vcodec", "hclk_vcodec";
		power-domains = <&power RK3288_PD_VIDEO>;
		rockchip,grf = <&grf>;
		resets = <&cru SRST_VCODEC_AXI>, <&cru SRST_VCODEC_AHB>;
		reset-names = "video_a", "video_h";
		iommus = <&vpu_mmu>;
		iommu_enabled = <1>;
		dev_mode = <0>;
		status = "okay";
		0 means ion, 1 means drm
		allocator = <1>;
	};

	...

}

End of Associated DTS */
