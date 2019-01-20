//
//liqiang@wind-mobi.com 20171013 create this file 
//

#ifndef __DIAG_WIND_H__
#define __DIAG_WIND_H__

#define SMT_CMD_PROINFO_READ  0
#define SMT_CMD_PROINFO_WRITE 1
#define SMT_CMD_SLEEP 2



extern int wind_diag_cmd_handler(unsigned char *rx_buf, int rx_len, unsigned char *tx_buf, int *tx_len);
extern int winddiag_proc_init(void);



#endif
//
//liqiang@wind-mobi.com 20171013 create this file 
//

