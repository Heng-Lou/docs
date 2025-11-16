# GDB debugging commands for three-port switch simulation

# Set source directory
directory /home/heng/workdir/doca/three_port_switch

# Enable pretty printing
set print pretty on
set print array on

# Set breakpoint at packet processing
break process_packet

# Set breakpoint at forwarding decision
break get_forward_port

# Custom command to show packet details
define show_packet
    if $argc == 1
        printf "=== Packet Details ===\n"
        printf "Port ID:   %d\n", $arg0->port_id
        printf "Size:      %d bytes\n", $arg0->size
        printf "Timestamp: %lu\n", $arg0->timestamp
        printf "Data[0-7]: "
        x/8bx $arg0->data
    else
        printf "Usage: show_packet <packet_ptr>\n"
    end
end

# Custom command to show all port stats
define show_all_stats
    printf "\n=== Port Statistics ===\n"
    set $i = 0
    while $i < 3
        printf "\nPort %d (%s):\n", $i, port_config[$i].name
        printf "  Enabled: %s\n", port_config[$i].enabled ? "Yes" : "No"
        printf "  RX: %lu pkts, %lu bytes, %lu errors\n", \
               port_stats[$i].rx_packets, \
               port_stats[$i].rx_bytes, \
               port_stats[$i].rx_errors
        printf "  TX: %lu pkts, %lu bytes, %lu errors\n", \
               port_stats[$i].tx_packets, \
               port_stats[$i].tx_bytes, \
               port_stats[$i].tx_errors
        set $i = $i + 1
    end
    printf "\n"
end

# Auto-display current port stats on each stop
# display port_stats[0].rx_packets
# display port_stats[1].rx_packets  
# display port_stats[2].rx_packets

# Enable logging
set logging file gdb_session.log
set logging on

printf "\n"
printf "========================================\n"
printf "Three-Port Switch GDB Debug Session\n"
printf "========================================\n"
printf "\n"
printf "Breakpoints set at:\n"
printf "  - process_packet\n"
printf "  - get_forward_port\n"
printf "\n"
printf "Custom commands available:\n"
printf "  show_packet <pkt>  - Display packet details\n"
printf "  show_all_stats     - Display all port statistics\n"
printf "\n"
printf "Common GDB commands:\n"
printf "  run test           - Run tests\n"
printf "  continue (c)       - Continue execution\n"
printf "  step (s)           - Step into function\n"
printf "  next (n)           - Step over function\n"
printf "  print <var>        - Print variable\n"
printf "  backtrace (bt)     - Show call stack\n"
printf "\n"
printf "Ready to debug! Type 'run test' to start.\n"
printf "========================================\n"
printf "\n"
