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
		buffer->pagina++;
	}else buffer->columna++;



	printk(KERN_INFO"POS- %d\n",pos);
	buffer->charBuffer[pos]=letra;
}
static ssize_t sdd1306_write(struct file *file, const char __user *buf,
                          size_t count, loff_t *ppos)
{
	char letra;

    if (copy_from_user( &letra, buf, 1 )) {
        return -EFAULT;
    }
	write_buffer(letra);
    printk(KERN_INFO " BUFFER-> )%s",buffer->charBuffer);
	printk(KERN_INFO "C: %d\tP: %d\n",buffer->columna,buffer->pagina);
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
