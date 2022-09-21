#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <unistd.h>

#define FT_IOCGTEMP_CPU             _IOR('v', 0, unsigned int)
#define FT_IOCGTEMP_GPU             _IOR('v', 1, unsigned int)

#define FT_IOCGFAN_CPU              _IOR('v', 2, unsigned int)
#define FT_IOCGFAN_GPU              _IOR('v', 3, unsigned int)

#define FT_IOCGEVENT                _IOR('v', 4, unsigned int)

#define FT_IOCGBAT_STATUS           _IOR('v', 7, unsigned int)
#define FT_IOCGBAT_REMQUA_PER       _IOR('v', 8, unsigned int)
#define FT_IOCGBAT_VOL              _IOR('v', 9, unsigned int)
#define FT_IOCGBAT_CURRENT          _IOR('v', 10, unsigned int)
#define FT_IOCGBAT_AVG_CURRENT      _IOR('v', 11, unsigned int)
#define FT_IOCGBAT_REMQUA           _IOR('v', 12, unsigned int)
#define FT_IOCGBAT_SERIAL           _IOR('v', 13, unsigned int)
#define FT_IOCGBAT_TEMP             _IOR('v', 14, unsigned int)

#define FT_IOCGPOWER_STATUS         _IOR('v', 15, unsigned int)
#define FT_IOCGBAT_QUA_FULL         _IOR('v', 16, unsigned int)
#define FT_IOCGBAT_VOL_DESIGN       _IOR('v', 17, unsigned int)
#define FT_IOCGBAT_CHARGE_STATUS    _IOR('v', 18, unsigned int)

#define FT_IOCGRANDOM               _IOR('v', 128, unsigned int)
#define FT_IOCSRANDOM               _IOW('v', 129, unsigned int)

#define SHOW_CPU_INFO     1
#define SHOW_GPU_INFO     2
#define SHOW_BATERRY_INFO 3
#define SHOW_ADDRESS_INFO 4
#define EVENT_DETECT_TEST 5
#define EXIT_TEST         6

#define CHECK_BATTERY_EVENT 0xB3
#define CHECK_SUSPEND_EVENT 0xD0

typedef enum{
    PRINTF_EN,
    PRINTF_CZ,
}Print_Type;

Print_Type print_type;

