################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/drivers/video/vdec/src/r_vdec.c \
../src/drivers/video/vdec/src/r_vdec_check_parameter.c \
../src/drivers/video/vdec/src/r_vdec_register.c \
../src/drivers/video/vdec/src/r_vdec_register_address.c 

LST += \
r_vdec.lst \
r_vdec_check_parameter.lst \
r_vdec_register.lst \
r_vdec_register_address.lst 

C_DEPS += \
./src/drivers/video/vdec/src/r_vdec.d \
./src/drivers/video/vdec/src/r_vdec_check_parameter.d \
./src/drivers/video/vdec/src/r_vdec_register.d \
./src/drivers/video/vdec/src/r_vdec_register_address.d 

OBJS += \
./src/drivers/video/vdec/src/r_vdec.o \
./src/drivers/video/vdec/src/r_vdec_check_parameter.o \
./src/drivers/video/vdec/src/r_vdec_register.o \
./src/drivers/video/vdec/src/r_vdec_register_address.o 

MAP += \
mcr_camera_2026base.map 


# Each subdirectory must supply rules for building sources it contributes
src/drivers/video/vdec/src/%.o: ../src/drivers/video/vdec/src/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-gcc @"$@.in"

