#include "power_manager.h"

static rt_thread_t power_monitor_thread = RT_NULL;
static rt_mutex_t power_data_mutex = RT_NULL;
static power_status_t power_status = {0.0f, RT_FALSE, RT_FALSE,RT_TRUE};
static rt_bool_t battery_exists = RT_FALSE;
static rt_adc_device_t adc1_dev = RT_NULL;
static rt_bool_t batt_channel_enabled = RT_FALSE;
static rt_bool_t adapter_channel_enabled = RT_FALSE;
/* ==================== Static Function Declarations ==================== */
/**
 * @brief Read ADC value with noise reduction
 * @param dev_name: ADC device name
 * @param channel: ADC channel number (2=Battery, 3=Adapter)
 * @param is_voltage: RT_TRUE = return voltage (V), RT_FALSE = return raw ADC value
 * @return Processed value (voltage/raw ADC), 0.0f on failure
 */
static float adc_read_value(const char *dev_name, rt_uint32_t channel, rt_bool_t is_voltage);
/**
 * @brief Control charging enable status via GPIO pin (PE12)
 * @param enable: RT_TRUE = enable charging (PE12 LOW), RT_FALSE = disable (PE12 HIGH)
 */
static void set_charging_enable(rt_bool_t enable);
/**
 * @brief Determine battery connection status (voltage range + fluctuation check)
 * @param volt_avg: Averaged battery voltage (V)
 * @param volt_fluctuation: Voltage fluctuation (raw ADC value)
 * @return RT_TRUE = battery connected, RT_FALSE = no battery
 */
static rt_bool_t batt_is_connected(float volt_avg, rt_uint32_t volt_fluctuation);
/**
 * @brief Power monitoring thread entry (500ms sampling cycle)
 * @param parameter: Thread parameter (unused)
 */
static void power_monitor_thread_entry(void *parameter);

static rt_err_t adc_device_init(void);

static rt_err_t charge_pin_init(void);

/* ==================== Static Function Implementations ==================== */
/**
 * @brief Initialize charging enable pin (PE12) - Executed during module init
 * @note Configures PE12 as push-pull output, initial state = charging disabled (HIGH)
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t charge_pin_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();

    gpio_init_struct.Pin = CHARGE_EN_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CHARGE_EN_GPIO_PORT, &gpio_init_struct);

    HAL_GPIO_WritePin(CHARGE_EN_GPIO_PORT, CHARGE_EN_PIN, GPIO_PIN_SET);
    
   // rt_kprintf("[PowerMgr] Charging pin (PE12) initialized (initial state: disabled)\n");
    return RT_EOK;
}

/**
 * @brief Initialize ADC1 device and enable battery/adapter channels
 * @note Executed once during module initialization (no runtime re-initialization)
 * @return RT_EOK on success, -RT_ERROR if ADC1 not found
 */
static rt_err_t adc_device_init(void)
{
    adc1_dev = (rt_adc_device_t)rt_device_find("adc1");
    if (adc1_dev == RT_NULL)
    {
       // rt_kprintf("[PowerMgr] ADC1 device not found! Check RT-Thread ADC driver config.\n");
        return -RT_ERROR;
    }
    if (!batt_channel_enabled)
    {
        rt_adc_enable(adc1_dev, BATTERY_VOLTAGE_CHANNEL);
        batt_channel_enabled = RT_TRUE;
    }
    if (!adapter_channel_enabled)
    {
        rt_adc_enable(adc1_dev, ADAPTER_DETECT_CHANNEL);
        adapter_channel_enabled = RT_TRUE;
    }

    //rt_kprintf("[PowerMgr] ADC1 init success (CH2=Battery, CH3=Adapter)\n");
    return RT_EOK;
}

/**
 * @brief Read ADC value with noise reduction (7 samples, discard max/min)
 * @note Thread-safe (uses RT-Thread ADC device API), no runtime enable/disable
 * @param dev_name: Unused (ADC1 handle is pre-initialized)
 * @param channel: ADC channel (2=Battery, 3=Adapter)
 * @param is_voltage: Convert raw value to voltage (V) if RT_TRUE
 * @return Processed value (voltage/raw ADC)
 */
