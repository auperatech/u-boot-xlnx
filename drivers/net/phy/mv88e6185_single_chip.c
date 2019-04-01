/*
 * PHY driver for mv88e6185 ethernet switches of auper v205.
 * create from mv88e61xx.c, change to single chip mode.
 * jian.zhang@auperastor.com
  */

#include <common.h>

#include <bitfield.h>
#include <errno.h>
#include <malloc.h>
#include <miiphy.h>
#include <netdev.h>

#define PHY_AUTONEGOTIATE_TIMEOUT	5000

#define PORT_COUNT			11
#define PORT_MASK			((1 << PORT_COUNT) - 1)

/* Device addresses */
#define DEVADDR_PHY(p)			(p)
#define DEVADDR_PORT(p)			(0x10 + (p))
#define DEVADDR_SERDES			0x0F
#define DEVADDR_GLOBAL_1		0x1B
#define DEVADDR_GLOBAL_2		0x1C

/* Global registers */
#define GLOBAL1_STATUS			0x00
#define GLOBAL1_CTRL			0x04
#define GLOBAL1_MON_CTRL		0x1A

/* Global 2 registers */
#define GLOBAL2_REG_PHY_CMD		0x18
#define GLOBAL2_REG_PHY_DATA		0x19

/* Port registers */
#define PORT_REG_STATUS			0x00
#define PORT_REG_PHYS_CTRL		0x01
#define PORT_REG_SWITCH_ID		0x03
#define PORT_REG_CTRL			0x04
#define PORT_REG_VLAN_MAP		0x06
#define PORT_REG_VLAN_ID		0x07

/* Phy registers */
#define PHY_REG_CTRL1			0x10
#define PHY_REG_STATUS1			0x11
#define PHY_REG_PAGE			0x16

/* Serdes registers */
#define SERDES_REG_CTRL_1		0x10

/* Phy page numbers */
#define PHY_PAGE_COPPER			0
#define PHY_PAGE_SERDES			1

/* Register fields */
#define GLOBAL1_CTRL_SWRESET		BIT(15)

#define GLOBAL1_MON_CTRL_CPUDEST_SHIFT	4
#define GLOBAL1_MON_CTRL_CPUDEST_WIDTH	4

#define PORT_REG_STATUS_LINK		BIT(11)
#define PORT_REG_STATUS_DUPLEX		BIT(10)

#define PORT_REG_STATUS_SPEED_SHIFT	8
#define PORT_REG_STATUS_SPEED_WIDTH	2
#define PORT_REG_STATUS_SPEED_10	0
#define PORT_REG_STATUS_SPEED_100	1
#define PORT_REG_STATUS_SPEED_1000	2

#define PORT_REG_STATUS_CMODE_MASK		0xF
#define PORT_REG_STATUS_CMODE_100BASE_X		0x8
#define PORT_REG_STATUS_CMODE_1000BASE_X	0x9
#define PORT_REG_STATUS_CMODE_SGMII		0xa

#define PORT_REG_PHYS_CTRL_PCS_AN_EN	BIT(10)
#define PORT_REG_PHYS_CTRL_PCS_AN_RST	BIT(9)
#define PORT_REG_PHYS_CTRL_FC_VALUE	BIT(7)
#define PORT_REG_PHYS_CTRL_FC_FORCE	BIT(6)
#define PORT_REG_PHYS_CTRL_LINK_VALUE	BIT(5)
#define PORT_REG_PHYS_CTRL_LINK_FORCE	BIT(4)
#define PORT_REG_PHYS_CTRL_DUPLEX_VALUE	BIT(3)
#define PORT_REG_PHYS_CTRL_DUPLEX_FORCE	BIT(2)
#define PORT_REG_PHYS_CTRL_SPD1000	BIT(1)
#define PORT_REG_PHYS_CTRL_SPD_MASK	(BIT(1) | BIT(0))

#define PORT_REG_CTRL_PSTATE_SHIFT	0
#define PORT_REG_CTRL_PSTATE_WIDTH	2

#define PORT_REG_VLAN_ID_DEF_VID_SHIFT	0
#define PORT_REG_VLAN_ID_DEF_VID_WIDTH	12

#define PORT_REG_VLAN_MAP_TABLE_SHIFT	0
#define PORT_REG_VLAN_MAP_TABLE_WIDTH	11

#define SERDES_REG_CTRL_1_FORCE_LINK	BIT(10)

