/*
   f3evobrushed.h : Board implmentation for Hyperion F3 Evo Brushed board

   Copyright (C) 2018 Simon D. Levy 

   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Hackflight is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Hackflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "f3evobrushed.h"

static const uint16_t BRUSHED_PWM_RATE     = 32000;
static const uint16_t BRUSHED_IDLE_PULSE   = 0; 

static const float    MOTOR_MIN = 1000;
static const float    MOTOR_MAX = 2000;

// Here we put code that interacts with Cleanflight
extern "C" {

#include "platform.h"

#include "drivers/system.h"
#include "drivers/timer.h"
#include "drivers/time.h"
#include "drivers/pwm_output.h"
#include "drivers/light_led.h"
#include "drivers/serial.h"
#include "drivers/serial_uart.h"
#include "drivers/serial_usb_vcp.h"
#include "drivers/bus_spi.h"
#include "drivers/bus_spi_impl.h"
#include "pg/bus_spi.h"

#include "io/serial.h"

#include "target.h"

#include "stm32f30x.h"

#include "drivers/accgyro/accgyro.h"
#include "drivers/accgyro/accgyro_mpu.h"
#include "drivers/accgyro/accgyro_mpu6500.h"
#include "drivers/accgyro/accgyro_spi_mpu6500.h"

    static serialPort_t * _serial0;

    static gyroDev_t _gyro;

    F3EvoBrushed::F3EvoBrushed(void)
    {
        initMotors();
        initUsb();
        initImu();

        RealBoard::init();
    }

    void F3EvoBrushed::initImu(void)
    {
        spiPinConfigure(spiPinConfig(0));
        spiPreInit();

        SPIDevice spiDevice = SPIINVALID;

        if (MPU6500_SPI_INSTANCE == SPI1) {
            spiDevice = SPIDEV_1;
        }

        else if (MPU6500_SPI_INSTANCE == SPI2) {
            spiDevice = SPIDEV_2;
        }

        else if (MPU6500_SPI_INSTANCE == SPI3) {
            spiDevice = SPIDEV_3;
        }

        spiInit(spiDevice);

        spiBusSetInstance(&_gyro.bus, MPU6500_SPI_INSTANCE);

        _gyro.bus.busdev_u.spi.csnPin = IOGetByTag(IO_TAG(MPU6500_CS_PIN));

        delay(100);

        IOInit(_gyro.bus.busdev_u.spi.csnPin, OWNER_MPU_CS, 0);
        IOConfigGPIO(_gyro.bus.busdev_u.spi.csnPin, SPI_IO_CS_CFG);
        IOHi(_gyro.bus.busdev_u.spi.csnPin);
        spiSetDivisor(_gyro.bus.busdev_u.spi.instance, SPI_CLOCK_FAST);

        delay(100);

        busWriteRegister(&_gyro.bus, MPU_RA_PWR_MGMT_1, MPU6500_BIT_RESET);
        delay(100);
        busWriteRegister(&_gyro.bus, MPU_RA_SIGNAL_PATH_RESET, 0x07);
        delay(100);
        busWriteRegister(&_gyro.bus, MPU_RA_PWR_MGMT_1, 0);
        delay(100);
        busWriteRegister(&_gyro.bus, MPU_RA_PWR_MGMT_1, INV_CLK_PLL);
        delay(15);
        busWriteRegister(&_gyro.bus, MPU_RA_GYRO_CONFIG, INV_FSR_2000DPS << 3);
        delay(15);
        busWriteRegister(&_gyro.bus, MPU_RA_ACCEL_CONFIG, INV_FSR_16G << 3);
        delay(15);
        busWriteRegister(&_gyro.bus, MPU_RA_CONFIG, 0); // no DLPF bits
        delay(15);
        busWriteRegister(&_gyro.bus, MPU_RA_SMPLRT_DIV, _gyro.mpuDividerDrops); // Get Divider Drops
        delay(100);

        // Data ready interrupt configuration
        busWriteRegister(&_gyro.bus, MPU_RA_INT_PIN_CFG, MPU6500_BIT_INT_ANYRD_2CLEAR);  // INT_ANYRD_2CLEAR
        delay(15);

        busWriteRegister(&_gyro.bus, MPU_RA_INT_ENABLE, MPU6500_BIT_RAW_RDY_EN); // RAW_RDY_EN interrupt enable
        delay(15);
    }

    void F3EvoBrushed::initUsb(void)
    {
        _serial0 = usbVcpOpen();
    }

    void F3EvoBrushed::initMotors(void)
    {
        motorDevConfig_t dev;

        dev.motorPwmRate = BRUSHED_PWM_RATE;
        dev.motorPwmProtocol = PWM_TYPE_BRUSHED;
        dev.motorPwmInversion = false;
        dev.useUnsyncedPwm = true;
        dev.useBurstDshot = false;

        dev.ioTags[0] = timerioTagGetByUsage(TIM_USE_MOTOR, 0);
        dev.ioTags[1] = timerioTagGetByUsage(TIM_USE_MOTOR, 1);
        dev.ioTags[2] = timerioTagGetByUsage(TIM_USE_MOTOR, 2);
        dev.ioTags[3] = timerioTagGetByUsage(TIM_USE_MOTOR, 3);

        motorDevInit(&dev, BRUSHED_IDLE_PULSE, 4);

        pwmEnableMotors();
    }

    void F3EvoBrushed::writeMotor(uint8_t index, float value)
    {
        pwmWriteMotor(index, MOTOR_MIN + value*(MOTOR_MAX-MOTOR_MIN));
    }

    void systemBeep(bool ignore) { (void)ignore; } // XXX to satisfy EXTI

    void F3EvoBrushed::delaySeconds(float sec)
    {
        delay((uint16_t)(sec*1000));
    }

    void F3EvoBrushed::setLed(bool is_on)
    {
        ledSet(0, is_on);
    }

    uint32_t F3EvoBrushed::getMicroseconds(void)
    {
        return micros();
    }

    void F3EvoBrushed::reboot(void)
    {
        systemResetToBootloader();
    }

    uint8_t F3EvoBrushed::serialAvailableBytes(void)
    {
        return serialRxBytesWaiting(_serial0);
    }

    uint8_t F3EvoBrushed::serialReadByte(void)
    {
        return serialRead(_serial0);
    }

    void F3EvoBrushed::serialWriteByte(uint8_t c)
    {
        serialWrite(_serial0, c);
    }

    bool F3EvoBrushed::imuRead(void)
    {
        static const uint8_t dataToSend[7] = {MPU_RA_GYRO_XOUT_H | 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        uint8_t data[7];

        if (!spiBusTransfer(&_gyro.bus, dataToSend, data, 7)) {
            return false;
        }

        int gy = (int16_t)((data[1] << 8) | data[2]);
        int gx = (int16_t)((data[3] << 8) | data[4]);
        int gz = (int16_t)((data[5] << 8) | data[6]);

        spiBusReadRegisterBuffer(&_gyro.bus, MPU_RA_ACCEL_XOUT_H | 0x80, data, 6);

        // Note reversed X/Y order because of IMU rotation
        int16_t ax = (int16_t)((data[0] << 8) | data[3]);
        int16_t ay = (int16_t)((data[2] << 8) | data[1]);
        int16_t az = (int16_t)((data[4] << 8) | data[5]);

        hf::Debug::printf("%d %d %d %d %d %d\n", ax, ay, az, gx, gy, gz);

        return true;
    }

    void hf::Board::outbuf(char * buf)
    {
        for (char *p=buf; *p; p++)
            serialWrite(_serial0, *p);
    }

} // extern "C"
