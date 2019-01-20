
//
//liqiang@wind-mobi.com 20171013 create this file 
//

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/fs_struct.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/kconfig.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/spmi.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/input.h>
#include <linux/log2.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/qpnp/power-on.h>


#define WIND_DIAG_TAG                  "[WIND/DIAG] "
#define WIND_DIAG_FUN(f)               printk(KERN_INFO WIND_DIAG_TAG"%s\n", __FUNCTION__)
#define WIND_DIAG_ERR(fmt, args...)    printk(KERN_ERR  WIND_DIAG_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define WIND_DIAG_LOG(fmt, args...)    printk(KERN_INFO WIND_DIAG_TAG fmt, ##args)
#define WIND_DIAG_DBG(fmt, args...)    printk(KERN_INFO WIND_DIAG_TAG fmt, ##args) 

#define SMT_CMD_PROINFO_READ  0
#define SMT_CMD_PROINFO_WRITE 1
#define SMT_CMD_PROINFO_BACKUP_READ 2
#define SMT_CMD_CAPACITY_READ  3
#define SMT_CMD_VOLTAGE_READ 4
#define SMT_CMD_CURRENT_READ 5

extern int diag_get_batt_info(int flag);

struct delayed_work wind_delay_work;
static void wind_delay_work_fun(struct work_struct *work);



struct mutex mutex_wind;


unsigned char cmd_table [][5] = 
{

{0x4b,0xc9,0xaa,0xff,0x00}, //SMT_CMD_PROINFO_READ 0
{0x4b,0xc9,0xaa,0xff,0x01}, //SMT_CMD_PROINFO_WRITE 1
{0x4b,0xc9,0xaa,0xff,0x02}, //SMT_CMD_SLEEP 2
{0x4b,0xc9,0xaa,0xff,0x03}, //SMT_CMD_CAPACITY_READ 3
{0x4b,0xc9,0xaa,0xff,0x04}, //SMT_CMD_VOLTAGE_READ 4
{0x4b,0xc9,0xaa,0xff,0x05}, //SMT_CMD_CURRENT_READ 5


};

#define  BUFFER_PROINFO_SIZE 1024*1
unsigned char buffer_proinfo[BUFFER_PROINFO_SIZE];
#define PROINFO_BUFFER_PATH_BACKUP "/dev/block/bootdevice/by-name/proinfo"
#define PROINFO_BUFFER_PATH "/factory/proinfo"
#define PROINFO_BUFFER_PATH_FACTORY_BACKUP "/factory/proinfo_backup"


int wind_diag_backup_file_write(char * path, char *w_buf, int w_len)
{
	struct file *fp;
	int ret = 0;

	mutex_lock(&mutex_wind);
	
	fp =filp_open(path, O_RDWR | O_SYNC, 0);
	
	if (IS_ERR(fp )) {
		
	 WIND_DIAG_ERR("error occured while opening file %s\n", path);
	 ret = -1;
	 
	}else{

	 WIND_DIAG_DBG("file %s is exist", path);
	 ret = fp->f_op->write(fp, w_buf, w_len, &fp->f_pos);
	 filp_close(fp, NULL);
	
	}
	
	mutex_unlock(&mutex_wind);

	return ret;

}


int wind_diag_file_write(char * path, char *w_buf, int w_len)
{
	struct file *fp;
	int ret = 0;

	mutex_lock(&mutex_wind);
	
	fp =filp_open(path, O_RDWR | O_CREAT | O_SYNC, 0660);
	
	if (IS_ERR(fp )) {
		
	 WIND_DIAG_ERR("error occured while opening file %s\n", path);
	 ret = -1;
	 
	}else{

	 WIND_DIAG_DBG("file %s is exist", path);
	 ret = fp->f_op->write(fp, w_buf, w_len, &fp->f_pos);
	 filp_close(fp, NULL);
	
	}
	mutex_unlock(&mutex_wind);

	return ret;

}

int wind_diag_file_read(char * path, char *r_buf, int r_len)
{
	struct file *fp;
	int ret = 0;

	mutex_lock(&mutex_wind);

	fp =filp_open(path, O_RDWR | O_CREAT, 0660);
	
	if (IS_ERR(fp )) {
		
	 WIND_DIAG_ERR("error occured while opening file %s\n", path);
	 ret = -1;
	 
	}else{

	 WIND_DIAG_DBG("file %s is exist\n", path);
	 ret = fp->f_op->read(fp, r_buf, r_len, &fp->f_pos);
	 filp_close(fp, NULL);
	
	}
	
	mutex_unlock(&mutex_wind);

	return ret;

}

