#include "oled_template.h"
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

static SDD1306 *screen_oled;

SDD1306 *SDD1306_i2c_register(struct device *dev,
				struct i2c_client * client){

	SDD1306 * screen;

	if((screen = kzalloc(sizeof(SDD1306),GFP_KERNEL))==NULL)
		goto free_driver_NULL;
	if((screen->textBuffer = kzalloc(sizeof(char)*128,GFP_KERNEL))==NULL)
		goto free_driver_NULL;
	if((screen->screenBuffer = kzalloc(sizeof(char)*128,GFP_KERNEL))==NULL)
		goto free_driver_NULL;
	
	screen->columna = 0;
	screen->pagina = 0;
	
	screen->dev = get_device(dev);
	

	/*
	 * static inline void dev_set_drvdata(struct device *dev, void      * data){
	 *	dev->driver_data = data;
	 * }
	 *
	 * Con esta funcion referenciamos nuestra estructura dentro
	 * de la estructura del device. Para que esté todo bien documentado en el device tree.
	 *
	 */
	dev_set_drvdata(dev,screen);
	screen->client = client;
	i2c_set_clientdata(client,screen);
	return screen;
	
	if(SDD1306_i2c_add_device(screen)<0)
		goto free_driver;
	

free_driver_NULL:
	printk(KERN_ERR "Error al reservar memoria en el device\n");
	return NULL;

free_driver:
	printk(KERN_ERR "Error al crear miscdev\n");

	/*
	 * void put_device(struct device *dev){
	 *		if(dev)
	 *			kobject_put(&dev_kobj);
	 *
	 * }
	 * 
	 * void kobject_put(struct kobject *kobj);
	 *
	 * Si KRef del Kobj llega a 0, lanza la funcion
	 * release() asociada a dicho kobject. 
	 *
	put_device(dev);	
*/
	put_device(screen->dev);
	vfree(screen->textBuffer);
	vfree(screen->screenBuffer);
	vfree(screen);


}
int SDD1306_i2c_probe(struct i2c_client * client,
					const struct i2c_device_id *id){
	SDD1306 *screen;
	struct device * dev = &client->dev;

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
		        printk(KERN_ERR "%s: needed i2c functionality is not supported\n", __func__);
        return -ENODEV;	
	}else{
		printk(KERN_INFO "%s: i2c functionality supported\n", __func__);
	}
	
	if((screen = SDD1306_i2c_register(dev,client))==NULL){
		printk(KERN_ERR "Error creando la estructura i2c\n");	
	}else{
		screen_oled = screen;
	}
	return 0;
}

int SDD1306_i2c_remove(struct i2c_client *client){
	struct device *dev = &(client->dev);
	SDD1306 * screen = dev_get_drvdata(dev);

	if(screen){
		misc_deregister(&screen->miscdev);
	 	vfree(screen->textBuffer);
		vfree(screen->screenBuffer);	
	}
	
	put_device(dev);
	vfree(screen);

	screen_oled = NULL;

	return 0;



}
ssize_t SDD1306_i2c_write(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos){
		// Prueba de funcionamiento 
	char letra;

	if(copy_from_user(&letra,buf,1)){
			return -EFAULT;
	}

	printk(KERN_NOTICE "Valor de letra: %c\n",letra);

	return 1;



}
static const struct file_operations SDD1306_fops = {
	.owner = THIS_MODULE,
	.write = SDD1306_i2c_write,
};
int SDD1306_i2c_add_device(SDD1306 * screen){
	int ret = 0;

	screen->miscdev.minor = MISC_DYNAMIC_MINOR;
	screen->miscdev.name = "write_oled";
	screen->miscdev.fops = &SDD1306_fops;
	screen->miscdev.parent = screen->dev;
	ret = misc_register(&screen->miscdev);
	if(ret<0) 
		printk(KERN_ERR "misc_register failed\n");
	else 
		printk(KERN_NOTICE "misc_register OK.. minor=%d\n",ret);

	return ret;

}
static const struct i2c_device_id SDD1306_i2c_id[]={
	{"SDD1306_i2c",0},
	{ }
};

MODULE_DEVICE_TABLE(i2c, SDD1306_i2c_id);

static struct i2c_driver SDD1306_i2c_driver = {
	.probe	= SDD1306_i2c_probe,
   	.remove = SDD1306_i2c_remove,
	.id_table=SDD1306_i2c_id,
	.driver = {
		.owner	 = THIS_MODULE,
		.name	 = "SDD1306_i2c" 	
	},	
};
static int i2c_driver_init(void){
    return i2c_add_driver(&SDD1306_i2c_driver);
}

static void __exit i2c_driver_exit(void){
    i2c_del_driver(&SDD1306_i2c_driver);
    printk(KERN_INFO "%s: i2c client driver deleted\n", __func__);
}
module_init(i2c_driver_init);
module_exit(i2c_driver_exit);

MODULE_DESCRIPTION(DESCRIPTION);
MODULE_LICENSE(LICENSE);