static float adc_read_value(const char *dev_name, rt_uint32_t channel, rt_bool_t is_voltage)
{
    rt_uint32_t adc_raw_buf[BATT_CHECK_SAMPLE_CNT] = {0}; // 7 raw samples buffer
    rt_uint32_t adc_avg_value = 0;                        // Averaged valid samples
    float result = 0.0f;                                  // Final return value
    rt_uint32_t max_val = 0;                              // Max sample (discarded)
    rt_uint32_t min_val = 0xFFFFFFFF;                     // Min sample (discarded)
    rt_uint32_t volt_fluctuation = 0;                     // Voltage fluctuation (raw ADC)
    float volt_avg = 0.0f;                                // Averaged battery voltage (V)

    for (int i = 0; i < BATT_CHECK_SAMPLE_CNT; i++)
    {
        adc_raw_buf[i] = rt_adc_read(adc1_dev, channel);
        rt_thread_mdelay(10); // Total sampling delay: 70ms
    }

    for (int i = 0; i < BATT_CHECK_SAMPLE_CNT; i++)
    {
        max_val = rt_max(max_val, adc_raw_buf[i]);
        min_val = rt_min(min_val, adc_raw_buf[i]);
    }

    for (int i = 0; i < BATT_CHECK_SAMPLE_CNT; i++)
    {
       
        adc_avg_value += adc_raw_buf[i];
    }
		    adc_avg_value -= (max_val+min_val);

    adc_avg_value /= (BATT_CHECK_SAMPLE_CNT - 2);
    volt_fluctuation = max_val - min_val;
    if (is_voltage)
    {
        result = ((float)adc_avg_value * ADC_REF_VOLTAGE) / (ADC_RESOLUTION * 1000.0f);
        volt_avg = result * BATTERY_DIV_RATIO; // Correct for voltage divider (100k+100k)
        if (volt_avg <= BATT_VOLTAGE_MINIMUM_VALID) 
        power_status.battery_Minimum_valid_voltage =RT_FALSE;
        else power_status.battery_Minimum_valid_voltage = RT_TRUE;
        battery_exists = batt_is_connected(volt_avg, volt_fluctuation);
    }
    else
    {
        result = (float)adc_avg_value;
    }

    int print_val = (int)(is_voltage ? volt_avg * 100 : result * 100);
    //rt_kprintf("[PowerMgr] ADC Channel %d: max=%d, min=%d, avg=%d, value=%d.%02d\n", 
     //          channel, max_val, min_val, adc_avg_value, print_val/100, print_val%100);

    return (is_voltage ? volt_avg : result);
}

/**
 * @brief Determine battery connection status (core anti-interference logic)
 * @param volt_avg: Averaged battery voltage (V)
 * @param volt_fluctuation: Voltage fluctuation (raw ADC value)
 * @return RT_TRUE = battery connected, RT_FALSE = no battery
 */
static rt_bool_t batt_is_connected(float volt_avg, rt_uint32_t volt_fluctuation)
{
    if (volt_fluctuation > BATT_FLUCTUATION_THRESH)
    {
      //  rt_kprintf("[PowerMgr] No battery (fluctuation: %d > threshold: %d)\n", 
       //            volt_fluctuation, BATT_FLUCTUATION_THRESH);
        return RT_FALSE;
    }

    if (volt_avg > BATT_VALID_MIN_VOLT && volt_avg < BATT_VALID_MAX_VOLT)
    {
      //  rt_kprintf("[PowerMgr] Battery connected (voltage: %.2fV)\n", volt_avg);
        return RT_TRUE;
    }
    else
    {
       // rt_kprintf("[PowerMgr] No battery (invalid voltage: %.2fV, range: %.1f~%.1fV)\n", 
       //            volt_avg, BATT_VALID_MIN_VOLT, BATT_VALID_MAX_VOLT);
        return RT_FALSE;
    }
}

/**
 * @brief Control charging enable status (direct GPIO control, no initialization)
 * @note GPIO pin (PE12) is pre-initialized in charge_pin_init()
 * @param enable: RT_TRUE = enable charging (PE12 LOW), RT_FALSE = disable (PE12 HIGH)
 */
static void set_charging_enable(rt_bool_t enable)
{
    if (enable)
    {
        HAL_GPIO_WritePin(CHARGE_EN_GPIO_PORT, CHARGE_EN_PIN, GPIO_PIN_RESET);
        power_status.charging_enabled = RT_TRUE;
     //   rt_kprintf("[PowerMgr] Charging enabled (PE12=LOW)\n");
    }
    else
    {
        HAL_GPIO_WritePin(CHARGE_EN_GPIO_PORT, CHARGE_EN_PIN, GPIO_PIN_SET);
        power_status.charging_enabled = RT_FALSE;
     //   rt_kprintf("[PowerMgr] Charging disabled (PE12=HIGH)\n");
    }
}

/**
 * @brief Get battery connection status (external API)
 * @return RT_TRUE = battery connected, RT_FALSE = no battery
 */
rt_bool_t power_mgr_get_battery_exists(void)
{
    return battery_exists;
}

/**
 * @brief Power monitoring thread main loop (500ms sampling cycle)
 * @note Thread-safe (uses mutex for shared status updates)
 * @param parameter: Unused thread parameter
 */
