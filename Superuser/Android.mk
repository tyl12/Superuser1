my_path := $(call my-dir)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v4
LOCAL_PACKAGE_NAME := Superuser
LOCAL_SRC_FILES := $(call all-java-files-under,src) $(call all-java-files-under,../Widgets/Widgets/src)

LOCAL_AAPT_INCLUDE_ALL_RESOURCES := true
LOCAL_AAPT_FLAGS := --extra-packages com.xiaomeng.widgets -S $(LOCAL_PATH)/../Widgets/Widgets/res --auto-add-overlay --rename-manifest-package $(SUPERUSER_PACKAGE)

include $(BUILD_PACKAGE)

#include $(LOCAL_PATH)/jni/Android.mk
