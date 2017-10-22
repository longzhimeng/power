/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
*/

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/uaccess.h>

extern void __iomem *GPIOMAPBASE;

#define APQ_NR_GPIOS 150
#define REG_SIZE 0x1000
#define STATUS_OFFSET 0x10 
#define APQ_GPIO_CFG(n)       (GPIOMAPBASE + (REG_SIZE * n))
#define APQ_GPIO_IN_OUT(n)    (GPIOMAPBASE + 4 + (REG_SIZE * n))


#define APQ_GPIO_PULL(x)    ((x.ctrl) & 0x3)
#define APQ_GPIO_FUNC(x)    (((x.ctrl) >> 2) & 0xF)
#define APQ_GPIO_DRV(x)     (((x.ctrl) >> 6) & 0x7)
#define APQ_GPIO_OUT_EN(x)  (((x.ctrl) >> 9) & 0x1)

#define APQ_GPIO_IN_VAL(x)  ((x.inout) & 0x1)
#define APQ_GPIO_OUT_VAL(x) (((x.inout) >> 1) & 0x1)


typedef void (*store_func) (struct device * dev, void *data, size_t num);                                                                                                               
typedef int (*show_func) (struct seq_file * m, void *data, size_t num);

static void apq_gpio_store(struct device *dev, void *data, size_t num); 
static int apq_gpio_show(struct seq_file *m, void *data, size_t num);

static void pm845_gpio_store(struct device *dev, void *data, size_t num);
static int pm845_gpio_show(struct seq_file *m, void *data, size_t num);
static void pm845_ldo_store(struct device *dev, void *data, size_t num);                                                                                                           
static int pm845_ldo_show(struct seq_file *m, void *data, size_t num);

static void pm8005_gpio_store(struct device *dev, void *data, size_t num);
static int pm8005_gpio_show(struct seq_file *m, void *data, size_t num);

static void pm8005_ldo_store(struct device *dev, void *data, size_t num);
static int pm8005_ldo_show(struct seq_file *m, void *data, size_t num);

static void pmi8998_gpio_store(struct device *dev, void *data, size_t num);
static int pmi8998_gpio_show(struct seq_file *m, void *data, size_t num);
static void pmi8998_ldo_store(struct device *dev, void *data, size_t num);
static int pmi8998_ldo_show(struct seq_file *m, void *data, size_t num);



static u32 debug_mask;
static bool sleep_saved;
static struct dentry *debugfs;

/* APQ GPIO */
struct apq_gpio {                                                                                                                                                                       
	uint32_t ctrl;
	uint32_t inout;
};

enum gpiomux_pull {
	GPIOMUX_PULL_NONE = 0,
	GPIOMUX_PULL_DOWN,
	GPIOMUX_PULL_KEEPER,
	GPIOMUX_PULL_UP,
};

enum {
	DUMP_APQ_GPIO,
	DUMP_PM845_GPIO,
	DUMP_PM845_LDO,
	DUMP_PM8005_GPIO,
	DUMP_PM8005_LDO,
	DUMP_PMI8998_GPIO,
	DUMP_PMI8998_LDO,
	DUMP_DEV_NUM
};


/* Registers fields decoding arrays */
static const char *apq_pull_map[4] = {
	[GPIOMUX_PULL_NONE] = "none",
	[GPIOMUX_PULL_DOWN] = "pd",
	[GPIOMUX_PULL_KEEPER] = "keeper",
	[GPIOMUX_PULL_UP] = "pu",
};

struct reg_property {
	char *regname;
	int regaddr;
};


struct ldo_property {
	char *name;
	uint8_t Onoff_value;
};

struct gpio_property {
	char *name;
	uint8_t Onoff_value;
};


struct dump_desc {
	const char *name;
	show_func show;
	store_func store;
	struct device *dev;
	size_t item_size;
	size_t item_count;
	void *sleep_data;
};

struct reg_property pm8005_reg_gpio_table[] = {
	/*pm8005 4 gpios*/
	{"GPIO1_STATUS", 0xC008},
	{"GPIO2_STATUS", 0xC108},
	{"GPIO3_STATUS", 0xC208},
	{"GPIO4_STATUS", 0xC308},

	{"GPIO1_DIG_OUT", 0xC044},
	{"GPIO2_DIG_OUT", 0xC144},
	{"GPIO3_DIG_OUT", 0xC244},
	{"GPIO4_DIG_OUT", 0xC344},
};

