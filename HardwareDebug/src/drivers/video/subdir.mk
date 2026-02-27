################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/drivers/video/DisplayBase.cpp 

C_SRCS += \
../src/drivers/video/gr_peach_vdc5.c \
../src/drivers/video/lcd_settings.c 

LST += \
DisplayBase.lst \
gr_peach_vdc5.lst \
lcd_settings.lst 

C_DEPS += \
./src/drivers/video/gr_peach_vdc5.d \
./src/drivers/video/lcd_settings.d 

OBJS += \
./src/drivers/video/DisplayBase.o \
./src/drivers/video/gr_peach_vdc5.o \
./src/drivers/video/lcd_settings.o 

MAP += \
mcr_camera_2026base.map 

CPP_DEPS += \
./src/drivers/video/DisplayBase.d 


# Each subdirectory must supply rules for building sources it contributes
src/drivers/video/%.o: ../src/drivers/video/%.cpp
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src" -fabi-version=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-g++ @"$@.in"
src/drivers/video/%.o: ../src/drivers/video/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-gcc @"$@.in"

