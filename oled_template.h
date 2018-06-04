#ifndef _OLED_TEMPLATE_H
#define _OLED_TEMPLATE_H
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h> 
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h> 
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/vmalloc.h> 
#include <linux/proc_fs.h>
#include <linux/uaccess.h> 
#include <linux/printk.h>
#include <linux/list.h> 
#include <linux/delay.h> 
#include <linux/gpio.h>
#include <linux/sched.h> 
#include <linux/fs.h>
#include <linux/timer.h>
#include "fonts/myc64_lower.h"
#include "oled_SSD1306.h"

#define DESCRIPTION "I2C SDD1306 OLED PRINT"
#define LICENSE "GPL"
#define DEVICENAME "SDD1306_I2C"
#define COLSIZE 16
#define ROWSIZE 8
#define CHARSIZE 8 

#define GPIO_BUTTON_UP 10
#define GPIO_BUTTON_DOWN 17
#define GPIO_BUTTON_LEFT 22
#define GPIO_BUTTON_RIGHT 23

#define GPIO_BUTTON_UP_DESC "Cursor arriba"
#define GPIO_BUTTON_DOWN_DESC "Cursor abajo"
#define GPIO_BUTTON_LEFT_DESC "Cursor izquierda"
#define GPIO_BUTTON_RIGHT_DESC "Cursor derecha"

#define GPIO_BUTTON_DEVICE_DESC "CURSOR"

struct _cursor_oled {
	int pagina;
	int columna;

};

typedef struct _cursor_oled cursor_oled;

struct oled_device {
	struct i2c_client *client;
	struct device *dev;
	cursor_oled * cursor;
	char *textBuffer;
	uint8_t *screenBuffer;

	int pagina;
	int columna;	
	
	struct miscdevice miscdev;
};

typedef struct oled_device SDD1306;

/* I2C Detection */ 

int SDD1306_i2c_probe(struct i2c_client * client, 
				const struct i2c_device_id *id);
int SDD1306_i2c_remove(struct i2c_client *client);

/* Device Registration */
/*
 * Linux Kernel Development Pagina Cap(17) 
 * Pagina 348 - The Device Model 
 *	
 * El device model nos proporciona un mecanismo
 * de representacion y descripcion de la topologia
 * de los Devices. 
 *
 * Su objetivo inicial fue proveer de un Device Tree
 * preciso y potente.
 *
 * KOBJECTS - Kernel Objects
 *
 * struct kobjet <linux/kobject.h>
 *
 * Crea una jerarquía de objetos de padres e hijos 
 *
 * struct kobject {
 *	const char 	*name         --> Nombre del Kobject
 *	struct list_head	entry
 *	struct kobject	*parent
 *	struct kset	*kset         --> Ptr. al padre.
 *	struct kobj_type	*ktype
 *	struct sysfs_dirent	*sd    --> Ref. al iNode
 *	struct kref	kref
 *	unsigned int 	state_initialized:1
 *	unsigned int	state_in_sysfs:1
 *	unsigned int	state_add_uevent_sent:1
 *	unsigned int	state_remove_uevent_setn:1
 *	unsigned int uevent_suppress:1
 *	
 * }
 *
 * KTYPES 
 *
 * Los Kobjects estan asociados a un tipo especifico
 * llamado KTYPES que estan definidos en
 * <linux/ktypes.h>
 *
 * struct kobj_type {
 *	void (*release)(struct kobject *);
 *	const struct sysfs_ops *sysfs_ops;
 *	struct attribute **default_attrs;
 *
 * }
 *
 * Este debe describir el comportamiento
 * de uan familia de Kobjects
 *
 * La funcion release se encarga de deshacer
 * la estructura cuando la referencia de 
 * un Kobject llegue a 0 , liberandolo del sistema
 * como un free.
 *
 * attribute hace referencia a un array de atributos.
 *
 * KSETS - kernel object sets
 *
 * Son coleccioines de Kobjects
 * evitamos tener KOBJECTS repetidos
 *
 * struct kset {
 *
 *	struct list_head 	list;  --> lista de ksets
 *	spinlock_t	list_lock;   --> Proteccion de escritura
 *	struct kobject 	kobj;   --> La base de la coleccion
 *	struct kset_uevent_ops *uevent_ops; --> para user events
 * }
 *
 * Todo esto no es al azar si no que el struct device *dev
 * es practicamente lo mismo que un KObject que se añade 
 * al Device Tree y solo que al ser de un device se le da 
 * su propia estructura
 * */
SDD1306* SSD1306_i2c_register(struct device * dev, 
				struct i2c_client * client);
int SDD1306_i2c_add_device(SDD1306 * screen);


void ssd1306_command(struct i2c_client * my_client, uint8_t value);
void sdd1306_writedatablock(struct i2c_client * my_client, uint8_t *values, uint8_t length);

void SDD1306_clear_buffer(struct i2c_client *my_client);
void SDD1306_display(struct i2c_client * my_client,uint16_t ascii);
void SDD1306_init_config_screen(struct i2c_client * my_client, uint8_t vccstate);
void write_struct(char letra);
void SDD1306_zero_init(struct i2c_client *my_client,SDD1306* SDDBUFFER);
void SDD1306_print(SDD1306*SDDBUFFER);
int SDD1306_scrollup(void);
void ioctl_clean_command(void);
void SDD1306_cambiar_ptr(int pagina,int columna);
#endif