struct reg_property pm845_reg_gpio_table[] = {
	/*PM845 26 GPIOs*/
	{"GPIO1_STATUS", 0xC008},
	{"GPIO2_STATUS", 0xC108},
	{"GPIO3_STATUS", 0xC208},
	{"GPIO4_STATUS", 0xC308},
	{"GPIO5_STATUS", 0xC408},
	{"GPIO6_STATUS", 0xC508},
	{"GPIO7_STATUS", 0xC608},
	{"GPIO8_STATUS", 0xC708},
	{"GPIO9_STATUS", 0xC808},
	{"GPIO10_STATUS", 0xC908},
	{"GPIO11_STATUS", 0xCA08},
	{"GPIO12_STATUS", 0xCB08},
	{"GPIO13_STATUS", 0xCC08},
	{"GPIO14_STATUS", 0xCD08},
	{"GPIO15_STATUS", 0xCE08},
	{"GPIO16_STATUS", 0xCF08},
	{"GPIO17_STATUS", 0xD008},
	{"GPIO18_STATUS", 0xD108},
	{"GPIO19_STATUS", 0xD208},
	{"GPIO20_STATUS", 0xD308},
	{"GPIO21_STATUS", 0xD408},
	{"GPIO22_STATUS", 0xD508},
	{"GPIO23_STATUS", 0xD608},
	{"GPIO24_STATUS", 0xD708},
	{"GPIO25_STATUS", 0xD808},
	{"GPIO26_STATUS", 0xD908},

	{"GPIO1_DIG_OUT", 0xC044},
	{"GPIO2_DIG_OUT", 0xC144},
	{"GPIO3_DIG_OUT", 0xC244},
	{"GPIO4_DIG_OUT", 0xC344},
	{"GPIO5_DIG_OUT", 0xC444},
	{"GPIO6_DIG_OUT", 0xC544},
	{"GPIO7_DIG_OUT", 0xC644},
	{"GPIO8_DIG_OUT", 0xC744},
	{"GPIO9_DIG_OUT", 0xC844},
	{"GPIO10_DIG_OUT", 0xC944},
	{"GPIO11_DIG_OUT", 0xCA44},
	{"GPIO12_DIG_OUT", 0xCB44},
	{"GPIO13_DIG_OUT", 0xCC44},
	{"GPIO14_DIG_OUT", 0xCD44},
	{"GPIO15_DIG_OUT", 0xCE44},
	{"GPIO16_DIG_OUT", 0xCF44},
	{"GPIO17_DIG_OUT", 0xD044},
	{"GPIO18_DIG_OUT", 0xD144},
	{"GPIO19_DIG_OUT", 0xD244},
	{"GPIO20_DIG_OUT", 0xD344},
	{"GPIO21_DIG_OUT", 0xD444},
	{"GPIO22_DIG_OUT", 0xD544},
	{"GPIO23_DIG_OUT", 0xD644},
	{"GPIO24_DIG_OUT", 0xD744},
	{"GPIO25_DIG_OUT", 0xD844},
	{"GPIO26_DIG_OUT", 0xD944},
};

struct reg_property pmi8998_reg_gpio_table[] = {
	/*pmi8998 14 gpios*/
	{"GPIO1_STATUS", 0x2C008},
	{"GPIO2_STATUS", 0x2C108},
	{"GPIO3_STATUS", 0x2C208},
	{"GPIO4_STATUS", 0x2C308},
	{"GPIO5_STATUS", 0x2C408},
	{"GPIO6_STATUS", 0x2C508},
	{"GPIO7_STATUS", 0x2C608},
	{"GPIO8_STATUS", 0x2C708},
	{"GPIO9_STATUS", 0x2C808},
	{"GPIO10_STATUS", 0x2C908},
	{"GPIO11_STATUS", 0x2CA08},
	{"GPIO12_STATUS", 0x2CB08},
	{"GPIO13_STATUS", 0x2CC08},
	{"GPIO14_STATUS", 0x2CD08},

