#include "oled_template.h"
#include "oled_ioctl.h"
static SDD1306 *screen_oled;

void write_struct(char letra){
	int posText,posScreen,scroll_real;
	uint16_t ascii,i;
	
	scroll_real = 0;
	ascii = (uint16_t) letra;
	ascii = ascii*8;
	if(screen_oled->pagina == 0){
		posText = screen_oled->columna;
		posScreen = screen_oled->columna * 8;

	}else{
		posText = (((screen_oled->pagina)%8)*16)+screen_oled->columna;
		if(screen_oled->pagina <=7) posScreen = (((screen_oled->pagina)*128)+ (screen_oled->columna*8));
		else posScreen = (7*128)+(screen_oled->columna*8);
	}
	screen_oled->textBuffer[posText] = letra;

	for(i = 0; i<8;i++){
		screen_oled->screenBuffer[ posScreen+i] = font[ascii+i];
	}
	if(((int)letra)==10){
   		if((screen_oled->pagina>=7)){
		//scroll_real = SDD1306_scrollup();
		scroll_real = SDD1306_scrollup();
		screen_oled->columna = 0;
		screen_oled->pagina +=2;
		}else{
			screen_oled->columna= 0;
			screen_oled->pagina++;
		}
	 SDD1306_cambiar_ptr(((screen_oled->pagina)%8),screen_oled->columna);
	
	}else{
		if(screen_oled->columna == 15){
			if(screen_oled->pagina>=7) (scroll_real = SDD1306_scrollup());
				screen_oled->columna = 0;
	 			screen_oled->pagina++;	
		}else{
			screen_oled->columna++;
		}
		if(!scroll_real){			
			SDD1306_display(screen_oled->client,ascii);	
	
		}
	}

}
void SDD1306_cambiar_ptr(int pagina, int columna){
	struct i2c_client * my_client = screen_oled->client;	
	
	pagina = screen_oled->pagina;
	columna = screen_oled->columna;
	ssd1306_command(my_client,SSD1306_COLUMNADDR);
	ssd1306_command(my_client,columna);
    ssd1306_command(my_client, SSD1306_LCDWIDTH-1); // Column end address (127 = reset)
	ssd1306_command(my_client,SSD1306_PAGEADDR);
	ssd1306_command(my_client,pagina);
	ssd1306_command(my_client,7);


}

void SDD1306_print(SDD1306*SDDBUFFER){
	int i;
	for(i =0 ; i<1024;i++){
			printk(KERN_CONT"%x ",SDDBUFFER->screenBuffer[i]);		
	}
}

int SDD1306_scrollup(void){
	int i;
	uint8_t *ptrBuffer = screen_oled->screenBuffer+128;
	struct i2c_client * my_client = screen_oled->client;
	memcpy(screen_oled->screenBuffer,ptrBuffer,((COLSIZE*ROWSIZE*CHARSIZE)-(COLSIZE*ROWSIZE)));
	memset(screen_oled->screenBuffer+((COLSIZE*ROWSIZE)*(CHARSIZE-1)),0,(COLSIZE*ROWSIZE));
	ssd1306_command(my_client, SSD1306_COLUMNADDR);
    ssd1306_command(my_client, 0);   // Column start address (0 = reset)
    ssd1306_command(my_client, SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

    ssd1306_command(my_client, SSD1306_PAGEADDR);
    ssd1306_command(my_client, 0); // Page start address (0 = reset)
    ssd1306_command(my_client, SSD1306_LCDHEIGHT/8 - 1); // Page end address// SSD1306_SWITCHCAPVCC porque lo vi en ejemplo

	for(i=0;i<(SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8);i+=16){
		sdd1306_writedatablock(my_client,screen_oled->screenBuffer+i,16);
	}
	ssd1306_command(my_client,SSD1306_COLUMNADDR);
	ssd1306_command(my_client,0);
    ssd1306_command(my_client, SSD1306_LCDWIDTH-1); // Column end address (127 = reset)
	ssd1306_command(my_client,SSD1306_PAGEADDR);
	ssd1306_command(my_client,7);
	ssd1306_command(my_client,7);

	return 1;

}
void ssd1306_command(struct i2c_client * my_client, uint8_t value){
 uint8_t command = 0x00;   // Co = 0, D/C = 0
    i2c_smbus_write_byte_data(my_client, command, value);

}

void sdd1306_writedatablock(struct i2c_client * my_client, uint8_t *values, uint8_t length){

    uint8_t command = 0x40;   // Co = 0, D/C = 0
    i2c_smbus_write_i2c_block_data(my_client, command, length, values);
}

void SDD1306_clear_buffer(struct i2c_client *my_client){
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
        sdd1306_writedatablock(my_client, font+clear,8);
    }
}
void SDD1306_display(struct i2c_client *my_client,uint16_t ascii){
	sdd1306_writedatablock(my_client,font+ascii,8);
}

