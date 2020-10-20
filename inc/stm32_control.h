#ifndef STM32_CONTROL_H
#define STM32_CONTROL_H


#ifdef __cplusplus
extern "C" {
#endif



typedef struct {
char valid;
char X_V;
char Y_V;

}CVFEED;
extern unsigned int velspeed ;
extern unsigned int angspeed ;
extern CVFEED cv_res;
extern unsigned int positionx;
extern int *stm_Loop();
extern void cmd_vel_callback(const char * cmd_vel);
extern void cmd_send(const char cmd_v,int speed);

#ifdef __cplusplus
}
#endif
#endif