	{"GPIO1_DIG_OUT", 0x2C044},
	{"GPIO2_DIG_OUT", 0x2C144},
	{"GPIO3_DIG_OUT", 0x2C244},
	{"GPIO4_DIG_OUT", 0x2C344},
	{"GPIO5_DIG_OUT", 0x2C444},
	{"GPIO6_DIG_OUT", 0x2C544},
	{"GPIO7_DIG_OUT", 0x2C644},
	{"GPIO8_DIG_OUT", 0x2C744},
	{"GPIO9_DIG_OUT", 0x2C844},
	{"GPIO10_DIG_OUT", 0x2C944},
	{"GPIO11_DIG_OUT", 0x2CA44},
	{"GPIO12_DIG_OUT", 0x2CB44},
	{"GPIO13_DIG_OUT", 0x2CC44},
	{"GPIO14_DIG_OUT", 0x2CD44},
};

struct reg_property pm845_reg_ldo_table[] = {
	/* pm845 29 ldos*/
	{"L1_CTRL_EN_CTL", 0x14046},
	{"L2_CTRL_EN_CTL", 0x14146},
	{"L3_CTRL_EN_CTL", 0x14246},
	{"L4_CTRL_EN_CTL", 0x14346}, 
	{"L5_CTRL_EN_CTL", 0x14446},
	{"L6_CTRL_EN_CTL", 0x14546},
	{"L7_CTRL_EN_CTL", 0x14646},
	{"L8_CTRL_EN_CTL", 0x14746},
	{"L9_CTRL_EN_CTL", 0x14846},
	{"L10_CTRL_EN_CTL", 0x14946},
	{"L11_CTRL_EN_CTL", 0x14A46},
	{"L12_CTRL_EN_CTL", 0x14B46},
	{"L13_CTRL_EN_CTL", 0x14C46},
	{"L14_CTRL_EN_CTL", 0x14D46},
	{"L15_CTRL_EN_CTL", 0x14E46},
	{"L16_CTRL_EN_CTL", 0x14F46},
	{"L17_CTRL_EN_CTL", 0x15046},
	{"L18_CTRL_EN_CTL", 0x15146},
	{"L19_CTRL_EN_CTL", 0x15246},
	{"L20_CTRL_EN_CTL", 0x15346}, 
	{"L21_CTRL_EN_CTL", 0x15446}, 
	{"L22_CTRL_EN_CTL", 0x15546}, 
	{"L23_CTRL_EN_CTL", 0x15646}, 
	{"L24_CTRL_EN_CTL", 0x15746}, 
	{"L25_CTRL_EN_CTL", 0x15846}, 
	{"L26_CTRL_EN_CTL", 0x15946}, 
	{"L27_CTRL_EN_CTL", 0x15A46}, 
	{"L28_CTRL_EN_CTL", 0x15B46}, 
	{"L29_CTRL_EN_CTL", 0x15C46}, 

	{"LVS1_CTRL_EN_CTL", 0x18046}, 
	{"LVS2_CTRL_EN_CTL", 0x18146}, 

	 /*pm845 13 smps*/
	{"S1_CTRL_EN_CTL", 0x11446},
	{"S2_CTRL_EN_CTL", 0x11746},
	{"S3_CTRL_EN_CTL", 0x11A46},
	{"S4_CTRL_EN_CTL", 0x11D46},
	{"S5_CTRL_EN_CTL", 0x12046},
	{"S6_CTRL_EN_CTL", 0x12346},	
	{"S7_CTRL_EN_CTL", 0x12646},
	{"S8_CTRL_EN_CTL", 0x12946},
	{"S9_CTRL_EN_CTL", 0x12C46},
	{"S10_CTRL_EN_CTL", 0x12F46},
	{"S11_CTRL_EN_CTL", 0x13246},
	{"S12_CTRL_EN_CTL", 0x13546},
	{"S13_CTRL_EN_CTL", 0x13846},
};

struct reg_property pmi8998_reg_ldo_table[] = {
	/*pmi8998 1 smps*/
	{"BOB_CTRL_EN_CTL", 0x3A046},
};

struct reg_property pm8005_reg_ldo_table[] = {
	/*pm8005 4 smps*/
	{"S1_CTRL_EN_CTL", 0x51446},
	{"S2_CTRL_EN_CTL", 0x51746},
	{"S3_CTRL_EN_CTL", 0x51A46},
	{"S4_CTRL_EN_CTL", 0x51D46},
};


static const u32 sdm845_tile_offsets[] = {0x500000, 0x900000, 0x100000};
static const unsigned int n_tile_offsets = ARRAY_SIZE(sdm845_tile_offsets);

