#include "InterruptIn.h"
#include "LittleFileSystem.h"
#include "PinNameAliases.h"
#include "ThisThread.h"
#include "mbed.h"
#include "mbed_debug.h"
#include "mstd_utility"
#include "rtos.h"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <stdlib.h>
#include <string>
#include <vector>
#include <array>
#include "comm.h"
#include "BMP2-Sensor-API/bmp2.h"

#define BMP2_64BIT_COMPENSATION


bmp2_dev dev;  // main BMP2 API object
int8_t rslt = 1;
uint8_t dev_addr = 0x76<<1;

I2C i2c(D14, D15);
I2C* i2c_ptr = &i2c;

int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
void user_delay_us(uint32_t period, void *intf_ptr);

// main() runs in its own thread in the OS
int main()
{ 

    dev.intf = BMP2_I2C_INTF;
    dev.read = user_i2c_read;
    dev.write = user_i2c_write;
    dev.delay_us = user_delay_us;

    ThisThread::sleep_for(500ms);
    lcd_initialize();
    std::string lcd_string;

    bmp2_data comp_data;
    uint32_t meas_time;
    uint8_t tmp;

    bool device_found = false;     

    std::string temp_dec;

    // Sweep across I2C device address space to find the sensor
    while (!device_found){  
        dev.intf_ptr = &dev_addr;
        rslt = bmp2_init(&dev);

        lcd_first_line();
        lcd_string = "Result: " + std::to_string(rslt) + "  ";
        lcd_write_string(lcd_string);

        lcd_second_line();
        lcd_string = "I2C Add: " + std::to_string(dev_addr) + "  ";
        lcd_write_string(lcd_string);

        ThisThread::sleep_for(50ms);

        if (rslt == 0 ) {
            bmp2_soft_reset(&dev);
            device_found = true;
            lcd_first_line();
            lcd_write_string("Address found...");
            ThisThread::sleep_for(1s);
            lcd_clear();
         }
        else dev_addr++; 
    }



    // Configure oversampling modes, filter coefficient and measurement period 
    bmp2_config conf;
	conf.os_mode = BMP2_OS_MODE_HIGH_RESOLUTION;
	conf.filter = BMP2_FILTER_COEFF_16;
	conf.odr = BMP2_ODR_250_MS;
    rslt = bmp2_set_config(&conf, &dev);

    // Configure power mode, normal == periodical measurements
    if (rslt==0) rslt = bmp2_set_power_mode(BMP2_POWERMODE_NORMAL, &conf, &dev);


    if (rslt==0){
        bmp2_get_power_mode(&tmp, &dev);
        lcd_first_line();
        lcd_write_string("Power mode:" + std::to_string(tmp));

        bmp2_get_regs(0xF4, &tmp, 1, &dev);
        lcd_second_line();
        lcd_write_string("Ctrl: " + std::to_string(tmp));

        ThisThread::sleep_for(2s);
        lcd_clear();
    }
    else{
         lcd_first_line();
         lcd_write_string("Sensor mode:");
         lcd_second_line();
         lcd_write_string("Error");
         ThisThread::sleep_for(2s);
        lcd_clear();
    }

    while (true) {
        rslt = bmp2_get_sensor_data(&comp_data, &dev);
        lcd_first_line();
        temp_dec = std::to_string(int(comp_data.temperature*100)%100);
        if (temp_dec.size()==1) temp_dec = "0" + temp_dec;
        lcd_string = "T: " + std::to_string(int(comp_data.temperature)) + 
        "." + temp_dec + " degC";
        lcd_write_string(lcd_string);

        lcd_second_line();
        lcd_string = "p: " + std::to_string(int(comp_data.pressure)) + 
        "." + std::to_string(int(comp_data.pressure*10)%10) + " Pa";
        lcd_write_string(lcd_string);

        ThisThread::sleep_for(250ms);
    } // while(1) end
} // Main end



int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    rslt = int8_t(i2c_ptr->write((*(int*)intf_ptr), (const char*)&reg_addr, 1));
    // if (rslt!=0) return rslt;
    rslt = int8_t(i2c_ptr->read((*(int*)intf_ptr), (char*)reg_data, int(len)));
    return rslt;
}

int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    std::vector<uint8_t> i2c_data_package; // Create data to send through I2C
    i2c_data_package.emplace_back(reg_addr); // First byte == register address
    i2c_data_package.insert(i2c_data_package.end(), &reg_data[0], &reg_data[len]);

    rslt = int8_t(i2c_ptr->write((*(int*)intf_ptr), (const char*)&i2c_data_package[0], len+1));
    return rslt;
}

void user_delay_us(uint32_t period, void *intf_ptr){
    wait_us(period);
}