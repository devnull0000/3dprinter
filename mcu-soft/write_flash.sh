script_dir=`dirname $0`
openocd -f ${script_dir}/../config.openocd -c "init; reset halt; stm32f2x mass_erase 0; reset halt; flash write_bank 0 ${script_dir}/build/mcu-soft.bin 0; reset halt; exit;" 
