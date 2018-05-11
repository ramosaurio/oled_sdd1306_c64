#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h> // misc dev
#include <linux/i2c.h>
#include <linux/vmalloc.h>
#include "fonts/myc64_lower.h"



/****************************************************************************/
// Globals
/****************************************************************************/
#include "oled_SSD1306.h"

/****************************************************************************/
/* Module init / cleanup block.                                             */
/****************************************************************************/
static struct i2c_adapter * my_adap;
static struct i2c_client * my_client;


/****************************************************************************/
/* Buffer de escritura                                                      */
/****************************************************************************/

typedef struct _buffer  Buffer;

struct _buffer {
	char *charBuffer;
	int pagina;
	int columna;	

};

 
static Buffer *buffer;

/*****************************************/
/* static void write_buffer(char letra); */
/* se encarga de inroducir las letras en 
 * el buffer.
 * Aun hay que implementar cuando el buff
 * er esta lleno                        */

static void write_buffer(char letra){
	int pos;

	if(buffer->pagina==0) pos = buffer->columna;
	else pos = ((buffer->pagina)*16)+buffer->columna;
	


	if(buffer->columna==15 ){
		buffer->columna = 0;
		if(pagina<7)buffer->pagina++; 
	}else buffer->columna++;



	printk(KERN_INFO"POS- %d\n",pos);
	buffer->charBuffer[pos]=letra;
}
/****************************************************************************/
// write data byte througth i2c client
/****************************************************************************/
static void ssd1306_command(struct i2c_client * my_client, uint8_t value)
{
    uint8_t command = 0x00;   // Co = 0, D/C = 0
    i2c_smbus_write_byte_data(my_client, command, value);
}
/****************************************************************************/
// write data block througth i2c client
/****************************************************************************/
static void ssd1306_writedatablock(struct i2c_client * my_client, uint8_t * values, uint8_t length) // máximo de 16 ?
{
    uint8_t command = 0x40;   // Co = 0, D/C = 0
    i2c_smbus_write_i2c_block_data(my_client, command, length, values);
}
/***************************************************************************/
// set frame buffer to zero
/****************************************************************************/
static void clear_buffer(void)
{
	uint16_t i, clear;
    clear = 32*8;
	
   // memset(buffer, 0, (SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8));
	ssd1306_command(my_client, SSD1306_COLUMNADDR);
    ssd1306_command(my_client, 0);   // Column start address (0 = reset)
    ssd1306_command(my_client, SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

    ssd1306_command(my_client, SSD1306_PAGEADDR);
    ssd1306_command(my_client, 0); // Page start address (0 = reset)
    ssd1306_command(my_client, SSD1306_LCDHEIGHT/8 - 1); // Page end address// SSD1306_SWITCHCAPVCC porque lo vi en ejemplo

    for (i=0; i<(SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/4); i+=16)
    {
        ssd1306_writedatablock(my_client, font+clear,8);
    }
}

/****************************************************************************/
// set scroll on
/****************************************************************************/
static void scroll_izquierda(struct i2c_client * my_client)
{
	// consultar datasheet para ver parámetros del comando
	ssd1306_command(my_client,SSD1306_LEFT_HORIZONTAL_SCROLL);
    ssd1306_command(my_client,0x00); // dummy
    ssd1306_command(my_client,0x00); // inicio
    ssd1306_command(my_client,0x06); // velocidad
    ssd1306_command(my_client,0x07); // fin
    ssd1306_command(my_client,0x00); // dummy
    ssd1306_command(my_client,0xFF); // dummy    
    ssd1306_command(my_client,SSD1306_ACTIVATE_SCROLL);
}

/****************************************************************************/
// write frame buffer to display
/****************************************************************************/
static void display(struct i2c_client * my_client,uint16_t ascii)
{
   	ssd1306_writedatablock(my_client,font+ascii,8);  
}

/****************************************************************************/
// display initialization
/****************************************************************************/
static void init_sequence(struct i2c_client * my_client, uint8_t vccstate) // vccstate ?
{
    // Init sequence
    ssd1306_command(my_client, SSD1306_DISPLAYOFF);                    // 0xAE
    ssd1306_command(my_client, SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
    ssd1306_command(my_client, 0x80);                                  // the suggested ratio 0x80

    ssd1306_command(my_client, SSD1306_SETMULTIPLEX);                  // 0xA8
    ssd1306_command(my_client, SSD1306_LCDHEIGHT - 1);

    ssd1306_command(my_client, SSD1306_SETDISPLAYOFFSET);              // 0xD3
    ssd1306_command(my_client, 0x0);                                   // no offset
    ssd1306_command(my_client, SSD1306_SETSTARTLINE | 0x0);            // line #0
    ssd1306_command(my_client, SSD1306_CHARGEPUMP);                    // 0x8D
    if (vccstate == SSD1306_EXTERNALVCC)
    {
        ssd1306_command(my_client, 0x10);
    }
    else
    {
        ssd1306_command(my_client, 0x14);
    }
    ssd1306_command(my_client, SSD1306_MEMORYMODE);                    // 0x20
    ssd1306_command(my_client, 0x00);                                  // 0x0 act like ks0108
    ssd1306_command(my_client, SSD1306_SEGREMAP | 0x1);
    ssd1306_command(my_client, SSD1306_COMSCANDEC);

    //-- SSD1306_128_64
    ssd1306_command(my_client, SSD1306_SETCOMPINS);                    // 0xDA
    ssd1306_command(my_client, 0x12);
    ssd1306_command(my_client, SSD1306_SETCONTRAST);                   // 0x81
    if (vccstate == SSD1306_EXTERNALVCC)
    {
        ssd1306_command(my_client, 0x9F);
    }
    else
    {
        ssd1306_command(my_client, 0xCF);
    }
    //--

    ssd1306_command(my_client, SSD1306_SETPRECHARGE);                  // 0xd9
    if (vccstate == SSD1306_EXTERNALVCC)
    {
        ssd1306_command(my_client, 0x22);
    }
    else
    {
        ssd1306_command(my_client, 0xF1);
    }
    ssd1306_command(my_client, SSD1306_SETVCOMDETECT);                 // 0xDB
    ssd1306_command(my_client, 0x40);
    ssd1306_command(my_client, SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    ssd1306_command(my_client, SSD1306_NORMALDISPLAY);                 // 0xA6

    ssd1306_command(my_client, SSD1306_DEACTIVATE_SCROLL);

    ssd1306_command(my_client, SSD1306_DISPLAYON);//--turn on oled panel
	ssd1306_command(my_client, SSD1306_COLUMNADDR);
    ssd1306_command(my_client, 0);   // Column start address (0 = reset)
    ssd1306_command(my_client, SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

    ssd1306_command(my_client, SSD1306_PAGEADDR);
    ssd1306_command(my_client, 0); // Page start address (0 = reset)
    ssd1306_command(my_client, SSD1306_LCDHEIGHT/8 - 1); // Page end address// SSD1306_SWITCHCAPVCC porque lo vi en ejemplo
} 



static ssize_t sdd1306_write(struct file *file, const char __user *buf,
                          size_t count, loff_t *ppos)
{
	char letra;

	uint16_t ascichar;

    if (copy_from_user( &letra, buf, 1 )) {
        return -EFAULT;
    }
	
	write_buffer(letra);
	
    printk(KERN_INFO " BUFFER-> )%s",buffer->charBuffer);
	printk(KERN_INFO "C: %d\tP: %d\n",buffer->columna,buffer->pagina);
    printk(KERN_INFO " uinst16_6 %d\n",(uint16_t)letra);	
	ascichar = (uint16_t)letra;
	ascichar = ascichar*8;
	display(my_client,ascichar);
	
	return 1;
}


static const struct file_operations sdd1306_fops = {
	.owner	= THIS_MODULE,
	.write	= sdd1306_write,
};
/****************************************************************************/
/* device struct                                                            */
/****************************************************************************/
static struct miscdevice sdd1306_miscdev = {
    .minor	= MISC_DYNAMIC_MINOR,
    .name	= "write_oled",
    .fops   = &sdd1306_fops,
};


static int r_dev_config_sdd1306(void){
	int ret=0;
	ret = misc_register(&sdd1306_miscdev);
	if(ret < 0){
	    printk(KERN_ERR "misc_register failed\n");
			
	}else{
		printk(KERN_NOTICE"misc_register OK... b_miscdev.minor=%d\n",ret);
	}
	
	return ret;
}
void r_cleanup(void)
{
	printk(KERN_NOTICE "%s module cleaning up...\n", KBUILD_MODNAME);
    if (sdd1306_miscdev.this_device) misc_deregister(&sdd1306_miscdev);
	vfree(buffer->charBuffer);
	vfree(buffer);
    printk(KERN_NOTICE "Unregistering i2c client\n");
    printk(KERN_NOTICE "Clearing buffer...\n");
    clear_buffer();
    printk(KERN_NOTICE "Write buffer to display...\n");
    ssd1306_command(my_client,SSD1306_DEACTIVATE_SCROLL);
 
    i2c_unregister_device(my_client);

	printk(KERN_NOTICE "Done. Bye from %s module\n", KBUILD_MODNAME);
    return;

}

int r_init(void)
{
	int res = 0;
	printk(KERN_NOTICE "Hello, loading %s module\n",KBUILD_MODNAME);
    printk(KERN_NOTICE "%s - devices config...\n", KBUILD_MODNAME);
        if((res = r_dev_config_sdd1306()))
    {
		r_cleanup();
		return res;
	}
 	printk(KERN_NOTICE "%s - Buffer config ... \n",KBUILD_MODNAME);
	buffer = (Buffer* )vmalloc(sizeof(Buffer));
	buffer->charBuffer = (char*)vmalloc(sizeof(char)*128);
	buffer->pagina = 0;
	buffer->columna = 0;
	
	memset((buffer->charBuffer),'\0',sizeof(char)*128);
    printk(KERN_NOTICE "Connecting to adapter i2c-1\n");
    my_adap = i2c_get_adapter(1); // 1 means i2c-1 bus
    if(my_adap==NULL)
    {
		printk(KERN_ERR "ERROR: No i2c adapter found\n");
		return -ENODEV;
	}
    printk(KERN_NOTICE "Creating new i2c client at 0x3c address\n");
    my_client = i2c_new_dummy (my_adap, SSD1306_I2C_ADDRESS);  //  i2c address = 0x3c
    if(my_client==NULL)
    {
		printk(KERN_ERR "ERROR: No client created\n");
		return -ENODEV;
	}
    printk(KERN_NOTICE "Display initialization\n");
    init_sequence(my_client, SSD1306_SWITCHCAPVCC); 
  
	printk(KERN_NOTICE "%s - Fin de la configuracion inicial\n",KBUILD_MODNAME);
	return res;
	
}


module_init(r_init);
module_exit(r_cleanup);

/****************************************************************************/
/* Module licensing/description block.                                      */
/****************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DAC");
MODULE_DESCRIPTION("Simple module for oled SDD1306 128x64 i2c initialization");
