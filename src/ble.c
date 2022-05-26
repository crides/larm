#include <device.h>
#include <settings/settings.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(larm_ble, CONFIG_LARM_LOG_LEVEL);

static uint16_t cts_handle = 0;

#define LARM_BLE_PARAM BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME, \
        BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL)
static const struct bt_data larm_ad_data[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, "Larm", 4),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_UUID_16_ENCODE(0x0100)),   // Generic clock
    /* BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_UUID_16_ENCODE(0x07C6)),   // Multi-color LED array */
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    /* BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)), */
};

static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t err);
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

static uint8_t gatt_disc(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params) {
    if (attr == NULL) {
        return BT_GATT_ITER_STOP;
    }

	if (!bt_uuid_cmp(params->uuid, BT_UUID_CTS)) {
        cts_handle = bt_gatt_attr_value_handle(attr);
        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_STOP;
}

static const char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *weekdays[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static uint8_t gatt_read(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params, const void *data, uint16_t length) {
    if (err) {
        if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
            return BT_GATT_ITER_STOP;
        }
        LOG_ERR("read %u", err);
        return BT_GATT_ITER_STOP;
    }

    LOG_INF("read len %u", length);
    const uint8_t *buf = data;
    uint16_t year;
    memcpy(&year, buf, sizeof(uint16_t));
    LOG_WRN("read time %u %s %u, %s, %u:%u:%u.%03u (%u)", sys_le16_to_cpu(year), months[buf[2] - 1], buf[3], weekdays[buf[7] - 1], buf[4], buf[5], buf[6], buf[8], buf[9]);
    return BT_GATT_ITER_STOP;
}

static struct bt_gatt_discover_params disc_params = {
    .uuid = BT_UUID_CTS,
    .func = gatt_disc,
    .start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
    .end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
    .type = BT_GATT_DISCOVER_PRIMARY,
};

static struct bt_gatt_read_params read_params = {
    .func = gatt_read,
    /* .handle_count = 1, */
    /* .single = { */
    /*     .handle = 0, */
    /*     .offset = 0, */
    /* }, */
    .handle_count = 0,
    .by_uuid = {
        .uuid = BT_UUID_CTS_CURRENT_TIME,
        .start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
        .end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
    },
};

static void connected(struct bt_conn *conn, uint8_t err) {
    int ret;
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_WRN("connect %s %u", log_strdup(addr), err);
        return;
    }

    ret = bt_conn_set_security(conn, BT_SECURITY_L2);
    if (ret < 0) {
        LOG_ERR("sec %d", ret);
        return;
    }

    /* ret = bt_gatt_discover(conn, &disc_params); */
    /* if (ret < 0) { */
    /*     LOG_ERR("disc %d", ret); */
    /* } */

    /* read_params.single.handle = cts_handle; */
    ret = bt_gatt_read(conn, &read_params);
    if (ret < 0) {
        LOG_ERR("read %d", ret);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t err) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_WRN("disconnect %s %u", log_strdup(addr), err);
        return;
    }
    cts_handle = 0;
}

static int ble_init(const struct device *_arg) {
    int ret;

    ret = bt_enable(NULL);
    if (ret < 0) {
        LOG_ERR("bt %d", ret);
        return ret;
    }

    ret = settings_subsys_init();
    if (ret < 0) {
        LOG_ERR("settings init %d", ret);
        return ret;
    }

    ret = settings_load();
    if (ret < 0) {
        LOG_ERR("settings load %d", ret);
        return ret;
    }

    bt_conn_cb_register(&conn_callbacks);

    ret = bt_le_adv_start(LARM_BLE_PARAM, larm_ad_data, ARRAY_SIZE(larm_ad_data), NULL, 0);
    if (ret < 0) {
        LOG_ERR("adv %d", ret);
        return ret;
    }

    LOG_INF("larm init");

    return 0;
}

SYS_INIT(ble_init, APPLICATION, CONFIG_BLE_INIT_PRIORITY);