int show_battery_info(int fd)
{
    unsigned Current, Avg_Current;
    unsigned int Status, RemQua_Per, Vol, RemQua, Serial, Temp;
    unsigned int Power_Status, Qua_Full, Vol_Design, Charge_Status;

    if( ioctl(fd, FT_IOCGBAT_STATUS, &Status) < 0 ){
        printf("FT_IOCGBAT_STATUS failed\n");
        Status = 0;
    }

    if( ioctl(fd, FT_IOCGBAT_REMQUA_PER, &RemQua_Per) < 0){
        printf("FT_IOCGBAT_REMQUA_PER failed\n");
        RemQua_Per = 0;
    }

    if( ioctl(fd, FT_IOCGBAT_VOL, &Vol) < 0 ){
        printf("FT_IOCGBAT_VOL failed\n");
        Vol = 0;
    }

    if( ioctl(fd, FT_IOCGBAT_CURRENT, &Current) <0 ){
        printf("FT_IOCGBAT_CURRENT failed\n");
        Current = 0;
    }
    printf("Current:%d Current:%u\n",Current, Current);
    if(Current & 0x8000 )
        Current = (~Current)&0xFFFF;

    if( ioctl(fd, FT_IOCGBAT_AVG_CURRENT, &Avg_Current) < 0 ){
        printf("FT_IOCGBAT_AVG_CURRENT failed\n");
        Avg_Current = 0;
    }

    if(Avg_Current & 0x8000 )
        Avg_Current = (~Avg_Current)&0xFFFF;

    if( ioctl(fd, FT_IOCGBAT_REMQUA, &RemQua) < 0 ){
        printf("FT_IOCGBAT_REMQUA failed\n");
        RemQua = 0;
    }

    if( ioctl(fd, FT_IOCGBAT_SERIAL, &Serial) < 0 ){
        printf("FT_IOCGBAT_SERIAL failed\n");
        Serial = 0;
    }

    if( ioctl(fd, FT_IOCGBAT_TEMP, &Temp) < 0 ){
        printf("FT_IOCGBAT_TEMP failed\n");
        Temp = 0;
    }

    if( ioctl(fd, FT_IOCGPOWER_STATUS, &Power_Status) < 0 ){
        printf("FT_IOCGPOWER_STATUS failed\n");
        Power_Status = 0;
    }

    if( ioctl(fd, FT_IOCGBAT_QUA_FULL, &Qua_Full) < 0 ){
        printf("FT_IOCGBAT_QUA_FULL failed\n");
        Qua_Full = 0;
    }

    if( ioctl(fd, FT_IOCGBAT_VOL_DESIGN, &Vol_Design) < 0 ){
        printf("FT_IOCGBAT_VOL_DESIGN failed\n");
        Vol_Design = 0;
    }

    if( ioctl(fd, FT_IOCGBAT_CHARGE_STATUS, &Charge_Status) < 0 ){
        printf("FT_IOCGBAT_CHARGE_STATUS failed\n");
        Charge_Status = 0;
    }

    printf("********************************\n");
    if( print_type == PRINTF_CZ){
        printf("电池信息统计:\n");
        printf("    电池状态:           %d\n", Status);
        printf("    剩余电量百分比:     %d%%\n", RemQua_Per);
        printf("    电压:               %dmV\n", Vol);
        printf("    电流:               %dmA\n", Current);
        printf("    平均电流:           %dmA\n", Avg_Current);
        printf("    剩余电量:           %dmA\n", RemQua);
        printf("    序列号:             %d\n", Serial);
        printf("    温度:               %d摄氏度\n", Temp/10 - 273);
        printf("    电源状态:           %d\n", Power_Status);
        printf("    设计电量:           %dmA\n", Qua_Full);
        printf("    设计电压:           %dmV\n", Vol_Design);
        printf("    满电量值:           %dmA\n", Charge_Status);
    }else {
        printf("Battery info:\n");
        printf("    status:                 %d\n", Status);
        printf("    RemainQua_Per:          %d%%\n", RemQua_Per);
        printf("    Vol:                    %dmV\n", Vol);
        printf("    Current:                %dmA\n", Current);
        printf("    Average Current:        %dmA\n", Avg_Current);
        printf("    Remain Qua:             %dmA\n", RemQua);
        printf("    Serial:                 %d\n", Serial);
        printf("    Temp:                   %d\n", Temp/10 - 273);
        printf("    Power Status:           %d\n", Power_Status);
        printf("    Design Qua:             %dmA\n", Qua_Full);
        printf("    Design Vol:             %dmV\n", Vol_Design);
        printf("    Full Qua:               %dmA\n", Charge_Status);
    }
    printf("********************************\n");

    return 0;
}

int show_gpu_info(int fd)
{
    unsigned int temp,fan;
    if( ioctl(fd, FT_IOCGTEMP_GPU, &temp) < 0 ){
        printf("FT_IOCGTEMP_CPU failed\n");
        temp = 0;
    }

    if( ioctl(fd, FT_IOCGFAN_GPU, &fan) < 0 ){
        printf("FT_IOCGFAN_CPU failed\n");
        fan = 0;
    }
    printf("********************************\n");
    if( print_type == PRINTF_CZ){
        printf("GPU 信息:\n");
        printf("    温度:   %d摄氏度\n",temp);
        printf("    转速:   %d转/分钟\n",fan);
    }else {
        printf("GPU info:\n");
        printf("    Temp:   %d\n",temp);
        printf("    Fan:   %dr/min\n",fan);   
    }
    printf("********************************\n");

    return 0;
}

int show_cpu_info(int fd)
{
    unsigned int temp,fan;
    if( ioctl(fd, FT_IOCGTEMP_CPU, &temp) < 0 ){
        printf("FT_IOCGTEMP_CPU failed\n");
        temp = 0;
    }

    if( ioctl(fd, FT_IOCGFAN_CPU, &fan) < 0 ){
        printf("FT_IOCGFAN_CPU failed\n");
        fan = 0;
    }
    printf("********************************\n");
    if( print_type == PRINTF_CZ){
        printf("CPU 信息:\n");
        printf("    温度:   %d摄氏度\n",temp);
        printf("    转速:   %d转/分钟\n",fan);
    }else{
        printf("CPU info:\n");
        printf("    Temp:   %d\n",temp);
        printf("    Fan:   %dr/m\n",fan);
    }
    printf("********************************\n");

    return 0;
}

