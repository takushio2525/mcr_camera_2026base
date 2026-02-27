################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/drivers/video/video_decoder/video_decoder.c 

LST += \
video_decoder.lst 

C_DEPS += \
./src/drivers/video/video_decoder/video_decoder.d 

OBJS += \
./src/drivers/video/video_decoder/video_decoder.o 

MAP += \
mcr_camera_2026base.map 


# Each subdirectory must supply rules for building sources it contributes
src/drivers/video/video_decoder/%.o: ../src/drivers/video/video_decoder/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-gcc @"$@.in"

