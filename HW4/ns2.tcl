
#!/usr/bin/awk
set ns [new Simulator]

$ns color 0 blue
$ns color 1 red

set src1 [$ns node]
set src2 [$ns node]
set r1 	 [$ns node]
set r2	 [$ns node]
set rcv1 [$ns node]
set rcv2 [$ns node]

# Initialize variables
set sum_0 0
set sum_1 0
set rat 0

#   src1            rcv1
#      	 r1-----r2     
#   src2            rcv2

set f [open tcp_0.tr w]
$ns trace-all $f
set nf [open tcp_0.nam w]
$ns namtrace-all $nf

if { $argc != 2 } {
        puts "Follow below syntax, Please try again."    
	puts "\t\tns2.tcl <TCP_flavor> <case_no>\t\t"
    } else {
	set TCP_flavor [lindex $argv 0]
	set case_no    [lindex $argv 1]
        puts "Given TCP_flavor =  $TCP_flavor"
	puts "Given case_no    =  $case_no"
        }



$ns duplex-link $r1 $r2 1Mb  5ms DropTail
$ns queue-limit $r1 $r2 10

if {$case_no == 1} {
	$ns duplex-link $src1 $r1 10Mb 5ms DropTail
	$ns duplex-link $src2 $r1 10Mb 12.5ms DropTail
	$ns duplex-link $r2 $rcv1 10Mb 5ms DropTail
	$ns duplex-link $r2 $rcv2 10Mb 12.5ms DropTail
} elseif {$case_no == 2} {
    	$ns duplex-link $src1 $r1 10Mb 5ms DropTail
	$ns duplex-link $src2 $r1 10Mb 20ms DropTail
	$ns duplex-link $r2 $rcv1 10Mb 5ms DropTail
	$ns duplex-link $r2 $rcv2 10Mb 20ms DropTail
} elseif {$case_no == 3} {
    	$ns duplex-link $src1 $r1 10Mb 5ms DropTail
	$ns duplex-link $src2 $r1 10Mb 27.5ms DropTail
	$ns duplex-link $r2 $rcv1 10Mb 5ms DropTail
	$ns duplex-link $r2 $rcv2 10Mb 27.5ms DropTail
} else { 
	puts "Case Not Supported"
}

puts	"case selected = $case_no"

$ns duplex-link-op $src1 $r1 orient right-down
$ns duplex-link-op $src2 $r1 orient right-up
$ns duplex-link-op $r1 	 $r2 orient right
$ns duplex-link-op $rcv1 $r2 orient left-down
$ns duplex-link-op $rcv2 $r2 orient left-up

$ns duplex-link-op $r1 $r2   queuePos 0.5
$ns duplex-link-op $r2 $rcv1 queuePos 0.5
$ns duplex-link-op $r2 $rcv2 queuePos 0.5


if { $TCP_flavor == "VEGAS"} {
		set tcp_0 [new Agent/TCP/Vegas]
		set tcp_1 [new Agent/TCP/Vegas]
puts "VEGAS it is"
} elseif { $TCP_flavor == "SACK"} {
		set tcp_0 [new Agent/TCP/Sack1]
		set tcp_1 [new Agent/TCP/Sack1]
puts "SACK it is"
} else {
puts "Incorrect TCP flavor"
}

$tcp_0 set class_ 0
$tcp_1 set class_ 1

set sink_0 [new Agent/TCPSink]
$ns attach-agent $src1 $tcp_0
$ns attach-agent $rcv1 $sink_0

$ns connect $tcp_0 $sink_0
# Equivelant of 
# set tcp_0 [$ns create-connection TCP $src1 TCPSink $rcv1 0]

set sink_1 [new Agent/TCPSink]
$ns attach-agent $src2 $tcp_1
$ns attach-agent $rcv2 $sink_1

$ns connect $tcp_1 $sink_1
# Equivelant of 
# set tcp_1 [$ns create-connection TCP $src2 TCPSink $rcv2 1]

set ftp_0 [new Application/FTP]
set ftp_1 [new Application/FTP]

$ftp_0 attach-agent $tcp_0
$ftp_1 attach-agent $tcp_1

$ns at   0.0 "record"
$ns at   1.0 "$ftp_0 start" 
$ns at   1.0 "$ftp_1 start"
$ns at 401.0 "$ftp_1 stop"
$ns at 401.0 "$ftp_0 stop"
$ns at 402.0 "finish"


proc record {} {
        global sink_0 sink_1 sum_0 sum_1 rat
	set ns [Simulator instance]
	set time 1.0
	set bw_0 [$sink_0 set bytes_]
	set bw_1 [$sink_1 set bytes_]
	set now [$ns now]
	#puts "$now-->\t[expr $bw_0/$time*8/1000000]  [expr $bw_1/$time*8/1000000]"
	if {$now >= 100 && $now < 400 }	{
		set sum_0 [expr $sum_0+$bw_0/$time*8/1000000]
		set sum_1 [expr $sum_1+$bw_1/$time*8/1000000]	
	}    
	if {$now == 400} {
		puts "average throughput for link1  = [expr $sum_0 * $time/300] Mbps"	
		puts "average throughput for link2  = [expr $sum_1 * $time/300] Mbps"
		set rat [expr $sum_0/$sum_1]
		puts "through put ration link1/link2 =  [expr $rat]"	
	}
	$sink_0 set bytes_ 0
	$sink_1 set bytes_ 0
	$ns at [expr $now+$time] "record"
}


proc finish {} {
	global ns f nf
	global ns data
	$ns flush-trace
	close $nf
	close $f
	puts "running nam..."
	exec nam tcp_0.nam &
	#exec xgraph throughput.data &
	exit 0
}

$ns run