/*TZ gpio*/
int gpio_tz[] = {0,1,2,3,81,82,83,84};

/* List of dump targets */
static struct dump_desc dump_devices[] = {
	[DUMP_APQ_GPIO] = {
		.name = "apq_gpio",
		.store = apq_gpio_store,
		.show = apq_gpio_show,
		.dev = NULL,
		.item_size = sizeof(struct apq_gpio),
		.item_count = APQ_NR_GPIOS,
	},

	[DUMP_PM8005_LDO] = {
		.name = "pm8005_ldo",
		.store = pm8005_ldo_store,
		.show = pm8005_ldo_show,
		.dev = NULL,
		.item_size = sizeof(struct ldo_property),
		.item_count =
		sizeof(pm8005_reg_ldo_table) / sizeof(pm8005_reg_ldo_table[0]),
	}
	,

	[DUMP_PM8005_GPIO] = {
		.name = "pm8005_gpio",
		.store = pm8005_gpio_store,
		.show = pm8005_gpio_show,
		.dev = NULL,
		.item_size = sizeof(struct gpio_property),
		.item_count =
		sizeof(pm8005_reg_gpio_table) / sizeof(pm8005_reg_gpio_table[0]),
	}
	,

	[DUMP_PM845_LDO] = {
		.name = "pm845_ldo",
		.store = pm845_ldo_store,
		.show = pm845_ldo_show,
		.dev = NULL,
		.item_size = sizeof(struct ldo_property),
		.item_count =
		sizeof(pm845_reg_ldo_table) / sizeof(pm845_reg_ldo_table[0]),
	}
	,

	[DUMP_PM845_GPIO] = {
		.name = "pm845_gpio",
		.store = pm845_gpio_store,
		.show = pm845_gpio_show,
		.dev = NULL,
		.item_size = sizeof(struct gpio_property),
		.item_count =
		sizeof(pm845_reg_gpio_table) / sizeof(pm845_reg_gpio_table[0]),
	}
	,

	[DUMP_PMI8998_LDO] = {
		.name = "pmi8998_ldo",
		.store = pmi8998_ldo_store,
		.show = pmi8998_ldo_show,
		.dev = NULL,
		.item_size = sizeof(struct ldo_property),
		.item_count =
		sizeof(pmi8998_reg_ldo_table) / sizeof(pmi8998_reg_ldo_table[0]),
	}
	,

	[DUMP_PMI8998_GPIO] = {
		.name = "pmi8998_gpio",
		.store = pmi8998_gpio_store,
		.show = pmi8998_gpio_show,
		.dev = NULL,
		.item_size = sizeof(struct gpio_property),
		.item_count =
		sizeof(pmi8998_reg_gpio_table) / sizeof(pmi8998_reg_gpio_table[0]),
	}

	,
};

static int tz_ctrl(int num)
{
	int i;
	int size;

	size = sizeof(gpio_tz) / sizeof(gpio_tz[0]);
	for (i = 0; i < size; i++) {
		if(num == gpio_tz[i])
			return 1;
	}

	return 0;
}


static u32 sdm845_pinctrl_find_base(u32 gpio_id)
{
	int i;
	u32 val; 
  
	if (gpio_id >= APQ_NR_GPIOS)                                                                                                                        
		return 0;

	for (i = 0; i < n_tile_offsets; i++) {
		val = readl_relaxed(APQ_GPIO_CFG(gpio_id)+ sdm845_tile_offsets[i]
				+ STATUS_OFFSET);
		if (val) 
			return sdm845_tile_offsets[i];
	}
    					       
	return 0;
}

static void apq_gpio_store(struct device *unused, void *data, size_t num)
{
	int gpio_id;
	u32 base;
	
	struct apq_gpio *gpios = (struct apq_gpio *)data;	
	memset(data,0, sizeof(struct apq_gpio) * num);

	if (num > APQ_NR_GPIOS){
		pr_err("apq gpio numbers is out of bound\n");	
		return ;
	}
	
	for(gpio_id = 0; gpio_id < num; gpio_id++){
		if (tz_ctrl(gpio_id)) {
			continue;
		}		

		base = sdm845_pinctrl_find_base(gpio_id);    
		gpios[gpio_id].ctrl = readl(APQ_GPIO_CFG(gpio_id) + base) & 0x3FF;
		gpios[gpio_id].inout = readl((APQ_GPIO_IN_OUT(gpio_id) + base)) & 0x3;
	}
}	