void SDD1306_zero_init(struct i2c_client *my_client,SDD1306 *SDDBUFFER){
	int i;
	memset(SDDBUFFER->screenBuffer,0,128);
	for(i=0;i<(SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8);i+=16){
		sdd1306_writedatablock(my_client,SDDBUFFER->screenBuffer+i,16);
	}
}
void SDD1306_init_config_screen(struct i2c_client * my_client, uint8_t vccstate){
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
SDD1306 *SDD1306_i2c_register(struct device *dev,
				struct i2c_client * client){

	SDD1306 * screen;
	int ret;
	printk(KERN_INFO "Registrando i2c_struct\n");
	if((screen = kzalloc(sizeof(SDD1306),GFP_KERNEL))==NULL)
		goto free_driver_NULL;
	if((screen->textBuffer = kzalloc(sizeof(char)*128,GFP_KERNEL))==NULL)
		goto free_driver_NULL;
	if((screen->screenBuffer = kzalloc(sizeof(char)*1024,GFP_KERNEL))==NULL)
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

	
	if((ret=SDD1306_i2c_add_device(screen))<0)
		goto free_driver;
	printk(KERN_NOTICE "i2c_struct_SDD1306 COMPLETED\n");
	return screen;
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
	return NULL;
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
	printk(KERN_NOTICE "Display initialization\n");
	SDD1306_init_config_screen(screen->client,SSD1306_SWITCHCAPVCC);
	SDD1306_zero_init(screen->client,screen);

	return 0;
}

int SDD1306_i2c_remove(struct i2c_client *client){
	struct device *dev = &(client->dev);
	SDD1306 * screen = dev_get_drvdata(dev);
	SDD1306_clear_buffer(client);
		misc_deregister(&screen->miscdev);
	 	

		vfree(screen->textBuffer);
		vfree(screen->screenBuffer);	


	put_device(dev);
	printk(KERN_NOTICE"PASO5\n");
	vfree(screen);
	printk(KERN_NOTICE"PASO6\n");

	screen_oled = NULL;

	return 0;



}
void ioctl_clean_command(void){
	int i;
	struct i2c_client * my_client = screen_oled->client;
	memset(screen_oled->screenBuffer,0,1024);
	ssd1306_command(my_client, SSD1306_COLUMNADDR);
    ssd1306_command(my_client, 0);   // Column start address (0 = reset)
    ssd1306_command(my_client, SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

    ssd1306_command(my_client, SSD1306_PAGEADDR);
    ssd1306_command(my_client, 0); // Page start address (0 = reset)
    ssd1306_command(my_client, SSD1306_LCDHEIGHT/8 - 1); // Page end address// SSD1306_SWITCHCAPVCC porque lo vi en ejemplo

	for(i=0;i<(SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8);i+=16){
		sdd1306_writedatablock(my_client,screen_oled->screenBuffer+i,16);
	}

	screen_oled->columna=0;
	screen_oled->pagina=0;



}

static long clean_all_ioctl(struct file *filep, unsigned int cmd, unsigned long arg){

	switch(cmd){
	
		case IO_CLEAN:
			ioctl_clean_command();
		break;
		default:
		return -EINVAL;
	
	}

	return 0;


}
ssize_t SDD1306_i2c_write(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos){
		// Prueba de funcionamiento 
	char letra;
	if(copy_from_user(&letra,buf,1)){
			return -EFAULT;
	}

	printk(KERN_NOTICE "Valor de letra: %c decimal: %d\n",letra,letra);
	write_struct(letra);
	return 1;

}
static const struct file_operations SDD1306_fops = {
	.owner = THIS_MODULE,
	.write = SDD1306_i2c_write,
	.unlocked_ioctl = clean_all_ioctl			
};

int SDD1306_i2c_add_device(SDD1306 * screen){
	int ret = 0;
	printk(KERN_INFO "Entra en i2c-add\n");
	screen->miscdev.minor = MISC_DYNAMIC_MINOR;
	screen->miscdev.name = "write_oled";
	screen->miscdev.fops = &SDD1306_fops;
	screen->miscdev.parent = screen->dev;
	screen->miscdev.mode=S_IRUGO;
	
	
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