#define PHY_REG_CTRL1_ENERGY_DET_SHIFT	8
#define PHY_REG_CTRL1_ENERGY_DET_WIDTH	2

/* Field values */
#define PORT_REG_CTRL_PSTATE_DISABLED	0
#define PORT_REG_CTRL_PSTATE_FORWARD	3

#define PHY_REG_CTRL1_ENERGY_DET_OFF	0
#define PHY_REG_CTRL1_ENERGY_DET_SENSE_ONLY	2
#define PHY_REG_CTRL1_ENERGY_DET_SENSE_XMIT	3

/* PHY Status Register */
#define PHY_REG_STATUS1_SPEED		0xc000
#define PHY_REG_STATUS1_GBIT		0x8000
#define PHY_REG_STATUS1_100		0x4000
#define PHY_REG_STATUS1_DUPLEX		0x2000
#define PHY_REG_STATUS1_SPDDONE		0x0800
#define PHY_REG_STATUS1_LINK		0x0400
#define PHY_REG_STATUS1_ENERGY		0x0010

/* ID register values for different switch models */
#define PORT_SWITCH_ID_6185		0x1A70

static int mv88e6185_single_chip_read(struct phy_device *phydev, int dev, int reg)
{
	debug("call %s, %#x, %#x, phydev@0x%p, mdio_bus @0x%p, bus->read @0x%p\n", __FUNCTION__, dev, reg, phydev, phydev->bus, phydev->bus->read);
	return phydev->bus->read(phydev->bus, dev, MDIO_DEVAD_NONE, reg);
}

static int mv88e6185_single_chip_write(struct phy_device *phydev, int dev, int reg, u16 val)
{
	debug("call %s, %#x, %#x, %#x, phydev@0x%p, mdio_bus @0x%p, bus->write @0x%p\n", __FUNCTION__, dev, reg, val, phydev, phydev->bus, phydev->bus->write);
	return phydev->bus->write(phydev->bus, dev, MDIO_DEVAD_NONE, reg, val);
}

static int mv88e6185_single_chip_get_switch_id(struct phy_device *phydev)
{
	int res;
	printf("call %s\n", __FUNCTION__);
	res = mv88e6185_single_chip_read(phydev, 0x16, PORT_REG_SWITCH_ID);
	if (res < 0)
		return res;

	printf("mv88e6185_single_chip_get_switch_id=0x%x\n", res);
	return res & 0xfff0;
}

static int mv88e6185_single_probe(struct phy_device *phydev)
{
	/* switch not a phy, must not be reset by core phy code */
	phydev->flags |= PHY_FLAG_BROKEN_RESET;
	return 0;
}

