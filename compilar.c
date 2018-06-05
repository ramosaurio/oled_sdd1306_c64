#include "oled_template.h"
#include "oled_ioctl.h"
static SDD1306 *screen_oled;
static short int irq_BUTTON_UP = 0;
static short int irq_BUTTON_DOWN = 0; 
static short int irq_BUTTON_LEFT = 0; 
static short int irq_BUTTON_RIGHT = 0; 
static short int blink = 0;

static uint8_t charblink[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

static void work_handler_up(struct work_struct *work);
static void work_handler_down(struct work_struct *work);
static void work_handler_left(struct work_struct *work);
static void work_handler_right(struct work_struct *work);
static void work_handler_blink(struct work_struct *work);

DECLARE_WORK(work_up,work_handler_up);
DECLARE_WORK(work_down,work_handler_down);
DECLARE_WORK(work_left,work_handler_left);
DECLARE_WORK(work_right,work_handler_right);
DECLARE_WORK(work_blink,work_handler_blink);

static unsigned long ticks_unsegundo;
static void timer_handler(unsigned long);
static void cursor_blink_timer(unsigned long);
DEFINE_TIMER(timer,timer_handler,0,0);
DEFINE_TIMER(blink_timer,cursor_blink_timer,0,0);
DEFINE_SEMAPHORE(semaforo);

static void work_handler_blink(struct work_struct *work){
		uint8_t command,valor;
		int i,bufferPos,colPos,pagPos;
	 	
		command = 0x40;
		if(down_interruptible(&semaforo))printk(KERN_ERR "BLINK DOWN_INTERRUPTIBLE ERR\n"); 	
		if(blink == 0){
 			for(i=0; i<8; i++){
				i2c_smbus_write_byte_data(screen_oled->client,command,charblink[i]);
			}

			blink=1;
		}else{
			colPos = screen_oled->cursor->columna*8;
			pagPos = screen_oled->cursor->pagina*128;
			for(i=0;i<8;i++){
				bufferPos = colPos + pagPos + i;
				valor = screen_oled->screenBuffer[bufferPos];
				i2c_smbus_write_byte_data(screen_oled->client,command,valor);
			}
			blink = 0;
		}
		SDD1306_cambiar_ptr(screen_oled->cursor->pagina,screen_oled->cursor->columna);
		mod_timer(&blink_timer,(jiffies+ticks_unsegundo));

		up(&semaforo);
}
static void cursor_blink_timer(unsigned long dato){
	schedule_work(&work_blink);

}
static void printcursor(void){
	printk(KERN_NOTICE "CURSOR X: %d\n",screen_oled->cursor->pagina);
	printk(KERN_NOTICE "CURSOR Y: %d\n",screen_oled->cursor->columna);
}
static int save_actual_pos(void){
	int i,bufferPos,pagPos,colPos,res;
	uint8_t command,valor;
	command = 0x40;
	res = 0;
	pagPos = screen_oled->cursor->pagina*PAGLENGTH;
	colPos = screen_oled->cursor->columna*CHARSIZE;
	if(down_interruptible(&semaforo))return -ERESTARTSYS;
	for(i=0;i<8;i++){
		bufferPos= pagPos+colPos + i;
		valor = screen_oled->screenBuffer[bufferPos];
		i2c_smbus_write_byte_data(screen_oled->client,command,valor);
	}
	up(&semaforo);
	return res;
}
static void work_handler_up(struct work_struct *work){
	printk(KERN_INFO "BOTON_UP\n");
	if(screen_oled->cursor->pagina>0){
		
		if(save_actual_pos()) printk(KERN_ERR"ERROR DOWN_INTERRUPTIBLE\n");
		screen_oled->pagina--;
		screen_oled->cursor->pagina--;
		SDD1306_cambiar_ptr((screen_oled->cursor->pagina), screen_oled->cursor->columna);
	}

	printcursor();
}
static void work_handler_down(struct work_struct *work){
	printk(KERN_INFO "BOTON DOWN\n");
	if((screen_oled->cursor->pagina)!=7){		
		if(save_actual_pos()) printk(KERN_ERR"ERROR DOWN_INTERRUPTIBLE\n");
		screen_oled->pagina++;
		screen_oled->cursor->pagina++;
	 	SDD1306_cambiar_ptr((screen_oled->cursor->pagina),screen_oled->cursor->columna);	
	}

	printcursor();
}
static void work_handler_left(struct work_struct *work){
	printk(KERN_INFO "BOTON LEFT\n");

	if(screen_oled->cursor->columna>0){
		if(save_actual_pos()) printk(KERN_ERR"ERROR DOWN_INTERRUPTIBLE\n");

		screen_oled->columna--;
		screen_oled->cursor->columna--;
	 	SDD1306_cambiar_ptr((screen_oled->cursor->pagina),screen_oled->cursor->columna);
	}

	printcursor();
}
static void work_handler_right(struct work_struct *work){
	printk(KERN_INFO "BOTON_RIGHT\n");

	if(screen_oled->cursor->columna<15){
		if(save_actual_pos()) printk(KERN_ERR"ERROR DOWN_INTERRUPTIBLE\n");

		screen_oled->columna++;
		screen_oled->cursor->columna++;
	 	SDD1306_cambiar_ptr((screen_oled->cursor->pagina),screen_oled->cursor->columna);	
	}

	printcursor();
}
static void timer_handler(unsigned long dato){
	enable_irq(irq_BUTTON_UP);
	enable_irq(irq_BUTTON_DOWN);
	enable_irq(irq_BUTTON_LEFT);
	enable_irq(irq_BUTTON_RIGHT);
}
static irqreturn_t r_irq_handler(int irq, void *dev_id, struct pt_regs *regs){
	disable_irq_nosync(irq_BUTTON_UP);
	disable_irq_nosync(irq_BUTTON_DOWN);
	
	disable_irq_nosync(irq_BUTTON_LEFT);
	disable_irq_nosync(irq_BUTTON_RIGHT);


	mod_timer(&timer,jiffies+ticks_unsegundo/2);
		
	if(irq==irq_BUTTON_UP) schedule_work(&work_up);
	else if(irq==irq_BUTTON_DOWN) schedule_work(&work_down);
	else if(irq==irq_BUTTON_LEFT) schedule_work(&work_left);
	else if(irq==irq_BUTTON_RIGHT) schedule_work(&work_right);

	return IRQ_HANDLED;
}

static int r_int_config(void)
{
	int res=0;
    if ((res=gpio_request(GPIO_BUTTON_UP, GPIO_BUTTON_UP_DESC))) {
        printk(KERN_ERR "GPIO request faiure: %s\n", GPIO_BUTTON_UP_DESC);
        return res;
    }
    
    if ((res=gpio_set_debounce(GPIO_BUTTON_UP, 200))) {
        printk(KERN_ERR "GPIO set_debounce failure: %s, error: %d\n", GPIO_BUTTON_UP_DESC, res);
        printk(KERN_ERR "errno: 524 => ENOTSUPP, Operation is not supported\n");
    }

    if ( (irq_BUTTON_UP = gpio_to_irq(GPIO_BUTTON_UP)) < 0 ) {
        printk(KERN_ERR "GPIO to IRQ mapping faiure %s\n", GPIO_BUTTON_UP_DESC);
        return irq_BUTTON_UP;
    }

    printk(KERN_NOTICE "  Mapped int %d for button1 in gpio %d\n", irq_BUTTON_UP, GPIO_BUTTON_UP);
  
  	if ((res=gpio_request(GPIO_BUTTON_DOWN, GPIO_BUTTON_DOWN_DESC))) {
        printk(KERN_ERR "GPIO request faiure: %s\n", GPIO_BUTTON_DOWN_DESC);
        return res;
    }
    
    if ((res=gpio_set_debounce(GPIO_BUTTON_DOWN, 200))) {
        printk(KERN_ERR "GPIO set_debounce failure: %s, error: %d\n", GPIO_BUTTON_DOWN_DESC, res);
        printk(KERN_ERR "errno: 524 => ENOTSUPP, Operation is not supported\n");
    }

    if ( (irq_BUTTON_DOWN = gpio_to_irq(GPIO_BUTTON_DOWN)) < 0 ) {
        printk(KERN_ERR "GPIO to IRQ mapping faiure %s\n", GPIO_BUTTON_DOWN_DESC);
        return irq_BUTTON_DOWN;
    }

    printk(KERN_NOTICE "  Mapped int %d for button1 in gpio %d\n", irq_BUTTON_DOWN, GPIO_BUTTON_DOWN);
    
	if ((res=gpio_request(GPIO_BUTTON_LEFT, GPIO_BUTTON_LEFT_DESC))) {
        printk(KERN_ERR "GPIO request faiure: %s\n", GPIO_BUTTON_LEFT_DESC);
        return res;
    }
    
    if ((res=gpio_set_debounce(GPIO_BUTTON_LEFT, 200))) {
        printk(KERN_ERR "GPIO set_debounce failure: %s, error: %d\n", GPIO_BUTTON_LEFT_DESC, res);
        printk(KERN_ERR "errno: 524 => ENOTSUPP, Operation is not supported\n");
    }

    if ( (irq_BUTTON_LEFT = gpio_to_irq(GPIO_BUTTON_LEFT)) < 0 ) {
        printk(KERN_ERR "GPIO to IRQ mapping faiure %s\n", GPIO_BUTTON_LEFT_DESC);
        return irq_BUTTON_LEFT;
    }

    printk(KERN_NOTICE "  Mapped int %d for button1 in gpio %d\n", irq_BUTTON_LEFT, GPIO_BUTTON_LEFT);
  
  	if ((res=gpio_request(GPIO_BUTTON_RIGHT, GPIO_BUTTON_RIGHT_DESC))) {
        printk(KERN_ERR "GPIO request faiure: %s\n", GPIO_BUTTON_RIGHT_DESC);
        return res;
    }
    
    if ((res=gpio_set_debounce(GPIO_BUTTON_RIGHT, 200))) {
        printk(KERN_ERR "GPIO set_debounce failure: %s, error: %d\n", GPIO_BUTTON_RIGHT_DESC, res);
        printk(KERN_ERR "errno: 524 => ENOTSUPP, Operation is not supported\n");
    }

    if ( (irq_BUTTON_RIGHT = gpio_to_irq(GPIO_BUTTON_RIGHT)) < 0 ) {
        printk(KERN_ERR "GPIO to IRQ mapping faiure %s\n", GPIO_BUTTON_RIGHT_DESC);
        return irq_BUTTON_RIGHT;
    }

    printk(KERN_NOTICE "  Mapped int %d for button1 in gpio %d\n", irq_BUTTON_RIGHT, GPIO_BUTTON_RIGHT);




    if ((res=request_irq(irq_BUTTON_UP,
                    (irq_handler_t ) r_irq_handler,
                    IRQF_TRIGGER_FALLING,
                    GPIO_BUTTON_UP_DESC,
                    GPIO_BUTTON_DEVICE_DESC))) {
        printk(KERN_ERR "Irq Request failure\n");
        return res;
    }

    if ((res=request_irq(irq_BUTTON_DOWN,
                    (irq_handler_t ) r_irq_handler,
                    IRQF_TRIGGER_FALLING,
                    GPIO_BUTTON_DOWN_DESC,
                    GPIO_BUTTON_DEVICE_DESC))) {
        printk(KERN_ERR "Irq Request failure\n");
        return res;
    }
    
	if ((res=request_irq(irq_BUTTON_LEFT,
                    (irq_handler_t ) r_irq_handler,
                    IRQF_TRIGGER_FALLING,
                    GPIO_BUTTON_LEFT_DESC,
                    GPIO_BUTTON_DEVICE_DESC))) {
        printk(KERN_ERR "Irq Request failure\n");
        return res;
    }

    if ((res=request_irq(irq_BUTTON_RIGHT,
                    (irq_handler_t ) r_irq_handler,
                    IRQF_TRIGGER_FALLING,
                    GPIO_BUTTON_RIGHT_DESC,
                    GPIO_BUTTON_DEVICE_DESC))) {
        printk(KERN_ERR "Irq Request failure\n");
        return res;
    }



    return res;
}

void desplazar(uint8_t *letra,int cuenta){
	uint8_t aux[8];
	int realscroll,vPag,vCol;
	
	realscroll = 0;
	vPag = cuenta/PAGLENGTH;
	vCol = (cuenta/CHARSIZE)%COLSIZE;

	if(cuenta<=screen_oled->lastWrite+8){
		memcpy(aux,screen_oled->screenBuffer+cuenta,8);
		memcpy(screen_oled->screenBuffer+cuenta,letra,8);

		if((vPag==7) &&(vCol==15)){
			SDD1306_scrollup();   
			cuenta = cuenta - PAGLENGTH;
			screen_oled->lastWrite = screen_oled->lastWrite - PAGLENGTH ;
			screen_oled->cursor->pagina = 6;
		}else{	
			SDD1306_cambiar_ptr(vPag,vCol); 	
			sdd1306_writedatablock(screen_oled->client,screen_oled->screenBuffer+cuenta,8);
		}

		cuenta = cuenta + 8;
		desplazar(aux,cuenta);		

	}else{

		if(screen_oled->cursor->columna==15){
			if(screen_oled->cursor->pagina==7){ 
				realscroll =SDD1306_scrollup();
			}else{
				screen_oled->lastWrite = screen_oled->lastWrite + 8;
				screen_oled->cursor->pagina++;
			}   
			screen_oled->cursor->columna=0;
		}else{
			screen_oled->cursor->columna++;
			screen_oled->lastWrite = screen_oled->lastWrite + 8;

		}

		if(!realscroll){
		
			SDD1306_cambiar_ptr(screen_oled->cursor->pagina,screen_oled->cursor->columna); 
			printcursor();
		}
	}
}
void write_struct(char letra){
	int posText,posScreen,scroll_real;
	uint16_t ascii,i;
	uint8_t letradesplazar[8];

	if(down_interruptible(&semaforo)) printk(KERN_ERR "ERROR DOWN_INTERRUPTIBLE WRITE\n"); 	
	scroll_real = 0;
	ascii = (uint16_t) letra;
	ascii = ascii*8;

	posText = screen_oled->cursor->pagina + screen_oled->cursor->columna;
	posScreen = (screen_oled->cursor->pagina*128)+(screen_oled->cursor->columna*8);


	screen_oled->textBuffer[posText] = letra;
	if(screen_oled->lastWrite != 0 && posScreen <= screen_oled->lastWrite){	
		memcpy(letradesplazar,(font+ascii),8);
		desplazar(letradesplazar,posScreen);
	}else{

		for(i = 0; i<8;i++){
			screen_oled->screenBuffer[ posScreen+i] = font[ascii+i];
		}
		if(((int)letra)==10){
		    scroll_real = 1;
			screen_oled->columna = 0;
			screen_oled->pagina++;
			screen_oled->cursor->columna = 0;
			if(screen_oled->cursor->pagina!=7){
				screen_oled->cursor->pagina++;
				SDD1306_cambiar_ptr((screen_oled->cursor->pagina),screen_oled->cursor->columna);
			}else scroll_real = SDD1306_scrollup();	 
	
		}else{
			if(screen_oled->cursor->columna == 15){
				if(screen_oled->cursor->pagina==7)	scroll_real = SDD1306_scrollup();
				else screen_oled->cursor->pagina++;	
				screen_oled->cursor->columna = 0;
				screen_oled->columna = 0;
	 			screen_oled->pagina++;	
			}else{
				SDD1306_cambiar_ptr((screen_oled->cursor->pagina),screen_oled->cursor->columna);
				screen_oled->columna++;
				screen_oled->cursor->columna++;	
			}	

			if(!scroll_real){
				SDD1306_display(screen_oled->client,ascii);	
			}
		}	
			screen_oled->lastWrite = screen_oled->cursor->pagina*128 + screen_oled->columna*8  - 8;

	}
	up(&semaforo);
	}
void SDD1306_cambiar_ptr(int pagina, int columna){
	struct i2c_client * my_client = screen_oled->client;	

	ssd1306_command(my_client,SSD1306_COLUMNADDR);
	ssd1306_command(my_client,columna*8);
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
	printk(KERN_NOTICE "ESCRIBO\n");

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
	printk(KERN_NOTICE "%d\n",ascii);
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
	if((screen->cursor = kzalloc(sizeof(cursor_oled),GFP_KERNEL))==NULL){
		goto free_driver_NULL;   
	}	
	screen->columna = 0;
	screen->pagina = 0;
	screen->lastWrite =0;
	(screen->cursor)->pagina=0;
	(screen->cursor)->columna=0;
	
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
	int res = 0;

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
	printk(KERN_NOTICE "IRQ_BUTTONS INITIALIZATION\n");
	if((res = r_int_config())){
		  printk("FALLA EL IRQ\n");
		  if(irq_BUTTON_UP) free_irq(irq_BUTTON_UP, GPIO_BUTTON_DEVICE_DESC);   //libera irq
		    gpio_free(GPIO_BUTTON_DOWN);  // libera GPIO
			printk(KERN_NOTICE"%s GPIO UP \n",KBUILD_MODNAME);
    	  if(irq_BUTTON_DOWN) free_irq(irq_BUTTON_DOWN, GPIO_BUTTON_DEVICE_DESC);   //libera irq
    		gpio_free(GPIO_BUTTON_DOWN);  // libera GPIO
			printk(KERN_NOTICE"%s GPIO DOWN\n",KBUILD_MODNAME);
	      if(irq_BUTTON_LEFT) free_irq(irq_BUTTON_LEFT, GPIO_BUTTON_DEVICE_DESC);   //libera irq
    		gpio_free(GPIO_BUTTON_LEFT);  // libera GPIO
			printk(KERN_NOTICE "%s GPIO LEFT\n",KBUILD_MODNAME);
    	  if(irq_BUTTON_RIGHT) free_irq(irq_BUTTON_RIGHT, GPIO_BUTTON_DEVICE_DESC);   //libera irq
    		gpio_free(GPIO_BUTTON_RIGHT);  // libera GPIO
 			printk(KERN_NOTICE "%s GPIO RIGHT\n",KBUILD_MODNAME);	
	}
	ticks_unsegundo = msecs_to_jiffies(1000);
	mod_timer(&blink_timer,jiffies+ticks_unsegundo*2);

	return 0;
}

int SDD1306_i2c_remove(struct i2c_client *client){
	struct device *dev = &(client->dev);
	SDD1306 * screen = dev_get_drvdata(dev);
	SDD1306_clear_buffer(client);
		misc_deregister(&screen->miscdev);
	 	

	kfree(screen->textBuffer);
	kfree(screen->screenBuffer);	

	put_device(dev);
	printk(KERN_NOTICE"PASO5\n");
	kfree(screen);
	printk(KERN_NOTICE"PASO6\n");

	screen_oled = NULL;
    printk(KERN_NOTICE "FREE GPIOS\n");
	if(irq_BUTTON_UP) free_irq(irq_BUTTON_UP, GPIO_BUTTON_DEVICE_DESC);   //libera irq
	gpio_free(GPIO_BUTTON_UP);  // libera GPIO
	printk(KERN_NOTICE"%s GPIO UP \n",KBUILD_MODNAME);
    if(irq_BUTTON_DOWN) free_irq(irq_BUTTON_DOWN, GPIO_BUTTON_DEVICE_DESC);   //libera irq
   gpio_free(GPIO_BUTTON_DOWN);  // libera GPIO
	printk(KERN_NOTICE"%s GPIO DOWN\n",KBUILD_MODNAME);
	if(irq_BUTTON_LEFT) free_irq(irq_BUTTON_LEFT, GPIO_BUTTON_DEVICE_DESC);   //libera irq
    gpio_free(GPIO_BUTTON_LEFT);  // libera GPIO
	printk(KERN_NOTICE "%s GPIO LEFT\n",KBUILD_MODNAME);
    if(irq_BUTTON_RIGHT) free_irq(irq_BUTTON_RIGHT, GPIO_BUTTON_DEVICE_DESC);   //libera irq
    gpio_free(GPIO_BUTTON_RIGHT);  // libera GPIO
 	printk(KERN_NOTICE "%s GPIO RIGHT\n",KBUILD_MODNAME);	
	flush_scheduled_work();
	del_timer(&timer);
	del_timer(&blink_timer);
	printk(KERN_NOTICE "Done. Bye form %s module\n",KBUILD_MODNAME);
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
	screen_oled->cursor->pagina = 0;
	screen_oled->cursor->columna = 0;
	screen_oled->lastWrite = 0;
 	SDD1306_cambiar_ptr(0,0);	



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
	screen->miscdev.mode=S_IWOTH | S_IWGRP | S_IWUSR;
	
	
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