static int apq_gpio_show(struct seq_file *m, void *data, size_t num)
{
	int i = 0;
	struct apq_gpio *gpios = (struct apq_gpio *)data;

	pr_debug("GPIOMAPBASE=0x%p \n", GPIOMAPBASE);
	seq_printf(m, "+--+-----+-----+-----+------+------+\n");
	seq_printf(m, "|#  | dir | val | drv | func | pull |\n");
	seq_printf(m, "+--+-----+-----+-----+------+------+\n");

	for (i = 0; i < num; i++) {
		if (tz_ctrl(i)) {
			seq_printf(m, "|%03u|%-5s|%-5s|%-4s |%-6s|%-6s|\n", i,
					"TZ", "TZ", "TZ", "TZ", "TZ");
			continue;
		}

		seq_printf(m, "|%03u|%-5s|%-5u|%-2umA |%-6u|%-6s|\n", i,
				   APQ_GPIO_OUT_EN(gpios[i]) ? "out" : "in",
				   APQ_GPIO_OUT_EN(gpios[i]) ?
				   APQ_GPIO_OUT_VAL(gpios[i]) :
				   APQ_GPIO_IN_VAL(gpios[i]),
				   APQ_GPIO_DRV(gpios[i]) * 2 + 2,
				   APQ_GPIO_FUNC(gpios[i]),
				   apq_pull_map[APQ_GPIO_PULL(gpios[i])]
				  );
	}

	seq_printf(m, "+--+-----+-----+-----+------+------+\n");
    return 0;
}

extern int read_pmic_data(u8 sid, u16 addr, u8 * buf, int len);
static void pmi8998_ldo_store(struct device *ssbi_dev, void *data, size_t num)
{
	int index;
	uint8_t sid;
	uint8_t buf[1];
	uint16_t addr;
	uint8_t ret;

	struct ldo_property *ldo_on_off = (struct ldo_property *)data;

	for (index = 0;
		 index < (sizeof(pmi8998_reg_ldo_table) / sizeof(struct reg_property));
		 index++) {
		sid = (pmi8998_reg_ldo_table[index].regaddr >> 16) & 0xF;
		addr = pmi8998_reg_ldo_table[index].regaddr & 0xFFFF;

		ret = read_pmic_data(sid, addr, buf, 1);
		if (ret < 0) {
			pr_err("SPMI read failed, err = %d\n", ret);
			goto done;
		}

		ldo_on_off[index].name = pmi8998_reg_ldo_table[index].regname;
		ldo_on_off[index].Onoff_value = buf[0];
		pr_debug("%s ,  ret=%d, value=0x%x\n", __func__, ret, buf[0]);
	}

done:
	return;

}

static int pmi8998_ldo_show(struct seq_file *m, void *data, size_t num)
{
	int i = 0;
	struct ldo_property *ldo_on_off = (struct ldo_property *)data;

	seq_printf(m, "+--+------+----ldo_show: On/Off--+---+--------\n");

	for (i = 0; i < num; i++) {
		seq_printf(m, "|%-15s| on_off=0x%x\n", ldo_on_off[i].name,
			   (ldo_on_off[i].Onoff_value >> 7) & 0x01);
	}

	seq_printf(m, "+--+-------+-----+----+---+-----------+----+--\n");
	return 0;
}

void pmi8998_gpio_store(struct device *dev, void *data, size_t num)
{
	int index;
	uint8_t sid;
	uint8_t buf[1];
	uint16_t addr;
	uint8_t ret;

	struct gpio_property *gpiomap_on_off = (struct gpio_property *)data;

	for (index = 0;
		 index < (sizeof(pmi8998_reg_gpio_table) / sizeof(struct reg_property));
		 index++) {
		sid = (pmi8998_reg_gpio_table[index].regaddr >> 16) & 0xF;
		addr = pmi8998_reg_gpio_table[index].regaddr & 0xFFFF;

		ret = read_pmic_data(sid, addr, buf, 1);
		if (ret < 0) {
			pr_err("SPMI read failed, err = %d\n", ret);
			goto done;
		}

		gpiomap_on_off[index].name = pmi8998_reg_gpio_table[index].regname;
		gpiomap_on_off[index].Onoff_value = buf[0];
		pr_debug("%s ,  ret=%d, value=0x%x\n", __func__, ret, buf[0]);
	}

done:
	return;
}

