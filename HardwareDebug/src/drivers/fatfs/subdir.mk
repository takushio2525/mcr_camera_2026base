################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables
C_SRCS += \
../src/drivers/fatfs/ff.c

CPP_SRCS += \
../src/drivers/fatfs/diskio.cpp

LST += \
ff.lst \
diskio.lst

OBJS += \
./src/drivers/fatfs/ff.o \
./src/drivers/fatfs/diskio.o

MAP += \
mcr_camera_2026base.map

C_DEPS += \
./src/drivers/fatfs/ff.d

CPP_DEPS += \
./src/drivers/fatfs/diskio.d


# Each subdirectory must supply rules for building sources it contributes
src/drivers/fatfs/%.o: ../src/drivers/fatfs/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src/drivers/fatfs" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-gcc @"$@.in"
src/drivers/fatfs/%.o: ../src/drivers/fatfs/%.cpp
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src/drivers/fatfs" -fabi-version=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-g++ @"$@.in"