static int mv88e6185_single_config(struct phy_device *phydev)
{
	int swid;
	int reg;
	int time;

	printf("call %s\n", __FUNCTION__);

	swid = mv88e6185_single_chip_get_switch_id(phydev);
	if (swid != PORT_SWITCH_ID_6185){
		printf("swid = 0x%x, not 0x%x\n", __FUNCTION__, swid, PORT_SWITCH_ID_6185);
		return -1;
	}

	/*soft reset*/
	reg = mv88e6185_single_chip_read(phydev, DEVADDR_GLOBAL_1, GLOBAL1_CTRL);
	if (reg < 0) return reg;
	reg |= GLOBAL1_CTRL_SWRESET;
	reg = mv88e6185_single_chip_write(phydev, DEVADDR_GLOBAL_1, GLOBAL1_CTRL, reg);
	for (time = 1000; time; time--) {
		reg = mv88e6185_single_chip_read(phydev, DEVADDR_GLOBAL_1, GLOBAL1_CTRL);
		if (reg >= 0 && ((reg & GLOBAL1_CTRL_SWRESET) == 0))	break;
		udelay(1000);
	}

	/*port 6/7/8/9 enable forwarding*/
	mv88e6185_single_chip_write(phydev, 0x16, 0x4, 0x77);
	mv88e6185_single_chip_write(phydev, 0x17, 0x4, 0x77);
	mv88e6185_single_chip_write(phydev, 0x18, 0x4, 0x77);
	mv88e6185_single_chip_write(phydev, 0x19, 0x4, 0x77);

   	/*1000basex uplink port 7/8 link auto nego*/
	reg = mv88e6185_single_chip_read(phydev, 0x17, 0x1);
   	printf ("[%s] port7 original PCS ctrl: 0x%x", __FUNCTION__, reg);
	reg = (reg | 0x600) & 0xffff;	//PCS Inband Auto-Negotiation Enable, and Restart PCS Inband Auto-Negotiation
	mv88e6185_single_chip_write(phydev, 0x17, 0x1, reg);
   	printf ("[%s] port7 set PCS ctrl= 0x%x", __FUNCTION__, reg);

	reg = mv88e6185_single_chip_read(phydev, 0x18, 0x1);
   	printf ("[%s] port8 original PCS ctrl: 0x%x", __FUNCTION__, reg);
	reg = (reg | 0x600) & 0xffff;	//PCS Inband Auto-Negotiation Enable, and Restart PCS Inband Auto-Negotiation
	mv88e6185_single_chip_write(phydev, 0x18, 0x1, reg);
   	printf ("[%s] port8 set PCS ctrl= 0x%x", __FUNCTION__, reg);

   	/*cpu port 6/9 set to auto nego then fixed link*/
/*
	reg = mv88e6185_single_chip_read(phydev, 0x17, PORT_REG_PHYS_CTRL);
	reg &= ~(PORT_REG_PHYS_CTRL_SPD_MASK | PORT_REG_PHYS_CTRL_FC_VALUE);
	reg |= (PORT_REG_PHYS_CTRL_PCS_AN_EN | PORT_REG_PHYS_CTRL_PCS_AN_RST | PORT_REG_PHYS_CTRL_FC_FORCE | PORT_REG_PHYS_CTRL_DUPLEX_VALUE | PORT_REG_PHYS_CTRL_DUPLEX_FORCE | PORT_REG_PHYS_CTRL_SPD1000);
	mv88e6185_single_chip_write(phydev, 0x17, PORT_REG_PHYS_CTRL, reg);

	reg = mv88e6185_single_chip_read(phydev, 0x18, PORT_REG_PHYS_CTRL);
	reg &= ~(PORT_REG_PHYS_CTRL_SPD_MASK | PORT_REG_PHYS_CTRL_FC_VALUE);
	reg |= (PORT_REG_PHYS_CTRL_PCS_AN_EN | PORT_REG_PHYS_CTRL_PCS_AN_RST | PORT_REG_PHYS_CTRL_FC_FORCE | PORT_REG_PHYS_CTRL_DUPLEX_VALUE | PORT_REG_PHYS_CTRL_DUPLEX_FORCE | PORT_REG_PHYS_CTRL_SPD1000);
	mv88e6185_single_chip_write(phydev, 0x18, PORT_REG_PHYS_CTRL, reg);
*/
	reg = mv88e6185_single_chip_read(phydev, 0x16, PORT_REG_PHYS_CTRL);
	reg &= ~(PORT_REG_PHYS_CTRL_SPD_MASK | PORT_REG_PHYS_CTRL_FC_VALUE);
	reg |= (PORT_REG_PHYS_CTRL_PCS_AN_EN | PORT_REG_PHYS_CTRL_PCS_AN_RST | PORT_REG_PHYS_CTRL_FC_FORCE | PORT_REG_PHYS_CTRL_DUPLEX_VALUE | PORT_REG_PHYS_CTRL_DUPLEX_FORCE | PORT_REG_PHYS_CTRL_SPD1000);
	mv88e6185_single_chip_write(phydev, 0x16, PORT_REG_PHYS_CTRL, reg);

	reg = mv88e6185_single_chip_read(phydev, 0x19, PORT_REG_PHYS_CTRL);
	reg &= ~(PORT_REG_PHYS_CTRL_SPD_MASK | PORT_REG_PHYS_CTRL_FC_VALUE);
	reg |= (PORT_REG_PHYS_CTRL_PCS_AN_EN | PORT_REG_PHYS_CTRL_PCS_AN_RST | PORT_REG_PHYS_CTRL_FC_FORCE | PORT_REG_PHYS_CTRL_DUPLEX_VALUE | PORT_REG_PHYS_CTRL_DUPLEX_FORCE | PORT_REG_PHYS_CTRL_SPD1000);
	mv88e6185_single_chip_write(phydev, 0x19, PORT_REG_PHYS_CTRL, reg);
/*
	mv88e6185_single_chip_write(phydev, 0x16, 0x1, 0x83E);
	mv88e6185_single_chip_write(phydev, 0x19, 0x1, 0x83E);
	udelay(500000);

	mv88e6185_single_chip_write(phydev, 0x16, 0x1, 0x603);
	mv88e6185_single_chip_write(phydev, 0x19, 0x1, 0x603);
	udelay(500000);

	mv88e6185_single_chip_write(phydev, 0x16, 0x1, 0x83E);
	mv88e6185_single_chip_write(phydev, 0x19, 0x1, 0x83E);
	udelay(500000);

	mv88e6185_single_chip_write(phydev, 0x16, 0x1, 0x3E);
	mv88e6185_single_chip_write(phydev, 0x19, 0x1, 0x3E);
	udelay(500000);
*/

	/*soft reset*/
	reg = mv88e6185_single_chip_read(phydev, DEVADDR_GLOBAL_1, GLOBAL1_CTRL);
	if (reg < 0) return reg;
	reg |= GLOBAL1_CTRL_SWRESET;
	reg = mv88e6185_single_chip_write(phydev, DEVADDR_GLOBAL_1, GLOBAL1_CTRL, reg);
	for (time = 1000; time; time--) {
		reg = mv88e6185_single_chip_read(phydev, DEVADDR_GLOBAL_1, GLOBAL1_CTRL);
		if (reg >= 0 && ((reg & GLOBAL1_CTRL_SWRESET) == 0))	break;
		udelay(1000);
	}

	return 0;
}


