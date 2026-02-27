################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/drivers/video/lvds/lvds_pll_data.c \
../src/drivers/video/lvds/lvds_pll_main.c 

LST += \
lvds_pll_data.lst \
lvds_pll_main.lst 

C_DEPS += \
./src/drivers/video/lvds/lvds_pll_data.d \
./src/drivers/video/lvds/lvds_pll_main.d 

OBJS += \
./src/drivers/video/lvds/lvds_pll_data.o \
./src/drivers/video/lvds/lvds_pll_main.o 

MAP += \
mcr_camera_2026base.map 


# Each subdirectory must supply rules for building sources it contributes
src/drivers/video/lvds/%.o: ../src/drivers/video/lvds/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-gcc @"$@.in"

