################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/drivers/Camera.cpp \
../src/drivers/Encoder.cpp \
../src/drivers/LineDetector.cpp \
../src/drivers/Motor.cpp \
../src/drivers/Onboard.cpp \
../src/drivers/SDCard.cpp \
../src/drivers/SDLogger.cpp \
../src/drivers/Serial.cpp \
../src/drivers/Servo.cpp

LST += \
Camera.lst \
Encoder.lst \
LineDetector.lst \
Motor.lst \
Onboard.lst \
SDCard.lst \
SDLogger.lst \
Serial.lst \
Servo.lst

OBJS += \
./src/drivers/Camera.o \
./src/drivers/Encoder.o \
./src/drivers/LineDetector.o \
./src/drivers/Motor.o \
./src/drivers/Onboard.o \
./src/drivers/SDCard.o \
./src/drivers/SDLogger.o \
./src/drivers/Serial.o \
./src/drivers/Servo.o

MAP += \
mcr_camera_2026base.map

CPP_DEPS += \
./src/drivers/Camera.d \
./src/drivers/Encoder.d \
./src/drivers/LineDetector.d \
./src/drivers/Motor.d \
./src/drivers/Onboard.d \
./src/drivers/SDCard.d \
./src/drivers/SDLogger.d \
./src/drivers/Serial.d \
./src/drivers/Servo.d


# Each subdirectory must supply rules for building sources it contributes
src/drivers/%.o: ../src/drivers/%.cpp
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src" -fabi-version=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-g++ @"$@.in"