static int mv88e6185_single_update_link(struct phy_device *phydev)
{
	unsigned int speed;
	unsigned int mii_reg;
	int val;
	int link = 0, link6 = 0, link7 = 0, link8 = 0, link9 = 0;

	printf("call %s\n", __FUNCTION__);

	val = mv88e6185_single_chip_read(phydev, 0x16, PORT_REG_STATUS);
	if (val < 0) return 0;
	link6 = (val & PORT_REG_STATUS_LINK) == 0;

	val = mv88e6185_single_chip_read(phydev, 0x17, PORT_REG_STATUS);
	if (val < 0) return 0;
	link7 = (val & PORT_REG_STATUS_LINK) == 0;

	val = mv88e6185_single_chip_read(phydev, 0x18, PORT_REG_STATUS);
	if (val < 0) return 0;
	link8 = (val & PORT_REG_STATUS_LINK) == 0;

	val = mv88e6185_single_chip_read(phydev, 0x19, PORT_REG_STATUS);
	if (val < 0) return 0;
	link9 = (val & PORT_REG_STATUS_LINK) == 0;

	printf("port=%d, link 6/7/8/9=%d/%d/%d/%d\n", phydev->port, link6, link7, link8, link9);

	if (phydev->port == 0){	//eth0 -> 88e6185 port 6
		link = (link6 && (link7 || link8));
	}else if(phydev->port == 1){	//eth1 -> 88e6185 port 9
		link = (link9 && (link7 || link8));
	}

	if (link){
			phydev->link = 1;
			phydev->duplex = DUPLEX_FULL;
			phydev->speed = SPEED_1000;
	}else{
			phydev->link = 0;
	}

	return 0;
}

static int mv88e6185_single_startup(struct phy_device *phydev)
{
	int i;
	int link = 0;
	int res;
	int speed = phydev->speed;
	int duplex = phydev->duplex;
	int link6 = 0, link7 = 0, link8 = 0, link9 = 0;
	// port 6 is eth0=gem2, port 9 is eth1=gem3, normally we use eth0
	// port 7 & 8 is uplink to dx3336

	int val;

	printf("call %s\n", __FUNCTION__);
	val = mv88e6185_single_chip_read(phydev, 0x16, PORT_REG_STATUS);
	if (val < 0) return 0;
	link6 = (val & PORT_REG_STATUS_LINK) == 0;

	val = mv88e6185_single_chip_read(phydev, 0x17, PORT_REG_STATUS);
	if (val < 0) return 0;
	link7 = (val & PORT_REG_STATUS_LINK) == 0;

	val = mv88e6185_single_chip_read(phydev, 0x18, PORT_REG_STATUS);
	if (val < 0) return 0;
	link8 = (val & PORT_REG_STATUS_LINK) == 0;

	val = mv88e6185_single_chip_read(phydev, 0x19, PORT_REG_STATUS);
	if (val < 0) return 0;
	link9 = (val & PORT_REG_STATUS_LINK) == 0;

	printf("[%s] port=%d, link 6/7/8/9=%d/%d/%d/%d\n", __FUNCTION__, phydev->port, link6, link7, link8, link9);

	if (phydev->port == 0){	//eth0 -> 88e6185 port 6
		link = (link6 && (link7 || link8));
	}else if(phydev->port == 1){	//eth1 -> 88e6185 port 9
		link = (link9 && (link7 || link8));
	}

	//if (link){
	//	res = mv88e6185_single_update_link(phydev);
	//}

	if (link){
			printf("phydev=@%p\n", phydev);
			phydev->link = 1;
			phydev->duplex = DUPLEX_FULL;
			phydev->speed = SPEED_1000;
	}else{
			phydev->link = 0;
	}

	return 0;
}

