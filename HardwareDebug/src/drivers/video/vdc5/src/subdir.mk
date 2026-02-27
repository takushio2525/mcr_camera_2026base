################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/drivers/video/vdc5/src/r_vdc5.c \
../src/drivers/video/vdc5/src/r_vdc5_check_parameter.c \
../src/drivers/video/vdc5/src/r_vdc5_interrupt.c \
../src/drivers/video/vdc5/src/r_vdc5_register.c \
../src/drivers/video/vdc5/src/r_vdc5_register_address.c \
../src/drivers/video/vdc5/src/r_vdc5_shared_param.c 

LST += \
r_vdc5.lst \
r_vdc5_check_parameter.lst \
r_vdc5_interrupt.lst \
r_vdc5_register.lst \
r_vdc5_register_address.lst \
r_vdc5_shared_param.lst 

C_DEPS += \
./src/drivers/video/vdc5/src/r_vdc5.d \
./src/drivers/video/vdc5/src/r_vdc5_check_parameter.d \
./src/drivers/video/vdc5/src/r_vdc5_interrupt.d \
./src/drivers/video/vdc5/src/r_vdc5_register.d \
./src/drivers/video/vdc5/src/r_vdc5_register_address.d \
./src/drivers/video/vdc5/src/r_vdc5_shared_param.d 

OBJS += \
./src/drivers/video/vdc5/src/r_vdc5.o \
./src/drivers/video/vdc5/src/r_vdc5_check_parameter.o \
./src/drivers/video/vdc5/src/r_vdc5_interrupt.o \
./src/drivers/video/vdc5/src/r_vdc5_register.o \
./src/drivers/video/vdc5/src/r_vdc5_register_address.o \
./src/drivers/video/vdc5/src/r_vdc5_shared_param.o 

MAP += \
mcr_camera_2026base.map 


# Each subdirectory must supply rules for building sources it contributes
src/drivers/video/vdc5/src/%.o: ../src/drivers/video/vdc5/src/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-gcc @"$@.in"

