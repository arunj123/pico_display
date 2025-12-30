# This file is mandatory. 
# It includes the makefiles for any custom packages you might add later.
include $(sort $(wildcard $(BR2_EXTERNAL_IOT_GATEWAY_PATH)/package/*/*.mk))
