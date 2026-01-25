################################################################################
#
# ble_daemon
#
################################################################################

BLE_DAEMON_VERSION = 1.0
BLE_DAEMON_SITE = $(BR2_EXTERNAL_IOT_GATEWAY_PATH)/package/ble_daemon/src
BLE_DAEMON_SITE_METHOD = local

# CMake package
$(eval $(cmake-package))

# Manual init script install
define BLE_DAEMON_INSTALL_INIT_SYSV
    $(INSTALL) -D -m 0755 $(BR2_EXTERNAL_IOT_GATEWAY_PATH)/board/overlay/etc/init.d/S98ble_daemon \
        $(TARGET_DIR)/etc/init.d/S98ble_daemon
endef

# Manual target install (since CMakeLists.txt might lack install)
define BLE_DAEMON_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/ble_daemon $(TARGET_DIR)/usr/bin/ble_daemon
endef
