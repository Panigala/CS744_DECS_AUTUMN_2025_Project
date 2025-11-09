#USAGE:
                                
#   ./load_test.sh  "<client_args>"  <CPU_CLIENT>  <CPU_SERVER>  <CPU_STATOOL>  <INTERVAL_MPSTAT>  <INTERVAL_IOSTAT>  <INTERVAL_VMSTAT>
 
#    "<client_args>"       "number_of_thread  time_duration  GET%  PUT%  DELETE%  POPULAR% "
#    <CPU_CLIENT>          cpu_for_client_run
#    <CPU_SERVER>          cpu_for_server_run
#    <CPU_STATOOL>         cpu_for_statistic_tools_run
#    <INTERVAL_MPSTAT>     stat_tool_time_interval_that_mpstat_take_statistics
#    <INTERVAL_IOSTAT>     stat_tool_time_interval_that_iostat_take_statistics
#    <INTERVAL_VMSTAT>     stat_tool_time_interval_that_vmstat_take_statistics

# Example:
#   ./load_test.sh "100 10 0 100 0 0"  2-7  7  8  1 1 1 
#...........................................................................

#          user configurattion if not default ones too  
CLIENT_ARGS="$1"                          # Client parameters string
CPU_CLIENT=${2:-"0-5"}                    # CPU cores for client workload
CPU_SERVER=${3:-"6"}                      # CPU core for server
CPU_STATOOL=${4:-"8"}                     # CPU core for statistical analysis tools
INTERVAL_mp=${5:-1}                       # time interval for taking mpstat
INTERVAL_io=${6:-1}                       # time interval for taking iostat
INTERVAL_vm=${7:-1}                       # time interval for taking vmstat
DEVICE="nvme0n1"                          # fix disk device ie, that database going to access


IFS=' ' read -r THREADS DURATION GET PUT DEL POP <<< "$CLIENT_ARGS" #parse client arguments
CMD="taskset -c $CPU_CLIENT ./bin/client $THREADS $DURATION $GET $PUT $DEL $POP" #command format for running client based on  given arguments else default
OUTPUT_DIR="results/monitor_logs" #setting the result directory path for saving output
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S") #getting current timestamp in defined format
OUTFILE="$OUTPUT_DIR/stats_${TIMESTAMP}_threads=${THREADS}_time=${DURATION}s_GET=${GET}%_PUT=${PUT}%_DEL=${DEL}%_POP=${POP}%.txt" #setting proper result file naming
TMPDIR="results/monitor_logs/temp_${TIMESTAMP}" #temporary directory for storing IOSTAT,MPSTAT,...  results for later processing

mkdir -p "$OUTPUT_DIR" "$TMPDIR" #making thos directories if not exists ie, if already it just give some error message thats it

echo 
echo "--------------------------------------------------------------"
echo ">>> Running workload: $CMD"
echo ">>> Client pinned to CPU(s): $CPU_CLIENT"
echo ">>> Server pinned to CPU(s): $CPU_SERVER"
echo ">>> Monitoring tools pinned to CPU core(s): $CPU_STATOOL"
echo "--------------------------------------------------------------"
echo 

#run workload in background in same terminal
eval "$CMD" &
CLIENT_PID=$!

#start monitoring tools pinned to CPU_STATOOL and mpstat for CPU_SERVER
taskset -c $CPU_STATOOL mpstat -P $CPU_SERVER $INTERVAL_mp > "$TMPDIR/cpu_server.log" 2>&1 & #the outputs save to temperory directory for processing later -->for following also same
MPSTAT_SERVER_PID=$! #getting pid for killing after client finishes for the following also
taskset -c $CPU_STATOOL mpstat -P $CPU_CLIENT $INTERVAL_mp > "$TMPDIR/cpu_client.log" 2>&1 &
MPSTAT_CLIENT_PID=$!
taskset -c $CPU_STATOOL iostat -dx $INTERVAL_io > "$TMPDIR/disk.log" 2>&1 &   #-y Omit first report (since it’s since-boot stats, not interval stats)  -z Omit devices with no activity
IOSTAT_PID=$! 
taskset -c $CPU_STATOOL vmstat $INTERVAL_vm > "$TMPDIR/mem.log" 2>&1 &
VMSTAT_PID=$!

#wait for client to finish
wait $CLIENT_PID

#top monitoring just after client finished
echo 
echo ">>> Workload finished, stopping monitors..."
kill $MPSTAT_SERVER_PID $MPSTAT_CLIENT_PID $IOSTAT_PID $VMSTAT_PID 2>/dev/null
sleep 1
echo 


#               DATA EXTRACTION
#average CPU usage for SERVER only ie CPU_SERVER
CPU_IDLE_AVG_SERVER=$(awk '/^[0-9]/ && $NF ~ /^[0-9.]+$/ {sum+=$NF; count++} END{if(count>0) printf("%.2f", sum/count); else print 0}' "$TMPDIR/cpu_server.log")
CPU_USAGE_AVG_SERVER=$(echo "scale=2; 100 - $CPU_IDLE_AVG_SERVER" | bc)

#average CPU usage for CLIENT only ie CPU_CLIENT
CPU_IDLE_AVG_CLIENT=$(awk '/^[0-9]/ && $NF ~ /^[0-9.]+$/ {sum+=$NF; count++} END{if(count>0) printf("%.2f", sum/count); else print 0}' "$TMPDIR/cpu_client.log")
CPU_USAGE_AVG_CLIENT=$(echo "scale=2; 100 - $CPU_IDLE_AVG_CLIENT" | bc)

#disk utilization ie, when server  accesses DB
DEVICE_LINE=$(awk -v dev="$DEVICE" '$1 == dev {print}' "$TMPDIR/disk.log" | tail -1)
DISK_UTIL_AVG=$(echo "$DEVICE_LINE" | awk '{print $(NF)}')


#   write stat report to the specified files as following specified format
{
echo "--------------------------------------------------------------"
echo "              SYSTEM RESOURCE MONITOR REPORT               "
echo "--------------------------------------------------------------"
echo 
echo "Timestamp: $(date)"
echo "Command  : $CMD"
echo "Client CPU(s): $CPU_CLIENT"
echo "Server CPU(s): $CPU_SERVER"
echo "Monitor CPU(s): $CPU_STATOOL"
echo "Disk Device: $DEVICE"
echo
echo ">>>>>>>>>>>>>>>>>>>>>> SUMMARY <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
echo "Average Server CPU Usage (%) : $CPU_USAGE_AVG_SERVER"
echo "Average Client CPU Usage (%) : $CPU_USAGE_AVG_CLIENT"
echo "Average Disk Util (%)        : $DISK_UTIL_AVG"
echo
echo echo "--------------------------------------------------------------"
echo
echo ">>> CPU Stats (mpstat -P $CPU_SERVER):"
cat "$TMPDIR/cpu_server.log"
echo
echo ">>> CPU Stats (mpstat -P $CPU_CLIENT):"
cat "$TMPDIR/cpu_client.log"
echo
echo ">>> Disk Stats (iostat -dx) [Filtered for $DEVICE only]:"
awk -v dev="$DEVICE" '$1 == dev {print}' "$TMPDIR/disk.log"
echo
echo ">>> Memory Stats (vmstat):"
cat "$TMPDIR/mem.log"
echo echo "--------------------------------------------------------------"
} > "$OUTFILE"

echo "Report saved: $OUTFILE"

# --- Cleanup temporary directory ---
# rm -rf "$TMPDIR" #remove temporary directory after but for diagnostics can keep too



