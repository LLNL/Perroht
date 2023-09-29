#!/bin/bash

# ------------------------- #
# Utility functions
# ------------------------- #
# Makes a directory if it does not exit
make_dir() {
  if [ ! -d "$1" ]; then
    mkdir -p $1
  fi
}

# Executes a command, showing some information
execute() {
  echo "Command: " "$@" | tee -a ${LOG_FILE}
  echo ">>>>>" | tee -a ${LOG_FILE}
  time "$@" | tee -a ${LOG_FILE}
  echo "<<<<<" | tee -a ${LOG_FILE}
}

# Executes a command, showing some information
execute_simple() {
  echo "$@" | tee -a ${LOG_FILE}
  "$@" | tee -a ${LOG_FILE}
  echo "" | tee -a ${LOG_FILE}
}

echo_simple() {
  echo "" | tee -a ${LOG_FILE}
}

# Logs a string with a heading style
log_header() {
  echo "" | tee -a ${LOG_FILE}
  echo "----------------------------------------" | tee -a ${LOG_FILE}
  echo "$@" | tee -a ${LOG_FILE}
  echo "----------------------------------------" | tee -a ${LOG_FILE}
  echo "" | tee -a ${LOG_FILE}
}

# Tries to get used compiler version from a given executable
try_to_get_compiler_info() {
  local exec_file_name=$1
  log_header "Compiler information in ${exec_file_name}"
  execute_simple sh -c "strings ${exec_file_name} | grep GCC"
}

# Logs some system information
log_system_info() {
  if [[ $OSTYPE == "linux"* ]]; then
    log_header "DRAM Usage"
    execute_simple free -g

    log_header "VM Configuration"
    execute_simple cat /proc/sys/vm/dirty_expire_centisecs
    execute_simple cat /proc/sys/vm/dirty_ratio
    execute_simple cat /proc/sys/vm/dirty_background_ratio
    execute_simple cat /proc/sys/vm/dirty_writeback_centisecs
  fi

  log_header "Storage Information"
  execute_simple df -lh
  execute_simple mount
}

run () {
    local scale=$1
    local dataset_dir="/dev/shm"
    local num_operations=$((2**${scale}))
    local batch_size=$((2**25))
    local dataset_path="${dataset_dir}/dataset.txt"
    local dataset2_path="${dataset_dir}/dataset2.txt"
    local datastore_path="/dev/shm/datastore"

    log_header "Insert"
    execute ./gen_insert_dataset -m 0 -n ${num_operations} -i ${dataset_path}
    echo_simple ""
    execute_simple rm -rf ${datastore_path}
    execute ./bench_insert -n 5 -b $batch_size -i ${dataset_path} -t 0 -d ${datastore_path}
    echo_simple ""
    execute_simple rm -f ${dataset_path}

    for hit_rate in 0.5 1.0; do
        log_header "Find with hit rate ${hit_rate}"
        execute ./gen_find_dataset -m 0 -n ${num_operations} -k ${num_operations} -i ${dataset_path} -f ${dataset2_path} -h ${hit_rate}
        echo_simple ""
        execute_simple rm -rf ${datastore_path}
        execute ./bench_find -n 5 -b $batch_size -i ${dataset_path} -f ${dataset2_path} -t 0 -d ${datastore_path}
        echo_simple ""
        execute_simple rm -f ${dataset_path}
        execute_simple rm -f ${dataset2_path}
    done

    for erase_ratio in 0.5; do
        log_header "Erase with erase rate ${erase_ratio}"
        execute ./gen_erase_dataset -m 1 -n $((${num_operations}*2)) -o ${dataset_path} -e ${erase_ratio}
        echo_simple ""
        execute_simple rm -rf ${datastore_path}
        execute ./bench_erase -n 5 -b $batch_size -e ${dataset_path} -t 0 -d ${datastore_path}
        echo_simple ""
        execute_simple rm -f ${dataset_path}
    done

    for erase_ratio in 0.2 0.4; do
        log_header "Mixed-erase with erase rate ${erase_ratio}"
        execute ./gen_erase_dataset -m 1 -n ${num_operations} -o ${dataset_path} -e ${erase_ratio}
        echo_simple ""
        execute_simple rm -rf ${datastore_path}
        execute ./bench_erase -n 5 -b $batch_size -e ${dataset_path} -t 0 -d ${datastore_path}
        echo_simple ""
        execute_simple rm -f ${dataset_path}
    done
}

main () {
    local scale=$1
    LOG_FILE="log.txt"
    # If there are two arguments, the second is the log file
    if [ $# -eq 2 ]; then
        LOG_FILE=$2
    fi

    run ${scale}
}

main "$@"