int turn_off_backlight(char *buf, int count)
{
    int ret = 0;
    char path[] = "/sys/class/leds/lcd-backlight/brightness";
    //set kernel domain begin
    mm_segment_t old_fs;
    struct file *fp;

    printk(KERN_WARNING "turn_off_backlight()...\n");
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    fp=filp_open(path, O_RDWR|O_SYNC, 0);
    if (!IS_ERR_OR_NULL(fp)){
    printk(KERN_WARNING "turn_off_backlight() fp != null\n");
        if (fp->f_op){
    printk(KERN_WARNING "turn_off_backlight() f_op not null\n");
            fp->f_pos = 0;
            if(fp->f_op->write){//O_RDWR|O_SYNC
                ret = fp->f_op->write(fp, buf, count, &fp->f_pos);
    printk(KERN_WARNING "turn_off_backlight() op_write ret: %d\n", ret);
            }else{
                ret = (int)vfs_write(fp, buf, count, &fp->f_pos);
    printk(KERN_WARNING "turn_off_backlight() vfs_write ret: %d\n", ret);
            }
        }
        filp_close(fp,NULL);
    }
    //set user domain again
    set_fs(old_fs);
    printk(KERN_WARNING "turn_off_backlight() ret: %d\n", ret);
    return ret > 0;
}

int wind_diag_cmd_handler(unsigned char *rx_buf, int rx_len, unsigned char *tx_buf, int *tx_len){

	int index = -1;
	int fg_val;
	for(index = 0; index < sizeof(cmd_table)/sizeof(cmd_table[0]); index++){
		if(memcmp(rx_buf, cmd_table[index], sizeof(cmd_table[0])) == 0){
			memcpy(tx_buf, rx_buf, 5);
			*tx_len +=5;
			WIND_DIAG_DBG("find cmd = %d\n", index);
			break;
		}
	}

	if(index >= sizeof(cmd_table)/sizeof(cmd_table[0])){
		WIND_DIAG_DBG("cmd not in table index = %d\n", index);
		return -1;
	}else{
			
			switch (index){
				
				case SMT_CMD_PROINFO_READ:
					memset(buffer_proinfo, 0, BUFFER_PROINFO_SIZE);

					if(-1 != wind_diag_file_read(PROINFO_BUFFER_PATH, buffer_proinfo, BUFFER_PROINFO_SIZE)){
						
						tx_buf[5] = 'O';
						tx_buf[6] = 'K';
						*tx_len +=2;
						memcpy(&tx_buf[7], buffer_proinfo, BUFFER_PROINFO_SIZE);
						*tx_len +=BUFFER_PROINFO_SIZE;
						
					}else{
					
						tx_buf[5] = 'N';
						tx_buf[6] = 'G';
						*tx_len +=2;
						
					}
					
					break;
					
				case SMT_CMD_PROINFO_WRITE:
					memset(buffer_proinfo, 0, BUFFER_PROINFO_SIZE);
					memcpy(buffer_proinfo, &rx_buf[5], rx_len-5);
					if(-1 != wind_diag_file_write(PROINFO_BUFFER_PATH, buffer_proinfo, BUFFER_PROINFO_SIZE)){
						
						if(-1 != wind_diag_backup_file_write(PROINFO_BUFFER_PATH_BACKUP, buffer_proinfo, BUFFER_PROINFO_SIZE)){

							if(-1 != wind_diag_file_write(PROINFO_BUFFER_PATH_FACTORY_BACKUP, buffer_proinfo, BUFFER_PROINFO_SIZE)){
							tx_buf[5] = 'O';
							tx_buf[6] = 'K';
							*tx_len +=2;}
							else {
							    tx_buf[5] = 'N';
								tx_buf[6] = 'G';
								*tx_len +=2;
							}
						}else{
							tx_buf[5] = 'N';
							tx_buf[6] = 'G';
							*tx_len +=2;
						}
						
					}else{
					
						tx_buf[5] = 'N';
						tx_buf[6] = 'G';
						*tx_len +=2;
						
					}
					break;
					
			 case SMT_CMD_PROINFO_BACKUP_READ:
			 	
				 memset(buffer_proinfo, 0, BUFFER_PROINFO_SIZE);
				 
				 if(-1 != wind_diag_file_read(PROINFO_BUFFER_PATH_BACKUP, buffer_proinfo, BUFFER_PROINFO_SIZE)){
					 
					 tx_buf[5] = 'O';
					 tx_buf[6] = 'K';
					 *tx_len +=2;
					 memcpy(&tx_buf[7], buffer_proinfo, BUFFER_PROINFO_SIZE);
					 *tx_len +=BUFFER_PROINFO_SIZE;
					 
				 }else{
				 
					 tx_buf[5] = 'N';
					 tx_buf[6] = 'G';
					 *tx_len +=2;
					 
				 }
				 
				 break;
				
				case SMT_CMD_CAPACITY_READ:
						   tx_buf[5] = 'O';
						   tx_buf[6] = 'K';
						   fg_val = diag_get_batt_info(0);
				   		   tx_buf[7] = fg_val;
						   *tx_len +=3;
						break;   
				case SMT_CMD_VOLTAGE_READ:
						   tx_buf[5] = 'O';
						   tx_buf[6] = 'K';
						   fg_val = diag_get_batt_info(1)/1000;
						   memcpy(&tx_buf[7], &fg_val, 2);
						   *tx_len +=4;	
						 break;  
				case SMT_CMD_CURRENT_READ:
						   tx_buf[5] = 'O';
						   tx_buf[6] = 'K';
						   fg_val = diag_get_batt_info(2)/1000;
						   memcpy(&tx_buf[7], &fg_val, 2);
						   *tx_len +=4;					   
				   break;

		}

	  return index;
			
	}
	
}