int show_address_info(int fd)
{
    unsigned int value;
    unsigned int addr;
    if( print_type == PRINTF_CZ)
        printf("请输入需要读取的地址:  ");
    else
        printf("Please Input addr:  ");

    scanf("%x", &addr);
    getchar();
    if(addr < 0 || addr > 255)
    {
        printf("invalid address(0x00-->0xff)\n");
        return -1;
    }

    if( ioctl(fd, FT_IOCSRANDOM, &addr) < 0 ){
        printf("ioctl FT_IOCSRADOM failed\n");
        return -1;
    }

    if( ioctl(fd,FT_IOCGRANDOM,&value) < 0){
        printf("ioctl FT_IOCGBATCAP failed\n");
        value = 0;
    }
    printf("********************************\n");
    if( print_type == PRINTF_CZ)
        printf("结果:   %d\n",value);
    else
        printf("result:     %d\n",value);
    printf("********************************\n");

    return 0;
}

int event_detect_test(int fd)
{
    printf("\ntest poll\n");
    int ret;
    int event;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    ret = select(fd+1, &rfds, NULL, NULL, NULL);
    if( ret < 0 ){
        perror("failed\n");
        return -1;;
    }
    else if(0 == ret){
        printf("timeout!\n");
        return 0;
    }
    else if(FD_ISSET(fd, &rfds)){
        ioctl(fd, FT_IOCGEVENT, &event);
        printf("get event:0x%x\n",event);
        if( event == CHECK_BATTERY_EVENT)
            show_battery_info(fd);
        else if( event == CHECK_SUSPEND_EVENT){
            if( print_type == PRINTF_CZ)
                printf("系统3秒后进入休眠!\n");
            else
                printf("System will be goto suspend after 3sec");
            sleep(3);
            system("echo mem > /sys/power/state");
        }
    }

    return 0;
}

void show_usage(void)
{
    printf("\n*******************************\n");
    if( print_type == PRINTF_CZ){
        printf("请输入测试项编号\n");
        printf("    1:显示CPU信息\n");
        printf("    2:显示GPU信息\n");
        printf("    3:显示电池信息\n");
        printf("    4:手动获取指定地址数据\n");
        printf("    5:事件检测测试\n");
        printf("    6:退出\n");
    }else{
        printf("Please input test case\n");
        printf("    1:show cpu info\n");
        printf("    2:show gpu info\n");
        printf("    3:show battery info\n");
        printf("    4:manual get address data\n");
        printf("    5:event detect test\n");
        printf("    6:quit\n");
    }
    printf("*******************************\n");
}

int main(int argc, char **argv)
{
    int fd;
    int testCase;
    fd = -1;
    print_type = PRINTF_EN;
    if( argc == 2 )
        print_type = PRINTF_CZ;
Test:
    show_usage();
    scanf("%d", &testCase);
    if( testCase < SHOW_CPU_INFO || testCase > EXIT_TEST ){
        printf("invalid testCase\n");
        getchar();
        goto Test;
    }
    getchar();

    if( testCase == EXIT_TEST ){
        printf("exit success\n");
        if( fd  >0 )
            close(fd);
        return 0;
    }
    
    fd = open("/dev/FT-Misc", O_RDWR);
    if( fd < 0 ){
        printf("open /dev/FT-Misc failed\n");
        return -1;
    }
    system("rw -w 1 0x20000066 0x80");
    system("rw -w 1 0x20000062 0x98");
    system("rw -r 1 0x20000062");
    switch(testCase)
    {
        case SHOW_CPU_INFO:{   
            show_cpu_info(fd);
            break;
        }

        case SHOW_GPU_INFO:{
            show_gpu_info(fd);
            break;
        }

        case SHOW_BATERRY_INFO:{
            show_battery_info(fd);
            break;
        }

        case EVENT_DETECT_TEST:{
            event_detect_test(fd);
            break;
        }

        case SHOW_ADDRESS_INFO:{
            show_address_info(fd);
            break;
        }
        default:
            printf("invalid input parameter!\n");
            break;
    }
   
    if( print_type == PRINTF_CZ)
    	printf("请按回车继续!\n");
    else
	printf("Please press enter key to continue!\n");

    getchar();
    goto Test;
}
