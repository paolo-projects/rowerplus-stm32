################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/data_storage.c \
../Core/Src/ehd_math.c \
../Core/Src/hall_parser.c \
../Core/Src/least_mean_squares.c \
../Core/Src/linear_regression.c \
../Core/Src/main.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/systemtime.c 

OBJS += \
./Core/Src/data_storage.o \
./Core/Src/ehd_math.o \
./Core/Src/hall_parser.o \
./Core/Src/least_mean_squares.o \
./Core/Src/linear_regression.o \
./Core/Src/main.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/systemtime.o 

C_DEPS += \
./Core/Src/data_storage.d \
./Core/Src/ehd_math.d \
./Core/Src/hall_parser.d \
./Core/Src/least_mean_squares.d \
./Core/Src/linear_regression.d \
./Core/Src/main.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/systemtime.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../Core/Inc -I"E:/STM32CubeIDE/workspace_1.9.0/RowerPlusFirmware/Drivers/CMSIS/DSP/Include" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/HID/Inc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/data_storage.d ./Core/Src/data_storage.o ./Core/Src/data_storage.su ./Core/Src/ehd_math.d ./Core/Src/ehd_math.o ./Core/Src/ehd_math.su ./Core/Src/hall_parser.d ./Core/Src/hall_parser.o ./Core/Src/hall_parser.su ./Core/Src/least_mean_squares.d ./Core/Src/least_mean_squares.o ./Core/Src/least_mean_squares.su ./Core/Src/linear_regression.d ./Core/Src/linear_regression.o ./Core/Src/linear_regression.su ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/systemtime.d ./Core/Src/systemtime.o ./Core/Src/systemtime.su

.PHONY: clean-Core-2f-Src

