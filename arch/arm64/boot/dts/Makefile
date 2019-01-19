dts-dirs += apm
dts-dirs += arm
dts-dirs += cavium
#liqiang@wind-mobi.com 20180207 begin
ifeq ($(CONFIG_WIND_PRO_E300LWW),y)
dts-dirs += qcom_E300LWW
else ifeq ($(CONFIG_WIND_PRO_A306),y)
dts-dirs +=  qcom_A306
else 
dts-dirs += qcom
endif
#liqiang@wind-mobi.com 20180207 end
subdir-y	:= $(dts-dirs)
