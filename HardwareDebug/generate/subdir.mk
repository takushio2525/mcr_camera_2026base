################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LINKER_SCRIPT += \
../generate/linker_script.ld \
../generate/linker_script_ram.ld 

S_UPPER_SRCS += \
../generate/start.S \
../generate/vect_table.S 

C_SRCS += \
../generate/hwsetup.c \
../generate/inthandler.c \
../generate/mbed_sf_boot.c \
../generate/vects.c 

LST += \
hwsetup.lst \
inthandler.lst \
mbed_sf_boot.lst \
start.lst \
vect_table.lst \
vects.lst 

C_DEPS += \
./generate/hwsetup.d \
./generate/inthandler.d \
./generate/mbed_sf_boot.d \
./generate/vects.d 

OBJS += \
./generate/hwsetup.o \
./generate/inthandler.o \
./generate/mbed_sf_boot.o \
./generate/start.o \
./generate/vect_table.o \
./generate/vects.o 

S_UPPER_DEPS += \
./generate/start.d \
./generate/vect_table.d 

MAP += \
mcr_camera_2026base.map 


# Each subdirectory must supply rules for building sources it contributes
generate/%.o: ../generate/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate" -I"C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/src" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-gcc @"$@.in"
generate/%.o: ../generate/%.S
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<")
	@arm-none-eabi-gcc @"$@.in"