#define WIND_DIAG_PROC_FOLDER 							"winddiag"
#define WIND_DIAG_PROC_PROINFO_FILE 					"proinfo"

//static struct proc_dir_entry *winddiag_proc_dir 			= NULL;
//static struct proc_dir_entry *proc_proinfo_file 		= NULL;
/*
static ssize_t proinfo_read(struct file *file, char *buf, size_t len, loff_t *pos)
{

	char proinfo_buffer[BUFFER_PROINFO_SIZE] = {0};

	if(-1 == wind_diag_file_read(PROINFO_BUFFER_PATH, proinfo_buffer, BUFFER_PROINFO_SIZE)){
			WIND_DIAG_ERR("wind_diag_file_write error.\n"); 
			return -EIO;
	}
	
	if (*pos)
		return 0;

	if (copy_to_user(buf, proinfo_buffer, BUFFER_PROINFO_SIZE)) {
		WIND_DIAG_ERR("proinfo_write error.\n");
		return -EFAULT;
	}
	
	*pos = BUFFER_PROINFO_SIZE;
	
	 return BUFFER_PROINFO_SIZE;

}

static ssize_t proinfo_write(struct file *file, const char *buff, size_t len, loff_t *pos)
{
	char proinfo_buffer[BUFFER_PROINFO_SIZE] = {0};
	//WIND_DIAG_LOG("len = %d\n", len);


	if ((len <= 0) || copy_from_user(proinfo_buffer, buff, len)) {
		WIND_DIAG_ERR("proinfo_write error.\n");
		return -EFAULT;
	}

	if(-1 == (wind_diag_file_write(PROINFO_BUFFER_PATH, proinfo_buffer, BUFFER_PROINFO_SIZE))){
		WIND_DIAG_ERR("wind_diag_file_write error.\n");	
		return -EIO;
	}
	
	return len;
}

static struct file_operations proinfo_file_ops =
{
	.owner = THIS_MODULE,
	.read = proinfo_read,
	.write = proinfo_write
};
*/
extern void power_key_input(void);

 void wind_delay_work_fun(struct work_struct *work)
{
	WIND_DIAG_FUN();

	//	turn_off_backlight("50", 2);
	//msleep(1000);
	//turn_off_backlight("0", 1);
	//pm_autosleep_set_state(PM_SUSPEND_MEM);
	//pm_suspend(PM_SUSPEND_MEM);

	power_key_input();


}

int winddiag_proc_init(void)
{

	WIND_DIAG_FUN(">>>\n");

	mutex_init(&mutex_wind);
/*
	winddiag_proc_dir = proc_mkdir(WIND_DIAG_PROC_FOLDER, NULL);

	if (winddiag_proc_dir == NULL)
	{
		printk(" %s: lightsensor_proc_dir file create failed!\n", __func__);
		return -ENOMEM;
	}

	proc_proinfo_file = proc_create(WIND_DIAG_PROC_PROINFO_FILE, (S_IWUSR|S_IRUGO|S_IWUGO), 
				winddiag_proc_dir, &proinfo_file_ops);

*/	
	INIT_DELAYED_WORK(&wind_delay_work, wind_delay_work_fun);
	return 0;

}


module_init(winddiag_proc_init);


//
//liqiang@wind-mobi.com 20171013 create this file 
//

