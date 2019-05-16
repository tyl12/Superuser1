###################################################################################
#only change binary name su->xmsu


include $(CLEAR_VARS)
APP_ALLOW_MISSING_DEPS = true

LOCAL_MODULE := xmsu
LOCAL_MODULE_TAGS := eng debug optional
LOCAL_LDFLAGS := -static

LOCAL_FORCE_STATIC_EXECUTABLE := true

ifdef FORCE_LOCAL_NDK_BUILD
$(info ## FORCE_LOCAL_NDK_BUILD)

LOCAL_STATIC_LIBRARIES := libc libcutils libselinux

LOCAL_C_INCLUDES := jni/libselinux/include/ jni/libsepol/include/ jni/sqlite3/
LOCAL_C_INCLUDES += libselinux/include/ libsepol/include/ sqlite3/

else
$(info ## AOSP_BUILD)

LOCAL_STATIC_LIBRARIES += libc libcutils libselinux libsepol
LOCAL_C_INCLUDES := $(LOCAL_PATH)/sqlite3/
endif

LOCAL_SRC_FILES := su_dir/su.c su_dir/daemon.c su_dir/activity.c su_dir/db.c su_dir/utils.c su_dir/pts.c sqlite3/sqlite3.c su_dir/hacks.c su_dir/binds.c
LOCAL_CFLAGS := -DSQLITE_OMIT_LOAD_EXTENSION -std=gnu11

#LOCAL_CFLAGS += -DREQUESTOR=\"$(shell cat $(LOCAL_PATH)/../packageName)\"
SPECIFIED_PKG_NAME=$(shell cat $(LOCAL_PATH)/../packageName)
LOCAL_CFLAGS += -DREQUESTOR=\"$(SPECIFIED_PKG_NAME)\"

ifeq ($(strip $(SPECIFIED_PKG_NAME)),)
$(error ## SPECIFIED_PKG_NAME is empty!)
else
$(info ## SPECIFIED_PKG_NAME=$(SPECIFIED_PKG_NAME))
endif


ifdef SUPERUSER_EMBEDDED
  LOCAL_CFLAGS += -DSUPERUSER_EMBEDDED
endif

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
#LOCAL_MODULE_PATH := $(PRODUCT_OUT)/data/bin
#$(shell mkdir -p $(PRODUCT_OUT)/data/bin)
include $(BUILD_EXECUTABLE)

###################################################################################
