#include <stdio.h>
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<mpu6050.h>
#include<bmp180.h>

#include "driver/gpio.h"

#include "unity.h"
#include "driver/i2c.h"
#include "mpu6050.h"


#define I2C_MASTER_SCL_IO 22      /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 21      /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_0  /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */


#define REFERENCE_PRESSURE 101325l

TaskHandle_t myTaskHandle = NULL;
static mpu6050_handle_t mpu6050 = NULL;



/**
 * @brief i2c master initialization
 */
static void i2c_bus_init(void)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "I2C config returned error");

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "I2C install returned error");
}

/**
 * @brief i2c master initialization
 */
static void i2c_sensor_mpu6050_init(void)
{
    esp_err_t ret;

    i2c_bus_init();
    mpu6050 = mpu6050_create(I2C_MASTER_NUM, MPU6050_I2C_ADDRESS);
    TEST_ASSERT_NOT_NULL_MESSAGE(mpu6050, "MPU6050 create returned NULL");

    ret = mpu6050_config(mpu6050, ACCE_FS_4G, GYRO_FS_500DPS);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    ret = mpu6050_wake_up(mpu6050);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}






void Read_MPU6050_Sensor(void *arg){
    esp_err_t ret;
    uint8_t mpu6050_deviceid;
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
    mpu6050_temp_value_t temp;

    while (1)
    {
        ret = mpu6050_get_deviceid(mpu6050, &mpu6050_deviceid);
        TEST_ASSERT_EQUAL(ESP_OK, ret);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(MPU6050_WHO_AM_I_VAL, mpu6050_deviceid, "Who Am I register does not contain expected data");

        ret = mpu6050_get_acce(mpu6050, &acce);
        TEST_ASSERT_EQUAL(ESP_OK, ret);
        printf("acce_x:%.2f, acce_y:%.2f, acce_z:%.2f\n", acce.acce_x, acce.acce_y, acce.acce_z);

        ret = mpu6050_get_gyro(mpu6050, &gyro);
        TEST_ASSERT_EQUAL(ESP_OK, ret);
        printf("gyro_x:%.2f, gyro_y:%.2f, gyro_z:%.2f\n", gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);

        ret = mpu6050_get_temp(mpu6050, &temp);
        TEST_ASSERT_EQUAL(ESP_OK, ret);
        printf("t:%.2f \n", temp.temp);
        printf("--------------------------------------------------\n");
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void Read_BMP180_Sensor(void *arg){
    while(1) {
        esp_err_t err;
        int pressure;
        float altitude;
        float temperature;

        err = bmp180_read_pressure(&pressure);
        if (err != ESP_OK) {
            printf("Reading of pressure from BMP180 failed, err = %d", err);
        }
        err = bmp180_read_altitude(REFERENCE_PRESSURE, &altitude);
        if (err != ESP_OK) {
            printf( "Reading of altitude from BMP180 failed, err = %d", err);
        }
        err = bmp180_read_temperature(&temperature);
        if (err != ESP_OK) {
            printf("Reading of temperature from BMP180 failed, err = %d", err);
        }
        printf("Pressure %d Pa, Altitude %.1f m, Temperature : %.1f degC \n", pressure, altitude, temperature);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    //xTaskCreate(Demo_Task,"Demo_Task",4096,NULL,10,&myTaskHandle);
    i2c_sensor_mpu6050_init();
    xTaskCreate(Read_MPU6050_Sensor,"Read_MPU6050_Sensor",4096,NULL,10,&myTaskHandle);
    
    esp_err_t err;

    err = bmp180_init(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    if(err == ESP_OK){
        xTaskCreate(Read_BMP180_Sensor,"Read_BMP180_Sensor",4096,NULL,10,&myTaskHandle);
    
    } else {
        printf("BMP180 init failed with error = %d", err);
    }
}