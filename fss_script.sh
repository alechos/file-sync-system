#!/usr/bin/env bash
reg_info="^\[.*\]\ \[.*\]\ \[.*\]\ \[.*\]\ \[.*\]\ \[.*\]\ \[.*\]$"
reg_monitor="^\[.*\]\ Monitoring started for .*$"
reg_sync="^\[.*\]\ Syncing directory: .*$"
reg_cancelled="^\[.*\]\ Monitoring stopped for .*$"


declare -A timestamp status monitored dest

parse_args() {
    while getopts ":p:c:" opt; do
        case $opt in
            p) INPUT_PATH="$OPTARG" ;;
            c) COMMAND="$OPTARG" ;;
            \?) 
                echo "Invalid Command"; exit 1 ;;  
            :) 
                echo "Usage: ./fss_script.sh -p <path> -c <command>"; exit 1 ;;
        esac
    done

}

read_maps() {
    while IFS= read -r line; do
        IFS=" "
        read -r -a fields <<< "$line";
        if [[ "$line" =~ $reg_info ]]; then 

            src="${fields[2]:1:-1}"
            timestamp["$src"]="[Last sync: ${fields[0]:1:${#fields[0]}} ${fields[1]:0:-1}]"
            status["$src"]="[${fields[6]:1:-1}]"
            dest["$src"]="${fields[3]:1:-1}"

        elif [[ "$line" =~ $reg_monitor ]]; then 
            src="${fields[5]}"
            monitored["$src"]=1
         elif [[ "$line" =~ $reg_sync ]]; then 
            src="${fields[4]}"
            monitored["$src"]=1
        elif [[ "$line" =~ ($reg_cancelled) ]]; then 
            src="${fields[5]}"
            monitored["$src"]=0
        fi

            

    done < "$INPUT_PATH"
}

listAll() {
for src in ${!status[@]}; do
    echo "$src -> ${dest[$src]} ${timestamp[$src]} ${status[$src]}"
done
}

listMonitored() {
    for src in ${!status[@]}; do
        if [[ ${monitored[$src]} -eq 1 ]]; then
            echo "$src -> ${dest[$src]} ${timestamp[$src]}"
        fi
    done
}

listStopped() {
    for src in ${!status[@]}; do
        if [[ ${monitored[$src]} -eq 0 ]]; then
            echo "$src -> ${dest[$src]} ${timestamp[$src]}"
        fi
    done
}

purge() {
    echo "Deleting $INPUT_PATH..."
    rm -r "$INPUT_PATH"
    echo "Purge complete."

}

run_command() {
    if [[ "$COMMAND" == "listAll" ]]; then
        listAll
    elif [[ "$COMMAND" == "listMonitored" ]]; then
        listMonitored
    elif [[ "$COMMAND" == "listStopped" ]]; then
        listStopped
    elif [[ "$COMMAND" == "purge" ]]; then
        purge
    fi
}

parse_args "$@"
if [[ "$COMMAND" != "purge" ]]; then
    read_maps 
fi
run_command  "$@"
