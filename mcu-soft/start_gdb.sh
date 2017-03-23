script_dir=`dirname $0`
#arm-elf-eabi-gdb --eval-command="target extended-remote localhost:3333" script_dir/build/mcu-soft.elf
arm-elf-eabi-gdb ${script_dir}/build/mcu-soft.elf --eval-command="target extended-remote | openocd -f ${script_dir}/../config.openocd -c 'gdb_port pipe;log_output ~/openocd.log'"