/** single chip mode 88e6185 init for v205 A0/A1 board only*/
static struct phy_driver mv88e6185_single_chip_driver = {
	.name = "Marvell MV88E6185 SingleChip",
	.uid = 0x088e6185,
	.mask = 0xffffffff,
	.features = PHY_GBIT_FEATURES,
	.probe = mv88e6185_single_probe,
	.config = mv88e6185_single_config,
	.startup = mv88e6185_single_startup,
	.shutdown = &genphy_shutdown,
};

int phy_mv88e6185_single_chip_init(void)
{
	printf("call %s, mv88e6185_single_chip_driver @0x%p\n", __FUNCTION__, mv88e6185_single_chip_driver);
	phy_register(&mv88e6185_single_chip_driver);

	return 0;
}

/*
 * Overload weak get_phy_id definition since we need non-standard functions
 * to read PHY registers
 */

/**my 88e6185 single chip mode get phy id*/
int get_phy_id(struct mii_dev *bus, int smi_addr, int devad, u32 *phy_id)
{
	int phy_reg;
	int addr, found6185;

	printf("[%s, %s] devad=%d\n", __FILE__, __FUNCTION__, devad);

	//check port 1-9 at address 0x10-0x19, register 3 is product identifier == (0x1a72 & 0xfff0)
	found6185=1;
	for(addr=0x10; addr<=0x19; addr++){
		phy_reg = bus->read(bus, addr, -1, 3);
		//printf("addr%#x %#x,",addr, phy_reg);
		if ((phy_reg & 0xfff0) != (0x1a72 & 0xfff0)){
			found6185=0;
			*phy_id = 0;
			break;
		}
	}
	if (found6185){
		*phy_id = 0x088e6185;
		printf("[%s, %s] set phy_id as 88e6185\n", __FILE__, __FUNCTION__);
		return 0;
	}

	phy_reg = bus->read(bus, smi_addr, devad, MII_PHYSID1);
	if (phy_reg < 0) return -EIO;
	*phy_id = (phy_reg & 0xffff) << 16;

	phy_reg = bus->read(bus, smi_addr, devad, MII_PHYSID2);
	if (phy_reg < 0) return -EIO;
	*phy_id |= (phy_reg & 0xffff);

	return 0;

/*
	phy_reg = bus->read(bus, 0, -1, MII_PHYSID1);

	phy_reg = bus->read(bus, 0, -1, MII_PHYSID1);

	if (phy_reg < 0) return -EIO;

	id1 = (phy_reg & 0xffff) << 16;

	phy_reg = bus->read(bus, 0, -1, MII_PHYSID2);

	if (phy_reg < 0) return -EIO;

	id2 = (phy_reg & 0xfff0);

	if (0x01411a70 == (id1 | id2)){

		int i,j;
		for (i=0; i<32; i++){
			printf("\nmdio addr:%d\n",i);
			for (j=0; j<32;j++){
				phy_reg = bus->read(bus, i, -1, j);
				printf("%x=%x,", j, phy_reg);
			}
		}
	}

	int val;

	printf("mii bus @0x%p, bus->priv =@0x%p, bus->name =%s\n", bus, bus->priv, bus->name);

	val = mv88e6185_single_chip_phy_read_direct(bus, 0, MII_PHYSID1);
	printf("MII_PHYSID1=0x%x\n", val);
	if (val < 0)
		return -EIO;

	*phy_id = val << 16;

	val = mv88e6185_single_chip_phy_read_direct(bus, 0, MII_PHYSID2);
	printf("MII_PHYSID2=0x%x\n", val);
	if (val < 0)
		return -EIO;

	*phy_id |= (val & 0xffff);

	return 0;
*/
}
