#ifndef __POWER_MANAGER_H__
#define __POWER_MANAGER_H__

#include <rtthread.h>
#include <rtdevice.h>
#include "stm32f4xx_hal.h"

#ifndef rt_max
#define rt_max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef rt_min
#define rt_min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

/* ==================== Hardware Configuration (Modify according to actual circuit) ==================== */
// ADC device name
#define BATTERY_ADC_DEV_NAME        "adc1"
#define ADAPTER_ADC_DEV_NAME        "adc1"
// ADC channel definition 
#define BATTERY_VOLTAGE_CHANNEL     2   // Battery voltage sampling channel(PA2) - ADC123_IN2
#define ADAPTER_DETECT_CHANNEL      3   // Adapter detection channel(PA3) - ADC123_IN3 (High=12V adapter exists, Low=none)

// Charging enable pin definition (PE12)
#define CHARGE_EN_PIN               GPIO_PIN_12
#define CHARGE_EN_GPIO_PORT         GPIOE

/* ==================== Voltage Calculation Parameters ==================== */
// ADC reference voltage(mV) - STM32F4 default 3.3V
#define ADC_REF_VOLTAGE             3300.0f 
// ADC resolution(12-bit)
#define ADC_RESOLUTION              4095.0f

// Battery valid voltage range (Li-ion battery)
#define BATT_VALID_MIN_VOLT         2.0f    // Minimum valid voltage (undervoltage protection)
#define BATT_VALID_MAX_VOLT         4.2f    // Maximum valid voltage (full charge)
#define BATTERY_DIV_RATIO           2.0f    // R37=100k, R38=100k → Divider ratio 0.5 → Coefficient=2
#define BATT_VOLTAGE_MINIMUM_VALID    3.3f    //  Minimum valid voltage (Li-ion battery)
// Adapter detection threshold (ADC raw value)
// 3.3V → 4095, 3000 raw ≈ 2.4V → Above this = adapter exists
#define ADAPTER_HIGH_THRESHOLD      2000.0f  

// Battery existence detection parameters
#define BATT_CHECK_SAMPLE_CNT       7       // 7 samples for stability
#define BATT_FLUCTUATION_THRESH     690     // Voltage fluctuation threshold (raw ADC value)

#define LORA_POWER_PIN_NAME "PD.11"
#define LORA_POWER_PIN   rt_pin_get(LORA_POWER_PIN_NAME)

#define EC200_POWER_PIN_NAME "PE.1"
#define EC200_POWER_PIN   rt_pin_get(EC200_POWER_PIN_NAME)

/* ==================== Power Status Structure ==================== */
typedef struct {
    float battery_voltage;          // Current battery voltage(V)
    rt_bool_t adapter_exists;       // Adapter status (RT_TRUE=exists, RT_FALSE=none)
    rt_bool_t charging_enabled;     // Charging status: RT_TRUE(enabled)/RT_FALSE(disabled)
    rt_bool_t battery_Minimum_valid_voltage; // Battery minimum valid voltage status
} power_status_t;

/* ==================== External Interface Function Declarations ==================== */
/**
 * @brief Initialize power management module (Create thread, mutex, initialize GPIO)
 * @return RT_EOK-Success, Others-Failure
 */
rt_err_t power_manager_init(void);

/**
 * @brief Get battery existence status
 * @return RT_TRUE-Battery exists, RT_FALSE-No battery
 */
rt_bool_t power_mgr_get_battery_exists(void);

/**
 * @brief Get current power status
 * @param status Pointer to power status structure (Memory needs to be allocated in advance)
 */
void power_manager_get_status(power_status_t *status);

/**
 * @brief Manually set charging enable status (Optional interface)
 * @param enable RT_TRUE-Enable charging, RT_FALSE-Disable charging
 */
void power_manager_set_charging(rt_bool_t enable);

/**
 * init & reset ec200 power enable
 */
rt_err_t ec200_power_pin_init(void);

#endif /* __POWER_MANAGER_H__ */