#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

/* --------------------------  -------------------------- */
#define STWD100_WDG_DI_PIN    GET_PIN(B, 0) 
#define STWD100_WDG_EN_PIN    GET_PIN(B, 1) 

#define STWD100_FEED_MS          1500     /* ( < 1120ms) */
#define STWD100_THREAD_STK_SIZE  256
#define STWD100_THREAD_PRIORITY  10  

static rt_thread_t stwd100_thread = RT_NULL;

/* ---------------------------------------------------------
 * Feed STWD100 watchdog
 * --------------------------------------------------------- */
static void stwd100_feed(void)
{
    /* 读取当前电平并翻转 */
    rt_uint8_t level = rt_pin_read(STWD100_WDG_DI_PIN);
    rt_pin_write(STWD100_WDG_DI_PIN, !level);
}

static void stwd100_thread_entry(void *arg)
{
    /* 1. Initialize WDI pin (push-pull output) */
    rt_pin_mode(STWD100_WDG_DI_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(STWD100_WDG_DI_PIN, PIN_LOW);  

    /* 2. Initialize EN pin (optional: circuit is pull-up, default high level enable, here can be manually pulled high to ensure) */
    rt_pin_mode(STWD100_WDG_EN_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(STWD100_WDG_EN_PIN, PIN_LOW);  // Ensure EN pin is high to enable watchdog
    rt_thread_mdelay(100); // Wait for watchdog to stabilize stable
    rt_uint8_t level = rt_pin_read(STWD100_WDG_EN_PIN);

    rt_kprintf("[STWD100] watchdog started, feed every %dms\n", STWD100_FEED_MS);

    /* 3. Feed watchdog in loop */
    while (1)
    {
        stwd100_feed();
        rt_thread_mdelay(STWD100_FEED_MS);
    }
}

/* ---------------------------------------------------------
 * Initialize STWD100 watchdog
 * --------------------------------------------------------- */
int stwd100_init(void)
{
    /* create watchdog thread */
    stwd100_thread = rt_thread_create("stwd100",
                                      stwd100_thread_entry,
                                      RT_NULL,
                                      STWD100_THREAD_STK_SIZE,
                                      STWD100_THREAD_PRIORITY,
                                      10);
    if (stwd100_thread != RT_NULL)
    {
        rt_thread_startup(stwd100_thread);
        return RT_EOK;
    }
    else
    {
        rt_kprintf("[STWD100] create thread failed\n");
        return -RT_ERROR;
    }
}

//INIT_APP_EXPORT(stwd100_init);