int pmi8998_gpio_show(struct seq_file *m, void *data, size_t num)
{
	int i = 0;
	struct gpio_property *gpiomap_on_off = (struct gpio_property *)data;

	seq_printf(m, "+--+------------+--pmic gpio_show----------+---\n");

	for (i = 0; i < num / 2; i++) {
		seq_printf(m, "|%-13s| value=0x%02x | %-7s | %-10s\n", gpiomap_on_off[i].name,
				   gpiomap_on_off[i].Onoff_value & 0xFF,
				   (gpiomap_on_off[i].Onoff_value >> 7) == 1 ? "enable" : "disable",
				   (gpiomap_on_off[i].Onoff_value & 0x01) == 1 ? "input_high" : "input_low"
				   );
	}

	for (i = num / 2; i < num; i++) {
		seq_printf(m, "|%-13s| invert=%d\n", gpiomap_on_off[i].name,
				gpiomap_on_off[i].Onoff_value >> 7);
	}

	seq_printf(m, "+--+-----------+-----+----+---+-----------+----+--\n");
	return 0;
}

static void pm8005_ldo_store(struct device *ssbi_dev, void *data, size_t num)
{
	int index;
	uint8_t sid;
	uint8_t buf[1];
	uint16_t addr;
	uint8_t ret;

	struct ldo_property *ldo_on_off = (struct ldo_property *)data;
	
	for (index = 0;
		 index < (sizeof(pm8005_reg_ldo_table) / sizeof(struct reg_property));
		 index++) {
		sid = (pm8005_reg_ldo_table[index].regaddr >> 16) & 0xF;
		addr = pm8005_reg_ldo_table[index].regaddr & 0xFFFF;

		ret = read_pmic_data(sid, addr, buf, 1);
		if (ret < 0) {
			pr_err("SPMI read failed, err = %d\n", ret);
			goto done;
		}
		ldo_on_off[index].name = pm8005_reg_ldo_table[index].regname;
		ldo_on_off[index].Onoff_value = buf[0];
		pr_debug("%s ,  ret=%d, value=0x%x\n", __func__, ret, buf[0]);
	}

done:
	return;

}

static int pm8005_ldo_show(struct seq_file *m, void *data, size_t num)
{
	int i = 0;
	struct ldo_property *ldo_on_off = (struct ldo_property *)data;

	seq_printf(m, "+--+------+----ldo_show: On/Off--+---+--------\n");

	for (i = 0; i < num; i++) {
		seq_printf(m, "|%-15s| on_off=0x%x\n", ldo_on_off[i].name,
			   (ldo_on_off[i].Onoff_value >> 7) & 0x01);
	}

	seq_printf(m, "+--+-------+-----+----+---+-----------+----+--\n");
	return 0;
}

void pm8005_gpio_store(struct device *dev, void *data, size_t num)
{
	int index;
	uint8_t sid;
	uint8_t buf[1];
	uint16_t addr;
	uint8_t ret;

	struct gpio_property *gpiomap_on_off = (struct gpio_property *)data;

	for (index = 0;
		 index < (sizeof(pm8005_reg_gpio_table) / sizeof(struct reg_property));
		 index++) {
		sid = (pm8005_reg_gpio_table[index].regaddr >> 16) & 0xF;
		addr = pm8005_reg_gpio_table[index].regaddr & 0xFFFF;

		ret = read_pmic_data(sid, addr, buf, 1);
		if (ret < 0) {
			pr_err("SPMI read failed, err = %d\n", ret);
			goto done;
		}
		gpiomap_on_off[index].name = pm8005_reg_gpio_table[index].regname;
		gpiomap_on_off[index].Onoff_value = buf[0];
		pr_debug("%s ,  ret=%d, value=0x%x\n", __func__, ret, buf[0]);
	}

done:
	return;
}

