#include <device.h>
#include <drivers/gpio.h>
#include <drivers/led_strip.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(larm, CONFIG_LARM_LOG_LEVEL);

#define NEOPIXEL_NODE DT_NODELABEL(neopixel)
#define NEOPIXEL_CHAIN_LEN DT_PROP(NEOPIXEL_NODE, chain_length)
#define NEOPIXEL_NUM_COLORS DT_PROP_LEN(NEOPIXEL_NODE, color_mapping)
#define NEOPIXEL_NUM_CHANS (NEOPIXEL_CHAIN_LEN * NEOPIXEL_NUM_COLORS)

static const struct device *neopixel = DEVICE_DT_GET(NEOPIXEL_NODE);

int main() {
    int ret;

    uint8_t chans[NEOPIXEL_NUM_CHANS] = {0};
    for (int i = 0; i < NEOPIXEL_CHAIN_LEN; i ++) {
        for (int j = 0; j < NEOPIXEL_CHAIN_LEN; j ++) {
            chans[j * NEOPIXEL_NUM_COLORS + 1] = i == j;
        }
        if ((ret = led_strip_update_channels(neopixel, chans, NEOPIXEL_NUM_CHANS)) < 0) {
            LOG_ERR("Can't update: %d", ret);
        }
        k_sleep(K_SECONDS(CONFIG_LARM_WAKE_TIME * 60 / NEOPIXEL_CHAIN_LEN));
    }
    for (int i = 0; i < 256; i ++) {
        for (int j = 0; j < NEOPIXEL_CHAIN_LEN; j ++) {
            chans[j * NEOPIXEL_NUM_COLORS + 2] = i;
            chans[j * NEOPIXEL_NUM_COLORS + 3] = i;
        }
        if ((ret = led_strip_update_channels(neopixel, chans, NEOPIXEL_NUM_CHANS)) < 0) {
            LOG_ERR("Can't update: %d", ret);
        }
        k_sleep(K_SECONDS(CONFIG_LARM_LENGTH * 60 / 256));
    }
    return 0;
}