static void power_monitor_thread_entry(void *parameter)
{
    set_charging_enable(RT_FALSE);

    while (1)
    {
        float bat_v = adc_read_value(BATTERY_ADC_DEV_NAME, BATTERY_VOLTAGE_CHANNEL, RT_TRUE);
			
        rt_thread_mdelay(20); // 20ms delay to avoid ADC channel crosstalk


        float adapter_raw = adc_read_value(ADAPTER_ADC_DEV_NAME, ADAPTER_DETECT_CHANNEL, RT_FALSE);
        rt_bool_t adapter_exists = (adapter_raw >= ADAPTER_HIGH_THRESHOLD) ? RT_TRUE : RT_FALSE;

        rt_mutex_take(power_data_mutex, RT_WAITING_FOREVER);
        power_status.battery_voltage = bat_v;
        power_status.adapter_exists = adapter_exists;
        rt_mutex_release(power_data_mutex);
        if (!battery_exists)
        {

            if (power_status.charging_enabled)
            {
                set_charging_enable(RT_FALSE);
               // rt_kprintf("[PowerMgr] Charging disabled (no battery detected)\n");
            }
        }
        else
        {          
            if (adapter_exists && !power_status.charging_enabled)
            {
                set_charging_enable(RT_TRUE); 
            }
            else if (!adapter_exists && power_status.charging_enabled)
            {
                set_charging_enable(RT_FALSE); 
            }
        }
        rt_thread_mdelay(500);
    }
}

/* ==================== External API Implementations ==================== */
/**
 * @brief Initialize power management module (entry point)
 * @note Initialization sequence: GPIO ? ADC ? Mutex ? Thread
 * @return RT_EOK on success, error code on failure
 */
rt_err_t power_manager_init(void)
{
    rt_err_t ret = RT_EOK;


    ret = charge_pin_init();
    if (ret != RT_EOK)
    {
        rt_kprintf("[PowerMgr] Charging pin initialization failed!\n");
        return ret;
    }

    ret = adc_device_init();
    if (ret != RT_EOK)
    {
        rt_kprintf("[PowerMgr] ADC1 initialization failed!\n");
        return ret;
    }


    power_data_mutex = rt_mutex_create("power_mtx", RT_IPC_FLAG_PRIO);
    if (power_data_mutex == RT_NULL)
    {
        rt_kprintf("[PowerMgr] Failed to create power status mutex!\n");
        return -RT_ERROR;
    }

    power_monitor_thread = rt_thread_create("power_moni", power_monitor_thread_entry, RT_NULL,512,16,10);    

    if (power_monitor_thread == RT_NULL)
    {
        rt_kprintf("[PowerMgr] Failed to create power monitoring thread!\n");
        ret = -RT_ERROR;
    }

    rt_thread_startup(power_monitor_thread);
    rt_kprintf("[PowerMgr] Initialization complete (ADC1: PA2=Battery, PA3=Adapter, PE12=Charge_EN)\n");

    return ret;
}

/**
 * @brief Get current power status (thread-safe external API)
 * @param status: Pointer to power_status_t structure (pre-allocated by caller)
 */
void power_manager_get_status(power_status_t *status)
{
    if (status == RT_NULL) 
    {
        rt_kprintf("[PowerMgr] Invalid pointer for power status!\n");
        return;
    }
    rt_mutex_take(power_data_mutex, RT_WAITING_FOREVER);
    *status = power_status;
    rt_mutex_release(power_data_mutex);
}

/**
 * @brief Manually set charging status (external API with safety check)
 * @param enable: RT_TRUE = enable charging, RT_FALSE = disable charging
 * @note Charging can only be enabled if battery is detected
 */
void power_manager_set_charging(rt_bool_t enable)
{

    if (enable && !battery_exists)
    {
        rt_kprintf("[PowerMgr] Cannot enable charging (no battery detected)\n");
        return;
    }
    set_charging_enable(enable);
}

/**
 * @brief Control LoRa module power on/off
 * @param enable: true - Power on (pull pin LOW), false - Power off (pull pin HIGH)
 */
void lora_power_on(rt_bool_t enable)
{
    if (LORA_POWER_PIN == RT_NULL)
        return;

    if (enable)
    {
        rt_pin_write(LORA_POWER_PIN, PIN_LOW);
        rt_thread_mdelay(10);
    }
    else
    {

        rt_pin_write(LORA_POWER_PIN, PIN_HIGH);
        rt_thread_mdelay(10);
    }
}

rt_err_t lora_power_pin_init(void){
	  rt_pin_mode(LORA_POWER_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(LORA_POWER_PIN, PIN_HIGH);
    rt_thread_mdelay(20);
    rt_pin_write(LORA_POWER_PIN, PIN_LOW);
    rt_thread_mdelay(10);	
}

rt_err_t ec200_power_pin_init(void){
	rt_pin_mode(EC200_POWER_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(EC200_POWER_PIN, PIN_HIGH);
    rt_thread_mdelay(20);
    rt_pin_write(EC200_POWER_PIN, PIN_LOW);
    rt_thread_mdelay(10);	    
};