int pm8005_gpio_show(struct seq_file *m, void *data, size_t num)
{
	int i = 0;
	struct gpio_property *gpiomap_on_off = (struct gpio_property *)data;

	seq_printf(m, "+--+------------+--pmic gpio_show----------+---\n");

	for (i = 0; i < num / 2; i++) {
		seq_printf(m, "|%-13s| value=0x%02x | %-7s | %-10s\n", gpiomap_on_off[i].name,
				   gpiomap_on_off[i].Onoff_value & 0xFF,
				   (gpiomap_on_off[i].Onoff_value >> 7) == 1 ? "enable" : "disable",
				   (gpiomap_on_off[i].Onoff_value & 0x01) == 1 ? "input_high" : "input_low"
				   );
	}

	for (i = num / 2; i < num; i++) {
		seq_printf(m, "|%-13s| invert=%d\n", gpiomap_on_off[i].name,
				gpiomap_on_off[i].Onoff_value >> 7);
	}

	seq_printf(m, "+--+-----------+-----+----+---+-----------+----+--\n");
	return 0;
}


static int pm845_ldo_show(struct seq_file *m, void *data, size_t num)
{
	int i = 0;
	struct ldo_property *ldo_on_off = (struct ldo_property *)data;

	seq_printf(m, "+--+------+----ldo_show: On/Off--+---+--------\n");

	for (i = 0; i < num; i++) {
		seq_printf(m, "|%-15s| on_off=0x%x\n", ldo_on_off[i].name,
			   (ldo_on_off[i].Onoff_value >> 7) & 0x01);
	}

	seq_printf(m, "+--+-------+-----+----+---+-----------+----+--\n");
	return 0;
}

static void pm845_ldo_store(struct device *ssbi_dev, void *data, size_t num)
{
	int index;
	uint8_t sid;
	uint8_t buf[1];
	uint16_t addr;
	uint8_t ret;

	struct ldo_property *ldo_on_off = (struct ldo_property *)data;

	for (index = 0;
		 index < (sizeof(pm845_reg_ldo_table) / sizeof(struct reg_property));
		 index++) {
		sid = (pm845_reg_ldo_table[index].regaddr >> 16) & 0xF;
		addr = pm845_reg_ldo_table[index].regaddr & 0xFFFF;

		ret = read_pmic_data(sid, addr, buf, 1);
		if (ret < 0) {
			pr_err("SPMI read failed, err = %d\n", ret);
			goto done;
		}
		ldo_on_off[index].name = pm845_reg_ldo_table[index].regname;
		ldo_on_off[index].Onoff_value = buf[0];
		pr_debug("%s ,  ret=%d, value=0x%x\n", __func__, ret, buf[0]);
	}

done:
	return;

}
static void pm845_gpio_store(struct device *dev, void *data, size_t num)
{
	int index;
	uint8_t sid;
	uint8_t buf[1];
	uint16_t addr;
	uint8_t ret;

	struct gpio_property *gpiomap_on_off = (struct gpio_property *)data;
	for (index = 0 ; 
		 index < (sizeof(pm845_reg_gpio_table)/sizeof(struct reg_property)); index++)
	{
		sid = (pm845_reg_gpio_table[index].regaddr >> 16 &0xF);
		addr = pm845_reg_gpio_table[index].regaddr & 0xFFFF;

		ret = read_pmic_data(sid,addr,buf,1);
		if (ret < 0)
		{
			pr_err("SPMI read failed,err = %d\n",ret);
			goto done;
		}
		
		gpiomap_on_off[index].name = pm845_reg_gpio_table[index].regname;
		gpiomap_on_off[index].Onoff_value = buf[0];
		pr_debug("%s,ret=%d,value=0x%x\n",__func__,ret,buf[0]);
	}

done:
	return;
}

static int pm845_gpio_show(struct seq_file *m,void *data,size_t num) 
{
	int i = 0;
	struct gpio_property *gpiomap_on_off = (struct gpio_property *)data;

	seq_printf(m, "+--+------------+--pmic gpio_show----------+---\n");
	for (i = 0; i < num / 2; i++) {
		seq_printf(m, "|%-13s| value=0x%02x | %-7s | %-10s\n", gpiomap_on_off[i].name,
				   gpiomap_on_off[i].Onoff_value & 0xFF,
				   (gpiomap_on_off[i].Onoff_value >> 7) == 1 ? "enable" : "disable",
				   (gpiomap_on_off[i].Onoff_value & 0x01) == 1 ? "input_high" : "input_low"
				   );
	}

	for (i = num / 2; i < num; i++) {
		seq_printf(m, "|%-13s| invert=%d\n", gpiomap_on_off[i].name,
				gpiomap_on_off[i].Onoff_value >> 7);
	}

	seq_printf(m, "+--+-----------+-----+----+---+-----------+----+--\n");
	return 0;
}


