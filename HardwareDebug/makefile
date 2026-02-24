################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

-include makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/drivers/subdir.mk
-include src/subdir.mk
-include generate/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_UPPER_DEPS)),)
-include $(CC_UPPER_DEPS)
endif
ifneq ($(strip $(C++M_UPPER_DEPS)),)
-include $(C++M_UPPER_DEPS)
endif
ifneq ($(strip $(C++_UPPER_DEPS)),)
-include $(C++_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_UPPER_DEPS)),)
-include $(CPP_UPPER_DEPS)
endif
ifneq ($(strip $(FSY_DEPS)),)
-include $(FSY_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(SRC_UPPER_DEPS)),)
-include $(SRC_UPPER_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(PP_UPPER_DEPS)),)
-include $(PP_UPPER_DEPS)
endif
ifneq ($(strip $(P_DEPS)),)
-include $(P_DEPS)
endif
ifneq ($(strip $(CXXM_DEPS)),)
-include $(CXXM_DEPS)
endif
ifneq ($(strip $(JMP_UPPER_DEPS)),)
-include $(JMP_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(SX_UPPER_DEPS)),)
-include $(SX_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(JMP_DEPS)),)
-include $(JMP_DEPS)
endif
ifneq ($(strip $(CXX_UPPER_DEPS)),)
-include $(CXX_UPPER_DEPS)
endif
ifneq ($(strip $(CCM_DEPS)),)
-include $(CCM_DEPS)
endif
ifneq ($(strip $(P_UPPER_DEPS)),)
-include $(P_UPPER_DEPS)
endif
ifneq ($(strip $(CXXM_UPPER_DEPS)),)
-include $(CXXM_UPPER_DEPS)
endif
ifneq ($(strip $(FSY_UPPER_DEPS)),)
-include $(FSY_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CCM_UPPER_DEPS)),)
-include $(CCM_UPPER_DEPS)
endif
ifneq ($(strip $(SRC_DEPS)),)
-include $(SRC_DEPS)
endif
ifneq ($(strip $(PP_DEPS)),)
-include $(PP_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(SX_DEPS)),)
-include $(SX_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C++M_DEPS)),)
-include $(C++M_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
mcr_camera_2026base.bin \

SECONDARY_SIZE += \
mcr_camera_2026base.siz \


# All Target
all: mcr_camera_2026base.elf secondary-outputs

# Tool invocations
mcr_camera_2026base.elf: $(OBJS) $(USER_OBJS) $(LIBRARY_GENERATOR_OUTPUTTYPE_OUTPUTS) $(LINKER_SCRIPT)
	$(file > mcr_camera_2026base.elf.in,-mcpu=cortex-a9 -marm -mlittle-endian -mfloat-abi=hard -mfpu=vfpv3-d16 -fdiagnostics-parseable-fixits -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wnull-dereference -Wstack-usage=100 -g -gdwarf-4 -o "mcr_camera_2026base.elf" $(OBJS) $(USER_OBJS) -T "C:/Users/shioz/e2_studio/workspace/mcr_camera_2026base/generate/linker_script.ld"  -Wl,--start-group -Wl,--end-group -nostartfiles -Xlinker --gc-sections -Wl,-Map,"mcr_camera_2026base.map" -Wl,-e_PowerON_Reset --specs=rdimon.specs)
	@echo Building target: $@ && arm-none-eabi-g++ @"mcr_camera_2026base.elf.in"
mcr_camera_2026base.bin: mcr_camera_2026base.elf
	arm-none-eabi-objcopy -O binary --gap-fill 0xff "mcr_camera_2026base.elf"  "mcr_camera_2026base.bin"
mcr_camera_2026base.siz: mcr_camera_2026base.elf
	arm-none-eabi-size --format=berkeley "mcr_camera_2026base.elf"
# Other Targets
clean:
	-$(file > clean_file.tmp,  $(CC_UPPER_DEPS)  $(C++M_UPPER_DEPS)  $(C++_UPPER_DEPS)  $(CPP_UPPER_DEPS)  $(FSY_DEPS)  $(C_UPPER_DEPS)  $(SECONDARY_SIZE)  $(SRC_UPPER_DEPS)  $(S_DEPS)  $(PP_UPPER_DEPS)  $(P_DEPS)  $(CXXM_DEPS)  $(JMP_UPPER_DEPS)  $(ASM_UPPER_DEPS)  $(SX_UPPER_DEPS)  $(CXX_DEPS)  $(JMP_DEPS)  $(CXX_UPPER_DEPS)  $(CCM_DEPS)  $(P_UPPER_DEPS)  $(LST)  $(CXXM_UPPER_DEPS)  $(FSY_UPPER_DEPS)  $(C_DEPS)  $(CCM_UPPER_DEPS)  $(SRC_DEPS)  $(PP_DEPS)  $(CC_DEPS)  $(SX_DEPS)  $(C++_DEPS)  $(OBJS)  $(SECONDARY_FLASH)  $(ASM_DEPS)  $(S_UPPER_DEPS)  $(MAP)  $(C++M_DEPS)  $(CPP_DEPS) )
	-@ xargs -P 2 -s 32000 -t -a clean_file.tmp $(RM)
	-@$(RM) clean_file.tmp
	-$(RM)  mcr_camera_2026base.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents 

-include ../makefile.targets
