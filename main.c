#include"stdio.h"
#include"serial.h"
#include"rfid.h"

//sudo gedit /etc/udev/rules.d/70-ttyusb.rules
//KERNEL=="ttyUSB[0-9]*", MODE="0666"


int main(void)
{
    rfid_init();
    debuge();
    return 0;
}