static int dump_current(struct seq_file *m,void *unused)
{
	struct dump_desc *dump_device = (struct dump_desc *)m->private;
	void *data;
	
	data = kcalloc(dump_device->item_count,dump_device->item_size,GFP_KERNEL);
	if (!data)
		return -ENOMEM;	

	dump_device->store(dump_device->dev,data,dump_device->item_count);
	dump_device->show(m,data,dump_device->item_count);
	kfree(data);
	return 0;
}

static int current_open(struct inode *inode, struct file *file)
{
	return single_open(file, dump_current, inode->i_private);
}

static struct file_operations current_fops = {
	.open = current_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};


static int dump_sleep(struct seq_file *m,void *unused)
{
	struct dump_desc *dump_device = (struct dump_desc *)m->private;
	void *data;

	pr_debug("%s dump_sleep,sleep_saved=%d, sleep_data=%p\n",__func__,sleep_saved,
			dump_device->sleep_data);

	if (sleep_saved && dump_device->sleep_data)
	{
		data = dump_device->sleep_data;
		dump_device->show(m,data,dump_device->item_count);	
		return 0;
	}else
	{
		seq_printf(m,"not recorded\n");
		return 0;
	}
}



static int sleep_open(struct inode *inode, struct file *file)
{
	return single_open(file, dump_sleep, inode->i_private);
}


static const struct file_operations sleep_fops = {
	.open = sleep_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int populate(struct dentry *base,const char *dir,
							struct dump_desc *dump_device)
{
	struct dentry *local_base;
	local_base = debugfs_create_dir(dir,base);
	if (!local_base)
		return -ENOMEM;

	if(!debugfs_create_file("current",0444,local_base,(void *)dump_device,&current_fops))
		return -ENOMEM;

	if(!debugfs_create_file("sleep",0444,local_base,(void *)dump_device,&sleep_fops))
		return -ENOMEM;

	return 0;
}

static int enable_set(void *data,u64 val)
{
	int i;
	debug_mask = (u32)val;
	sleep_saved = false;

	for (i = 0; i < DUMP_DEV_NUM; i++)
	{
		struct dump_desc *dump_device = &dump_devices[i];
		if (val)
		{
			if (dump_device->sleep_data)
				continue;
			dump_device->sleep_data = kcalloc(dump_device->item_count,
					dump_device->item_size,GFP_KERNEL);
			if (!dump_device->sleep_data)
				return -ENOMEM;
		}else {
			kfree(dump_device->sleep_data);
			dump_device->sleep_data = NULL;
		}
	}

	return 0;
}

static int enable_get(void *data,u64 *val)
{
	*val = (u64)debug_mask;
	sleep_saved = false;
	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(enable_fops, enable_get, enable_set, "0x%08llx\n");

static int __init power_debug_init(void)
{
	int ret = 0;	
	int i;
		
	debugfs = debugfs_create_dir("power_debug",NULL);
	if (!debugfs)
	{
		pr_err("can't create the debugfs dir power_debug\n");
		return -ENOMEM;
	}
	

    if (!debugfs_create_file("enable",0644,debugfs,NULL,&enable_fops)){
		ret = -ENOMEM;	
		goto fail;
    }
	
	for (i = 0; i < DUMP_DEV_NUM; i++)
	{
		struct dump_desc *dump_device = &dump_devices[i];
		ret = populate(debugfs,dump_device->name,dump_device);
		if (ret)
			goto fail;					
	}
	
	pr_debug("power debug init OK\n");
	return 0;

fail:
	debugfs_remove_recursive(debugfs);
	pr_err("power_debug_init failed\n");
	return ret;
}


void power_debug_collapse(void)
{
	int i;
	if (debug_mask) {
		pr_debug("%s save sleep state\n", __func__);
		for (i = 0; i < DUMP_DEV_NUM; i++) {
			struct dump_desc *dump_device = &dump_devices[i];
			if (dump_device->sleep_data)
				dump_device->store(dump_device->dev,
				dump_device->sleep_data,
				dump_device->item_count);
		}
		sleep_saved = true;
	}
}

EXPORT_SYMBOL(power_debug_collapse);

late_initcall(power_debug_init);
