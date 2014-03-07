/*
 *	N U T T C P . C						v7.1.1
 *
 * Copyright(c) 2000 - 2009 Bill Fink.  All rights reserved.
 * Copyright(c) 2003 - 2009 Rob Scott.  All rights reserved.
 *
 * nuttcp is free, opensource software.  You can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU General Public
 * License (GPL), as published by the GNU Project (http://www.gnu.org).
 * A copy of the license can also be found in the LICENSE file.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * Based on nttcp
 * Developed by Bill Fink, billfink@mindspring.com
 *          and Rob Scott, rob@hpcmo.hpc.mil
 * Latest version available at:
 *	http://lcp.nrl.navy.mil/nuttcp/
 *
 * Test TCP connection.  Makes a connection on port 5000(ctl)/5101(data)
 * and transfers fabricated buffers or data copied from stdin.
 *
 * Run nuttcp with no arguments to get a usage statement
 *
 * Modified for operation under 4.2BSD, 18 Dec 84
 *      T.C. Slattery, USNA
 * Minor improvements, Mike Muuss and Terry Slattery, 16-Oct-85.
 *
 * 7.1.1, Bill Fink, 24-Dec-09
 *	Provide summary TCP retrans info for multi-stream TCP
 *	Fix bug with retrans interval info when -fparse
 *	Add "+stride" or "+n.n.n.n" syntax for multi-stream TCP (IPv4)
 *	Fix third party bug with "-xc" option adding extraneous 't' character
 *	Add optional client-side name resolution for third party host
 *	Add "-N##m" option for multilink aggregation for multiple streams
 *	Add "-xc#/#" and "-P#/#" options to Usage: statement
 *	Some minor whitespace cleanup
 * 7.0.1, Bill Fink, 18-Sep-09
 *	Enable jitter measurements with "-j" option
 *	Enable one-way delay measurements with "-o" option
 *	Fix bug with RTT and -fparse
 * 6.2.10, Bill Fink, 1-Aug-09
 *	Change ctl/data port checks to < 1024 instead of < 5000
 *	Fix "--idle-data-timeout" Usage: statement for new default minimum
 *	Improve transmit performance with "-i" by setting poll() timeout to 0
 *	Remove commented out code for unused normal_eod
 *	Don't output interval retrans info if non-sinkmode (for nuttscp)
 * 6.2.9, Bill Fink, 24-Jun-09
 *	Make retrans info reporting work again on newer Linux distros
 *	Skip check for unACKed data at end of transfer if -DBROKEN_UNACKED
 * 6.2.8, Bill Fink, 8-Jun-09
 *	Play nice with iperf (change default data port to 5101)
 *	Delay sending of server "OK" until after successful server bind()
 *	Client check for server errors before starting data transfer
 *	Continue checking for server output while draining client transmission
 *	Correct "server not ACKing data" error message (server -> receiver)
 *	Add "--packet-burst" option for Rob
 *	Fix "--idle-data-timeout" Usage: statement for client
 *	Improve accuracy of retrans info timing synchronization (client xmitter)
 *	Change reference to nuttcp repository from ftp:// to http://
 *	Fix bug affecting nuttscp introduced in 6.2.4 (not honoring oneshot)
 *	Whitespace cleanup: get rid of <tab><spaces><tab>, <tab>$, & <spaces>$
 *	Whitespace cleanup: convert 8 spaces to <tab> where appropriate
 * 6.2.7, Bill Fink, 22-May-09
 *	Allow rate limit to be exceeded temporarily by n packets ("-Rixxx/n")
 *	Fix several reqval parameter settings
 * 6.2.6, Bill Fink, 17-Apr-09
 *	Allow setting server CPU affinity from client via "-xcs" option
 *	Allow setting client & server CPU affinity via third party
 *	Fix bug with option processing when reqval is set
 * 6.2.5, Bill Fink, 16-Apr-09
 *	Allow passing of third party control port via "-Pctlport/ctlport3"
 *	Up default idle data minimum to 15 sec to better handle net transients
 *	Don't reset nstream until after last use (fix getaddrinfo() memory leak)
 * 6.2.4, Bill Fink, 10-Apr-09
 *	Fix bug with simultaneous server connections to manually started server
 * 6.2.3, Bill Fink, 5-Apr-09
 *	Add "-xc" option to set CPU affinity (Linux only)
 *	Fix Usage: statement: "--idle-data-timeout" both server & client option
 *	Don't reset priority on server cleanup
 *	Fix priority output for "-fparse"
 * 6.2.2, Bill Fink, 3-Apr-09
 *	Fix bad third party bug causing >= 1 minute transfers to silently fail
 *	Fix Usage: statement: "--idle-data-timeout" not just a server option
 * 6.2.1, Rob Scott, 22-Mar-09
 *	Added IPv6 and SSM MC support
 *	Ported from Rob's 5.5.5 based code by Bill Fink
 * 6.1.5, Bill Fink, 5-Mar-09
 *	Fix client lockup with third party when network problem (for scripts)
 * 6.1.4, Bill Fink, 5-Jan-09
 *	Updated Copyright notice
 *	Bugfix: set chk_idle_data on client (now also checks no data received)
 *	Use argv[0] instead of "nuttcp" for third party
 *	Bugfix: give error message again on error starting server
 * 6.1.3, Bill Fink, 17-Sep-08
 *	Timeout client accept() too and give nice error message (for scripts)
 * 6.1.2, Bill Fink, 29-Aug-08
 *	Don't wait forever for unacked data at EOT (limit to 1 minute max)
 *	Extend no data received protection to client too (for scripts)
 *	Give nice error messages to client for above cases
 *	Don't hang getting server info if server exited (timeout reads)
 * 6.1.1, Bill Fink, 26-Aug-08
 *	Remove beta designation
 *	Report RTT by default (use "-f-rtt" to suppress)
 *	Moved RTT info to "connect to" line
 *	Correct bogus IP frag warning for e.g. "-l1472" or "-l8972"
 *	Don't report interval host-retrans if no data rcvd (e.g. initial PMTU)
 *	Correct reporting of retrans info with "-fparse" option
 *	Correct reporting of RTT with "-F" flip option
 *	Report doubled send and receive window sizes (for Linux)
 *	Add report of available send and receive window sizes (for Linux)
 *	Touchup TODO list to remove some already completed items
 * 6.0.7, Bill Fink, 19-Aug-08
 *	Add delay (default 0.5 sec) to "-a" option & change max retries to 10
 *	Updated Copyright notice
 * 6.0.6, Bill Fink, 11-Aug-08
 *	Delay server forking until after listen() for better error status
 *	Above suggested by Hans Blom (jblom@science.uva.nl)
 *	Make forced server mode the default behavior
 *	Check address family on getpeername() so "ssh host nuttcp -S" works
 *	Add setsid() call for manually started server to create new session
 *	Some minor code cleanup
 * 6.0.5, Bill Fink, 10-Aug-08
 *	Allow "-s" from/to stdin/stdout with "-1" oneshot server mode
 *	Switch beta vers message to stderr to not interfere with "-s" option
 *	Don't set default timeout if "-s" specified
 *	Give error message for "-s" with "-S" (xinetd or manually started)
 * 6.0.4, Bill Fink, 18-Jul-08
 *	Forgot about 3rd party support for RTT info - fix that
 * 6.0.3, Bill Fink, 17-Jul-08
 *	A better (and accurate) way to get RTT info (and not just Linux)
 * 6.0.2, Bill Fink, 16-Jul-08
 *	Add RTT info to brief output for Linux
 * 6.0.1, Bill Fink, 17-Dec-07
 *	Add reporting of TCP retransmissions (interval reports on Linux TX only)
 *	Add reporting of data transfer RTT for verbose Linux output
 *	Add beta version messages and "-f-beta" option to suppress
 *	Automatically switch "classic" mode to oneshot server mode
 *	Fix UDP loss info bug not updating stream_idx when not need_swap
 *	Fix compiler warning doing sprintf of timeout
 *	Correct comment on NODROPS define
 * 5.5.5, Bill Fink, 1-Feb-07
 *	Change default MC addr to be based on client addr instead of xmit addr
 * 5.5.4, Bill Fink, 3-Nov-06
 *	Fix bug with negative loss causing huge drop counts on interval reports
 *	Updated Copyright notice and added GPL license notice
 * 5.5.3, Bill Fink, 23-Oct-06
 *	Fix bug with "-Ri" instantaneous rate limiting not working properly
 * 5.5.2, Bill Fink, 25-Jul-06
 *	Make manually started server multi-threaded
 *	Add "--single-threaded" server option to restore old behavior
 *	Add "-a" client option to retry a failed server connection "again"
 *	(for certain possibly transient errors)
 * 5.5.1, Bill Fink, 22-Jul-06
 *	Fix bugs with nbuf_bytes and rate_pps used with 3rd party
 *	Pass "-D" option to server (and also make work for third party)
 *	Allow setting precedence with "-c##p"
 * 5.4.3, Rob Scott & Bill Fink, 17-Jul-06
 *	Fix bug with buflen passed to server when no buflen option speicified
 *	(revert 5.3.2: Fix bug with default UDP buflen for 3rd party)
 *	Better way to fix bug with default UDP buflen for 3rd party
 *	Trim trailing '\n' character from err() calls
 *	Use fcntl() to set O_NONBLOCK instead of MSG_DONTWAIT to send() ABORT
 *	(and remove MSG_DONTWAIT from recv() because it's not needed)
 *	Don't re-initialize buflen at completion of server processing
 *	(non inetd: is needed to check for buffer memory allocation change,
 *	caused bug if smaller "-l" followed by larger default "-l")
 * 5.4.2, Bill Fink, 1-Jul-06
 *	Fix bug with interrupted UDP receive reporting negative packet loss
 *	Make sure errors (or debug) from server are propagated to the client
 *	Make setsockopt SO_SNDBUF/SO_RCVBUF error not be fatal to server
 *	Don't send stderr to client if nofork is set (manually started server)
 * 5.4.1, Bill Fink, 30-Jun-06
 *	Fix bug with UDP reporting > linerate because of bad correction
 *	Send 2nd UDP BOD packet in case 1st is lost, e.g. waiting for ARP reply
 *	(makes UDP BOD more robust for new separate control and data paths)
 *	Fix bug with interval reports after EOD for UDP with small interval
 *	Don't just exit inetd server on no data so can get partial results
 *	(keep an eye out that servers don't start hanging again)
 *	Make default idle data timeout 1/2 of timeout if interval not set
 *	(with a minimum of 5 seconds and a maximum of 60 seconds)
 *	Make server send abort via urgent TCP data if no UDP data received
 *	(non-interval only: so client won't keep transmitting for full period)
 *	Workaround for Windows not handling TCP_MAXSEG getsockopt()
 * 5.3.4, Bill Fink, 21-Jun-06
 *	Add "--idle-data-timeout" server option
 *	(server ends transfer if no data received for the specified
 *	timeout interval, previously it was a fixed 60 second timeout)
 *	Shutdown client control connection for writing at end of UDP transfer
 *	(so server can cope better with loss of all EOD packets, which is
 *	mostly of benefit when using separate control and data paths)
 * 5.3.3, Bill Fink & Mark S. Mathews, 18-Jun-06
 *	Add new capability for separate control and data paths
 *	(using syntax:  nuttcp ctl_name_or_IP/data_name_or_IP)
 *	Extend new capability for multiple independent data paths
 *	(using syntax:  nuttcp ctl/data1/data2/.../datan)
 *	Above only supported for transmit or flipped/reversed receive
 *	Fix -Wall compiler warnings on 64-bit systems
 *	Make manually started server also pass stderr to client
 *	(so client will get warning messages from server)
 * 5.3.2, Bill Fink, 09-Jun-06
 *	Fix bug with default UDP buflen for 3rd party
 *	Fix compiler warnings with -Wall on FreeBSD
 *	Give warning that windows doesn't support TCP_MAXSEG
 * 5.3.1, Rob Scott, 06-Jun-06
 *	Add "-c" COS option for setting DSCP/TOS setting
 *	Fix builds on latest MacOS X
 *	Fix bug with 3rd party unlimited rate UDP not working
 *	Change "-M" option to require a value
 *	Fix compiler warnings with -Wall (thanks to Daniel J Blueman)
 *	Remove 'v' from nuttcp version (simplify RPM packaging)
 * V5.2.2, Bill Fink, 13-May-06
 *	Have client report server warnings even if not verbose
 * V5.2.1, Bill Fink, 12-May-06
 *	Pass "-M" option to server so it also works for receives
 *	Make "-uu" be a shortcut for "-u -Ru"
 * V5.1.14, Bill Fink, 11-May-06
 *	Fix cancellation of UDP receives to work properly
 *	Allow easy building without IPv6 support
 *	Set default UDP buflen to largest 2^n less than MSS of ctlconn
 *	Add /usr/local/sbin and /usr/etc to path
 *	Allow specifying rate in pps by using 'p' suffix
 *	Give warning if actual send/receive window size is less than requested
 *	Make UDP transfers have a default rate limit of 1 Mbps
 *	Allow setting MSS for client transmitter TCP transfers with "-M" option
 *	Give more precision on reporting small UDP percentage data loss
 *	Disallow UDP transfers in "classic" mode
 *	Notify when using "classic" mode
 * V5.1.13, Bill Fink, 8-Apr-06
 *	Make "-Ri" instantaneous rate limit for very high rates more accurate
 *	(including compensating for microsecond gettimeofday() granularity)
 *	Fix bug giving bogus time/stats on UDP transmit side with "-Ri"
 *	Allow fractional rate limits (for 'm' and 'g' only)
 * V5.1.12, Bill Fink & Rob Scott, 4-Oct-05
 *	Terminate server receiver if client control connection goes away
 *	or if no data received from client within CHECK_CLIENT_INTERVAL
 * V5.1.11, Rob Scott, 25-Jun-04
 *	Add support for scoped ipv6 addresses
 * V5.1.10, Bill Fink, 16-Jun-04
 *	Allow 'b' suffix on "-w" option to specify window size in bytes
 * V5.1.9, Bill Fink, 23-May-04
 *	Fix bug with client error on "-d" option putting server into bad state
 *	Set server accept timeout (currently 5 seconds) to prevent stuck server
 *	Add nuttcp version info to error message from err() exit
 * V5.1.8, Bill Fink, 22-May-04
 *	Allow 'd|D' suffix to "-T" option to specify days
 *	Fix compiler warning about unused variable cp in getoptvalp routine
 *	Interval value check against timeout value should be >=
 * V5.1.7, Bill Fink, 29-Apr-04
 *	Drop "-nb" option in favor of "-n###[k|m|g|t|p]"
 * V5.1.6, Bill Fink, 25-Apr-04
 *	Fix bug with using interval option without timeout
 * V5.1.5, Bill Fink, 23-Apr-04
 *	Modification to allow space between option parameter and its value
 *	Permit 'k' or 'm' suffix on "-l" option
 *	Add "-nb" option to specify number of bytes to transfer
 *	Permit 'k', 'm', 'g', 't', or 'p' suffix on "-n" and "-nb" options
 * V5.1.4, Bill Fink, 21-Apr-04
 *	Change usage statement to use standard out instead of standard error
 *	Fix bug with interval > timeout, give warning and ignore interval
 *	Fix bug with counting error value in nbytes on interrupted transfers
 *	Fix bug with TCP transmitted & received nbytes not matching
 *	Merge "-t" and "-r" options in Usage: statement
 * V5.1.3, Bill Fink, 9-Apr-04
 *	Add "-Sf" force server mode (useful for starting server via rsh/ssh)
 *	Allow non-root user to find nuttcp binary in "."
 *	Fix bug with receives terminating early with manual server mode
 *	Fix bug with UDP receives not terminating with "-Ri" option
 *	Clean up output formatting of nbuf (from "%d" to "%llu")
 *	Add "-SP" to have 3rd party use same outgoing control port as incoming
 * V5.1.2, Bill Fink & Rob Scott, 18-Mar-04
 *	Fix bug with nbuf wrapping on really long transfers (int -> uint64_t)
 *	Fix multicast address to be unsigned long to allow shift
 *	Add MacOS uint8_t definition for new use of uint8_t
 * V5.1.1, Bill Fink, 8-Nov-03
 *	Add IPv4 multicast support
 *	Delay receiver EOD until EOD1 (for out of order last data packet)
 *	Above also drains UDP receive buffer (wait for fragmentation reassembly)
 * V5.0.4, Bill Fink, 6-Nov-03
 *	Fix bug reporting 0 drops when negative loss percentage
 * V5.0.3, Bill Fink, 6-Nov-03
 *	Kill server transmission if control connection goes away
 *	Kill 3rd party nuttcp if control connection goes away
 * V5.0.2, Bill Fink, 4-Nov-03
 *	Fix bug: some dummy wasn't big enough :-)
 * V5.0.1, Bill Fink, 3-Nov-03
 *	Add third party support
 *	Correct usage statement for "-xt" traceroute option
 *	Improved error messages on failed options requiring client/server mode
 * V4.1.1, David Lapsley and Bill Fink, 24-Oct-03
 *	Added "-fparse" format option to generate key=value parsable output
 *	Fix bug: need to open data connection on abortconn to clear listen
 * V4.0.3, Rob Scott, 13-Oct-03
 *	Minor tweaks to output format for alignment
 *	Interval option "-i" with no explicit value sets interval to 1.0
 * V4.0.2, Bill Fink, 10-Oct-03
 *	Changed "-xt" option to do both forward and reverse traceroute
 *	Changed to use brief output by default ("-v" for old default behavior)
 * V4.0.1, Rob Scott, 10-Oct-03
 *	Added IPv6 code
 *	Changed inet get functions to protocol independent versions
 *	Added fakepoll for hosts without poll() (macosx)
 *	Added ifdefs to only include setprio if os supports it (non-win)
 *	Added bits to handle systems without new inet functions (solaris < 2.8)
 *	Removed SYSV obsolete code
 *	Added ifdefs and code to handle cygwin and beginning of windows support
 *	Window size can now be in meg (m|M) and gig (g|G)
 *	Added additional directories to search for traceroute
 *	Changed default to transmit, time limit of 10 seconds, no buffer limit
 *	Added (h|H) as option to specify time in hours
 *	Added getservbyname calls for port, if all fails use old defaults
 *	Changed sockaddr to sockaddr_storage to handle v6 addresses
 * v3.7.1, Bill Fink, 10-Aug-03
 *	Add "-fdebugpoll" option to help debug polling for interval reporting
 *	Fix Solaris compiler warning
 *	Use poll instead of separate process for interval reports
 * v3.6.2, Rob Scott, 18-Mar-03
 *	Allow setting server window to use default value
 *	Cleaned out BSD42 old code
 *	Marked SYSV code for future removal as it no longer appears necessary
 *	Also set RCVBUF/SNDBUF for udp transfers
 *	Changed transmit SO_DEBUG code to be like receive
 *	Some code rearrangement for setting options before accept/connect
 * v3.6.1, Bill Fink, 1-Mar-03
 *	Add -xP nuttcp process priority option
 *	Add instantaneous rate limit capability ("-Ri")
 *	Don't open data connection if server error or doing traceroute
 *	Better cleanup on server connection error (close open data connections)
 *	Don't give normal nuttcp output if server error requiring abort
 *	Implement -xt traceroute option
 * v3.5.1, Bill Fink, 27-Feb-03
 *	Don't allow flip option to be used with UDP
 *	Fix bug with UDP and transmit interval option (set stdin unbuffered)
 *	Fix start of UDP timing to be when get BOD
 *	Fix UDP timing when don't get first EOD
 *	Fix ident option used with interval option
 *	Add "-f-percentloss" option to not give %loss info on brief output
 *	Add "-f-drops" option to not give packet drop info on brief output
 *	Add packet drop info to UDP brief output (interval report and final)
 *	Add "-frunningtotal" option to give cumulative stats for "-i"
 *	Add "-fdebuginterval" option to help debug interval reporting
 *	Add "-fxmitstats" option to give transmitter stats
 *	Change flip option from "-f" to "-F"
 *	Fix divide by zero bug with "-i" option and very low rate limit
 *	Fix to allow compiling with Irix native compiler
 *	Fix by Rob Scott to allow compiling on MacOS X
 * v3.4.5, Bill Fink, 29-Jan-03
 *	Fix client/server endian issues with UDP loss info for interval option
 * v3.4.4, Bill Fink, 29-Jan-03
 *	Remove some debug printout for interval option
 *	Fix bug when using interval option reporting 0.000 MB on final
 * v3.4.3, Bill Fink, 24-Jan-03
 *	Added UDP approximate loss info for interval reporting
 *	Changed nbytes and pbytes from double to uint64_t
 *	Changed SIGUSR1 to SIGTERM to kill sleeping child when done
 * v3.4.2, Bill Fink, 15-Jan-03
 *	Make <control-C> work right with receive too
 * v3.4.1, Bill Fink, 13-Jan-03
 *	Fix bug interacting with old servers
 *	Add "-f" flip option to reverse direction of data connection open
 *	Fix bug by disabling interval timer when server done
 * v3.3.2, Bill Fink, 11-Jan-03
 *	Make "-i" option work for client transmit too
 *	Fix bug which forced "-i" option to be at least 0.1 seconds
 * v3.3.1, Bill Fink, 7-Jan-03
 *	Added -i option to set interval timer (client receive only)
 *	Fixed server bug not setting socket address family
 * v3.2.1, Bill Fink, 25-Feb-02
 *	Fixed bug so second <control-C> will definitely kill nuttcp
 *	Changed default control port to 5000 (instead of data port - 1)
 *	Modified -T option to accept fractional seconds
 * v3.1.10, Bill Fink, 6-Feb-02
 *	Added -I option to identify nuttcp output
 *	Made server always verbose (filtering is done by client)
 *	Update to usage statement
 *	Minor fix to "-b" output when "-D" option is used
 *	Fix bug with "-s" that appends nuttcp output to receiver data file
 *	Fix bug with "-b" that gave bogus CPU utilization on > 1 hour transfers
 * v3.1.9, Bill Fink, 21-Dec-01
 *	Fix bug with "-b" option on SGI systems reporting 0% CPU utilization
 * v3.1.8, Bill Fink, 21-Dec-01
 *	Minor change to brief output format to make it simpler to awk
 * v3.1.7, Bill Fink, 20-Dec-01
 *	Implement "-b" option for brief output (old "-b" -> "-wb")
 *	Report udp loss percentage when using client/server mode
 *	Fix bug with nbytes on transmitter using timed transfer
 *	Combined send/receive window size printout onto a single line
 * v3.1.6, Bill Fink, 11-Jun-01
 *	Fixed minor bug reporting error connecting to inetd server
 * Previously, Bill Fink, 7-Jun-01
 *	Added -h (usage) and -V (version) options
 *	Fixed SGI compilation warnings
 *	Added reporting server version to client
 *	Added version info and changed ttcp prints to nuttcp
 *	Fixed bug with inetd server and client using -r option
 *	Added ability to run server from inetd
 *	Added udp capability to server option
 *	Added -T option to set timeout interval
 *	Added -ws option to set server window
 *	Added -S option to support running receiver as daemon
 *	Allow setting UDP buflen up to MAXUDPBUFLEN
 *	Provide -b option for braindead Solaris 2.8
 *	Added printing of transmit rate limit
 *	Added -w option to usage statement
 *	Added -N option to support multiple streams
 *	Added -R transmit rate limit option
 *	Fix setting of destination IP address on 64-bit Irix systems
 *	Only set window size in appropriate direction to save memory
 *	Fix throughput calculation for large transfers (>= 2 GB)
 *	Fix reporting of Mb/s to give actual millions of bits per second
 *	Fix setting of INET address family in local socket
 *	Fix setting of receiver window size
 *
 * TODO/Wish-List:
 *	Transmit interval marking option
 *	Allow at least some traceroute options
 *	Add "-ut" option to do both UDP and TCP simultaneously
 *	Default rate limit UDP if too much loss
 *	Ping option
 *	Other brief output formats
 *	Linux window size bug/feature note
 *	Network interface interrupts (for Linux only)
 *	netstat -i info
 *	Man page
 *	Forking for multiple streams
 *	Bidirectional option
 *	Graphical interface
 *	MTU info
 *	Warning for window size limiting throughput
 *	Auto window size optimization
 *	Transmitter profile and playback options
 *	Server side limitations (per client host/network)
 *	Server side logging
 *	Client/server security (password)
 *	nuttcp server registration
 *	nuttcp proxy support (firewalls)
 *	nuttcp network idle time
 *
 * Distribution Status -
 *	OpenSource(tm)
 *	Licensed under version 2 of the GNU GPL
 *	Please send source modifications back to the authors
 *	Derivative works should be redistributed with a modified
 *	source and executable name
 */

/*
#ifndef lint
static char RCSid[] = "@(#)$Revision: 1.2 $ (BRL)";
#endif
*/

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>		/* struct timeval */
#include <stdlib.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/resource.h>
#else
#include "win32nuttcp.h"			/* from win32 */
#endif /* _WIN32 */

#include <limits.h>
#include <string.h>
#include <fcntl.h>

/* Let's try changing the previous unwieldy check */
/* #if defined(linux) || defined(__FreeBSD__) || defined (sgi) || (defined(__MACH__) && defined(_SOCKLEN_T)) || defined(sparc) || defined(__CYGWIN__) */
/* to the following (hopefully equivalent) simpler form like we use
 * for HAVE_POLL */
#if !defined(_WIN32) && (!defined(__MACH__) || defined(_SOCKLEN_T))
#include <unistd.h>
#include <sys/wait.h>
#include <strings.h>
#endif

#ifndef ULLONG_MAX
#define ULLONG_MAX	18446744073709551615ULL
#endif

#define MAXRATE 0xffffffffUL

#if !defined(__CYGWIN__) && !defined(_WIN32)
#define HAVE_SETPRIO
#endif

#if defined(linux)
#define HAVE_SETAFFINITY
#define __USE_GNU
#endif

#if !defined(_WIN32) && (!defined(__MACH__) || defined(_SOCKLEN_T))
#define HAVE_POLL
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define uint64_t u_int64_t
#define uint32_t u_int32_t
#define uint16_t u_int16_t
#define uint8_t u_int8_t
#endif

#ifdef HAVE_POLL
#include <sys/poll.h>
#else
#include "fakepoll.h"			/* from missing */
#endif

#ifdef HAVE_SETAFFINITY
#include <sched.h>
#endif

#ifndef CPU_SETSIZE
#undef HAVE_SETAFFINITY
#endif

/*
 * _SOCKLEN_T is now defined by apple when they typedef socklen_t
 *
 * EAI_NONAME has nothing to do with socklen, but on sparc without it tells
 * us it's an old enough solaris to need the typedef
 */
#if (defined(__APPLE__) && defined(__MACH__)) && !defined(_SOCKLEN_T) || (defined(sparc) && !defined(EAI_NONAME))
typedef int socklen_t;
#endif

#if defined(sparc) && !defined(EAI_NONAME) /* old sparc */
#define sockaddr_storage sockaddr
#define ss_family sa_family
#endif /* old sparc */

#if defined(_AIX)
#define ss_family __ss_family
#endif

#if !defined(EAI_NONAME)
#include "addrinfo.h"			/* from missing */
#endif

#define BETA_STR	"-beta8"
#define BETA_FEATURES	"jitter/owd"

static struct timeval time0;	/* Time at which timing started */
static struct timeval timepk;	/* Time at which last packet sent */
static struct timeval timepkr;	/* Time at which last packet received */
static struct timeval timepkri;	/* timepkr for interval reports */
static struct timeval timep;	/* Previous time - for interval reporting */
static struct timeval timetx;	/* Transmitter timestamp */
static struct timeval timerx;	/* Receive timestamp */
static struct rusage ru0;	/* Resource utilization at the start */

static struct	sigaction sigact;	/* signal handler for alarm */
static struct	sigaction savesigact;

#define PERF_FMT_OUT	  "%.4f MB in %.2f real seconds = %.2f KB/sec" \
			  " = %.4f Mbps\n"
#define PERF_FMT_BRIEF	  "%10.4f MB / %6.2f sec = %9.4f Mbps %d %%TX %d %%RX"
#define PERF_FMT_BRIEF2	  "%10.4f MB / %6.2f sec = %9.4f Mbps %d %%%s"
#define PERF_FMT_BRIEF3	  " Trans: %.4f MB"
#define PERF_FMT_INTERVAL  "%10.4f MB / %6.2f sec = %9.4f Mbps"
#define PERF_FMT_INTERVAL2 " Tot: %10.4f MB / %6.2f sec = %9.4f Mbps"
#define PERF_FMT_INTERVAL3 " Trans: %10.4f MB"
#define PERF_FMT_INTERVAL4 " Tot: %10.4f MB"
#define PERF_FMT_IN	  "%lf MB in %lf real seconds = %lf KB/sec = %lf Mbps\n"
#define CPU_STATS_FMT_IN  "%*fuser %*fsys %*d:%*dreal %d%%"
#define CPU_STATS_FMT_IN2 "%*fuser %*fsys %*d:%*d:%*dreal %d%%"

#define LOSS_FMT	" %.2f%% data loss"
#define LOSS_FMT_BRIEF	" %.2f %%loss"
#define LOSS_FMT_INTERVAL " %5.2f ~%%loss"
#define LOSS_FMT5	" %.5f%% data loss"
#define LOSS_FMT_BRIEF5	" %.5f %%loss"
#define LOSS_FMT_INTERVAL5 " %7.5f ~%%loss"
#define DROP_FMT	" %lld / %lld drop/pkt"
#define DROP_FMT_BRIEF	" %lld / %lld drop/pkt"
#define DROP_FMT_INTERVAL " %5lld / %5lld ~drop/pkt"
#define JITTER_MIN		1
#define JITTER_AVG		2
#define JITTER_MAX		4
#define JITTER_IGNORE_OOO	8
#define JITTER_FMT	"min-jitter = %.4f ms, avg-jitter = %.4f ms, " \
			"max-jitter = %.4f ms"
#define JITTER_MIN_FMT_BRIEF " %.4f msMinJitter"
#define JITTER_AVG_FMT_BRIEF " %.4f msAvgJitter"
#define JITTER_MAX_FMT_BRIEF " %.4f msMaxJitter"
#define JITTER_MIN_FMT_INTERVAL " %.4f msMinJitter"
#define JITTER_AVG_FMT_INTERVAL " %.4f msAvgJitter"
#define JITTER_MAX_FMT_INTERVAL " %.4f msMaxJitter"
#define JITTER_FMT_IN	"jitter = %lf ms, avg-jitter = %lf ms, " \
			"max-jitter = %lf ms"
#define OWD_MIN			1
#define OWD_AVG			2
#define OWD_MAX			4
#define OWD_FMT		"min-OWD = %.4f ms, avg-OWD = %.4f ms, " \
			"max-OWD = %.4f ms"
#define OWD_MIN_FMT_BRIEF " %.4f msMinOWD"
#define OWD_AVG_FMT_BRIEF " %.4f msAvgOWD"
#define OWD_MAX_FMT_BRIEF " %.4f msMaxOWD"
#define OWD_MIN_FMT_INTERVAL " %.4f msMinOWD"
#define OWD_AVG_FMT_INTERVAL " %.4f msAvgOWD"
#define OWD_MAX_FMT_INTERVAL " %.4f msMaxOWD"
#define OWD_FMT_IN	"OWD = %lf ms, avg-OWD = %lf ms, max-OWD = %lf ms"
#define RETRANS_FMT	"%sretrans = %d"
#define RETRANS_FMT_BRIEF " %d %sretrans"
#define RETRANS_FMT_BRIEF_STR1 " %d = %d"
#define RETRANS_FMT_BRIEF_STR2 " retrans"
#define RETRANS_FMT_INTERVAL " %5d %sretrans"
#define RETRANS_FMT_IN	"retrans = %d"
#define RTT_FMT		" RTT=%.3f ms"
#define RTT_FMT_BRIEF	" %.2f msRTT"
#define RTT_FMT_IN	"RTT=%lf"
#define RTT_FMT_INB	"RTT = %lf"
#define SIZEOF_TCP_INFO_RETRANS		104

/* define NEW_TCP_INFO if struct tcp_info in /usr/include/netinet/tcp.h
 * contains tcpi_total_retrans member
 *
 * tcpi_rcv_rtt, tcpi_rcv_space, & tcpi_total_retrans were added
 * in glibc-headers-2.7 (Fedora 8) which fortunately also defined
 * TCP_MD5SIG at the same time, so key off of that
 */
#if defined(linux) && defined(TCP_MD5SIG)
#define NEW_TCP_INFO
#endif

#ifndef NEW_TCP_INFO
#define OLD_TCP_INFO
#endif

/* Parsable output formats */

#define P_PERF_FMT_OUT	  "megabytes=%.4f real_seconds=%.2f " \
			  "rate_KBps=%.2f rate_Mbps=%.4f\n"
#define P_PERF_FMT_BRIEF  "megabytes=%.4f real_seconds=%.2f rate_Mbps=%.4f " \
			  "tx_cpu=%d rx_cpu=%d"
#define P_PERF_FMT_BRIEF3 " tx_megabytes=%.4f"
#define P_PERF_FMT_INTERVAL  "megabytes=%.4f real_sec=%.2f rate_Mbps=%.4f"
#define P_PERF_FMT_INTERVAL2 " total_megabytes=%.4f total_real_sec=%.2f" \
			     " total_rate_Mbps=%.4f"
#define P_PERF_FMT_INTERVAL3 " tx_megabytes=%.4f"
#define P_PERF_FMT_INTERVAL4 " tx_total_megabytes=%.4f"
#define P_PERF_FMT_IN	  "megabytes=%lf real_seconds=%lf rate_KBps=%lf " \
			  "rate_Mbps=%lf\n"
#define P_CPU_STATS_FMT_IN  "user=%*f system=%*f elapsed=%*d:%*d cpu=%d%%"
#define P_CPU_STATS_FMT_IN2 "user=%*f system=%*f elapsed=%*d:%*d:%*d cpu=%d%%"

#define P_LOSS_FMT		" data_loss=%.5f"
#define P_LOSS_FMT_BRIEF	" data_loss=%.5f"
#define P_LOSS_FMT_INTERVAL	" data_loss=%.5f"
#define P_DROP_FMT		" drop=%lld pkt=%lld"
#define P_DROP_FMT_BRIEF	" drop=%lld pkt=%lld"
#define P_DROP_FMT_INTERVAL	" drop=%lld pkt=%lld"
#define P_JITTER_FMT		"msminjitter=%.4f msavgjitter=%.4f " \
				"msmaxjitter=%.4f"
#define P_JITTER_MIN_FMT_BRIEF	" msminjitter=%.4f"
#define P_JITTER_AVG_FMT_BRIEF	" msavgjitter=%.4f"
#define P_JITTER_MAX_FMT_BRIEF	" msmaxjitter=%.4f"
#define P_JITTER_MIN_FMT_INTERVAL " msminjitter=%.4f"
#define P_JITTER_AVG_FMT_INTERVAL " msavgjitter=%.4f"
#define P_JITTER_MAX_FMT_INTERVAL " msmaxjitter=%.4f"
#define P_JITTER_FMT_IN		"jitter=%lf msavgjitter=%lf msmaxjitter=%lf"
#define P_OWD_FMT		"msminOWD=%.4f msavgOWD=%.4f msmaxOWD=%.4f"
#define P_OWD_MIN_FMT_BRIEF	" msminOWD=%.4f"
#define P_OWD_AVG_FMT_BRIEF	" msavgOWD=%.4f"
#define P_OWD_MAX_FMT_BRIEF	" msmaxOWD=%.4f"
#define P_OWD_MIN_FMT_INTERVAL	" msminOWD=%.4f"
#define P_OWD_AVG_FMT_INTERVAL	" msavgOWD=%.4f"
#define P_OWD_MAX_FMT_INTERVAL	" msmaxOWD=%.4f"
#define P_OWD_FMT_IN		"OWD=%lf msavgOWD=%lf msmaxOWD=%lf"
#define P_RETRANS_FMT		"%sretrans=%d"
#define P_RETRANS_FMT_STREAMS	" retrans_by_stream=%d"
#define P_RETRANS_FMT_BRIEF	" %sretrans=%d"
#define P_RETRANS_FMT_INTERVAL	" %sretrans=%d"
#define P_RETRANS_FMT_IN	"retrans=%d"
#define P_RTT_FMT		" rtt_ms=%.3f"
#define P_RTT_FMT_BRIEF		" rtt_ms=%.2f"
#define P_RTT_FMT_IN		"rtt_ms=%lf"

#define HELO_FMT	"HELO nuttcp v%d.%d.%d\n"

#ifndef MAXSTREAM
#define MAXSTREAM		128
#endif
#define DEFAULT_NBUF		2048
#define DEFAULT_NBYTES		134217728	/* 128 MB */
#define DEFAULT_TIMEOUT		10.0
#define DEFAULT_UDP_RATE	1000
#define DEFAULTUDPBUFLEN	8192
#define DEFAULT_MC_UDPBUFLEN	1024
#define MAXUDPBUFLEN		65507
#define LOW_RATE_HOST3		1000
#define MINMALLOC		1024
#define HI_MC			231ul
#define HI_MC_SSM		232ul
/* locally defined global scope IPv6 multicast, FF3E::8000:0-FF3E::FFFF:FFFF */
#define HI_MC6			"FF3E::8000:0000"
#define HI_MC6_LEN		13
#ifndef LISTEN_BACKLOG
#define LISTEN_BACKLOG		64
#endif
#define ACCEPT_TIMEOUT		5
#ifndef MAX_CONNECT_TRIES
#define MAX_CONNECT_TRIES	10	/* maximum server connect attempts */
#endif
#ifndef SERVER_RETRY_USEC
#define SERVER_RETRY_USEC	500000	/* server retry time in usec */
#endif
#define MAX_EOT_WAIT_SEC	60.0	/* max wait for unacked data at EOT */
#define SRVR_INFO_TIMEOUT	60	/* timeout for reading server info */
#define IDLE_DATA_MIN		15.0	/* minimum value for chk_idle_data */
#define DEFAULT_IDLE_DATA	30.0	/* default value for chk_idle_data */
#define IDLE_DATA_MAX		60.0	/* maximum value for chk_idle_data */
#define NON_JUMBO_ETHER_MSS	1448	/* 1500 - 20:IP - 20:TCP -12:TCPOPTS */
#define TCP_UDP_HDRLEN_DELTA	12	/* difference in tcp & udp hdr sizes */

#if defined(linux)
#define TCP_ADV_WIN_SCALE	"/proc/sys/net/ipv4/tcp_adv_win_scale"
#endif

#define BRIEF_RETRANS_STREAMS	0x2	/* brief per stream retrans info */

#define XMITSTATS		0x1	/* also give transmitter stats (MB) */
#define DEBUGINTERVAL		0x2	/* add info to assist with
					 * debugging interval reports */
#define	RUNNINGTOTAL		0x4	/* give cumulative stats for "-i" */
#define	NODROPS			0x8	/* no packet drop stats for "-i" */
#define	NOPERCENTLOSS		0x10	/* don't give percent loss for "-i" */
#define DEBUGPOLL		0x20	/* add info to assist with debugging
					 * polling for interval reports */
#define PARSE			0x40	/* generate key=value parsable output */
#define DEBUGMTU		0x80	/* debug info for MTU/MSS code */
#define	NORETRANS		0x100	/* no retrans stats for "-i" */
#define	DEBUGRETRANS		0x200	/* output info for debugging collection
					 * of TCP retransmission info */
#define	NOBETAMSG		0x400	/* suppress beta version message */
#define	WANTRTT			0x800	/* output RTT info (default) */
#define DEBUGJITTER		0x1000	/* debugging info for jitter option */

#ifdef NO_IPV6				/* Build without IPv6 support */
#undef AF_INET6
#undef IPV6_V6ONLY
#endif

void sigpipe( int signum );
void sigint( int signum );
void ignore_alarm( int signum );
void sigalarm( int signum );
static void err( char *s );
static void mes( char *s );
static void errmes( char *s );
void pattern( char *cp, int cnt );
void prep_timer();
double read_timer( char *str, int len );
static void prusage( struct rusage *r0,  struct rusage *r1, struct timeval *e, struct timeval *b, char *outp );
static void tvadd( struct timeval *tsum, struct timeval *t0, struct timeval *t1 );
static void tvsub( struct timeval *tdiff, struct timeval *t1, struct timeval *t0 );
static void psecs( long l, char *cp );
int Nread( int fd, char *buf, int count );
int Nwrite( int fd, char *buf, int count );
int delay( int us );
int mread( int fd, char *bufp, unsigned n);
char *getoptvalp( char **argv, int index, int reqval, int *skiparg );

int get_retrans( int sockfd );

#if defined(linux) && defined(TCPI_OPT_TIMESTAMPS)
void print_tcpinfo();
#endif

int vers_major = 7;
int vers_minor = 1;
int vers_delta = 1;
int ivers;
int rvers_major = 0;
int rvers_minor = 0;
int rvers_delta = 0;
int irvers;
int beta = 0;

struct sockaddr_in sinme[MAXSTREAM + 1];
struct sockaddr_in sinhim[MAXSTREAM + 1];
struct sockaddr_in save_sinhim, save_mc;

#ifdef AF_INET6
struct sockaddr_in6 sinme6[MAXSTREAM + 1];
struct sockaddr_in6 sinhim6[MAXSTREAM + 1];
struct sockaddr_in6 save_sinhim6, save_mc6;
struct in6_addr hi_mc6;
#endif

struct sockaddr_storage frominet;

int domain = PF_UNSPEC;
int af = AF_UNSPEC;
int explicitaf = 0;		/* address family explicit specified (-4|-6) */
int fd[MAXSTREAM + 1];		/* fd array of network sockets */
int nfd;			/* fd for accept call */
struct pollfd pollfds[MAXSTREAM + 4];	/* used for reading interval reports */
socklen_t fromlen;

int buflen = 64 * 1024;		/* length of buffer */
int nbuflen;
int mallocsize;
char *buf;			/* ptr to dynamic buffer */
unsigned long long nbuf = 0;	/* number of buffers to send in sinkmode */
int nbuf_bytes = 0;		/* set to 1 if nbuf is actually bytes */

/*  nick code  */
int sendwin=0, sendwinval=0, origsendwin=0;
socklen_t optlen;
int rcvwin=0, rcvwinval=0, origrcvwin=0;
int srvrwin=0;
/*  end nick code  */

#if defined(linux)
int sendwinavail=0, rcvwinavail=0, winadjust=0;
#endif

#if defined(linux) && defined(TCPI_OPT_TIMESTAMPS)
#ifdef OLD_TCP_INFO
struct tcpinfo {		/* for collecting TCP retransmission info */
	struct tcp_info	_tcpinf;
	/* add missing structure elements */
	u_int32_t	tcpi_rcv_rtt;
	u_int32_t	tcpi_rcv_space;
	u_int32_t	tcpi_total_retrans;
} tcpinf;
#define tcpinfo_state		_tcpinf.tcpi_state
#define tcpinfo_ca_state	_tcpinf.tcpi_ca_state
#define tcpinfo_retransmits	_tcpinf.tcpi_retransmits
#define tcpinfo_unacked		_tcpinf.tcpi_unacked
#define tcpinfo_sacked		_tcpinf.tcpi_sacked
#define tcpinfo_lost		_tcpinf.tcpi_lost
#define tcpinfo_retrans		_tcpinf.tcpi_retrans
#define tcpinfo_fackets		_tcpinf.tcpi_fackets
#define tcpinfo_rtt		_tcpinf.tcpi_rtt
#define tcpinfo_rttvar		_tcpinf.tcpi_rttvar
#define tcpinfo_snd_ssthresh	_tcpinf.tcpi_snd_ssthresh
#define tcpinfo_snd_cwnd	_tcpinf.tcpi_snd_cwnd
#else
struct tcp_info tcpinf;
#define tcpinfo_state		tcpi_state
#define tcpinfo_ca_state	tcpi_ca_state
#define tcpinfo_retransmits	tcpi_retransmits
#define tcpinfo_unacked		tcpi_unacked
#define tcpinfo_sacked		tcpi_sacked
#define tcpinfo_lost		tcpi_lost
#define tcpinfo_retrans		tcpi_retrans
#define tcpinfo_fackets		tcpi_fackets
#define tcpinfo_rtt		tcpi_rtt
#define tcpinfo_rttvar		tcpi_rttvar
#define tcpinfo_snd_ssthresh	tcpi_snd_ssthresh
#define tcpinfo_snd_cwnd	tcpi_snd_cwnd
#endif
#endif

int udp = 0;			/* 0 = tcp, !0 = udp */
int udplossinfo = 0;		/* set to 1 to give UDP loss info for
				 * interval reporting */
int do_jitter = 0;		/* set to 1 to enable jitter measurements */
int do_owd = 0;			/* set to 1 to enable one-way delay reports */

int retransinfo = 0;		/* set to 1 to give TCP retransmission info
				 * for interval reporting */
int force_retrans = 0;		/* set to force sending retrans info */
int send_retrans = 1;		/* set to 0 if no need to send retrans info */
int do_retrans = 0;		/* set to 1 for client transmitter */
int read_retrans = 1;		/* set to 0 if no need to read retrans info */
int got_0retrans = 0;		/* set to 1 by client transmitter after
				 * processing initial server output
				 * having "0 retrans" */

int need_swap;			/* client and server are different endian */
int options = 0;		/* socket options */
int one = 1;			/* for 4.3 BSD style setsockopt() */
/* default port numbers if command arg or getserv doesn't get a port # */
#define DEFAULT_PORT	5101
#define DEFAULT_CTLPORT	5000
#define IPERF_PORT	5001
unsigned short port = 0;	/* TCP port number */
unsigned short ctlport = 0;	/* control port for server connection */
unsigned short ctlport3 = 0;	/* control port for 3rd party server conn */
int tmpport;
char *host;			/* ptr to name of host */
char *stride = NULL;		/* ptr to address stride for multi-stream */
char *host3 = NULL;		/* ptr to 3rd party host */
int thirdparty = 0;		/* set to 1 indicates doing 3rd party nuttcp */
int no3rd = 0;			/* set to 1 by server to disallow 3rd party */
int forked = 0;			/* set to 1 after server has forked */
int pass_ctlport = 0;		/* set to 1 to use same outgoing control port
				   as incoming with 3rd party usage */
char *nut_cmd;			/* command used to invoke nuttcp */
char *cmdargs[50];		/* command arguments array */
char tmpargs[50][40];

#ifndef AF_INET6
#define ADDRSTRLEN 16
#else
#define ADDRSTRLEN INET6_ADDRSTRLEN
int v4mapped = 0;		/* set to 1 to enable v4 mapping in v6 server */
#endif

#define HOSTNAMELEN	80
#define HOST3BUFLEN	HOSTNAMELEN + 2 + ADDRSTRLEN + 1 + ADDRSTRLEN
				/* host3=[=]host3addr[+host3stride] */

char hostbuf[ADDRSTRLEN];	/* buffer to hold text of address */
char host3addr[ADDRSTRLEN];	/* buffer to hold text of 3rd party address */
char host3buf[HOST3BUFLEN + 1];	/* buffer to hold 3rd party name or address */
int trans = 1;			/* 0=receive, !0=transmit mode */
int sinkmode = 1;		/* 0=normal I/O, !0=sink/source mode */
int nofork = 0;			/* set to 1 to not fork server */
int verbose = 0;		/* 0=print basic info, 1=print cpu rate, proc
				 * resource usage. */
int nodelay = 0;		/* set TCP_NODELAY socket option */
unsigned long rate = MAXRATE;	/* transmit rate limit in Kbps */
int maxburst = 1;		/* number of packets allowed to exceed rate */
int nburst = 1;			/* number of packets currently exceeding rate */
int irate = -1;			/* instantaneous rate limit if set */
double pkt_time;		/* packet transmission time in seconds */
double pkt_time_ms;		/* packet transmission time in milliseconds */
uint64_t irate_pk_usec;		/* packet transmission time in microseconds */
double irate_pk_nsec;		/* nanosecond portion of pkt xmit time */
double irate_cum_nsec = 0.0;	/* cumulative nanaseconds over several pkts */
int rate_pps = 0;		/* set to 1 if rate is given as pps */
double timeout = 0.0;		/* timeout interval in seconds */
double interval = 0.0;		/* interval timer in seconds */
double chk_idle_data = 0.0;	/* server receiver checks this often */
				/* for client having gone away */
double chk_interval = 0.0;	/* timer (in seconds) for checking client */
int ctlconnmss;			/* control connection maximum segment size */
int datamss = 0;		/* data connection maximum segment size */
unsigned int tos = 0;		/* 8-bit TOS field for setting DSCP/TOS */
char intervalbuf[256+2];	/* buf for interval reporting */
char linebuf[256+2];		/* line buffer */
int do_poll = 0;		/* set to read interval reports (client xmit) */
int got_done = 0;		/* set when read last of interval reports */
int reverse = 0;		/* reverse direction of data connection open */
int format = 0;			/* controls formatting of output */
char fmt[257];
int traceroute = 0;		/* do traceroute back to client if set */
int skip_data = 0;		/* skip opening of data channel */
#if defined(linux)
int multicast = 0;		/* set to 1 for multicast UDP transfer */
#else
uint8_t multicast = 0;		/* set to 1 for multicast UDP transfer */
#endif
int ssm = -1;			/* set to 1 for Source Specific Multicast */
				/* set to 0 to NOT do SSM */
				/* set to -1 to have SSM follow protocol */
				/* (off for ipv4, on for ipv6) */
int mc_param;
struct ip_mreq mc_group;	/* holds multicast group address */
#ifdef AF_INET6
struct ipv6_mreq mc6_group;	/* holds multicast group address */
#endif
#ifdef MCAST_JOIN_SOURCE_GROUP
struct group_source_req group_source_req;  /* holds multicast SSM group and */
					   /* source information */
#endif

#ifdef HAVE_SETPRIO
int priority = 0;		/* nuttcp process priority */
#endif

/* affinity and srvr_affinity need to be defined even if don't
 * HAVE_SETAFFINITY, to make parameter passing between client and
 * server work out OK, since far end may HAVE_SETAFFINITY
 *
 * they are set to -1 so they have no effect even if don't
 * HAVE_SETAFFINITY
 */
int affinity = -1;		/* nuttcp process CPU affinity */
int srvr_affinity = -1;		/* nuttcp server process CPU affinity */

#ifdef HAVE_SETAFFINITY
int ncores = 1;			/* number of CPU cores */
cpu_set_t cpu_set;		/* processor CPU set */
#endif

long timeout_sec = 0;
struct itimerval itimer;	/* for setitimer */
int srvr_helo = 1;		/* set to 0 if server doesn't send HELO */
char ident[40 + 1 + 1] = "";	/* identifier for nuttcp output */
int intr = 0;
int abortconn = 0;
int braindead = 0;		/* for braindead Solaris 2.8 systems */
int brief = 1;			/* set for brief output */
int brief3 = 1;			/* for third party nuttcp */
int done = 0;			/* don't output interval report if done */
int got_begin = 0;		/* don't output interval report if not begun */
int two_bod = 0;		/* newer versions send 2 BOD packets for UDP */
int handle_urg = 0;		/* newer versions send/recv urgent TCP data */
int got_eod0 = 0;		/* got EOD0 packet - marks end of UDP xfer */
int buflenopt = 0;		/* whether or not user specified buflen */
int haverateopt = 0;		/* whether or not user specified rate */
int clientserver = 0;		/* client server mode (use control channel) */
int client = 0;			/* 0=server side, 1=client (initiator) side */
int oneshot = 0;		/* 1=run server only once */
int inetd = 0;			/* set to 1 if server run from inetd */
pid_t pid;			/* process id when forking server process */
pid_t wait_pid;			/* return of wait system call */
int pidstat;			/* status of forked process */
FILE *ctlconn;			/* uses fd[0] for control channel */
int savestdin;			/* used to save real standard in */
int savestdout;			/* used to save real standard out */
int firsttime = 1;		/* flag for first pass through server */
struct in_addr clientaddr;	/* IP address of client connecting to server */

#ifdef AF_INET6
struct in6_addr clientaddr6;	/* IP address of client connecting to server */
uint32_t clientscope6;		/* scope part of IP address of client */
#endif

struct hostent *addr;
extern int errno;

char Usage[] = "\
Usage: nuttcp or nuttcp -h	prints this usage info\n\
Usage: nuttcp -V		prints version info\n\
Usage: nuttcp -xt [-m] host	forward and reverse traceroute to/from server\n\
Usage (transmitter): nuttcp [-t] [-options] [ctl_addr/]host [3rd-party] [<in]\n\
      |(receiver):   nuttcp -r [-options] [host] [3rd-party] [>out]\n\
	-4	Use IPv4\n"
#ifdef AF_INET6
"	-6	Use IPv6\n"
#endif
"	-c##	cos dscp value on data streams (t|T suffix for full TOS field)\n\
	-l##	length of network write|read buf (default 1K|8K/udp, 64K/tcp)\n\
	-s	use stdin|stdout for data input|output instead of pattern data\n\
	-n##	number of source bufs written to network (default unlimited)\n\
	-w##	transmitter|receiver window size in KB (or (m|M)B or (g|G)B)\n\
	-ws##	server receive|transmit window size in KB (or (m|M)B or (g|G)B)\n\
	-wb	braindead Solaris 2.8 (sets both xmit and rcv windows)\n\
	-p##	port number to send to|listen at (default 5101)\n\
	-P##	port number for control connection (default 5000)\n\
	-P#/#	control port to/from 3rd-party host (default 5000)\n\
	-u	use UDP instead of TCP\n\
	-m##	use multicast with specified TTL instead of unicast (UDP)\n\
	-M##	MSS for data connection (TCP)\n\
	-N##	number of streams (starting at port number), implies -B\n\
	-R##	transmit rate limit in Kbps (or (m|M)bps or (g|G)bps or (p)ps)\n\
	-Ri#[/#] instantaneous rate limit with optional packet burst\n\
	-T##	transmit timeout in seconds (or (m|M)inutes or (h|H)ours)\n\
	-j	enable jitter measurements (assumes -u and -Ri options)\n\
	-o	enable one-way delay reports (needs synchronized clocks)\n\
	-i##	receiver interval reporting in seconds (or (m|M)inutes)\n\
	-Ixxx	identifier for nuttcp output (max of 40 characters)\n\
	-F	flip option to reverse direction of data connection open\n\
	-a	retry failed server connection \"again\" for transient errors\n"
#ifdef HAVE_SETPRIO
"	-xP##	set nuttcp process priority (must be root)\n"
#endif
#ifdef HAVE_SETAFFINITY
"	-xc##	set nuttcp client process CPU affinity\n"
"	-xcs##	set nuttcp server process CPU affinity\n"
"	-xc#/#	set nuttcp client/server process CPU affinity\n"
#endif
"	-d	set TCP SO_DEBUG option on data socket\n\
	-v[v]	verbose [or very verbose] output\n\
	-b	brief output (default)\n\
	-br	add per-stream TCP retrans info to brief summary (Linux only)\n\
	-D	xmit only: don't buffer TCP writes (sets TCP_NODELAY sockopt)\n\
	-B	recv only: only output full blocks of size from -l## (for TAR)\n"
"	--packet-burst packet burst value for instantaneous rate limit option\n"
"	--idle-data-timeout <value|minimum/default/maximum>  (default: 15/30/60)\n"
"		     client timeout in seconds for idle data connection\n"
#ifdef IPV6_V6ONLY
"	--disable-v4-mapped disable v4 mapping in v6 server (default)\n"
"	--enable-v4-mapped enable v4 mapping in v6 server\n"
#endif
"\n\
Usage (server): nuttcp -S[f][P] [-options]\n\
		note server mode excludes use of -s\n\
		'P' suboption makes 3rd party {in,out}bound control ports same\n\
	-4	Use IPv4 (default)\n"
#ifdef AF_INET6
"	-6	Use IPv6\n"
#endif
"	-1	oneshot server mode (implied with inetd/xinetd), implies -S\n\
	-P##	port number for server connection (default 5000)\n\
		note don't use with inetd/xinetd (use services file instead)\n"
#ifdef HAVE_SETPRIO
"	-xP##	set nuttcp process priority (must be root)\n"
#endif
#ifdef HAVE_SETAFFINITY
"	-xc##	set nuttcp server process CPU affinity\n"
#endif
"	--idle-data-timeout <value|minimum/default/maximum>  (default: 15/30/60)\n"
"		     server timeout in seconds for idle data connection\n"
"	--no3rdparty don't allow 3rd party capability\n"
"	--nofork     don't fork server\n"
"	--single-threaded  make manually started server be single threaded\n"
#ifdef IPV6_V6ONLY
"	--disable-v4-mapped disable v4 mapping in v6 server (default)\n"
"	--enable-v4-mapped enable v4 mapping in v6 server\n"
#endif
"\n\
Multilink aggregation options (TCP only):\n\
         nuttcp [-options] -N##  [ctl_addr]/host1/host2/.../host## (xmit only)\n\
         nuttcp [-options] -N##  [ctl_addr/]host+addr_stride (IPv4 only)\n\
         nuttcp [-options] -N##  [ctl_addr/]host+n.n.n.n (IPv4 only)\n\
         nuttcp [-options] -N##m [ctl_addr/]host (xmit only)\n\
                                 where host resolves to multiple addresses\n\
\n\
                        separate [ctl_addr/] option available only for xmit\n\
\n\
Format options:\n\
	-fxmitstats	also give transmitter stats (MB) with -i (UDP only)\n\
	-frunningtotal	also give cumulative stats on interval reports\n\
	-f-drops	don't give packet drop info on brief output (UDP)\n\
	-f-retrans	don't give retrans info on brief output (TCP)\n\
	-f-percentloss	don't give %%loss info on brief output (UDP)\n\
	-fparse		generate key=value parsable output\n\
	-f-beta		suppress beta version message\n\
	-f-rtt		suppress RTT info \n\
";

char stats[128];
char srvrbuf[4096];
char tmpbuf[257];
uint64_t nbytes = 0;		/* bytes on net */
int64_t pbytes = 0;		/* previous bytes - for interval reporting */
int64_t ntbytes = 0;		/* bytes sent by transmitter */
int64_t ptbytes = 0;		/* previous bytes sent by transmitter */
uint64_t ntbytesc = 0;		/* bytes sent by transmitter that have
				 * been counted */
uint64_t ntbytescp = 0;		/* previous ntbytesc count */
uint64_t ntbytescpi = 0;	/* ntbytescp for interval reports */
uint64_t chk_nbytes = 0;	/* byte counter used to test if no more data
				 * being received by server (presumably because
				 * client transmitter went away */

double rtt = 0.0;		/* RTT between client and server in ms */
uint32_t nretrans[MAXSTREAM+1];	/* number of TCP retransmissions */
uint32_t iretrans[MAXSTREAM+1];	/* initial number of TCP retransmissions */
uint32_t pretrans = 0;		/* previous number of TCP retransmissions */
uint32_t sretrans = 0;		/* number of system TCP retransmissions */

int numCalls = 0;		/* # of NRead/NWrite calls. */
int nstream = 1;		/* number of streams */
int multilink = 0;		/* set to use multilink aggregation */
int stream_idx = 0;		/* current stream */
int start_idx = 1;		/* set to use or bypass control channel */
int b_flag = 1;			/* use mread() */
int got_srvr_output = 0;	/* set when server output has been read */
int reading_srvr_info = 0;	/* set when starting to read server info */
int retry_server = 0;		/* set to retry control connect() to server */
int num_connect_tries = 0;	/* tracks attempted connects to server */
int single_threaded = 0;	/* set to make server single threaded */
double srvr_MB;
double srvr_realt;
double srvr_KBps;
double srvr_Mbps;
int srvr_cpu_util;

double cput = 0.000001, realt = 0.000001;	/* user, real time (seconds) */
double realtd = 0.000001;	/* real time delta - for interval reporting */
double pkt_delta;		/* time delta between packets in ms */
double jitter;			/* current jitter measurement in ms */
unsigned long long njitter;	/* number of jitter measurements */
double jitter_min;		/* jitter minimum */
double jitter_max;		/* jitter maximum */
double jitter_avg;		/* jitter average */
double jitteri;			/* current jitter interval measurement in ms */
unsigned long long njitteri;	/* number of jitter interval measurements */
double jitter_mini;		/* jitter minimum for interval report */
double jitter_maxi;		/* jitter maximum for interval report */
double jitter_avgi;		/* jitter average for interval report */
double owd;			/* current one-way delay measurement in ms */
unsigned long long nowd;	/* number of one-way delay measurements */
double owd_min;			/* one-way delay minimum */
double owd_max;			/* one-way delay maximum */
double owd_avg;			/* one-way delay average */
unsigned long long nowdi;	/* number of OWD interval measurements */
double owd_mini;		/* OWD minimum for interval report */
double owd_maxi;		/* OWD maximum for interval report */
double owd_avgi;		/* OWD average for interval report */

void
close_data_channels()
{
	if (fd[1] == -1) return;

	if (clientserver && client && !host3 && udp && trans) {
		/* If all the EOD packets get lost at the end of a UDP
		 * transfer, having the client do a shutdown() for writing
		 * on the control connection allows the server to more
		 * quickly realize that the UDP transfer has completed
		 * (mostly of benefit for separate control and data paths)
		 *
		 * Can't do this in the opposite direction since the
		 * server needs to send info back to client */
		shutdown(0, SHUT_WR);
	}

	if (multicast && !trans) {
		/* Leave the multicast group */
		if ((af == AF_INET) && !ssm) {
			if (setsockopt(fd[1], IPPROTO_IP, IP_DROP_MEMBERSHIP,
				       (void *)&mc_group,
				       sizeof(mc_group)) < 0) {
				 err("setsockopt: IP_DROP_MEMBERSHIP");
			}
		}
#ifdef AF_INET6
		else if ((af == AF_INET6) && !ssm) {
			if (setsockopt(fd[1], IPPROTO_IPV6, IPV6_LEAVE_GROUP,
				       (void *)&mc6_group,
				       sizeof(mc6_group)) < 0) {
				err("setsockopt: IPV6_LEAVE_GROUP");
			}
		}
#endif
#ifdef MCAST_JOIN_SOURCE_GROUP
		else if ((af == AF_INET) && ssm) {
			/* Leave the source specific multicast group */
			if (setsockopt(fd[1], IPPROTO_IP,
				       MCAST_LEAVE_SOURCE_GROUP,
				       &group_source_req,
				       sizeof(group_source_req)) < 0) {
				err("setsockopt: MCAST_LEAVE_SOURCE_GROUP");
			}
		}
#ifdef AF_INET6
		else if ((af == AF_INET6) && ssm) {
			/* Leave the source specific multicast group */
			if (setsockopt(fd[1], IPPROTO_IPV6,
				       MCAST_LEAVE_SOURCE_GROUP,
				       &group_source_req,
				       sizeof(group_source_req)) < 0) {
				err("setsockopt: MCAST_LEAVE_SOURCE_GROUP");
			}
		}
#endif /* AF_INET6 */
#endif /* MCAST_JOIN_SOURCE_GROUP */
	}

	for ( stream_idx = 1; stream_idx <= nstream; stream_idx++ ) {
		close(fd[stream_idx]);
		fd[stream_idx] = -1;
	}
}

#ifdef SIGPIPE
void
sigpipe( int signum )
{
	signal(SIGPIPE, sigpipe);
}
#endif

void
sigint( int signum )
{
	signal(SIGINT, SIG_DFL);
	fputs("\n*** transfer interrupted ***\n", stdout);
	if (clientserver && client && !host3 && udp && !trans)
		shutdown(0, SHUT_WR);
	else
		intr = 1;
	done++;
	return;
}

void
ignore_alarm( int signum )
{
	return;
}

void
sigalarm( int signum )
{
	struct	timeval timec;	/* Current time */
	struct	timeval timed;	/* Delta time */
	int64_t nrbytes;
	uint64_t deltarbytes, deltatbytes;
	double fractloss;
	int nodata;
	int i;
	char *cp1, *cp2;
	short save_events;
	long flags, saveflags;

	if (host3 && clientserver) {
		if (client)
			intr = 1;
		return;
	}

	if (clientserver && client && reading_srvr_info) {
		mes("Error: not receiving server info");
		exit(1);
	}

	if (interval && !trans) {
		/* Get real time */
		gettimeofday(&timec, (struct timezone *)0);
		tvsub( &timed, &timec, &timep );
		realtd = timed.tv_sec + ((double)timed.tv_usec) / 1000000;
		if( realtd <= 0.0 )  realtd = 0.000001;
		tvsub( &timed, &timec, &time0 );
		realt = timed.tv_sec + ((double)timed.tv_usec)
						    / 1000000;
		if( realt <= 0.0 )  realt = 0.000001;
	}

	if (clientserver && !trans) {
		struct sockaddr_in peer;
		socklen_t peerlen = sizeof(peer);

		nodata = 0;

		if (getpeername(fd[0], (struct sockaddr *)&peer, &peerlen) < 0)
			nodata = 1;

		if (!client && udp && got_begin) {
			/* checks if client did a shutdown() for writing
			 * on the control connection */
			pollfds[0].fd = fileno(ctlconn);
			save_events = pollfds[0].events;
			pollfds[0].events = POLLIN | POLLPRI;
			pollfds[0].revents = 0;
			if ((poll(pollfds, 1, 0) > 0) &&
			    (pollfds[0].revents & (POLLIN | POLLPRI))) {
				nodata = 1;
			}
			pollfds[0].events = save_events;
		}

		if (interval) {
			chk_interval += realtd;
			if (chk_interval >= chk_idle_data) {
				chk_interval = 0;
				if ((nbytes - chk_nbytes) == 0)
					nodata = 1;
				chk_nbytes = nbytes;
			}
		}
		else {
			if ((nbytes - chk_nbytes) == 0)
				nodata = 1;
			chk_nbytes = nbytes;
		}

		if (nodata) {
			/* Don't just exit anymore so can get partial results
			 * (shouldn't be a problem but keep an eye out that
			 * servers don't start hanging again) */
			if (!client && udp && !interval && handle_urg) {
				/* send 'A' for ABORT as urgent TCP data
				 * on control connection (don't block)
				 *
				 * Only server can do this since client
				 * does a shutdown() for writing on the
				 * control connection */
				saveflags = fcntl(fd[0], F_GETFL, 0);
				if (saveflags != -1) {
					flags = saveflags | O_NONBLOCK;
					fcntl(fd[0], F_SETFL, flags);
				}
				send(fd[0], "A", 1, MSG_OOB);
				if (saveflags != -1) {
					flags = saveflags;
					fcntl(fd[0], F_SETFL, flags);
				}
			}
			if (client) {
				mes("Error: not receiving data from server");
				exit(1);
			}
			close_data_channels();
			intr = 1;
			return;
		}

		if (!interval)
			return;
	}

	if (interval && !trans) {
		if ((udp && !got_begin) || done) {
			timep.tv_sec = timec.tv_sec;
			timep.tv_usec = timec.tv_usec;
			return;
		}
		if (clientserver) {
			nrbytes = nbytes;
			if (udplossinfo) {
				ntbytes = *(int64_t *)(buf + 24);
				if (need_swap) {
					cp1 = (char *)&ntbytes;
					cp2 = buf + 31;
					for ( i = 0; i < 8; i++ )
						*cp1++ = *cp2--;
				}
				if (ntbytes > ntbytesc)
					/* received bytes not counted yet */
					nrbytes += buflen;
				if ((nrbytes > ntbytes) ||
				    ((nrbytes - pbytes) > (ntbytes - ptbytes)))
					/* yes they were counted */
					nrbytes -= buflen;
			}
			if (read_retrans) {
				nretrans[1] = *(uint32_t *)(buf + 24);
				if (need_swap) {
					cp1 = (char *)&nretrans[1];
					cp2 = buf + 27;
					for ( i = 0; i < 4; i++ )
						*cp1++ = *cp2--;
				}
			}
			if (*ident)
				fprintf(stdout, "%s: ", ident + 1);
			if (format & PARSE)
				strcpy(fmt, P_PERF_FMT_INTERVAL);
			else
				strcpy(fmt, PERF_FMT_INTERVAL);
			fprintf(stdout, fmt,
				(double)(nrbytes - pbytes)/(1024*1024), realtd,
				(double)(nrbytes - pbytes)/realtd/125000);
			if (udplossinfo) {
				if (!(format & NODROPS)) {
					if (format & PARSE)
						strcpy(fmt,
						       P_DROP_FMT_INTERVAL);
					else
						strcpy(fmt, DROP_FMT_INTERVAL);
					fprintf(stdout, fmt,
						((ntbytes - ptbytes)
							- (nrbytes - pbytes))
								/buflen,
						(ntbytes - ptbytes)/buflen);
				}
				if (!(format & NOPERCENTLOSS)) {
					deltarbytes = nrbytes - pbytes;
					deltatbytes = ntbytes - ptbytes;
					fractloss = (deltatbytes ?
						1.0 -
						    (double)deltarbytes
							/(double)deltatbytes :
						0.0);
					if (format & PARSE)
						strcpy(fmt,
						       P_LOSS_FMT_INTERVAL);
					else if ((fractloss != 0.0) &&
						 (fractloss < 0.001))
						strcpy(fmt,
							LOSS_FMT_INTERVAL5);
					else
						strcpy(fmt, LOSS_FMT_INTERVAL);
					fprintf(stdout, fmt, fractloss * 100);
				}
			}
			if ((do_jitter & JITTER_MIN) && njitteri) {
				if (format & PARSE)
					strcpy(fmt, P_JITTER_MIN_FMT_INTERVAL);
				else
					strcpy(fmt, JITTER_MIN_FMT_INTERVAL);
				fprintf(stdout, fmt, jitter_mini);
			}
			if ((do_jitter & JITTER_AVG) && njitteri) {
				if (format & PARSE)
					strcpy(fmt, P_JITTER_AVG_FMT_INTERVAL);
				else
					strcpy(fmt, JITTER_AVG_FMT_INTERVAL);
				fprintf(stdout, fmt, jitter_avgi/njitteri);
			}
			if ((do_jitter & JITTER_MAX) && njitteri) {
				if (format & PARSE)
					strcpy(fmt, P_JITTER_MAX_FMT_INTERVAL);
				else
					strcpy(fmt, JITTER_MAX_FMT_INTERVAL);
				fprintf(stdout, fmt, jitter_maxi);
			}
			if (do_jitter && njitteri) {
				njitteri = 0;
				jitter_mini = 1000000.0;
				jitter_maxi = -1000000.0;
				jitter_avgi = 0.0;
			}
			if (read_retrans && sinkmode) {
				if (format & PARSE)
					fprintf(stdout, P_RETRANS_FMT_INTERVAL,
						((retransinfo == 1) ||
						 !nrbytes) ?  "" : "host-",
						(nretrans[1] - pretrans));
				else
					fprintf(stdout, RETRANS_FMT_INTERVAL,
						(nretrans[1] - pretrans),
						((retransinfo == 1) ||
						 !nrbytes) ?  "" : "host-");
			}
			if ((do_owd & OWD_MIN) && nowdi) {
				if (format & PARSE)
					strcpy(fmt, P_OWD_MIN_FMT_INTERVAL);
				else
					strcpy(fmt, OWD_MIN_FMT_INTERVAL);
				fprintf(stdout, fmt, owd_mini);
			}
			if ((do_owd & OWD_AVG) && nowdi) {
				if (format & PARSE)
					strcpy(fmt, P_OWD_AVG_FMT_INTERVAL);
				else
					strcpy(fmt, OWD_AVG_FMT_INTERVAL);
				fprintf(stdout, fmt, owd_avgi/nowdi);
			}
			if ((do_owd & OWD_MAX) && nowdi) {
				if (format & PARSE)
					strcpy(fmt, P_OWD_MAX_FMT_INTERVAL);
				else
					strcpy(fmt, OWD_MAX_FMT_INTERVAL);
				fprintf(stdout, fmt, owd_maxi);
			}
			if (do_owd && nowdi) {
				nowdi = 0;
				owd_mini = 1000000.0;
				owd_maxi = -1000000.0;
				owd_avgi = 0.0;
			}
			if (format & RUNNINGTOTAL) {
				if (format & PARSE)
					strcpy(fmt, P_PERF_FMT_INTERVAL2);
				else
					strcpy(fmt, PERF_FMT_INTERVAL2);
				fprintf(stdout, fmt,
					(double)nrbytes/(1024*1024), realt,
					(double)nrbytes/realt/125000 );
				if (udplossinfo) {
					if (!(format & NODROPS)) {
						if (format & PARSE)
							strcpy(fmt,
							  P_DROP_FMT_INTERVAL);
						else
							strcpy(fmt,
							  DROP_FMT_INTERVAL);
						fprintf(stdout, fmt,
							(ntbytes - nrbytes)
								/buflen,
							ntbytes/buflen);
					}
					if (!(format & NOPERCENTLOSS)) {
						fractloss = (ntbytes ?
							1.0 -
							    (double)nrbytes
							      /(double)ntbytes :
							0.0);
						if (format & PARSE)
							strcpy(fmt,
							  P_LOSS_FMT_INTERVAL);
						else if ((fractloss != 0.0) &&
							 (fractloss < 0.001))
							strcpy(fmt,
							  LOSS_FMT_INTERVAL5);
						else
							strcpy(fmt,
							  LOSS_FMT_INTERVAL);
						fprintf(stdout, fmt,
							fractloss * 100);
					}
				}
				if (read_retrans && sinkmode) {
					if (format & PARSE)
						fprintf(stdout,
							P_RETRANS_FMT_INTERVAL,
							((retransinfo == 1) ||
							 !nrbytes) ?
							    "" : "host-",
							nretrans[1]);
					else
						fprintf(stdout,
							RETRANS_FMT_INTERVAL,
							nretrans[1],
							((retransinfo == 1) ||
							 !nrbytes) ?
							    "" : "host-");
				}
			}
			if (udplossinfo && (format & XMITSTATS)) {
				if (format & PARSE)
					strcpy(fmt, P_PERF_FMT_INTERVAL3);
				else
					strcpy(fmt, PERF_FMT_INTERVAL3);
				fprintf(stdout, fmt,
					(double)(ntbytes - ptbytes)/1024/1024);
				if (format & RUNNINGTOTAL) {
					if (format & PARSE)
						strcpy(fmt,
						       P_PERF_FMT_INTERVAL4);
					else
						strcpy(fmt, PERF_FMT_INTERVAL4);
					fprintf(stdout, fmt,
						(double)ntbytes/1024/1024);
					if (format & DEBUGINTERVAL)
						fprintf(stdout, " Pre: %.4f MB",
							(double)ntbytesc
								  /1024/1024);
				}
			}
			fprintf(stdout, "\n");
			fflush(stdout);
			timep.tv_sec = timec.tv_sec;
			timep.tv_usec = timec.tv_usec;
			pbytes = nrbytes;
			ptbytes = ntbytes;
			pretrans = nretrans[1];
		}
	}
	else
		intr = 1;
	return;
}

int
main( int argc, char **argv )
{
	double MB;
	double rate_opt;
	double fractloss;
	int cpu_util;
	int first_read;
	int first_jitter, first_jitteri;
	int ocorrection = 0;
	double  correction = 0.0;
	int pollst = 0;
	int i = 0, j = 0;
	char *cp1 = NULL, *cp2 = NULL, *cp3 = NULL;
	char *hostaddr;
	char ch = '\0';
	int error_num = 0;
	int sockopterr = 0;
	int save_errno;
	struct servent *sp = 0;
	struct addrinfo hints, *res[MAXSTREAM + 1] = { NULL }, *host3res;
	struct sockaddr_storage dummy;
	struct timeval time_eod;	/* time EOD packet was received */
	struct timeval time_eod0;	/* time EOD0 packet was received */
	struct timeval timed;		/* time delta */
	struct timeval timeconn1;	/* time before connect() for RTT */
	struct timeval timeconn2;	/* time after connect() for RTT */
	struct timeval timeconn;	/* time to connect() == RTT */
	union {
		unsigned char	buf[sizeof(struct in_addr)];
		uint32_t	ip32;
	} ipad_stride;			/* IPv4 address stride */
	short save_events;
	int skiparg;
	int reqval;
	int got_srvr_retrans;
	uint32_t total_retrans = 0;
	double idle_data_min = IDLE_DATA_MIN;
	double idle_data_max = IDLE_DATA_MAX;
	double default_idle_data = DEFAULT_IDLE_DATA;
	char multsrc[ADDRSTRLEN] = "\0";
	char multaddr[ADDRSTRLEN] = "\0";
	long flags;
	int nameinfo_flags;
	int implicit_hostaddr;

	sendwin = 0;
	rcvwin = 0;
	srvrwin = -1;
	format |= WANTRTT;

	if (argc < 2) goto usage;

	nut_cmd = argv[0];
	argv++; argc--;
	while( argc>0 && argv[0][0] == '-' )  {
		skiparg = 0;
		switch (argv[0][1]) {

		case '4':
			domain = PF_INET;
			af = AF_INET;
			explicitaf = 1;
			break;
#ifdef AF_INET6
		case '6':
			domain = PF_INET6;
			af = AF_INET6;
			explicitaf = 1;
			break;
#endif
		case 'B':
			b_flag = 1;
			break;
		case 't':
			trans = 1;
			break;
		case 'r':
			trans = 0;
			break;
		case 'd':
			options |= SO_DEBUG;
			break;
		case 'D':
			nodelay = 1;
			break;
		case 'n':
			reqval = 0;
			if (argv[0][2] == 'b') {
				fprintf(stderr, "option \"-nb\" no longer supported, use \"-n###[k|m|g|t|p]\" instead\n");
				fflush(stderr);
				exit(1);
			}
			cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			nbuf = strtoull(cp1, NULL, 0);
			if (nbuf == 0) {
				if (errno == EINVAL) {
					fprintf(stderr, "invalid nbuf = %s\n",
						&argv[0][2]);
					fflush(stderr);
					exit(1);
				}
				else {
					nbuf = DEFAULT_NBUF;
					break;
				}
			}
			if (*cp1)
				ch = *(cp1 + strlen(cp1) - 1);
			else
				ch = '\0';
			if ((ch == 'b') || (ch == 'B'))
				nbuf_bytes = 1;
			else if ((ch == 'k') || (ch == 'K')) {
				nbuf *= 1024;
				nbuf_bytes = 1;
			}
			else if ((ch == 'm') || (ch == 'M')) {
				nbuf *= 1048576;
				nbuf_bytes = 1;
			}
			else if ((ch == 'g') || (ch == 'G')) {
				nbuf *= 1073741824;
				nbuf_bytes = 1;
			}
			else if ((ch == 't') || (ch == 'T')) {
				nbuf *= 1099511627776ull;
				nbuf_bytes = 1;
			}
			else if ((ch == 'p') || (ch == 'P')) {
				nbuf *= 1125899906842624ull;
				nbuf_bytes = 1;
			}
			break;
		case 'l':
			reqval = 1;
			cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			buflen = atoi(cp1);
			buflenopt = 1;
			if (buflen < 1) {
				fprintf(stderr, "invalid buflen = %d\n", buflen);
				fflush(stderr);
				exit(1);
			}
			if (*cp1)
				ch = *(cp1 + strlen(cp1) - 1);
			else
				ch = '\0';
			if ((ch == 'k') || (ch == 'K'))
				buflen *= 1024;
			else if ((ch == 'm') || (ch == 'M'))
				buflen *= 1048576;
			break;
		case 'w':
			reqval = 1;
			if (argv[0][2] == 's') {
				cp1 = getoptvalp(argv, 3, reqval, &skiparg);
				srvrwin = atoi(cp1);
				if (*cp1)
					ch = *(cp1 + strlen(cp1) - 1);
				else
					ch = '\0';
				if ((ch == 'k') || (ch == 'K'))
					srvrwin *= 1024;
				else if ((ch == 'm') || (ch == 'M'))
					srvrwin *= 1048576;
				else if ((ch == 'g') || (ch == 'G'))
					srvrwin *= 1073741824;
				else if ((ch != 'b') && (ch != 'B'))
					srvrwin *= 1024;
				if (srvrwin < 0) {
					fprintf(stderr, "invalid srvrwin = %d\n", srvrwin);
					fflush(stderr);
					exit(1);
				}
			}
			else {
				if (argv[0][2] == 'b') {
					braindead = 1;
					cp1 = getoptvalp(argv, 3, reqval,
							 &skiparg);
					if (*cp1 == '\0')
						break;
					sendwin = atoi(cp1);
				}
				else {
					cp1 = getoptvalp(argv, 2, reqval,
							 &skiparg);
					sendwin = atoi(cp1);
				}

				if (*cp1)
					ch = *(cp1 + strlen(cp1) - 1);
				else
					ch = '\0';
				if ((ch == 'k') || (ch == 'K'))
					sendwin *= 1024;
				else if ((ch == 'm') || (ch == 'M'))
					sendwin *= 1048576;
				else if ((ch == 'g') || (ch == 'G'))
					sendwin *= 1073741824;
				else if ((ch != 'b') && (ch != 'B'))
					sendwin *= 1024;
				rcvwin = sendwin;
				if (sendwin < 0) {
					fprintf(stderr, "invalid sendwin = %d\n", sendwin);
					fflush(stderr);
					exit(1);
				}
			}
			if (srvrwin == -1) {
				srvrwin = sendwin;
			}
			break;
		case 's':
			sinkmode = 0;	/* sink/source data */
			break;
		case 'p':
			reqval = 1;
			tmpport = atoi(getoptvalp(argv, 2, reqval, &skiparg));
			if ((tmpport < 1024) || (tmpport > 65535)) {
				fprintf(stderr, "invalid port = %d\n", tmpport);
				fflush(stderr);
				exit(1);
			}
			port = tmpport;
			break;
		case 'P':
			reqval = 1;
			cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			tmpport = atoi(cp1);
			if ((tmpport < 1024) || (tmpport > 65535)) {
				fprintf(stderr,
					"invalid ctlport = %d\n", tmpport);
				fflush(stderr);
				exit(1);
			}
			ctlport = tmpport;
			if ((cp2 = strchr(cp1, '/'))) {
				tmpport = atoi(cp2 + 1);
				if ((tmpport < 1024) || (tmpport > 65535)) {
					fprintf(stderr,
						"invalid third party "
						"ctlport = %d\n", tmpport);
					fflush(stderr);
					exit(1);
				}
				ctlport3 = tmpport;
			}
			break;
		case 'u':
			udp = 1;
			if (!buflenopt) buflen = DEFAULTUDPBUFLEN;
			if (argv[0][2] == 'u') {
				haverateopt = 1;
				rate = MAXRATE;
			}
			break;
		case 'j':
			reqval = 0;
			cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			if (strchr(cp1, 'm'))
				do_jitter |= JITTER_MIN;
			if (strchr(cp1, 'a'))
				do_jitter |= JITTER_AVG;
			if (strchr(cp1, 'x'))
				do_jitter |= JITTER_MAX;
			if (do_jitter == 0)
				do_jitter = JITTER_MAX;
			if (strchr(cp1, 'o'))
				do_jitter |= JITTER_IGNORE_OOO;
			udp = 1;
			if (!buflenopt) buflen = DEFAULTUDPBUFLEN;
			break;
		case 'o':
			reqval = 0;
			cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			if (strchr(cp1, 'm'))
				do_owd |= OWD_MIN;
			if (strchr(cp1, 'a'))
				do_owd |= OWD_AVG;
			if (strchr(cp1, 'x'))
				do_owd |= OWD_MAX;
			if (do_owd == 0)
				do_owd = OWD_AVG;
			break;
		case 'v':
			brief = 0;
			if (argv[0][2] == 'v')
				verbose = 1;
			break;
		case 'N':
			reqval = 1;
			cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			nstream = atoi(cp1);
			if (strchr(cp1, 'm'))
				multilink = 1;
			if (nstream < 1) {
				fprintf(stderr, "invalid nstream = %d\n", nstream);
				fflush(stderr);
				exit(1);
			}
			if (nstream > MAXSTREAM) {
				fprintf(stderr, "nstream = %d > MAXSTREAM, set to %d\n",
				    nstream, MAXSTREAM);
				nstream = MAXSTREAM;
			}
			if (nstream > 1) {
				b_flag = 1;
				send_retrans = 0;
				read_retrans = 0;
			}
			break;
		case 'R':
			reqval = 1;
			haverateopt = 1;
			if (argv[0][2] == 'i') {
				cp1 = getoptvalp(argv, 3, reqval, &skiparg);
				sscanf(cp1, "%lf", &rate_opt);
				irate = 1;
			}
			else if (argv[0][2] == 'a') {
				cp1 = getoptvalp(argv, 3, reqval, &skiparg);
				sscanf(cp1, "%lf", &rate_opt);
				irate = 0;
			}
			else if (argv[0][2] == 'u') {
				rate_opt = 0.0;
				irate = 0;
				cp1 = &argv[0][3];
			}
			else {
				cp1 = getoptvalp(argv, 2, reqval, &skiparg);
				sscanf(cp1, "%lf", &rate_opt);
			}
			if ((cp2 = strchr(cp1, '/'))) {
				*cp2++ = '\0';
				maxburst = atoi(cp2);
				if (maxburst <= 0) {
					fprintf(stderr,
						"invalid maxburst = %d\n",
						maxburst);
					fflush(stderr);
					exit(1);
				}
			}
			if (*cp1)
				ch = *(cp1 + strlen(cp1) - 1);
			else
				ch = '\0';
			if ((ch == 'm') || (ch == 'M'))
				rate_opt *= 1000;
			else if ((ch == 'g') || (ch == 'G'))
				rate_opt *= 1000000;
			else if (ch == 'p') {
				rate_pps = 1;
				if (strlen(cp1) >= 2) {
					ch = *(cp1 + strlen(cp1) - 2);
					if ((ch == 'k') || (ch == 'K'))
						rate_opt *= 1000;
					if ((ch == 'm') || (ch == 'M'))
						rate_opt *= 1000000;
				}
			}
			rate = rate_opt;
			if (rate == 0)
				rate = MAXRATE;
			break;
		case 'T':
			reqval = 0;
			cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			sscanf(cp1, "%lf", &timeout);
			if (timeout < 0) {
				fprintf(stderr, "invalid timeout = %f\n", timeout);
				fflush(stderr);
				exit(1);
			}
			else if (timeout == 0.0)
				timeout = DEFAULT_TIMEOUT;
			if (*cp1)
				ch = *(cp1 + strlen(cp1) - 1);
			else
				ch = '\0';
			if ((ch == 'm') || (ch == 'M'))
				timeout *= 60.0;
			else if ((ch == 'h') || (ch == 'H'))
				timeout *= 3600.0;
			else if ((ch == 'd') || (ch == 'D'))
				timeout *= 86400.0;
			itimer.it_value.tv_sec = timeout;
			itimer.it_value.tv_usec =
				(timeout - itimer.it_value.tv_sec)*1000000;
			if (timeout && !nbuf)
				nbuf = INT_MAX;
			break;
		case 'i':
			reqval = 0;
			cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			sscanf(cp1, "%lf", &interval);
			if (interval < 0.0) {
				fprintf(stderr, "invalid interval = %f\n", interval);
				fflush(stderr);
				exit(1);
			}
			else if (interval == 0.0)
				interval = 1.0;
			if (*cp1)
				ch = *(cp1 + strlen(cp1) - 1);
			else
				ch = '\0';
			if ((ch == 'm') || (ch == 'M'))
				interval *= 60.0;
			else if ((ch == 'h') || (ch == 'H'))
				interval *= 3600.0;
			break;
		case 'I':
			reqval = 1;
			ident[0] = '-';
			strncpy(&ident[1],
				getoptvalp(argv, 2, reqval, &skiparg), 40);
			ident[41] = '\0';
			break;
		case 'F':
			reverse = 1;
			break;
		case 'b':
			reqval = 0;
			cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			if (*cp1) {
				if (isalpha((int)(*cp1)))
					brief = 1;
				else
					brief = atoi(cp1);
				if (strchr(cp1, 'r'))
					brief |= BRIEF_RETRANS_STREAMS;
			}
			else
				brief = 1;
			break;
		case 'S':
			if (strchr(&argv[0][2], 'P'))
				pass_ctlport = 1;
			trans = 0;
			clientserver = 1;
			brief = 0;
			verbose = 1;
			break;
		case '1':
			oneshot = 1;
			trans = 0;
			clientserver = 1;
			brief = 0;
			verbose = 1;
			break;
		case 'V':
			fprintf(stdout, "nuttcp-%d.%d.%d%s\n", vers_major,
					vers_minor, vers_delta,
					beta ? BETA_STR : "");
			exit(0);
		case 'f':
			if (strcmp(&argv[0][2], "xmitstats") == 0)
				format |= XMITSTATS;
			else if (strcmp(&argv[0][2], "debuginterval") == 0)
				format |= DEBUGINTERVAL;
			else if (strcmp(&argv[0][2], "runningtotal") == 0)
				format |= RUNNINGTOTAL;
			else if (strcmp(&argv[0][2], "-percentloss") == 0)
				format |= NOPERCENTLOSS;
			else if (strcmp(&argv[0][2], "-drops") == 0)
				format |= NODROPS;
			else if (strcmp(&argv[0][2], "-retrans") == 0)
				format |= NORETRANS;
			else if (strcmp(&argv[0][2], "debugretrans") == 0)
				format |= DEBUGRETRANS;
			else if (strcmp(&argv[0][2], "debugpoll") == 0)
				format |= DEBUGPOLL;
			else if (strcmp(&argv[0][2], "debugmtu") == 0)
				format |= DEBUGMTU;
			else if (strcmp(&argv[0][2], "debugjitter") == 0)
				format |= DEBUGJITTER;
			else if (strcmp(&argv[0][2], "parse") == 0)
				format |= PARSE;
			else if (strcmp(&argv[0][2], "-beta") == 0)
				format |= NOBETAMSG;
			/* below is for compatibility with 6.0.x beta */
			else if (strcmp(&argv[0][2], "rtt") == 0)
				format |= WANTRTT;
			else if (strcmp(&argv[0][2], "-rtt") == 0)
				format &= ~WANTRTT;
			else {
				if (argv[0][2]) {
					fprintf(stderr, "invalid format option \"%s\"\n", &argv[0][2]);
					fflush(stderr);
					exit(1);
				}
				else {
					fprintf(stderr, "invalid null format option\n");
					fprintf(stderr, "perhaps the \"-F\" flip option was intended\n");
					fflush(stderr);
					exit(1);
				}
			}
			break;
		case 'x':
			reqval = 1;
			if (argv[0][2] == 't') {
				traceroute = 1;
				brief = 1;
			}
#ifdef HAVE_SETPRIO
			else if (argv[0][2] == 'P') {
				priority = atoi(getoptvalp(argv, 3, reqval,
						&skiparg));
			}
#endif
#ifdef HAVE_SETAFFINITY
			else if (argv[0][2] == 'c') {
				reqval = 1;
				if (argv[0][3] == 's') {
					cp1 = getoptvalp(argv, 4, reqval,
							 &skiparg);
					srvr_affinity = atoi(cp1);
					if (srvr_affinity < 0) {
						fprintf(stderr,
							"invalid srvr_affinity "
							"= %d\n",
							srvr_affinity);
						fflush(stderr);
						exit(1);
					}
				}
				else {
					cp1 = getoptvalp(argv, 3, reqval,
							 &skiparg);
					affinity = atoi(cp1);
					if ((affinity < 0) ||
					    (affinity >= CPU_SETSIZE)) {
						fprintf(stderr,
							"invalid affinity "
							"= %d\n", affinity);
						fflush(stderr);
						exit(1);
					}
					if ((cp2 = strchr(cp1, '/'))) {
						srvr_affinity = atoi(cp2 + 1);
						if (srvr_affinity < 0) {
							fprintf(stderr,
								"invalid "
								"srvr_affinity "
								"= %d\n",
								srvr_affinity);
							fflush(stderr);
							exit(1);
						}
					}
				}
			}
#endif
			else {
				if (argv[0][2]) {
					fprintf(stderr, "invalid x option \"%s\"\n", &argv[0][2]);
					fflush(stderr);
					exit(1);
				}
				else {
					fprintf(stderr, "invalid null x option\n");
					fflush(stderr);
					exit(1);
				}
			}
			break;
		case '3':
			thirdparty = 1;
			break;
		case 'm':
			reqval = 0;
			if (argv[0][2] == 'a') {
				ssm = 0;
				cp1 = getoptvalp(argv, 3, reqval, &skiparg);
			}
			else if (argv[0][2] == 's') {
#ifdef MCAST_JOIN_SOURCE_GROUP
				ssm = 1;
				cp1 = getoptvalp(argv, 3, reqval, &skiparg);
#else
				fprintf(stderr,
					"This system does not support SSM\n");
				fflush(stderr);
				exit(1);
#endif
			}
			else {
				cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			}
			if (*cp1)
				mc_param = atoi(cp1);
			else
				mc_param = 1;
			if ((mc_param < 1) || (mc_param > 255)) {
				fprintf(stderr, "invalid multicast ttl = %d\n", mc_param);
				fflush(stderr);
				exit(1);
			}
			multicast = mc_param;
			break;
		case 'M':
			reqval = 1;
			datamss = atoi(getoptvalp(argv, 2, reqval, &skiparg));
			if (datamss < 0) {
				fprintf(stderr, "invalid datamss = %d\n", datamss);
				fflush(stderr);
				exit(1);
			}
			break;
		case 'c':
			reqval = 1;
			cp1 = getoptvalp(argv, 2, reqval, &skiparg);
			tos = strtol(cp1, NULL, 0);
			if (*cp1)
				ch = *(cp1 + strlen(cp1) - 1);
			else
				ch = '\0';
			if ((ch == 'p') || (ch == 'P')) {
				/* Precedence */
				if (tos > 7) {
					fprintf(stderr, "invalid precedence = %d\n", tos);
					fflush(stderr);
					exit(1);
				}
				tos <<= 5;
			}
			else if ((ch != 't') && (ch != 'T')) {
				/* DSCP */
				if (tos > 63) {
					fprintf(stderr, "invalid dscp = %d\n", tos);
					fflush(stderr);
					exit(1);
				}
				tos <<= 2;
			}
			if (tos > 255) {
				fprintf(stderr, "invalid tos = %d\n", tos);
				fflush(stderr);
				exit(1);
			}
			break;
		case 'a':
			retry_server = 1;
			break;
		case '-':
			if (strcmp(&argv[0][2], "nofork") == 0) {
				nofork=1;
			}
			else if (strcmp(&argv[0][2], "no3rdparty") == 0) {
				no3rd=1;
			}
			else if (strcmp(&argv[0][2],
				 "idle-data-timeout") == 0) {
				if ((cp1 = strchr(argv[1], '/'))) {
					if (strchr(cp1 + 1, '/')) {
						if (sscanf(argv[1],
							"%lf/%lf/%lf",
							&idle_data_min,
							&default_idle_data,
							&idle_data_max) != 3) {
							fprintf(stderr, "error scanning idle-data-timeout parameter = %s\n", argv[1]);
							fflush(stderr);
							exit(1);
						}
						if (idle_data_min <= 0.0) {
							fprintf(stderr, "invalid value for idle-data-timeout minimum = %f\n", idle_data_min);
							fflush(stderr);
							exit(1);
						}
						if (default_idle_data <= 0.0) {
							fprintf(stderr, "invalid value for idle-data-timeout default = %f\n", default_idle_data);
							fflush(stderr);
							exit(1);
						}
						if (idle_data_max <= 0.0) {
							fprintf(stderr, "invalid value for idle-data-timeout maximum = %f\n", idle_data_max);
							fflush(stderr);
							exit(1);
						}
						if (idle_data_max <
							idle_data_min) {
							fprintf(stderr, "error: idle-data-timeout maximum of %f < minimum of %f\n", idle_data_max, idle_data_min);
							fflush(stderr);
							exit(1);
						}
					}
					else {
						fprintf(stderr, "invalid idle-data-timeout parameter = %s\n", argv[1]);
						fflush(stderr);
						exit(1);
					}
				}
				else {
					sscanf(argv[1], "%lf", &idle_data_min);
					if (idle_data_min <= 0.0) {
						fprintf(stderr, "invalid value for idle-data-timeout = %f\n", idle_data_min);
						fflush(stderr);
						exit(1);
					}
					idle_data_max = idle_data_min;
					default_idle_data = idle_data_min;
				}
				argv++;
				argc--;
			}
			else if (strcmp(&argv[0][2], "single-threaded") == 0) {
				single_threaded=1;
			}
			else if (strcmp(&argv[0][2], "packet-burst") == 0) {
				maxburst = atoi(argv[1]);
				if (maxburst <= 0) {
					fprintf(stderr,
						"invalid maxburst = %d\n",
						maxburst);
					fflush(stderr);
					exit(1);
				}
				argv++;
				argc--;
			}
#ifdef IPV6_V6ONLY
			else if (strcmp(&argv[0][2], "disable-v4-mapped") == 0) {
				v4mapped=0;
			}
			else if (strcmp(&argv[0][2], "enable-v4-mapped") == 0) {
				v4mapped=1;
			}
#endif
			else {
				goto usage;
			}
			break;
		case 'h':
		default:
			goto usage;
		}
		argv++;
		argc--;
		if (skiparg) {
			argv++;
			argc--;
		}
	}

	if (argc > 2) goto usage;
	if (trans && (argc < 1)) goto usage;
	if (clientserver && (argc != 0)) goto usage;

	if (!clientserver && !trans && (argc < 1)) {
		fprintf(stderr,
			"nuttcp: Warning: Using obsolete \"classic\" mode:\n");
		fprintf(stderr,
			"                 Automatically switching to "
					  "oneshot server mode "
					  "(\"nuttcp -1\")\n");
		oneshot = 1;
		trans = 0;
		clientserver = 1;
		brief = 0;
		verbose = 1;
	}

	host3 = NULL;
	if (argc == 2) {
		host3 = argv[1];
		if (strlen(host3) > HOSTNAMELEN) {
			fprintf(stderr, "3rd party host '%s' too long\n", host3);
			fflush(stderr);
			exit(1);
		}
		cp1 = host3;
		while (*cp1) {
			if (!isalnum((int)(*cp1)) && (*cp1 != '-') && (*cp1 != '.')
					   && (*cp1 != ':') && (*cp1 != '/')
					   && (*cp1 != '+') && (*cp1 != '=')) {
				fprintf(stderr, "invalid 3rd party host '%s'\n", host3);
				fflush(stderr);
				exit(1);
			}
			cp1++;
		}
	}

	if (multicast) {
		udp = 1;
		if (!buflenopt) buflen = DEFAULT_MC_UDPBUFLEN;
		nstream = 1;
	}

#ifdef AF_INET6
	if (!inet_pton(AF_INET6, HI_MC6, &hi_mc6)) {
		err("inet_pton");
	}
#endif

	if (udp && !haverateopt)
		rate = DEFAULT_UDP_RATE;

	bzero((char *)&frominet, sizeof(frominet));
	bzero((char *)&clientaddr, sizeof(clientaddr));

#ifdef AF_INET6
	bzero((char *)&clientaddr6, sizeof(clientaddr6));
	clientscope6 = 0;
#endif

	if (!nbuf) {
		if (timeout == 0.0) {
			if (sinkmode) {
				timeout = DEFAULT_TIMEOUT;
				itimer.it_value.tv_sec = timeout;
				itimer.it_value.tv_usec =
					(timeout - itimer.it_value.tv_sec)
						*1000000;
			}
			nbuf = INT_MAX;
		}
	}

	if (srvrwin == -1) {
		srvrwin = sendwin;
	}

	if ((argc == 0) && !explicitaf) {
		domain = PF_INET;
		af = AF_INET;
	}

	if (multilink) {
		if (nstream == 1) {
			fprintf(stderr, "Warning: multilink mode not meaningful for a single stream\n");
			fflush(stderr);
		}
	}

	if (argc >= 1) {
		host = argv[0];
		if ((cp1 = strchr(host, '+'))) {
			*cp1++ = '\0';
			if (*cp1)
				stride = cp1;
		}
		if (multilink) {
			if (stride) {
				fprintf(stderr, "don't use both multilink and address stride\n");
				fflush(stderr);
				exit(1);
			}
			if ((cp1 = strchr(host, '/')) && strchr(cp1 + 1, '/')) {
				fprintf(stderr, "multilink mode not compatible with multiple hosts %s\n", host);
				fflush(stderr);
				exit(1);
			}
		}
		hostaddr = NULL;
		implicit_hostaddr = 0;
		if ((cp1 = strchr(host, '='))) {
			*cp1++ = '\0';
			if (strchr(cp1, '/')) {
				fprintf(stderr, "host=addr format not supported for multiple control/data paths\n");
				fflush(stderr);
				exit(1);
			}
			if (*cp1) {
				if (*cp1 == '=') {
					implicit_hostaddr = 1;
					cp1++;
				}
				hostaddr = cp1;
				if (!implicit_hostaddr)
					host = hostaddr;
			}
		}
		stream_idx = 0;
		res[0] = NULL;
		cp1 = host;
		if (host[strlen(host) - 1] == '/') {
			fprintf(stderr, "bad hostname or address: trailing '/' not allowed: %s\n", host);
			fflush(stderr);
			exit(1);
		}
		if (strchr(host, '/') && !trans && !reverse) {
			fprintf(stderr, "multiple control/data paths not supported for receive\n");
			fflush(stderr);
			exit(1);
		}
		if (strchr(host, '/') && trans && reverse) {
			fprintf(stderr, "multiple control/data paths not supported for flipped transmit\n");
			fflush(stderr);
			exit(1);
		}
		if (host[0] == '/') {
			host++;
			cp1++;
			stream_idx = 1;
		}
		else if ((cp2 = strchr(host, '/'))) {
			host = cp2 + 1;
		}

		while (stream_idx <= nstream) {
			bzero(&hints, sizeof(hints));
			res[stream_idx] = NULL;
			if (explicitaf) hints.ai_family = af;
			if (udp)
				hints.ai_socktype = SOCK_DGRAM;
			else
				hints.ai_socktype = SOCK_STREAM;
			if ((cp2 = strchr(cp1, '/'))) {
				if (stream_idx == nstream) {
					fprintf(stderr, "bad hostname or address: too many data paths for nstream=%d: %s\n", nstream, argv[0]);
					fflush(stderr);
					exit(1);
				}
				*cp2 = '\0';
			}
			if (!(multilink && (stream_idx > 1)) &&
			    (error_num = getaddrinfo(cp1, NULL, &hints,
						     &res[stream_idx]))) {
				if (implicit_hostaddr && hostaddr) {
					if (res[stream_idx]) {
						freeaddrinfo(res[stream_idx]);
						res[stream_idx] = NULL;
					}
					error_num =
						getaddrinfo(hostaddr, NULL,
							    &hints,
							    &res[stream_idx]);
				}
				if (error_num) {
					if (cp2)
						*cp2++ = '/';
					if (hostaddr) {
						if (implicit_hostaddr)
							*(hostaddr - 2) = '=';
						else
							*(hostaddr - 1) = '=';
					}
					fprintf(stderr, "bad hostname or address: %s: %s\n", gai_strerror(error_num), argv[0]);
					fflush(stderr);
					exit(1);
				}
				if (implicit_hostaddr && hostaddr &&
				    (stream_idx == 1)) {
					if (stride)
						*(stride - 1) = '+';
					cp3 = hostaddr;
					while (*cp3) {
						*(cp3 - 1) = *cp3;
						cp3++;
					}
					*(cp3 - 1) = '\0';
					hostaddr--;
					implicit_hostaddr = 0;
					if (stride) {
						stride--;
						*(stride - 1) = '\0';
					}
				}
			}
			else if (multilink && (stream_idx > 1)) {
				if (res[stream_idx - 1]->ai_next)
					res[stream_idx] =
						res[stream_idx - 1]->ai_next;
				else
					res[stream_idx] = res[1];
			}
			else if (!(multilink && (stream_idx > 1)) &&
				 implicit_hostaddr && hostaddr) {
				if (stride) {
					strcat(host, "+");
					strncat(host, stride, ADDRSTRLEN);
					*(hostaddr - 2) = '\0';
					stride = hostaddr - 1;
				}
				hostaddr = NULL;
			}
			af = res[stream_idx]->ai_family;
/*
 * At the moment PF_ matches AF_ but are maintained seperate and the socket
 * call is supposed to be PF_
 *
 * For now we set domain from the address family we looked up, but if these
 * ever get changed to not match some code will have to go here to find the
 * domain appropriate for the family
 */
			domain = af;
			stream_idx++;
			if (cp2) {
				*cp2++ = '/';
				cp1 = cp2;
			}
			else
				cp1 = host;
		}
		if (!res[0]) {
			if ((cp1 = strchr(host, '/')))
				*cp1 = '\0';
			if ((error_num = getaddrinfo(host, NULL, &hints, &res[0]))) {
				if (cp1)
					*cp1++ = '/';
				fprintf(stderr, "bad hostname or address: %s: %s\n", gai_strerror(error_num), argv[0]);
				fflush(stderr);
				exit(1);
			}
			af = res[0]->ai_family;
			/* see previous comment about domain */
			domain = af;
			if (cp1)
				*cp1 = '/';
		}
		if (hostaddr) {
			host = argv[0];
			if (implicit_hostaddr)
				*(hostaddr - 2) = '=';
			else
				*(hostaddr - 1) = '=';
		}
	}

	ipad_stride.ip32 = 0;
	if (stride) {
		if (strlen(stride) >= ADDRSTRLEN) {
			fprintf(stderr, "address stride '%s' too long\n", stride);
			fflush(stderr);
			exit(1);
		}
		if (nstream == 1) {
			fprintf(stderr, "Warning: stride %s not meaningful for a single stream\n", stride);
			fflush(stderr);
		}
		if (udp) {
			fprintf(stderr, "stride %s not valid for UDP\n",
				stride);
			fflush(stderr);
			exit(1);
		}
		if ((cp1 = strchr(argv[0], '/')) && strchr(cp1 + 1, '/')) {
			fprintf(stderr, "stride %s not compatible with multiple hosts %s\n", stride, argv[0]);
			fflush(stderr);
			exit(1);
		}
		if (af == AF_INET) {
			if (strchr(stride, '.')) {
				error_num = inet_pton(AF_INET, stride,
						ipad_stride.buf);
				if (error_num == 0) {
					fprintf(stderr,
						"stride %s not in correct presentation format\n",
						stride);
					fflush(stderr);
					exit(1);
				}
				else if (error_num < 0)
					err("inet_pton: stride");
			}
			else {
				ipad_stride.ip32 = atoi(stride);
				ipad_stride.ip32 = htonl(ipad_stride.ip32);
			}
		}
		else {
			fprintf(stderr, "stride %s not valid for IPv6\n",
				stride);
			fflush(stderr);
			exit(1);
		}
		*(stride - 1) = '+';
	}

	if (host3 && !strchr(host3, '=') && !strchr(host3, '/')) {
		cp1 = strchr(host3, '+');
		if (cp1) {
			if (strlen(cp1 + 1) >= ADDRSTRLEN) {
				fprintf(stderr, "3rd party address stride '%s' too long\n", cp1 + 1);
				fflush(stderr);
				exit(1);
			}
			*cp1 = '\0';
		}
		if (inet_pton(af, host3, &dummy) != 1) {
			bzero(&hints, sizeof(hints));
			hints.ai_family = af;
			if (udp)
				hints.ai_socktype = SOCK_DGRAM;
			else
				hints.ai_socktype = SOCK_STREAM;
			host3res = NULL;
			error_num = getaddrinfo(host3, NULL, &hints, &host3res);
			if (error_num == 0) {
				nameinfo_flags = NI_NUMERICHOST;
				error_num = getnameinfo(host3res->ai_addr,
							host3res->ai_addrlen,
							host3addr, ADDRSTRLEN,
							NULL, 0,
							nameinfo_flags);
			}
			if (host3res) {
				freeaddrinfo(host3res);
				host3res = NULL;
			}
			if (error_num == 0) {
				strncpy(host3buf, host3, HOSTNAMELEN);
				strcat(host3buf, "==");
				strncat(host3buf, host3addr, ADDRSTRLEN);
				if (cp1) {
					strcat(host3buf, "+");
					strncat(host3buf, cp1 + 1, ADDRSTRLEN);
				}
				host3 = host3buf;
			}
		}
		if (cp1)
			*cp1 = '+';
	}

	if (!port) {
		if (af == AF_INET) {
			if ((sp = getservbyname( "nuttcp-data", "tcp" )))
				port = ntohs(sp->s_port);
			else
				port = DEFAULT_PORT;
		}
#ifdef AF_INET6
		else if (af == AF_INET6) {
			if ((sp = getservbyname( "nuttcp6-data", "tcp" )))
				port = ntohs(sp->s_port);
			else {
				if ((sp = getservbyname( "nuttcp-data", "tcp" )))
					port = ntohs(sp->s_port);
				else
					port = DEFAULT_PORT;
			}
		}
#endif
		else {
			err("unsupported AF");
		}
	}

	if (!ctlport) {
		if (af == AF_INET) {
			if ((sp = getservbyname( "nuttcp", "tcp" )))
				ctlport = ntohs(sp->s_port);
			else
				ctlport = DEFAULT_CTLPORT;
		}
#ifdef AF_INET6
		else if (af == AF_INET6) {
			if ((sp = getservbyname( "nuttcp6", "tcp" )))
				ctlport = ntohs(sp->s_port);
			else {
				if ((sp = getservbyname( "nuttcp", "tcp" )))
					ctlport = ntohs(sp->s_port);
				else
					ctlport = DEFAULT_CTLPORT;
			}
		}
#endif
		else {
			err("unsupported AF");
		}
	}

	if ((port < 1024) || ((port + nstream - 1) > 65535)) {
		fprintf(stderr, "invalid port/nstream = %d/%d\n", port, nstream);
		fflush(stderr);
		exit(1);
	}

	if ((ctlport >= port) && (ctlport <= (port + nstream - 1))) {
		fprintf(stderr, "ctlport = %d overlaps port/nstream = %d/%d\n", ctlport, port, nstream);
		fflush(stderr);
		exit(1);
	}

	if (timeout && (interval >= timeout)) {
		fprintf(stderr, "ignoring interval=%f which is greater than or equal timeout=%f\n", interval, timeout);
		fflush(stderr);
		interval = 0;
	}

	if (clientserver) {
		if (trans) {
			fprintf(stderr, "server mode only allowed for receiver\n");
			goto usage;
		}
		udp = 0;
		start_idx = 0;
		ident[0] = '\0';
		if (af == AF_INET) {
		  struct sockaddr_in peer;
		  socklen_t peerlen = sizeof(peer);
		  if (getpeername(0, (struct sockaddr *)&peer, &peerlen) == 0) {
#ifndef AF_INET6
			if (peer.sin_family == AF_INET)
#else
			if ((peer.sin_family == AF_INET) ||
			    (peer.sin_family == AF_INET6))
#endif
			{
				clientaddr = peer.sin_addr;
				inetd = 1;
				oneshot = 1;
				start_idx = 1;
			}
		  }
		}
#ifdef AF_INET6
		else if (af == AF_INET6) {
		  struct sockaddr_in6 peer;
		  socklen_t peerlen = sizeof(peer);
		  if (getpeername(0, (struct sockaddr *)&peer, &peerlen) == 0) {
			if ((peer.sin6_family == AF_INET) ||
			    (peer.sin6_family == AF_INET6)) {
				clientaddr6 = peer.sin6_addr;
				clientscope6 = peer.sin6_scope_id;
				inetd = 1;
				oneshot = 1;
				start_idx = 1;
			}
		  }
		}
#endif
		else {
			err("unsupported AF");
		}
	}

	if (clientserver && !inetd && !oneshot && !sinkmode) {
		fprintf(stderr, "option \"-s\" invalid with \"-S\" server mode\n");
		fprintf(stderr, "option \"-s\" can be used with \"-1\" oneshot server mode\n");
		fflush(stderr);
		exit(1);
	}

#ifdef HAVE_SETPRIO
	if (priority) {
		if (setpriority(PRIO_PROCESS, 0, priority) != 0)
			err("couldn't change priority");
	}
#endif

#ifdef HAVE_SETAFFINITY
	if ((affinity >= 0) && !host3) {
		if ((ncores = sysconf(_SC_NPROCESSORS_CONF)) <= 0)
			err("sysconf: couldn't get _SC_NPROCESSORS_CONF");
		CPU_ZERO(&cpu_set);
		CPU_SET(affinity, &cpu_set);
		if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0)
			err("couldn't change CPU affinity");
	}
#endif

	if (argc >= 1) {
		start_idx = 0;
		client = 1;
		clientserver = 1;
	}

	if (!host3 && clientserver && client && (ssm < 0)) {
		if (af == AF_INET) {
			ssm = 0;
		}
#ifdef AF_INET6
		else if (af == AF_INET6) {
#ifdef MCAST_JOIN_SOURCE_GROUP
			ssm = 1;
#else
			ssm = 0;
#endif
		}
#endif
	}

	if (irate < 0) {
		if (do_jitter)
			irate = 1;
		else
			irate = 0;
	}
	if (do_jitter && (rate == MAXRATE)) {
		fprintf(stderr, "jitter option not supported for "
				"unlimited rate\n");
		fflush(stderr);
		exit(1);
	}
	if (do_jitter && !irate) {
		fprintf(stderr, "jitter option requires"
				" \"-Ri\" instantaneous rate limit option\n");
		fflush(stderr);
		exit(1);
	}
	if (interval && !clientserver) {
		fprintf(stderr, "interval option only supported for client/server mode\n");
		fflush(stderr);
		exit(1);
	}
	if (reverse && !clientserver) {
		fprintf(stderr, "flip option only supported for client/server mode\n");
		fflush(stderr);
		exit(1);
	}
	if (reverse && udp) {
		fprintf(stderr, "flip option not supported for UDP\n");
		fflush(stderr);
		exit(1);
	}
	if (traceroute) {
		nstream = 1;
		if (!clientserver) {
			fprintf(stderr, "traceroute option only supported for client/server mode\n");
			fflush(stderr);
			exit(1);
		}
	}
	if (host3) {
		if (!clientserver) {
			fprintf(stderr, "3rd party nuttcp only supported for client/server mode\n");
			fflush(stderr);
			exit(1);
		}
	}

	if (udp && (buflen < 5)) {
	    fprintf(stderr, "UDP buflen = %d < 5, set to 5\n", buflen);
	    buflen = 5;		/* send more than the sentinel size */
	}

	if (udp && (buflen > MAXUDPBUFLEN)) {
	    fprintf(stderr, "UDP buflen = %d > MAXUDPBUFLEN, set to %d\n",
		buflen, MAXUDPBUFLEN);
	    buflen = MAXUDPBUFLEN;
	}

	if (nbuf_bytes && !host3 && !traceroute) {
		nbuf /= buflen;
	}

	if ((rate != MAXRATE) && rate_pps && !host3 && !traceroute) {
		uint64_t llrate = rate;

		llrate *= ((double)buflen * 8 / 1000);
		rate = llrate;
	}

	if (udp && interval) {
		if (buflen >= 32)
			udplossinfo = 1;
		else
			fprintf(stderr, "Unable to print interval loss information if UDP buflen < 32\n");
	}

	if (udp && (do_jitter & JITTER_IGNORE_OOO)) {
		if (buflen >= 32)
			udplossinfo = 1;
		else
			fprintf(stderr, "Unable to check out of order when calculating jitter if UDP buflen < 32\n");
	}

	if (!udp && trans) {
		if (buflen >= 32) {
			retransinfo = 1;
			b_flag = 1;
		}
		else
			fprintf(stderr, "Unable to print retransmission information if TCP buflen < 32\n");
	}

	if (udp && do_owd && (buflen < 16)) {
		fprintf(stderr, "Unable to calculate one-way delay if UDP buflen < 16\n");
	}

	ivers = vers_major*10000 + vers_minor*100 + vers_delta;

	mallocsize = buflen;
	if (mallocsize < MINMALLOC) mallocsize = MINMALLOC;
	if( (buf = (char *)malloc(mallocsize)) == (char *)NULL)
		err("malloc");

	pattern( buf, buflen );

#ifdef SIGPIPE
	signal(SIGPIPE, sigpipe);
#endif

	signal(SIGINT, sigint);

	if (clientserver && client && !thirdparty &&
	    beta && !(format & NOBETAMSG) && (do_jitter || do_owd)) {
		fprintf(stderr, "nuttcp-%d.%d.%d: ",
				vers_major, vers_minor, vers_delta);
		fprintf(stderr, "Using beta vers: %s interface/output "
				"subject to change\n", BETA_FEATURES);
		fprintf(stderr, "              (to suppress this message "
				"use \"-f-beta\")\n\n");
		fflush(stderr);
	}

doit:
	if (!udp && trans && (format & DEBUGRETRANS)) {
		sretrans = get_retrans(-1);
		fprintf(stdout, "initial system retrans = %d\n", sretrans);
	}
	nretrans[0] = 0;

	for ( stream_idx = 1; stream_idx <= nstream; stream_idx++ ) {
		fd[stream_idx] = -1;
		nretrans[stream_idx] = 0;
	}

	for ( stream_idx = start_idx; stream_idx <= nstream; stream_idx++ ) {
		if (clientserver && (stream_idx == 1)) {
			retransinfo = 0;
			if (nstream == 1) {
				send_retrans = 1;
				read_retrans = 1;
			}
			do_retrans = 0;
			got_0retrans = 0;
			if (client) {
				if (udp && !host3 && !traceroute) {
					ctlconnmss = 0;
					optlen = sizeof(ctlconnmss);
					if (getsockopt(fd[0], IPPROTO_TCP, TCP_MAXSEG,  (void *)&ctlconnmss, &optlen) < 0)
						err("get ctlconn maximum segment size didn't work");
					if (!ctlconnmss) {
						ctlconnmss = NON_JUMBO_ETHER_MSS;
						if (format & DEBUGMTU) {
							fprintf(stderr, "nuttcp%s%s: Warning: Control connection MSS reported as 0, using %d\n", trans?"-t":"-r", ident, ctlconnmss);
							fflush(stderr);
						}
					}
					else if (format & DEBUGMTU)
						fprintf(stderr, "ctlconnmss = %d\n", ctlconnmss);
					if (buflenopt) {
						if (buflen >
						    ctlconnmss +
						      TCP_UDP_HDRLEN_DELTA) {
							if (format & PARSE)
								fprintf(stderr, "nuttcp%s%s: Warning=\"IP_frags_or_no_data_reception_since_buflen=%d_>_ctlconnmss=%d\"\n", trans?"-t":"-r", ident, buflen, ctlconnmss);
							else
								fprintf(stderr, "nuttcp%s%s: Warning: IP frags or no data reception since buflen=%d > ctlconnmss=%d\n", trans?"-t":"-r", ident, buflen, ctlconnmss);
							fflush(stderr);
						}
					}
					else {
						while (buflen > ctlconnmss) {
							buflen >>= 1;
							if (nbuf_bytes)
								nbuf <<= 1;
							if ((rate != MAXRATE) &&
							    rate_pps)
								rate >>= 1;
						}
					}
					if (format & DEBUGMTU)
						fprintf(stderr, "buflen = %d\n", buflen);
				}
				if (!(ctlconn = fdopen(fd[0], "w")))
					err("fdopen: ctlconn for writing");
				if (!sinkmode) {
					if (trans)
						savestdin=dup(0);
					else {
						savestdout=dup(1);
						close(1);
						dup(2);
					}
				}
				close(0);
				dup(fd[0]);
				if (srvr_helo) {
					fprintf(ctlconn,
						HELO_FMT, vers_major,
						vers_minor, vers_delta);
					fflush(ctlconn);
					if (!fgets(buf, mallocsize, stdin)) {
						if ((errno == ECONNRESET) &&
						    (num_connect_tries <
							  MAX_CONNECT_TRIES) &&
						    retry_server) {
							/* retry control
							 * connection to server
							 * for certain possibly
							 * transient errors */
							fclose(ctlconn);
							goto doit;
						}
						mes("error from server");
						fprintf(stderr, "server aborted connection\n");
						fflush(stderr);
						exit(1);
					}
					if (sscanf(buf, HELO_FMT,
						   &rvers_major,
						   &rvers_minor,
						   &rvers_delta) < 3) {
						rvers_major = 0;
						rvers_minor = 0;
						rvers_delta = 0;
						srvr_helo = 0;
						while (fgets(buf, mallocsize,
							     stdin)) {
							if (strncmp(buf, "KO", 2) == 0)
								break;
						}
						fclose(ctlconn);
						goto doit;
					}
					irvers = rvers_major*10000
							+ rvers_minor*100
							+ rvers_delta;
				}
				if (host3 && nbuf_bytes && (irvers < 50501))
					nbuf /= buflen;
				if (host3 && (rate != MAXRATE) && rate_pps &&
					     (irvers < 50501)) {
					uint64_t llrate = rate;

					llrate *= ((double)buflen * 8 / 1000);
					rate = llrate;
				}
				if (host3 && !buflenopt && (irvers >= 50302))
					buflen = 0;
				fprintf(ctlconn, "buflen = %d, nbuf = %llu, win = %d, nstream = %d, rate = %lu, port = %hu, trans = %d, braindead = %d", buflen, nbuf, srvrwin, nstream, rate, port, trans, braindead);
				if (irvers >= 30200)
					fprintf(ctlconn, ", timeout = %f", timeout);
				else {
					timeout_sec = timeout;
					if (itimer.it_value.tv_usec)
						timeout_sec++;
					fprintf(ctlconn, ", timeout = %ld", timeout_sec);
					if (!trans && itimer.it_value.tv_usec &&
					    (brief <= 0)) {
						fprintf(stdout, "nuttcp-r%s: transmit timeout value rounded up to %ld second%s for old server\n",
							ident, timeout_sec,
							(timeout_sec == 1)?"":"s");
					}
				}
				fprintf(ctlconn, ", udp = %d, vers = %d.%d.%d", udp, vers_major, vers_minor, vers_delta);
				if (irvers >= 30302)
					fprintf(ctlconn, ", interval = %f", interval);
				else {
					if (interval) {
						fprintf(stdout, "nuttcp%s%s: interval option not supported by server version %d.%d.%d, need >= 3.3.2\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						interval = 0.0;
						abortconn = 1;
					}
				}
				if (irvers >= 30401)
					fprintf(ctlconn, ", reverse = %d", reverse);
				else {
					if (reverse) {
						fprintf(stdout, "nuttcp%s%s: flip option not supported by server version %d.%d.%d, need >= 3.4.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						reverse = 0;
						abortconn = 1;
					}
				}
				if (irvers >= 30501)
					fprintf(ctlconn, ", format = %d", format);
				else {
					if (format) {
						fprintf(stdout, "nuttcp%s%s: format option not supported by server version %d.%d.%d, need >= 3.5.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						format = 0;
					}
				}
				if (irvers >= 30601) {
					fprintf(ctlconn, ", traceroute = %d", traceroute);
					if (traceroute)
						skip_data = 1;
					fprintf(ctlconn, ", irate = %d", irate);
				}
				else {
					if (traceroute) {
						fprintf(stdout, "nuttcp%s%s: traceroute option not supported by server version %d.%d.%d, need >= 3.6.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						traceroute = 0;
						abortconn = 1;
					}
					if (irate && !trans) {
						fprintf(stdout, "nuttcp%s%s: instantaneous rate option not supported by server version %d.%d.%d, need >= 3.6.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						irate = 0;
					}
				}
				if (srvrwin && udp && (irvers < 30602)) {
					fprintf(stdout, "nuttcp%s%s: server version %d.%d.%d ignores UDP window parameter, need >= 3.6.2\n",
						trans?"-t":"-r",
						ident, rvers_major,
						rvers_minor,
						rvers_delta);
					fflush(stdout);
				}
				if ((irvers < 40101) && (format & PARSE)) {
					fprintf(stdout, "nuttcp%s%s: \"-fparse\" option not supported by server version %d.%d.%d, need >= 4.1.1\n",
						trans?"-t":"-r",
						ident, rvers_major,
						rvers_minor,
						rvers_delta);
					fflush(stdout);
					format &= ~PARSE;
					abortconn = 1;
				}
				if (irvers >= 50001) {
					cp1 = NULL;
					if (host3 && (irvers < 70101) &&
					    (cp1 = strchr(host3, '=')))
						*cp1 = '\0';
					fprintf(ctlconn, ", thirdparty = %.*s", HOST3BUFLEN, host3 ? host3 : "_NULL_");
					if (host3) {
						skip_data = 1;
						fprintf(ctlconn, " , brief3 = %d", brief);
					}
					if (cp1)
						*cp1 = '=';
				}
				else {
					if (host3) {
						fprintf(stdout, "nuttcp%s%s: 3rd party nuttcp not supported by server version %d.%d.%d, need >= 5.0.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						host3 = NULL;
						abortconn = 1;
					}
				}
				if (host3 && !abortconn) {
					if (irvers >= 60205) {
						fprintf(ctlconn,
							" , ctlport3 = %hu",
							ctlport3);
					}
					else {
						if (ctlport3) {
							fprintf(stdout, "nuttcp%s%s: ctlport3 option not supported by server version %d.%d.%d, need >= 6.2.5\n",
								trans?"-t":"-r",
								ident,
								rvers_major,
								rvers_minor,
								rvers_delta);
							fflush(stdout);
							ctlport3 = 0;
							abortconn = 1;
						}
					}
				}
				if (irvers >= 50101) {
					fprintf(ctlconn, " , multicast = %d", multicast);
				}
				else {
					if (multicast) {
						fprintf(stdout, "nuttcp%s%s: multicast not supported by server version %d.%d.%d, need >= 5.1.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						multicast = 0;
						abortconn = 1;
					}
				}
				if (irvers >= 60201) {
					fprintf(ctlconn, " , ssm = %d", ssm);
				}
				else {
					if (multicast && (ssm == 1)) {
						fprintf(stdout, "nuttcp%s%s: ssm not supported by server version %d.%d.%d, need >= 6.2.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						ssm = 0;
						abortconn = 1;
					}
				}
				if (irvers >= 50201) {
					fprintf(ctlconn, " , datamss = %d", datamss);
				}
				else {
					if (datamss && !trans) {
						fprintf(stdout, "nuttcp%s%s: mss option not supported by server version %d.%d.%d, need >= 5.2.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						datamss = 0;
						abortconn = 1;
					}
				}
				if (irvers >= 50301) {
					fprintf(ctlconn, " , tos = %X", tos);
				}
				else {
					if (tos && !trans) {
						fprintf(stdout, "nuttcp%s%s: tos option not supported by server version %d.%d.%d, need >= 5.3.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						tos = 0;
						abortconn = 1;
					}
				}
				if (irvers >= 50501) {
					fprintf(ctlconn, " , nbuf_bytes = %d", nbuf_bytes);
					fprintf(ctlconn, " , rate_pps = %d", rate_pps);
					fprintf(ctlconn, " , nodelay = %d", nodelay);
				}
				else {
					if (host3 && udp && nbuf_bytes) {
						fprintf(stdout, "nuttcp%s%s: Warning: \"-n\" option in bytes for third party not supported\n",
							trans?"-t":"-r", ident);
						fprintf(stdout, "          Warning: by server version %d.%d.%d, need >= 5.5.1\n",
							rvers_major,
							rvers_minor,
							rvers_delta);
						fprintf(stdout, "          Warning: third party request may not transfer\n");
						fprintf(stdout, "          Warning: desired number of bytes in some UDP cases\n");
						fflush(stdout);
						nbuf_bytes = 0;
					}
					if (host3 && udp && rate_pps) {
						fprintf(stdout, "nuttcp%s%s: Warning: \"-R\" option in pps for third party not supported\n",
							trans?"-t":"-r", ident);
						fprintf(stdout, "          Warning: by server version %d.%d.%d, need >= 5.5.1\n",
							rvers_major,
							rvers_minor,
							rvers_delta);
						fprintf(stdout, "          Warning: third party request may not produce\n");
						fprintf(stdout, "          Warning: desired pps rate in some UDP cases\n");
						fflush(stdout);
						rate_pps = 0;
					}
					if (nodelay && !trans) {
						fprintf(stdout, "nuttcp%s%s: TCP_NODELAY opt not supported by server version %d.%d.%d, need >= 5.5.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						nodelay = 0;
						abortconn = 1;
					}
				}
				if (irvers >= 60206) {
					if (host3) {
						fprintf(ctlconn,
							" , affinity = %d",
							affinity);
						fprintf(ctlconn,
							" , srvr_affinity = %d",
							srvr_affinity);
					}
					else {
						fprintf(ctlconn,
							" , affinity = %d",
							srvr_affinity);
					}
				}
				else {
					if (srvr_affinity >= 0) {
						fprintf(stdout, "nuttcp%s%s: affinity option not supported by server version %d.%d.%d, need >= 6.2.6\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						srvr_affinity = -1;
						abortconn = 1;
					}
				}
				if (irvers >= 60207) {
					fprintf(ctlconn, " , maxburst = %d",
						maxburst);
				}
				else {
					if ((maxburst > 1) &&
					    (!trans || host3)) {
						fprintf(stdout, "nuttcp%s%s: packet burst not supported by server version %d.%d.%d, need >= 6.2.7\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						maxburst = 1;
						abortconn = 1;
					}
				}
				if (irvers >= 70001) {
					fprintf(ctlconn, " , do_jitter = %d", do_jitter);
				}
				else {
					if (do_jitter && trans) {
						fprintf(stdout, "nuttcp%s%s: jitter not supported by server version %d.%d.%d, need >= 7.0.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						do_jitter = 0;
						abortconn = 1;
					}
					if ((do_jitter & JITTER_IGNORE_OOO) &&
					    !trans && !(udp && interval)) {
						udplossinfo = 0;
						fprintf(stdout, "nuttcp%s%s: Unable to check out of order when calculating jitter\n",
							trans?"-t":"-r", ident);
						fprintf(stdout, "          due to using older server version %d.%d.%d, need >= 7.0.1\n",
							rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
					}
				}
				if (irvers >= 70001) {
					fprintf(ctlconn, " , do_owd = %d", do_owd);
				}
				else {
					if (do_owd) {
						fprintf(stdout, "nuttcp%s%s: owd not supported by server version %d.%d.%d, need >= 7.0.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						do_owd = 0;
						abortconn = 1;
					}
				}
				if (irvers >= 70101) {
					fprintf(ctlconn, " , stride = %d", ipad_stride.ip32);
				}
				else {
					if (ipad_stride.ip32) {
						fprintf(stdout, "nuttcp%s%s: stride not supported by server version %d.%d.%d, need >= 7.1.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						ipad_stride.ip32 = 0;
						abortconn = 1;
					}
				}
				if (irvers >= 70101) {
					fprintf(ctlconn, " , multilink = %d", multilink);
				}
				else {
					if (multilink) {
						fprintf(stdout, "nuttcp%s%s: multilink not supported by server version %d.%d.%d, need >= 7.1.1\n",
							trans?"-t":"-r",
							ident, rvers_major,
							rvers_minor,
							rvers_delta);
						fflush(stdout);
						multilink = 0;
						abortconn = 1;
					}
				}
				fprintf(ctlconn, "\n");
				fflush(ctlconn);
				if (abortconn) {
					brief = 1;
					if ((!trans && !reverse) ||
					    (trans && reverse))
						skip_data = 1;
				}
				if (!fgets(buf, mallocsize, stdin)) {
					mes("error from server");
					fprintf(stderr, "server aborted connection\n");
					fflush(stderr);
					exit(1);
				}
				if (irvers < 30403)
					udplossinfo = 0;
				if (irvers >= 50401) {
					two_bod = 1;
					handle_urg = 1;
				}
				if (udp || (buflen < 32) || (irvers < 60001)) {
					if (trans)
						send_retrans = 0;
					else
						read_retrans = 0;
				}
				if (strncmp(buf, "OK", 2) != 0) {
					mes("error from server");
					fprintf(stderr, "server ");
					while (fgets(buf, mallocsize, stdin)) {
						if (strncmp(buf, "KO", 2) == 0)
							break;
						fputs(buf, stderr);
					}
					fflush(stderr);
					exit(1);
				}
				if (sscanf(buf, "OK v%d.%d.%d\n", &rvers_major, &rvers_minor, &rvers_delta) < 3) {
					rvers_major = 0;
					rvers_minor = 0;
					rvers_delta = 0;
				}
				irvers = rvers_major*10000
						+ rvers_minor*100
						+ rvers_delta;
				usleep(10000);
			}
			else {
				if (inetd) {
					ctlconn = stdin;
				}
				else {
					if (!(ctlconn = fdopen(fd[0], "r")))
						err("fdopen: ctlconn for reading");
				}
				fflush(stdout);
				if (!inetd) {
					/* manually started server */
					/* send stdout to client   */
					savestdout=dup(1);
					close(1);
					dup(fd[0]);
					if (!nofork) {
					    /* send stderr to client */
					    close(2);
					    dup(1);
					}
				}
				fgets(buf, mallocsize, ctlconn);
				if (sscanf(buf, HELO_FMT, &rvers_major,
					   &rvers_minor, &rvers_delta) == 3) {
					fprintf(stdout, HELO_FMT, vers_major,
						vers_minor, vers_delta);
					fflush(stdout);
					fgets(buf, mallocsize, ctlconn);
				}
				irvers = rvers_major*10000
						+ rvers_minor*100
						+ rvers_delta;
				if (sscanf(buf, "buflen = %d, nbuf = %llu, win = %d, nstream = %d, rate = %lu, port = %hu, trans = %d, braindead = %d, timeout = %lf, udp = %d, vers = %d.%d.%d", &nbuflen, &nbuf, &sendwin, &nstream, &rate, &port, &trans, &braindead, &timeout, &udp, &rvers_major, &rvers_minor, &rvers_delta) < 13) {
					trans = !trans;
					fputs("KO\n", stdout);
					mes("error scanning parameters");
					fprintf(stdout, "may be using older client version than server\n");
					fputs(buf, stdout);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				irvers = rvers_major*10000
						+ rvers_minor*100
						+ rvers_delta;
				if (irvers >= 30302)
					sscanf(strstr(buf, ", interval =") + 13,
						"%lf", &interval);
				else
					interval = 0.0;
				if (irvers >= 30401)
					sscanf(strstr(buf, ", reverse =") + 12,
						"%d", &reverse);
				else
					reverse = 0;
				if (irvers >= 30501)
					sscanf(strstr(buf, ", format =") + 11,
						"%d", &format);
				else
					format = 0;
				if (irvers >= 30601) {
					sscanf(strstr(buf, ", traceroute =") + 15,
						"%d", &traceroute);
					if (traceroute) {
						skip_data = 1;
						brief = 1;
					}
					sscanf(strstr(buf, ", irate =") + 10,
						"%d", &irate);
				}
				else {
					traceroute = 0;
					irate = 0;
				}
				if (irvers >= 50001) {
					sprintf(fmt, "%%%ds", HOST3BUFLEN);
					sscanf(strstr(buf, ", thirdparty =") + 15,
						fmt, host3buf);
					host3buf[HOST3BUFLEN] = '\0';
					if (strcmp(host3buf, "_NULL_") == 0)
						host3 = NULL;
					else
						host3 = host3buf;
					if (host3) {
						if (no3rd) {
							fputs("KO\n", stdout);
							fprintf(stdout, "doesn't allow 3rd party nuttcp\n");
							fputs("KO\n", stdout);
							goto cleanup;
						}
						cp1 = host3;
						while (*cp1) {
							if (!isalnum((int)(*cp1))
							     && (*cp1 != '-')
							     && (*cp1 != '.')
							     && (*cp1 != ':')
							     && (*cp1 != '/')
							     && (*cp1 != '+')
							     && (*cp1 != '=')) {
								fputs("KO\n", stdout);
								mes("invalid 3rd party host");
								fprintf(stdout, "3rd party host = '%s'\n", host3);
								fputs("KO\n", stdout);
								goto cleanup;
							}
							cp1++;
						}
						skip_data = 1;
						brief = 1;
						sscanf(strstr(buf, ", brief3 =") + 11,
							"%d", &brief3);
					}
				}
				else {
					host3 = NULL;
				}
				if (host3 && (irvers >= 60205)) {
					sscanf(strstr(buf, ", ctlport3 =") + 13,
						"%hu", &ctlport3);
				}
				else {
					ctlport3 = 0;
				}
				if (irvers >= 50101) {
					sscanf(strstr(buf, ", multicast =") + 14,
						"%d", &mc_param);
				}
				else {
					mc_param = 0;
				}
				if (irvers >= 60201) {
					sscanf(strstr(buf, ", ssm =") + 7,
						"%d", &ssm);
				}
				else {
					ssm = 0;
				}
#ifndef MCAST_JOIN_SOURCE_GROUP
				if (mc_param && (ssm == 1)) {
					fputs("KO\n", stdout);
					fprintf(stdout, "server does not support ssm\n");
					fputs("KO\n", stdout);
					goto cleanup;
				}
#endif
				if (irvers >= 50201) {
					sscanf(strstr(buf, ", datamss =") + 12,
						"%d", &datamss);
				}
				else {
					datamss = 0;
				}
				if (irvers >= 50301) {
					sscanf(strstr(buf, ", tos =") + 8,
						"%X", &tos);
				}
				else {
					tos = 0;
				}
				if (irvers >= 50501) {
					sscanf(strstr(buf, ", nbuf_bytes =")
							+ 15,
						"%d", &nbuf_bytes);
					sscanf(strstr(buf, ", rate_pps =") + 13,
						"%d", &rate_pps);
					sscanf(strstr(buf, ", nodelay =") + 12,
						"%d", &nodelay);
				}
				else {
					nbuf_bytes = 0;
					rate_pps = 0;
					nodelay = 0;
				}
				if (irvers >= 60206) {
					if (host3) {
						sscanf(strstr(buf,
							   ", affinity =") + 13,
						       "%X", &affinity);
						sscanf(strstr(buf,
							   ", srvr_affinity =")
								+ 18,
						       "%X", &srvr_affinity);
					}
					else {
						sscanf(strstr(buf,
							   ", affinity =") + 13,
						       "%X", &srvr_affinity);
					}
				}
				else {
					srvr_affinity = -1;
				}
				if (irvers >= 60207) {
					sscanf(strstr(buf, ", maxburst =") + 13,
					       "%d", &maxburst);
				}
				else {
					maxburst = 1;
				}
				if (irvers >= 70001) {
					sscanf(strstr(buf, ", do_jitter =") + 14,
					       "%d", &do_jitter);
				}
				else {
					do_jitter = 0;
				}
				if (irvers >= 70001) {
					sscanf(strstr(buf, ", do_owd =") + 11,
					       "%d", &do_owd);
				}
				else {
					do_owd = 0;
				}
				if (irvers >= 70101) {
					sscanf(strstr(buf, ", stride =") + 11,
					       "%d", &ipad_stride.ip32);
				}
				else {
					ipad_stride.ip32 = 0;
				}
				if (irvers >= 70101) {
					sscanf(strstr(buf,
						      ", multilink =") + 14,
						      "%d", &multilink);
				}
				else {
					multilink = 0;
				}
				trans = !trans;
				if (inetd && !sinkmode) {
					fputs("KO\n", stdout);
					mes("option \"-s\" invalid with inetd server");
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (!traceroute && !host3 &&
				    (nbuflen != buflen)) {
					if (nbuflen < 1) {
						fputs("KO\n", stdout);
						mes("invalid buflen");
						fprintf(stdout, "buflen = %d\n", nbuflen);
						fputs("KO\n", stdout);
						goto cleanup;
					}
					free(buf);
					mallocsize = nbuflen;
					if (mallocsize < MINMALLOC) mallocsize = MINMALLOC;
					if( (buf = (char *)malloc(mallocsize)) == (char *)NULL)
						err("malloc");
					pattern( buf, nbuflen );
				}
				buflen = nbuflen;
				if (nbuf < 1) {
					fputs("KO\n", stdout);
					mes("invalid nbuf");
					fprintf(stdout, "nbuf = %llu\n", nbuf);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				rcvwin = sendwin;
				if (sendwin < 0) {
					fputs("KO\n", stdout);
					mes("invalid win");
					fprintf(stdout, "win = %d\n", sendwin);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if ((nstream < 1) || (nstream > MAXSTREAM)) {
					fputs("KO\n", stdout);
					mes("invalid nstream");
					fprintf(stdout, "nstream = %d\n", nstream);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (nstream > 1) {
					b_flag = 1;
					send_retrans = 0;
					read_retrans = 0;
				}
				if (rate == 0)
					rate = MAXRATE;
				if (timeout < 0) {
					fputs("KO\n", stdout);
					mes("invalid timeout");
					fprintf(stdout, "timeout = %f\n", timeout);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				itimer.it_value.tv_sec = timeout;
				itimer.it_value.tv_usec =
					(timeout - itimer.it_value.tv_sec)
						*1000000;
				if ((port < 1024) || ((port + nstream - 1) > 65535)) {
					fputs("KO\n", stdout);
					mes("invalid port/nstream");
					fprintf(stdout, "port/nstream = %hu/%d\n", port, nstream);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if ((ctlport >= port) && (ctlport <= (port + nstream - 1))) {
					fputs("KO\n", stdout);
					mes("ctlport overlaps port/nstream");
					fprintf(stdout, "ctlport = %hu, port/nstream = %hu/%d\n", ctlport, port, nstream);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (host3 && ctlport3 && (ctlport3 < 1024)) {
					fputs("KO\n", stdout);
					mes("invalid ctlport3");
					fprintf(stdout, "ctlport3 = %hu\n",
						ctlport3);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (interval < 0) {
					fputs("KO\n", stdout);
					mes("invalid interval");
					fprintf(stdout, "interval = %f\n", interval);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (mc_param) {
					if ((mc_param < 1) ||
					    (mc_param > 255)) {
						fputs("KO\n", stdout);
						mes("invalid multicast ttl");
						fprintf(stdout, "multicast ttl = %d\n", mc_param);
						fputs("KO\n", stdout);
						goto cleanup;
					}
					udp = 1;
					nstream = 1;
					if (rate == MAXRATE)
						rate = DEFAULT_UDP_RATE;
				}
				multicast = mc_param;
				if ((!host3 && ((ssm < 0) || (ssm > 1))) ||
				    (host3 && ((ssm < -1) || (ssm > 1)))) {
					fputs("KO\n", stdout);
					mes("invalid ssm value");
					fprintf(stdout, "ssm = %d\n", ssm);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (datamss < 0) {
					fputs("KO\n", stdout);
					mes("invalid datamss");
					fprintf(stdout, "datamss = %d\n", datamss);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (tos > 255) {
					fputs("KO\n", stdout);
					mes("invalid tos");
					fprintf(stdout, "tos = %d\n", tos);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (nbuf_bytes < 0) {
					fputs("KO\n", stdout);
					mes("invalid nbuf_bytes");
					fprintf(stdout, "nbuf_bytes = %d\n",
						nbuf_bytes);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (rate_pps < 0) {
					fputs("KO\n", stdout);
					mes("invalid rate_pps");
					fprintf(stdout, "rate_pps = %d\n",
						rate_pps);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (nodelay < 0) {
					fputs("KO\n", stdout);
					mes("invalid nodelay");
					fprintf(stdout, "nodelay = %d\n",
						nodelay);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if ((srvr_affinity >= 0) && !host3) {
#ifdef HAVE_SETAFFINITY
					CPU_ZERO(&cpu_set);
					CPU_SET(srvr_affinity, &cpu_set);
					if (sched_setaffinity(0,
							      sizeof(cpu_set_t),
							      &cpu_set) != 0) {
						fputs("KO\n", stdout);
						mes("couldn't change server "
						    "CPU affinity");
						fprintf(stdout,
							"srvr_affinity = %d\n",
							srvr_affinity);
						fputs("KO\n", stdout);
						goto cleanup;
					}
#else
					fputs("KO\n", stdout);
					mes("server doesn't support setting "
					    "CPU affinity");
					fprintf(stdout,
						"srvr_affinity = %d\n",
						srvr_affinity);
					fputs("KO\n", stdout);
					goto cleanup;
#endif
				}
				if (maxburst < 1) {
					fputs("KO\n", stdout);
					mes("invalid maxburst");
					fprintf(stdout, "maxburst = %d\n",
						maxburst);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (do_jitter < 0) {
					fputs("KO\n", stdout);
					mes("invalid do_jitter");
					fprintf(stdout, "do_jitter = %d\n",
						do_jitter);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (do_jitter && !udp) {
					fputs("KO\n", stdout);
					fprintf(stdout,
						"jitter option"
						" not supported for TCP\n");
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (do_jitter && (!rate || (rate == MAXRATE))) {
					fputs("KO\n", stdout);
					fprintf(stdout,
						"jitter option not supported"
						" for unlimited rate\n");
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (do_jitter && !irate) {
					fputs("KO\n", stdout);
					fprintf(stdout,
						"jitter option requires"
						" instantaneous rate limit\n");
					fputs("KO\n", stdout);
					goto cleanup;
				}
				if (do_owd < 0) {
					fputs("KO\n", stdout);
					mes("invalid do_owd");
					fprintf(stdout, "do_owd = %d\n",
						do_owd);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				/* used to send server "OK" here -
				 * now delay sending of server OK until
				 * after successful server bind() -
				 * catches data port collision */
				if (udp && (interval ||
					    (do_jitter & JITTER_IGNORE_OOO)) &&
				    (buflen >= 32) && (irvers >= 30403))
					udplossinfo = 1;
				if (irvers >= 50401) {
					two_bod = 1;
					handle_urg = 1;
				}
				if (udp || (buflen < 32) || (irvers < 60001)) {
					if (trans)
						send_retrans = 0;
					else
						read_retrans = 0;
				}
			}
		}

		if (clientserver && client && (stream_idx == 1)) {
			reading_srvr_info = 1;
			pollfds[0].fd = fileno(ctlconn);
			pollfds[0].events = POLLIN | POLLPRI;
			pollfds[0].revents = 0;
			flags = fcntl(0, F_GETFL, 0);
			if (flags < 0)
				err("fcntl 1");
			flags |= O_NONBLOCK;
			if (fcntl(0, F_SETFL, flags) < 0)
				err("fcntl 2");
			itimer.it_value.tv_sec = SRVR_INFO_TIMEOUT;
			itimer.it_value.tv_usec = 0;
			itimer.it_interval.tv_sec = 0;
			itimer.it_interval.tv_usec = 0;
			setitimer(ITIMER_REAL, &itimer, 0);
		}
		if (clientserver && client && (stream_idx == 1) &&
		    ((pollst = poll(pollfds, 1, 0)) > 0) &&
		    (pollfds[0].revents & (POLLIN | POLLPRI)) && !got_done) {
			/* check for server output (mainly for server error) */
			while (fgets(intervalbuf, sizeof(intervalbuf), stdin)) {
				setitimer(ITIMER_REAL, &itimer, 0);
				if (strncmp(intervalbuf, "DONE", 4) == 0) {
					if (format & DEBUGPOLL) {
						fprintf(stdout, "got DONE\n");
						fflush(stdout);
					}
					got_done = 1;
					intr = 1;
					break;
				}
				else if (strncmp(intervalbuf,
						 "nuttcp-", 7) == 0) {
					if ((brief <= 0) ||
					    strstr(intervalbuf, "Warning") ||
					    strstr(intervalbuf, "Error") ||
					    strstr(intervalbuf, "Debug")) {
						if (*ident) {
							fputs("nuttcp", stdout);
							fputs(trans ?
								"-r" : "-t",
							      stdout);
							fputs(ident, stdout);
							fputs(intervalbuf + 8,
							      stdout);
						}
						else
							fputs(intervalbuf,
							      stdout);
						fflush(stdout);
					}
					if (strstr(intervalbuf, "Error"))
						exit(1);
				}
				else {
					if (*ident)
						fprintf(stdout, "%s: ",
							ident + 1);
					fputs(intervalbuf, stdout);
					fflush(stdout);
				}
			}
		}
		if (clientserver && client && (stream_idx == 1)) {
			reading_srvr_info = 0;
			flags = fcntl(0, F_GETFL, 0);
			if (flags < 0)
				err("fcntl 1");
			flags &= ~O_NONBLOCK;
			if (fcntl(0, F_SETFL, flags) < 0)
				err("fcntl 2");
			itimer.it_value.tv_sec = 0;
			itimer.it_value.tv_usec = 0;
			setitimer(ITIMER_REAL, &itimer, 0);
		}

		if (!client) {
		    if (af == AF_INET) {
			inet_ntop(af, &clientaddr.s_addr, hostbuf, sizeof(hostbuf));
		    }
#ifdef AF_INET6
		    else if (af == AF_INET6) {
			inet_ntop(af, clientaddr6.s6_addr, hostbuf, sizeof(hostbuf));
		    }
#endif
		    host = hostbuf;
		}

		if ((stream_idx > 0) && skip_data) {
			if (clientserver && !client && (stream_idx == 1)) {
				/* send server "OK" message */
				fprintf(stdout, "OK v%d.%d.%d\n", vers_major,
						vers_minor, vers_delta);
				fflush(stdout);
			}
			break;
		}

		bzero((char *)&sinme[stream_idx], sizeof(sinme[stream_idx]));
		bzero((char *)&sinhim[stream_idx], sizeof(sinhim[stream_idx]));

#ifdef AF_INET6
		bzero((char *)&sinme6[stream_idx], sizeof(sinme6[stream_idx]));
		bzero((char *)&sinhim6[stream_idx], sizeof(sinhim6[stream_idx]));
#endif

		if (((trans && !reverse) && (stream_idx > 0)) ||
		    ((!trans && reverse) && (stream_idx > 0)) ||
		    (client && (stream_idx == 0))) {
			/* xmitr initiates connections (unless reversed) */
			if (client) {
				if (af == AF_INET) {
				    sinhim[stream_idx].sin_family = af;
				    if (ipad_stride.ip32 && (stream_idx > 1)) {
					sinhim[stream_idx].sin_addr.s_addr =
					  sinhim[stream_idx - 1].sin_addr.s_addr
						+ ipad_stride.ip32;
				    }
				    else {
					bcopy((char *)&(((struct sockaddr_in *)res[stream_idx]->ai_addr)->sin_addr),
					      (char *)&sinhim[stream_idx].sin_addr.s_addr,
					      sizeof(sinhim[stream_idx].sin_addr.s_addr));
				    }
				}
#ifdef AF_INET6
				else if (af == AF_INET6) {
				    sinhim6[stream_idx].sin6_family = af;
				    bcopy((char *)&(((struct sockaddr_in6 *)res[stream_idx]->ai_addr)->sin6_addr),
					  (char *)&sinhim6[stream_idx].sin6_addr.s6_addr,
					  sizeof(sinhim6[stream_idx].sin6_addr.s6_addr));
				    sinhim6[stream_idx].sin6_scope_id = ((struct sockaddr_in6 *)res[stream_idx]->ai_addr)->sin6_scope_id;
				}
#endif
				else {
					err("unsupported AF");
				}
			} else {
				sinhim[stream_idx].sin_family = af;
				if (ipad_stride.ip32 && (stream_idx > 1)) {
					sinhim[stream_idx].sin_addr.s_addr =
					  sinhim[stream_idx - 1].sin_addr.s_addr
						+ ipad_stride.ip32;
				}
				else {
					sinhim[stream_idx].sin_addr =
						clientaddr;
				}
#ifdef AF_INET6
				sinhim6[stream_idx].sin6_family = af;
				sinhim6[stream_idx].sin6_addr = clientaddr6;
				sinhim6[stream_idx].sin6_scope_id = clientscope6;
#endif
			}
			if (stream_idx == 0) {
				sinhim[stream_idx].sin_port = htons(ctlport);
#ifdef AF_INET6
				sinhim6[stream_idx].sin6_port = htons(ctlport);
#endif
			} else {
				sinhim[stream_idx].sin_port = htons(port + stream_idx - 1);
#ifdef AF_INET6
				sinhim6[stream_idx].sin6_port = htons(port + stream_idx - 1);
#endif
			}
			sinme[stream_idx].sin_port = 0;		/* free choice */
#ifdef AF_INET6
			sinme6[stream_idx].sin6_port = 0;	/* free choice */
#endif
		} else {
			/* rcvr listens for connections (unless reversed) */
			if (stream_idx == 0) {
				sinme[stream_idx].sin_port =   htons(ctlport);
#ifdef AF_INET6
				sinme6[stream_idx].sin6_port = htons(ctlport);
#endif
			} else {
				sinme[stream_idx].sin_port =   htons(port + stream_idx - 1);
#ifdef AF_INET6
				sinme6[stream_idx].sin6_port = htons(port + stream_idx - 1);
#endif
			}
		}
		sinme[stream_idx].sin_family = af;
#ifdef AF_INET6
		sinme6[stream_idx].sin6_family = af;
#endif

		if ((fd[stream_idx] = socket(domain, (udp && (stream_idx != 0))?SOCK_DGRAM:SOCK_STREAM, 0)) < 0) {
			if (clientserver && !client && (stream_idx == 1)) {
				save_errno = errno;
				fputs("KO\n", stdout);
				mes("Error: socket() on data stream failed");
				fputs("Error: ", stdout);
				fputs(strerror(save_errno), stdout);
				fputs("\n", stdout);
				fputs("KO\n", stdout);
				goto cleanup;
			}
			err("socket");
		}

		if (setsockopt(fd[stream_idx], SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(one)) < 0) {
			if (clientserver && !client && (stream_idx == 1)) {
				save_errno = errno;
				fputs("KO\n", stdout);
				mes("Error: setsockopt()"
				    " to so_reuseaddr failed");
				fputs("Error: ", stdout);
				fputs(strerror(save_errno), stdout);
				fputs("\n", stdout);
				fputs("KO\n", stdout);
				goto cleanup;
			}
			err("setsockopt: so_reuseaddr");
		}

#ifdef IPV6_V6ONLY
		if ((af == AF_INET6) && !v4mapped) {
			if (setsockopt(fd[stream_idx], IPPROTO_IPV6, IPV6_V6ONLY, (void *)&one, sizeof(int)) < 0) {
				if (clientserver && !client &&
				    (stream_idx == 1)) {
					save_errno = errno;
					fputs("KO\n", stdout);
					mes("Error: setsockopt()"
					    " to ipv6_only failed");
					fputs("Error: ", stdout);
					fputs(strerror(save_errno), stdout);
					fputs("\n", stdout);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				err("setsockopt: ipv6_only");
			}
		}
#endif

		if (af == AF_INET) {
		    if (bind(fd[stream_idx], (struct sockaddr *)&sinme[stream_idx], sizeof(sinme[stream_idx])) < 0) {
			if (clientserver && !client && (stream_idx == 1)) {
				save_errno = errno;
				fputs("KO\n", stdout);
				mes("Error: bind() on data stream failed");
				fputs("Error: ", stdout);
				fputs(strerror(save_errno), stdout);
				fputs("\n", stdout);
				if (((!trans && !reverse)
					|| (trans && reverse)) &&
				    (errno == EADDRINUSE) &&
				    (port == IPERF_PORT))
					fputs("Info: Possible collision"
					      " with iperf server\n", stdout);
				fputs("KO\n", stdout);
				goto cleanup;
			}
			if (clientserver && client && (stream_idx == 1) &&
			    ((!trans && !reverse) || (trans && reverse)) &&
			    (errno == EADDRINUSE) && (port == IPERF_PORT)) {
				errmes("bind");
				fputs("Info: Possible collision"
				      " with iperf server\n", stderr);
				fflush(stderr);
				exit(1);
			}
			err("bind");
		    }
		}
#ifdef AF_INET6
		else if (af == AF_INET6) {
		    if (bind(fd[stream_idx], (struct sockaddr *)&sinme6[stream_idx], sizeof(sinme6[stream_idx])) < 0) {
			if (clientserver && !client && (stream_idx == 1)) {
				save_errno = errno;
				fputs("KO\n", stdout);
				mes("Error: bind() on data stream failed");
				fputs("Error: ", stdout);
				fputs(strerror(save_errno), stdout);
				fputs("\n", stdout);
				fputs("KO\n", stdout);
				goto cleanup;
			}
			err("bind");
		    }
		}
#endif
		else {
		    if (clientserver && !client && (stream_idx == 1)) {
			save_errno = errno;
			fputs("KO\n", stdout);
			mes("Error: unsupported AF on data stream");
			fputs("Error: ", stdout);
			fputs(strerror(save_errno), stdout);
			fputs("\n", stdout);
			fputs("KO\n", stdout);
			goto cleanup;
		    }
		    err("unsupported AF");
		}

		if (clientserver && !client && (stream_idx == 1)) {
			/* finally OK to send server "OK" message */
			fprintf(stdout, "OK v%d.%d.%d\n", vers_major,
					vers_minor, vers_delta);
			fflush(stdout);
			if ((trans && !reverse) || (!trans && reverse))
				usleep(50000);
		}

		if (clientserver && (stream_idx == 1)) {
			if (!udp && trans) {
				nretrans[0] = get_retrans(fd[0]);
				if (retransinfo > 0)
					b_flag = 1;
			}
		}

		if (stream_idx == nstream) {
			if (brief <= 0)
				mes("socket");
#ifdef HAVE_SETPRIO
			if (priority && (brief <= 0)) {
				errno = 0;
				priority = getpriority(PRIO_PROCESS, 0);
				if (errno)
					mes("couldn't get priority");
				else {
					if (format & PARSE)
						fprintf(stdout,
							"nuttcp%s%s: "
							"priority=%d\n",
							trans ? "-t" : "-r",
							ident, priority);
					else
						fprintf(stdout,
							"nuttcp%s%s: "
							"priority = %d\n",
							trans ? "-t" : "-r",
							ident, priority);
				}
			}
#endif
#ifdef HAVE_SETAFFINITY
			if ((affinity >= 0) && (brief <= 0) && !host3) {
				int cpu_affinity;

				errno = 0;
				sched_getaffinity(0, sizeof(cpu_set_t),
					 &cpu_set);
				if (errno)
					mes("couldn't get affinity");
				else {
					for ( cpu_affinity = 0;
					      cpu_affinity < ncores;
					      cpu_affinity++ ) {
						if (CPU_ISSET(cpu_affinity,
							      &cpu_set))
							break;
					}
					if (format & PARSE)
						fprintf(stdout,
							"nuttcp%s%s: "
							"cpu_affinity=%d\n",
							trans ? "-t" : "-r",
							ident, cpu_affinity);
					else
						fprintf(stdout,
							"nuttcp%s%s: "
							"affinity = CPU %d\n",
							trans ? "-t" : "-r",
							ident, cpu_affinity);
				}
			}
#endif
			if (trans) {
			    if ((brief <= 0) && (format & PARSE)) {
				fprintf(stdout,"nuttcp-t%s: buflen=%d ",
					ident, buflen);
				if (nbuf != INT_MAX)
				    fprintf(stdout,"nbuf=%llu ", nbuf);
				fprintf(stdout,"nstream=%d port=%d mode=%s host=%s",
				    nstream, port,
				    udp?"udp":"tcp",
				    host);
				if (multicast)
				    fprintf(stdout, " multicast_ttl=%d",
					    multicast);
				fprintf(stdout, "\n");
				if (timeout)
				    fprintf(stdout,"nuttcp-t%s: time_limit=%.2f\n",
				    ident, timeout);
				if ((rate != MAXRATE) || tos)
				    fprintf(stdout,"nuttcp-t%s:", ident);
				if (rate != MAXRATE) {
				    fprintf(stdout," rate_limit=%.3f rate_unit=Mbps rate_mode=%s",
					(double)rate/1000,
					irate ? "instantaneous" : "aggregate");
				    if (maxburst > 1)
					fprintf(stdout," packet_burst=%d",
						maxburst);
				    if (udp) {
					unsigned long long ppsrate =
					    ((uint64_t)rate * 1000)/8/buflen;

					fprintf(stdout," pps_rate=%llu",
					    ppsrate);
				    }
				}
				if (tos)
				    fprintf(stdout," tos=0x%X", tos);
				if ((rate != MAXRATE) || tos)
				    fprintf(stdout,"\n");
			    }
			    else if (brief <= 0) {
				fprintf(stdout,"nuttcp-t%s: buflen=%d, ",
					ident, buflen);
				if (nbuf != INT_MAX)
				    fprintf(stdout,"nbuf=%llu, ", nbuf);
				fprintf(stdout,"nstream=%d, port=%d %s -> %s",
				    nstream, port,
				    udp?"udp":"tcp",
				    host);
				if (multicast)
				    fprintf(stdout, " ttl=%d", multicast);
				fprintf(stdout, "\n");
				if (timeout)
				    fprintf(stdout,"nuttcp-t%s: time limit = %.2f second%s\n",
					ident, timeout,
					(timeout == 1.0)?"":"s");
				if ((rate != MAXRATE) || tos)
				    fprintf(stdout,"nuttcp-t%s:", ident);
				if (rate != MAXRATE) {
				    fprintf(stdout," rate limit = %.3f Mbps (%s)",
					(double)rate/1000,
					irate ? "instantaneous" : "aggregate");
				    if (maxburst > 1)
					fprintf(stdout,", packet burst = %d",
						maxburst);
				    if (udp) {
					unsigned long long ppsrate =
					    ((uint64_t)rate * 1000)/8/buflen;

					fprintf(stdout,", %llu pps", ppsrate);
				    }
				    if (tos)
					fprintf(stdout,",");
				}
				if (tos)
				    fprintf(stdout," tos = 0x%X", tos);
				if ((rate != MAXRATE) || tos)
				    fprintf(stdout,"\n");
			    }
			} else {
			    if ((brief <= 0) && (format & PARSE)) {
				fprintf(stdout,"nuttcp-r%s: buflen=%d ",
					ident, buflen);
				if (nbuf != INT_MAX)
				    fprintf(stdout,"nbuf=%llu ", nbuf);
				fprintf(stdout,"nstream=%d port=%d mode=%s\n",
				    nstream, port,
				    udp?"udp":"tcp");
				if (tos)
				    fprintf(stdout,"nuttcp-r%s: tos=0x%X\n",
					ident, tos);
				if (interval)
				    fprintf(stdout,"nuttcp-r%s: reporting_interval=%.2f\n",
					ident, interval);
			    }
			    else if (brief <= 0) {
				fprintf(stdout,"nuttcp-r%s: buflen=%d, ",
					ident, buflen);
				if (nbuf != INT_MAX)
				    fprintf(stdout,"nbuf=%llu, ", nbuf);
				fprintf(stdout,"nstream=%d, port=%d %s\n",
				    nstream, port,
				    udp?"udp":"tcp");
				if (tos)
				    fprintf(stdout,"nuttcp-r%s: tos = 0x%X\n",
					ident, tos);
				if (interval)
				    fprintf(stdout,"nuttcp-r%s: interval reporting every %.2f second%s\n",
					ident, interval,
					(interval == 1.0)?"":"s");
			    }
			}
		}

		if (stream_idx > 0)  {
		    if (trans) {
			/* Set the transmitter options */
			if (sendwin) {
				if( setsockopt(fd[stream_idx], SOL_SOCKET, SO_SNDBUF,
					(void *)&sendwin, sizeof(sendwin)) < 0)
					errmes("unable to setsockopt SO_SNDBUF");
				if (braindead && (setsockopt(fd[stream_idx], SOL_SOCKET, SO_RCVBUF,
					(void *)&rcvwin, sizeof(rcvwin)) < 0))
					errmes("unable to setsockopt SO_RCVBUF");
			}
			if (tos) {
				if( setsockopt(fd[stream_idx], IPPROTO_IP, IP_TOS,
					(void *)&tos, sizeof(tos)) < 0)
					err("setsockopt");
			}
			if (nodelay && !udp) {
				struct protoent *p;
				p = getprotobyname("tcp");
				if( p && setsockopt(fd[stream_idx], p->p_proto, TCP_NODELAY,
				    (void *)&one, sizeof(one)) < 0)
					err("setsockopt: nodelay");
				if ((stream_idx == nstream) && (brief <= 0))
					mes("nodelay");
			}
		    } else {
			/* Set the receiver options */
			if (rcvwin) {
				if( setsockopt(fd[stream_idx], SOL_SOCKET, SO_RCVBUF,
					(void *)&rcvwin, sizeof(rcvwin)) < 0)
					errmes("unable to setsockopt SO_RCVBUF");
				if (braindead && (setsockopt(fd[stream_idx], SOL_SOCKET, SO_SNDBUF,
					(void *)&sendwin, sizeof(sendwin)) < 0))
					errmes("unable to setsockopt SO_SNDBUF");
			}
			if (tos) {
				if( setsockopt(fd[stream_idx], IPPROTO_IP, IP_TOS,
					(void *)&tos, sizeof(tos)) < 0)
					err("setsockopt");
			}
		    }
		}
		if (!udp || (stream_idx == 0))  {
		    if (((trans && !reverse) && (stream_idx > 0)) ||
			((!trans && reverse) && (stream_idx > 0)) ||
			(client && (stream_idx == 0))) {
			/* The transmitter initiates the connection
			 * (unless reversed by the flip option)
			 */
			if (options && (stream_idx > 0))  {
				if( setsockopt(fd[stream_idx], SOL_SOCKET, options, (void *)&one, sizeof(one)) < 0)
					errmes("unable to setsockopt options");
			}
			usleep(20000);
			if (trans && (stream_idx > 0) && datamss) {
#if defined(__CYGWIN__) || defined(_WIN32)
				if (format & PARSE)
					fprintf(stderr, "nuttcp%s%s: Warning=\"setting_maximum_segment_size_not_supported_on_windows\"\n",
						trans?"-t":"-r", ident);
				else
					fprintf(stderr, "nuttcp%s%s: Warning: setting maximum segment size not supported on windows\n",
						trans?"-t":"-r", ident);
				fflush(stderr);
#endif
				optlen = sizeof(datamss);
				if ((sockopterr = setsockopt(fd[stream_idx], IPPROTO_TCP, TCP_MAXSEG,  (void *)&datamss, optlen)) < 0)
					if (errno != EINVAL)
						err("unable to set maximum segment size");
			}
			if (clientserver && !client && (stream_idx == 1)) {
				/* check if client went away */
				pollfds[0].fd = fileno(ctlconn);
				save_events = pollfds[0].events;
				pollfds[0].events = POLLIN | POLLPRI;
				pollfds[0].revents = 0;
				if ((poll(pollfds, 1, 0) > 0) &&
				    (pollfds[0].revents & (POLLIN | POLLPRI)))
					goto cleanup;
				pollfds[0].events = save_events;
			}
			num_connect_tries++;
			if (stream_idx == 1)
				gettimeofday(&timeconn1, (struct timezone *)0);
			if (af == AF_INET) {
				error_num = connect(fd[stream_idx], (struct sockaddr *)&sinhim[stream_idx], sizeof(sinhim[stream_idx]));
			}
#ifdef AF_INET6
			else if (af == AF_INET6) {
				error_num = connect(fd[stream_idx], (struct sockaddr *)&sinhim6[stream_idx], sizeof(sinhim6[stream_idx]));
			}
#endif
			else {
			    err("unsupported AF");
			}
			if(error_num < 0) {
				if (clientserver && client && (stream_idx == 0)
						 && ((errno == ECONNREFUSED) ||
						     (errno == ECONNRESET))
						 && (num_connect_tries <
							MAX_CONNECT_TRIES)
						 && retry_server) {
					/* retry control connection to
					 * server for certain possibly
					 * transient errors */
					usleep(SERVER_RETRY_USEC);
					goto doit;
				}
				if (!trans && (stream_idx == 0))
					err("connect");
				if (stream_idx > 0) {
					if (clientserver && !client) {
						for ( i = 1; i <= stream_idx;
							     i++ )
							close(fd[i]);
						goto cleanup;
					}
					err("connect");
				}
				if (stream_idx == 0) {
					clientserver = 0;
					if (thirdparty) {
						perror("3rd party connect failed");
						fprintf(stderr, "3rd party nuttcp only supported for client/server mode\n");
						fflush(stderr);
						exit(1);
					}
					if (interval) {
						perror("connect failed");
						fprintf(stderr, "interval option only supported for client/server mode\n");
						fflush(stderr);
						exit(1);
					}
					if (reverse) {
						perror("connect failed");
						fprintf(stderr, "flip option only supported for client/server mode\n");
						fflush(stderr);
						exit(1);
					}
					if (traceroute) {
						perror("connect failed");
						fprintf(stderr, "traceroute option only supported for client/server mode\n");
						fflush(stderr);
						exit(1);
					}
					if (host3) {
						perror("connect failed");
						fprintf(stderr, "3rd party nuttcp only supported for client/server mode\n");
						fflush(stderr);
						exit(1);
					}
					if (multicast) {
						perror("connect failed");
						fprintf(stderr, "multicast only supported for client/server mode\n");
						fflush(stderr);
						exit(1);
					}
					if (udp) {
						perror("connect failed");
						fprintf(stderr, "UDP transfers only supported for client/server mode\n");
						fflush(stderr);
						exit(1);
					}
					if (format & PARSE) {
						fprintf(stderr, "nuttcp%s%s: Info=\"attempting_to_switch_to_deprecated_classic_mode\"\n",
							trans?"-t":"-r", ident);
						fprintf(stderr, "nuttcp%s%s: Info=\"will_use_less_reliable_transmitter_side_statistics\"\n",
							trans?"-t":"-r", ident);
					}
					else {
						fprintf(stderr, "nuttcp%s%s: Info: attempting to switch to deprecated \"classic\" mode\n",
							trans?"-t":"-r", ident);
						fprintf(stderr, "nuttcp%s%s: Info: will use less reliable transmitter side statistics\n",
							trans?"-t":"-r", ident);
					}
					fflush(stderr);
				}
			}
			if (stream_idx == 1) {
				gettimeofday(&timeconn2, (struct timezone *)0);
				tvsub( &timeconn, &timeconn2, &timeconn1 );
				rtt = timeconn.tv_sec*1000 +
						((double)timeconn.tv_usec)/1000;
			}
			if (sockopterr && trans &&
			    (stream_idx > 0) && datamss) {
				optlen = sizeof(datamss);
				if ((sockopterr = setsockopt(fd[stream_idx], IPPROTO_TCP, TCP_MAXSEG,  (void *)&datamss, optlen)) < 0) {
					if (errno != EINVAL)
						err("unable to set maximum segment size");
					else
						err("setting maximum segment size not supported on this OS");
				}
			}
			if (stream_idx == nstream) {
				optlen = sizeof(datamss);
				if (getsockopt(fd[stream_idx], IPPROTO_TCP, TCP_MAXSEG,  (void *)&datamss, &optlen) < 0)
					err("get dataconn maximum segment size didn't work");
				if (format & DEBUGMTU)
					fprintf(stderr, "datamss = %d\n", datamss);
			}
			if ((stream_idx == nstream) && (brief <= 0)) {
				char tmphost[ADDRSTRLEN] = "\0";
				if (af == AF_INET) {
				    inet_ntop(af, &sinhim[stream_idx].sin_addr.s_addr,
					      tmphost, sizeof(tmphost));
				}
#ifdef AF_INET6
				else if (af == AF_INET6) {
				    inet_ntop(af, sinhim6[stream_idx].sin6_addr.s6_addr,
					      tmphost, sizeof(tmphost));
				}
#endif
				else {
				    err("unsupported AF");
				}

				if (format & PARSE) {
					fprintf(stdout,
						"nuttcp%s%s: connect=%s",
						trans?"-t":"-r", ident,
						tmphost);
					if (trans && datamss) {
						fprintf(stdout, " mss=%d",
							datamss);
					}
					if (rtt)
						fprintf(stdout, P_RTT_FMT, rtt);
				}
				else {
					fprintf(stdout,
						"nuttcp%s%s: connect to %s",
						trans?"-t":"-r", ident,
						tmphost);
					if (rtt || (trans && datamss))
						fprintf(stdout, " with");
					if (trans && datamss) {
						fprintf(stdout, " mss=%d",
							datamss);
						if (rtt)
							fprintf(stdout, ",");
					}
					if (rtt)
						fprintf(stdout, RTT_FMT, rtt);
				}
				fprintf(stdout, "\n");
			}
		    } else {
			/* The receiver listens for the connection
			 * (unless reversed by the flip option)
			 */
			if (trans && (stream_idx > 0) && datamss) {
#if defined(__CYGWIN__) || defined(_WIN32)
				if (format & PARSE)
					fprintf(stderr, "nuttcp%s%s: Warning=\"setting_maximum_segment_size_not_supported_on_windows\"\n",
						trans?"-t":"-r", ident);
				else
					fprintf(stderr, "nuttcp%s%s: Warning: setting maximum segment size not supported on windows\n",
						trans?"-t":"-r", ident);
				fflush(stderr);
#endif
				optlen = sizeof(datamss);
				if ((sockopterr = setsockopt(fd[stream_idx], IPPROTO_TCP, TCP_MAXSEG,  (void *)&datamss, optlen)) < 0)
					if (errno != EINVAL)
						err("unable to set maximum segment size");
			}
			listen(fd[stream_idx], LISTEN_BACKLOG);
			if (clientserver && !client && (stream_idx == 0)
					 && !inetd && !nofork && !forked) {
				if ((pid = fork()) == (pid_t)-1)
					err("can't fork");
				if (pid != 0)
					exit(0);
				forked = 1;
				if (sinkmode) {
					close(0);
					close(1);
					close(2);
					open("/dev/null", O_RDWR);
					dup(0);
					dup(0);
				}
				setsid();
			}
			if (options && (stream_idx > 0))  {
				if( setsockopt(fd[stream_idx], SOL_SOCKET, options, (void *)&one, sizeof(one)) < 0)
					errmes("unable to setsockopt options");
			}
			if (sockopterr && trans &&
			    (stream_idx > 0) && datamss) {
				optlen = sizeof(datamss);
				if ((sockopterr = setsockopt(fd[stream_idx], IPPROTO_TCP, TCP_MAXSEG,  (void *)&datamss, optlen)) < 0)
					if (errno != EINVAL)
						err("unable to set maximum segment size");
			}
			if (clientserver && (stream_idx > 0)) {
				sigact.sa_handler = ignore_alarm;
				sigemptyset(&sigact.sa_mask);
				sigact.sa_flags = 0;
				sigaction(SIGALRM, &sigact, &savesigact);
				alarm(ACCEPT_TIMEOUT);
			}
acceptnewconn:
			fromlen = sizeof(frominet);
			nfd=accept(fd[stream_idx], (struct sockaddr *)&frominet, &fromlen);
			save_errno = errno;
			if (clientserver && (stream_idx > 0)) {
				alarm(0);
				sigact.sa_handler = savesigact.sa_handler;
				sigact.sa_mask = savesigact.sa_mask;
				sigact.sa_flags = savesigact.sa_flags;
				sigaction(SIGALRM, &sigact, 0);
			}
			if (nfd < 0) {
				/* check for interrupted system call - on
				 * server, close data streams, cleanup and
				 * try again - all other errors just die
				 */
				if ((save_errno == EINTR) && clientserver
							  && (stream_idx > 0)) {
					if (client) {
						/* if client, just give nice
						 * error message and exit
						 */
						mes("Error: accept() timeout");
						exit(1);
					}
					for ( i = 1; i <= stream_idx; i++ )
						close(fd[i]);
					goto cleanup;
				}
				err("accept");
			}
			if (clientserver && !client && (stream_idx == 0)
					 && !inetd && !nofork
					 && !single_threaded) {
				/* multi-threaded manually started server */
				if ((pid = fork()) == (pid_t)-1)
					err("can't fork");
				if (pid != 0) {
					/* parent just waits for quick
					 * child exit */
					while ((wait_pid = wait(&pidstat))
						    != pid) {
						if (wait_pid == (pid_t)-1) {
							if (errno == ECHILD)
								break;
							err("wait failed");
						}
					}
					/* and then accept()s another client
					 * connection */
					close(nfd);
					stream_idx = 0;
					if (oneshot)
						goto cleanup;
					goto acceptnewconn;
				}
				/* child just makes a grandchild and then
				 * immediately exits (avoid zombie processes) */
				if ((pid = fork()) == (pid_t)-1)
					err("can't fork");
				if (pid != 0)
					exit(0);
				/* grandkid does all the work */
				oneshot = 1;
			}
			af = frominet.ss_family;
			close(fd[stream_idx]);
			fd[stream_idx]=nfd;
			if (sockopterr && trans &&
			    (stream_idx > 0) && datamss) {
				optlen = sizeof(datamss);
				if ((sockopterr = setsockopt(fd[stream_idx], IPPROTO_TCP, TCP_MAXSEG,  (void *)&datamss, optlen)) < 0) {
					if (errno != EINVAL)
						err("unable to set maximum segment size");
					else
						err("setting maximum segment size not supported on this OS");
				}
			}
			if (stream_idx == nstream) {
				optlen = sizeof(datamss);
				if (getsockopt(fd[stream_idx], IPPROTO_TCP, TCP_MAXSEG,  (void *)&datamss, &optlen) < 0)
					err("get dataconn maximum segment size didn't work");
				if (format & DEBUGMTU)
					fprintf(stderr, "datamss = %d\n", datamss);
			}
			if (af == AF_INET) {
			    struct sockaddr_in peer;
			    socklen_t peerlen = sizeof(peer);
			    if (getpeername(fd[stream_idx],
					    (struct sockaddr *)&peer,
					    &peerlen) < 0) {
				err("getpeername");
			    }
			    if ((stream_idx == nstream) && (brief <= 0)) {
				char tmphost[ADDRSTRLEN] = "\0";
				inet_ntop(af, &peer.sin_addr.s_addr,
					  tmphost, sizeof(tmphost));

				if (format & PARSE) {
					fprintf(stdout,
						"nuttcp%s%s: accept=%s",
						trans?"-t":"-r", ident,
						tmphost);
					if (trans && datamss) {
						fprintf(stdout, " mss=%d",
							datamss);
					}
				}
				else {
					fprintf(stdout,
						"nuttcp%s%s: accept from %s",
						trans?"-t":"-r", ident,
						tmphost);
					if (trans && datamss) {
						fprintf(stdout, " with mss=%d",
							datamss);
					}
				}
				fprintf(stdout, "\n");
			    }
			    if (stream_idx == 0) clientaddr = peer.sin_addr;
			}
#ifdef AF_INET6
			else if (af == AF_INET6) {
			    struct sockaddr_in6 peer;
			    socklen_t peerlen = sizeof(peer);
			    if (getpeername(fd[stream_idx],
					    (struct sockaddr *)&peer,
					    &peerlen) < 0) {
				err("getpeername");
			    }
			    if ((stream_idx == nstream) && (brief <= 0)) {
				char tmphost[ADDRSTRLEN] = "\0";
				inet_ntop(af, peer.sin6_addr.s6_addr,
					  tmphost, sizeof(tmphost));
				if (format & PARSE) {
				    fprintf(stdout,
					    "nuttcp%s%s: accept=%s",
					    trans?"-t":"-r", ident,
					    tmphost);
				    if (trans && datamss) {
					fprintf(stdout, " mss=%d", datamss);
				    }
				}
				else {
				    fprintf(stdout,
					    "nuttcp%s%s: accept from %s",
					    trans?"-t":"-r", ident,
					    tmphost);
				    if (trans && datamss) {
					fprintf(stdout, " with mss=%d",
						datamss);
				    }
				}
				fprintf(stdout, "\n");
			    }
			    if (stream_idx == 0) {
				clientaddr6 = peer.sin6_addr;
				clientscope6 = peer.sin6_scope_id;
			    }
			}
#endif
			else {
			    err("unsupported AF");
			}
		    }
		}
		if (!udp && trans && (stream_idx >= 1) && (retransinfo > 0)) {
			nretrans[stream_idx] = get_retrans(fd[stream_idx]);
			iretrans[stream_idx] = nretrans[stream_idx];
		}
		optlen = sizeof(sendwinval);
		if (getsockopt(fd[stream_idx], SOL_SOCKET, SO_SNDBUF,  (void *)&sendwinval, &optlen) < 0)
			err("get send window size didn't work");
#if defined(linux)
		sendwinval /= 2;
#endif
		if ((stream_idx > 0) && sendwin && (trans || braindead) &&
		    (sendwinval < (0.98 * sendwin))) {
			if (format & PARSE)
				fprintf(stderr, "nuttcp%s%s: Warning=\"send_window_size_%d_<_requested_window_size_%d\"\n",
					trans?"-t":"-r", ident,
					sendwinval, sendwin);
			else
				fprintf(stderr, "nuttcp%s%s: Warning: send window size %d < requested window size %d\n",
					trans?"-t":"-r", ident,
					sendwinval, sendwin);
			fflush(stderr);
		}
		optlen = sizeof(rcvwinval);
		if (getsockopt(fd[stream_idx], SOL_SOCKET, SO_RCVBUF,  (void *)&rcvwinval, &optlen) < 0)
			err("Get recv window size didn't work");
#if defined(linux)
		rcvwinval /= 2;
#endif
		if ((stream_idx > 0) && rcvwin && (!trans || braindead) &&
		    (rcvwinval < (0.98 * rcvwin))) {
			if (format & PARSE)
				fprintf(stderr, "nuttcp%s%s: Warning=\"receive_window_size_%d_<_requested_window_size_%d\"\n",
					trans?"-t":"-r", ident,
					rcvwinval, rcvwin);
			else
				fprintf(stderr, "nuttcp%s%s: Warning: receive window size %d < requested window size %d\n",
					trans?"-t":"-r", ident,
					rcvwinval, rcvwin);
			fflush(stderr);
		}

		if (firsttime) {
			firsttime = 0;
			origsendwin = sendwinval;
			origrcvwin = rcvwinval;
		}

		if ((stream_idx == nstream) && (brief <= 0)) {
#if defined(linux)
			FILE *adv_ws;

			sendwinval *= 2;
			rcvwinval  *= 2;
			if ((adv_ws = fopen(TCP_ADV_WIN_SCALE, "r"))) {
				fscanf(adv_ws, "%d", &winadjust);
			}
			fclose(adv_ws);
			if (winadjust < 0) {
				sendwinavail = sendwinval >> -winadjust;
				rcvwinavail  = rcvwinval  >> -winadjust;
			}
			else if (winadjust > 0) {
				sendwinavail = sendwinval -
						   (sendwinval >> winadjust);
				rcvwinavail  = rcvwinval  -
						   (rcvwinval  >> winadjust);
			}
#endif
			if (format & PARSE)
				fprintf(stdout,"nuttcp%s%s: send_window_size=%d receive_window_size=%d\n", trans?"-t":"-r", ident, sendwinval, rcvwinval);
			else
				fprintf(stdout,"nuttcp%s%s: send window size = %d, receive window size = %d\n", trans?"-t":"-r", ident, sendwinval, rcvwinval);
#if defined(linux)
			if (format & PARSE)
				fprintf(stdout,"nuttcp%s%s: send_window_avail=%d receive_window_avail=%d\n", trans?"-t":"-r", ident, sendwinavail, rcvwinavail);
			else
				fprintf(stdout,"nuttcp%s%s: available send window = %d, available receive window = %d\n", trans?"-t":"-r", ident, sendwinavail, rcvwinavail);
#endif
		}
	}

	if (abortconn)
		exit(1);

	if (host3 && clientserver) {
		char path[64];
		char *cmd;

		fflush(stdout);
		fflush(stderr);
		cmd = nut_cmd;

		if (client) {
			if ((pid = fork()) == (pid_t)-1)
				err("can't fork");
			if (pid == 0) {
				if (interval) {
					itimer.it_value.tv_sec = interval;
				}
				else if (timeout) {
					itimer.it_value.tv_sec = timeout;
				}
				else {
					if (rate != MAXRATE)
						itimer.it_value.tv_sec =
							(double)(2*nbuf*buflen)
								/rate/125;
					else
						itimer.it_value.tv_sec =
							(double)(nbuf*buflen)
							    /LOW_RATE_HOST3/125;
					if (itimer.it_value.tv_sec < 7200)
						itimer.it_value.tv_sec = 7200;
				}
				itimer.it_value.tv_sec += SRVR_INFO_TIMEOUT;
				itimer.it_value.tv_usec = 0;
				itimer.it_interval.tv_sec = 0;
				itimer.it_interval.tv_usec = 0;
				setitimer(ITIMER_REAL, &itimer, 0);
				while (fgets(linebuf, sizeof(linebuf),
					     stdin) && !intr) {
					setitimer(ITIMER_REAL, &itimer, 0);
					if (strncmp(linebuf, "DONE", 4)
							== 0)
						exit(0);
					if (*ident && (*linebuf != '\n'))
						fprintf(stdout, "%s: ",
							ident + 1);
					fputs(linebuf, stdout);
					fflush(stdout);
				}
				itimer.it_value.tv_sec = 0;
				itimer.it_value.tv_usec = 0;
				setitimer(ITIMER_REAL, &itimer, 0);
				exit(0);
			}
			signal(SIGINT, SIG_IGN);
			while ((wait_pid = wait(&pidstat)) != pid) {
				if (wait_pid == (pid_t)-1) {
					if (errno == ECHILD)
						break;
					err("wait failed");
				}
			}
			exit(0);
		}
		else {
			if ((pid = fork()) == (pid_t)-1)
				err("can't fork");
			if (pid != 0) {
				sigact.sa_handler = &sigalarm;
				sigemptyset(&sigact.sa_mask);
				sigact.sa_flags = 0;
				sigaction(SIGALRM, &sigact, 0);
				alarm(10);
				while ((wait_pid = wait(&pidstat))
						!= pid) {
					if (wait_pid == (pid_t)-1) {
						if (errno == ECHILD)
							break;
						if (errno == EINTR) {
							pollfds[0].fd =
							    fileno(ctlconn);
							pollfds[0].events =
							    POLLIN | POLLPRI;
							pollfds[0].revents = 0;
							if ((poll(pollfds, 1, 0)
									> 0)
								&& (pollfds[0].revents &
									(POLLIN | POLLPRI))) {
								kill(pid,
								     SIGINT);
								sleep(1);
								kill(pid,
								     SIGINT);
								continue;
							}
							sigact.sa_handler =
								&sigalarm;
							sigemptyset(&sigact.sa_mask);
							sigact.sa_flags = 0;
							sigaction(SIGALRM,
								  &sigact, 0);
							alarm(10);
							continue;
						}
						err("wait failed");
					}
				}
				fprintf(stdout, "DONE\n");
				fflush(stdout);
				goto cleanup;
			}
			close(2);
			dup(1);
			i = 0;
			j = 0;
			cmdargs[i++] = cmd;
			cmdargs[i++] = "-3";
			if (ctlport3) {
				sprintf(tmpargs[j], "-P%hu", ctlport3);
				cmdargs[i++] = tmpargs[j++];
			}
			else {
				if (pass_ctlport) {
					sprintf(tmpargs[j], "-P%hu", ctlport);
					cmdargs[i++] = tmpargs[j++];
				}
			}
			if (affinity >= 0) {
				sprintf(tmpargs[j], "-xc%d", affinity);
				cmdargs[i++] = tmpargs[j++];
			}
			if (srvr_affinity >= 0) {
				sprintf(tmpargs[j], "-xcs%d", srvr_affinity);
				cmdargs[i++] = tmpargs[j++];
			}
			if (irvers < 50302) {
				if ((udp && !multicast
					 && (buflen != DEFAULTUDPBUFLEN)) ||
				    (udp && multicast
					 && (buflen != DEFAULT_MC_UDPBUFLEN)) ||
				    (!udp && (buflen != 65536))) {
					sprintf(tmpargs[j], "-l%d", buflen);
					cmdargs[i++] = tmpargs[j++];
				}
			}
			else if (buflen) {
				sprintf(tmpargs[j], "-l%d", buflen);
				cmdargs[i++] = tmpargs[j++];
			}
			if (nbuf != INT_MAX) {
				if (nbuf_bytes)
					sprintf(tmpargs[j], "-n%llub", nbuf);
				else
					sprintf(tmpargs[j], "-n%llu", nbuf);
				cmdargs[i++] = tmpargs[j++];
			}
			if (brief3 != 1) {
				sprintf(tmpargs[j], "-b%d", brief3);
				cmdargs[i++] = tmpargs[j++];
			}
			if (sendwin) {
				sprintf(tmpargs[j], "-w%d", sendwin/1024);
				cmdargs[i++] = tmpargs[j++];
			}
			if (nstream != 1) {
				sprintf(tmpargs[j], "-N%d%s", nstream,
					multilink ? "m" : "");
				cmdargs[i++] = tmpargs[j++];
			}
			if (rate != MAXRATE) {
				if (maxburst > 1) {
					if (rate_pps)
						sprintf(tmpargs[j],
							"-R%s%lup/%d",
							irate ? "i" : "",
							rate, maxburst);
					else
						sprintf(tmpargs[j],
							"-R%s%lu/%d",
							irate ? "i" : "",
							rate, maxburst);
				}
				else {
					if (rate_pps)
						sprintf(tmpargs[j], "-R%s%lup",
							irate ? "i" : "", rate);
					else
						sprintf(tmpargs[j], "-R%s%lu",
							irate ? "i" : "", rate);
				}
				cmdargs[i++] = tmpargs[j++];
			} else {
				if (udp && !multicast)
					cmdargs[i++] = "-R0";
			}
			if (port != DEFAULT_PORT) {
				sprintf(tmpargs[j], "-p%hu", port);
				cmdargs[i++] = tmpargs[j++];
			}
			if (trans)
				cmdargs[i++] = "-r";
			if (braindead)
				cmdargs[i++] = "-wb";
			if (timeout && (timeout != DEFAULT_TIMEOUT)) {
				sprintf(tmpargs[j], "-T%f", timeout);
				cmdargs[i++] = tmpargs[j++];
			}
			if (udp) {
				if (multicast) {
					if (ssm == 1)
						sprintf(tmpargs[j], "-ms%d",
							multicast);
					else if (ssm == 0)
						sprintf(tmpargs[j], "-ma%d",
							multicast);
					else
						sprintf(tmpargs[j], "-m%d",
							multicast);
					cmdargs[i++] = tmpargs[j++];
				}
				else
					cmdargs[i++] = "-u";
			}
			if (do_jitter) {
				cmdargs[i++] = "-j";
			}
			if (do_owd) {
				cmdargs[i++] = "-o";
			}
			if (interval) {
				sprintf(tmpargs[j], "-i%f", interval);
				cmdargs[i++] = tmpargs[j++];
			}
			if (reverse)
				cmdargs[i++] = "-F";
			if (format) {
				if (format & XMITSTATS)
					cmdargs[i++] = "-fxmitstats";
				if (format & RUNNINGTOTAL)
					cmdargs[i++] = "-frunningtotal";
				if (format & NOPERCENTLOSS)
					cmdargs[i++] = "-f-percentloss";
				if (format & NODROPS)
					cmdargs[i++] = "-f-drops";
				if (format & NORETRANS)
					cmdargs[i++] = "-f-retrans";
				if (format & PARSE)
					cmdargs[i++] = "-fparse";
			}
			else {
				cmdargs[i++] = "-f-rtt";
			}
			if (traceroute)
				cmdargs[i++] = "-xt";
			if (datamss) {
				sprintf(tmpargs[j], "-M%d", datamss);
				cmdargs[i++] = tmpargs[j++];
			}
			if (tos) {
				sprintf(tmpargs[j], "-c0x%Xt", tos);
				cmdargs[i++] = tmpargs[j++];
			}
			if (nodelay)
				cmdargs[i++] = "-D";
			if (ipad_stride.ip32) {
				sprintf(tmpargs[j], "%s+%d", host3,
					ipad_stride.ip32);
				cmdargs[i++] = tmpargs[j++];
			}
			else
				cmdargs[i++] = host3;
			cmdargs[i] = NULL;
			execvp(cmd, cmdargs);
			if (errno == ENOENT) {
				strcpy(path, "/usr/local/sbin/");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			if (errno == ENOENT) {
				strcpy(path, "/usr/local/bin/");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			if (errno == ENOENT) {
				strcpy(path, "/usr/sbin/");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			if (errno == ENOENT) {
				strcpy(path, "/sbin/");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			if (errno == ENOENT) {
				strcpy(path, "/usr/etc/");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			if ((errno == ENOENT) && (getuid() != 0)
					      && (geteuid() != 0)) {
				strcpy(path, "./");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			perror("execvp failed");
			fprintf(stderr, "failed to execute %s\n", cmd);
			fflush(stdout);
			fflush(stderr);
			if (!inetd)
				exit(0);
			goto cleanup;
		}
	}

	if (traceroute && clientserver) {
		char path[64];
		char *cmd;

		fflush(stdout);
		fflush(stderr);
		if (multicast) {
			cmd = "mtrace";
#ifdef AF_INET6
			if (af == AF_INET6)
				cmd = "mtrace6";
#endif
		}
		else {
			cmd = "traceroute";
#ifdef AF_INET6
			if (af == AF_INET6)
				cmd = "traceroute6";
#endif
		}
		if (client) {
			if ((pid = fork()) == (pid_t)-1)
				err("can't fork");
			if (pid != 0) {
				while ((wait_pid = wait(&pidstat)) != pid) {
					if (wait_pid == (pid_t)-1) {
						if (errno == ECHILD)
							break;
						err("wait failed");
					}
				}
				fflush(stdout);
			}
			else {
				signal(SIGINT, SIG_DFL);
				close(2);
				dup(1);
				i = 0;
				cmdargs[i++] = cmd;
				cmdargs[i++] = host;
				cmdargs[i] = NULL;
				execvp(cmd, cmdargs);
				if (errno == ENOENT) {
					strcpy(path, "/usr/local/sbin/");
					strcat(path, cmd);
					execv(path, cmdargs);
				}
				if (errno == ENOENT) {
					strcpy(path, "/usr/local/bin/");
					strcat(path, cmd);
					execv(path, cmdargs);
				}
				if (errno == ENOENT) {
					strcpy(path, "/usr/sbin/");
					strcat(path, cmd);
					execv(path, cmdargs);
				}
				if (errno == ENOENT) {
					strcpy(path, "/sbin/");
					strcat(path, cmd);
					execv(path, cmdargs);
				}
				if (errno == ENOENT) {
					strcpy(path, "/usr/etc/");
					strcat(path, cmd);
					execv(path, cmdargs);
				}
				perror("execvp failed");
				fprintf(stderr, "failed to execute %s\n", cmd);
				fflush(stdout);
				fflush(stderr);
				exit(0);
			}
		}
		fprintf(stdout, "\n");
		if (intr) {
			intr = 0;
			fprintf(stdout, "\n");
			signal(SIGINT, sigint);
		}
		if (!skip_data) {
			for ( stream_idx = 1; stream_idx <= nstream;
					      stream_idx++ )
				close(fd[stream_idx]);
		}
		if (client) {
			if ((pid = fork()) == (pid_t)-1)
				err("can't fork");
			if (pid == 0) {
				while (fgets(linebuf, sizeof(linebuf),
					     stdin) && !intr) {
					if (strncmp(linebuf, "DONE", 4)
							== 0)
						exit(0);
					fputs(linebuf, stdout);
					fflush(stdout);
				}
				exit(0);
			}
			signal(SIGINT, SIG_IGN);
			while ((wait_pid = wait(&pidstat)) != pid) {
				if (wait_pid == (pid_t)-1) {
					if (errno == ECHILD)
						break;
					err("wait failed");
				}
			}
			exit(0);
		}
		else {
			if (!inetd) {
				if ((pid = fork()) == (pid_t)-1)
					err("can't fork");
				if (pid != 0) {
					while ((wait_pid = wait(&pidstat))
							!= pid) {
						if (wait_pid == (pid_t)-1) {
							if (errno == ECHILD)
								break;
							err("wait failed");
						}
					}
					fprintf(stdout, "DONE\n");
					fflush(stdout);
					goto cleanup;
				}
			}
			close(2);
			dup(1);
			i = 0;
			cmdargs[i++] = cmd;
			cmdargs[i++] = host;
			cmdargs[i] = NULL;
			execvp(cmd, cmdargs);
			if (errno == ENOENT) {
				strcpy(path, "/usr/local/sbin/");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			if (errno == ENOENT) {
				strcpy(path, "/usr/local/bin/");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			if (errno == ENOENT) {
				strcpy(path, "/usr/sbin/");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			if (errno == ENOENT) {
				strcpy(path, "/sbin/");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			if (errno == ENOENT) {
				strcpy(path, "/usr/etc/");
				strcat(path, cmd);
				execv(path, cmdargs);
			}
			perror("execvp failed");
			fprintf(stderr, "failed to execute %s\n", cmd);
			fflush(stdout);
			fflush(stderr);
			if (!inetd)
				exit(0);
			goto cleanup;
		}
	}

	if (multicast) {
		struct sockaddr_in peer;
		socklen_t peerlen = sizeof(peer);
		struct sockaddr_in me;
		socklen_t melen = sizeof(me);
#ifdef AF_INET6
		struct sockaddr_in6 peer6;
		socklen_t peer6len = sizeof(peer6);
		struct sockaddr_in6 me6;
		socklen_t me6len = sizeof(me6);
#endif
		if (af == AF_INET) {
			if (getpeername(fd[0], (struct sockaddr *)&peer,
					&peerlen) < 0) {
				err("getpeername");
			}
			if (getsockname(fd[0], (struct sockaddr *)&me,
					&melen) < 0) {
				err("getsockname");
			}
		}
#ifdef AF_INET6
		else if (af == AF_INET6) {
			if (getpeername(fd[0], (struct sockaddr *)&peer6,
					&peer6len) < 0) {
				err("getpeername");
			}
			if (getsockname(fd[0], (struct sockaddr *)&me6,
					&me6len) < 0) {
				err("getsockname");
			}
		}
#endif /* AF_INET6 */
		else {
			err("unsupported AF");
		}

		if (!trans) {
		    if ((af == AF_INET) && !ssm) { /* IPv4 ASM */
			/* The multicast receiver must join the mc group */
			if (client && (irvers >= 50505)) {
				bcopy((char *)&me.sin_addr.s_addr,
				      (char *)&mc_group.imr_multiaddr.s_addr,
				      sizeof(struct in_addr));
			}
			else {
				bcopy((char *)&peer.sin_addr.s_addr,
				      (char *)&mc_group.imr_multiaddr.s_addr,
				      sizeof(struct in_addr));
			}
			mc_group.imr_multiaddr.s_addr &= htonl(0xFFFFFF);
			mc_group.imr_multiaddr.s_addr |= htonl(HI_MC << 24);
			if (setsockopt(fd[1], IPPROTO_IP, IP_ADD_MEMBERSHIP,
				       (void *)&mc_group, sizeof(mc_group)) < 0)
				err("setsockopt: IP_ADD_MEMBERSHIP");
			if (brief <= 0) {
				inet_ntop(af, &peer.sin_addr.s_addr,
					  multsrc, sizeof(multsrc));
				inet_ntop(af, &mc_group.imr_multiaddr,
					  multaddr, sizeof(multaddr));

				if (format & PARSE)
					fprintf(stdout,
						"nuttcp%s%s: multicast_source=%s multicast_group=%s ssm=0\n",
						trans?"-t":"-r", ident,
						multsrc, multaddr);
				else
					fprintf(stdout,
						"nuttcp%s%s: receive from multicast source %s\n",
						trans?"-t":"-r", ident,
						multsrc);
					fprintf(stdout,
						"nuttcp%s%s: using asm on multicast group %s\n",
						trans?"-t":"-r", ident,
						multaddr);
			}
		    }
#ifdef AF_INET6
		    else if ((af == AF_INET6) && !ssm) { /* IPv6 ASM */
			/* The multicast receiver must join the mc group */
			if (client) {
				bcopy((char *)&me6.sin6_addr,
				      (char *)&mc6_group.ipv6mr_multiaddr,
				      sizeof(struct in6_addr));
			}
			else {
				bcopy((char *)&peer6.sin6_addr,
				      (char *)&mc6_group.ipv6mr_multiaddr,
				      sizeof(struct in6_addr));
			}
			bcopy((char *)&hi_mc6,
			      (char *)&mc6_group.ipv6mr_multiaddr,
			      HI_MC6_LEN);
			if (setsockopt(fd[1], IPPROTO_IPV6, IPV6_JOIN_GROUP,
				       (void *)&mc6_group,
				       sizeof(mc6_group)) < 0)
				err("setsockopt: IPV6_JOIN_GROUP");
			if (brief <= 0) {
				inet_ntop(af, &peer6.sin6_addr,
					  multsrc, sizeof(multsrc));
				inet_ntop(af, &mc6_group.ipv6mr_multiaddr,
					  multaddr, sizeof(multaddr));

				if (format & PARSE)
					fprintf(stdout,
						"nuttcp%s%s: multicast_source=%s multicast_group=%s ssm=0\n",
						trans?"-t":"-r", ident,
						multsrc, multaddr);
				else
					fprintf(stdout,
						"nuttcp%s%s: receive from multicast source %s\n",
						trans?"-t":"-r", ident,
						multsrc);
					fprintf(stdout,
						"nuttcp%s%s: using asm on multicast group %s\n",
						trans?"-t":"-r", ident,
						multaddr);
			}
		    }
#endif /* AF_INET6 */
#ifdef MCAST_JOIN_SOURCE_GROUP
		    else if ((af == AF_INET) && ssm) { /* IPv4 SSM */
			/* multicast receiver joins the mc source group */
			struct sockaddr_in *group;
			struct sockaddr_in *source;
			group =
			    (struct sockaddr_in*)&group_source_req.gsr_group;
			source =
			    (struct sockaddr_in*)&group_source_req.gsr_source;
			group_source_req.gsr_interface = 0;  /* any interface */
			if (client)
				bcopy((char *)&me, (char *)group,
				      sizeof(struct sockaddr_in));
			else
				bcopy((char *)&peer, (char *)group,
				      sizeof(struct sockaddr_in));
			bcopy((char *)&peer, (char *)source,
			      sizeof(struct sockaddr_in));
			group->sin_addr.s_addr &= htonl(0xFFFFFF);
			group->sin_addr.s_addr |= htonl(HI_MC_SSM << 24);
			if (setsockopt(fd[1], IPPROTO_IP,
				       MCAST_JOIN_SOURCE_GROUP,
				       &group_source_req,
				       sizeof(group_source_req)) < 0)
				err("setsockopt: MCAST_JOIN_SOURCE_GROUP");
			if (brief <= 0) {
				inet_ntop(af, &source->sin_addr.s_addr,
					  multsrc, sizeof(multsrc));
				inet_ntop(af, &group->sin_addr.s_addr,
					  multaddr, sizeof(multaddr));
				if (format & PARSE)
					fprintf(stdout,
						"nuttcp%s%s: multicast_source=%s multicast_group=%s ssm=1\n",
						trans?"-t":"-r", ident,
						multsrc, multaddr);
				else
					fprintf(stdout,
						"nuttcp%s%s: receive from multicast source %s\n",
						trans?"-t":"-r", ident,
						multsrc);
					fprintf(stdout,
						"nuttcp%s%s: using ssm on multicast group %s\n",
						trans?"-t":"-r", ident,
						multaddr);
			}
		    }
#ifdef AF_INET6
		    else if ((af == AF_INET6) && ssm) { /* IPv6 SSM */
			/* multicast receiver joins the mc source group */
			struct sockaddr_in6 *group;
			struct sockaddr_in6 *source;
			group =
			    (struct sockaddr_in6*)&group_source_req.gsr_group;
			source =
			    (struct sockaddr_in6*)&group_source_req.gsr_source;
			group_source_req.gsr_interface = 0;  /* any interface */
			if (client)
				bcopy((char *)&me6, (char *)group,
				      sizeof(struct sockaddr_in6));
			else
				bcopy((char *)&peer6, (char *)group,
				      sizeof(struct sockaddr_in6));
			bcopy((char *)&peer6, (char *)source,
			      sizeof(struct sockaddr_in6));
			bcopy((char *)&hi_mc6, (char *)&group->sin6_addr,
			      HI_MC6_LEN);
			if (setsockopt(fd[1], IPPROTO_IPV6,
				       MCAST_JOIN_SOURCE_GROUP,
				       &group_source_req,
				       sizeof(group_source_req)) < 0)
				err("setsockopt: MCAST_JOIN_SOURCE_GROUP");
			if (brief <= 0) {
				inet_ntop(af, &source->sin6_addr.s6_addr,
					  multsrc, sizeof(multsrc));
				inet_ntop(af, &group->sin6_addr.s6_addr,
					  multaddr, sizeof(multaddr));
				if (format & PARSE)
					fprintf(stdout,
						"nuttcp%s%s: multicast_source=%s multicast_group=%s ssm=1\n",
						trans?"-t":"-r", ident,
						multsrc, multaddr);
				else
					fprintf(stdout,
						"nuttcp%s%s: receive from multicast source %s\n",
						trans?"-t":"-r", ident,
						multsrc);
					fprintf(stdout,
						"nuttcp%s%s: using ssm on multicast group %s\n",
						trans?"-t":"-r", ident,
						multaddr);
			}
		    }
#endif /* AF_INET6 */
#endif /* MCAST_JOIN_SOURCE_GROUP */
		    else {
			err("unsupported AF");
		    }
		}
		else { /* trans */
		    if (af == AF_INET) {
			bcopy((char *)&sinhim[1].sin_addr.s_addr,
			      (char *)&save_sinhim.sin_addr.s_addr,
			      sizeof(struct in_addr));
		    }
#ifdef AF_INET6
		    else if (af == AF_INET6) {
			bcopy((char *)&sinhim6[1], (char *)&save_sinhim6,
			      sizeof(struct sockaddr_in6));
		    }
#endif
		    if ((af == AF_INET) && !ssm) { /* IPv4 ASM */
			/* The multicast transmitter just sends to mc group */
			if (client || (irvers < 50505)) {
				bcopy((char *)&me.sin_addr.s_addr,
				      (char *)&sinhim[1].sin_addr.s_addr,
				      sizeof(struct in_addr));
			}
			else {
				bcopy((char *)&peer.sin_addr.s_addr,
				      (char *)&sinhim[1].sin_addr.s_addr,
				      sizeof(struct in_addr));
			}
			sinhim[1].sin_addr.s_addr &= htonl(0xFFFFFF);
			sinhim[1].sin_addr.s_addr |= htonl(HI_MC << 24);
			if (setsockopt(fd[1], IPPROTO_IP, IP_MULTICAST_TTL,
				       (void *)&multicast,
				       sizeof(multicast)) < 0)
				err("setsockopt: IP_MULTICAST_TTL");
			if (brief <= 0) {
				inet_ntop(af, &me.sin_addr.s_addr,
					  multsrc, sizeof(multsrc));
				inet_ntop(af, &sinhim[1].sin_addr.s_addr,
					  multaddr, sizeof(multaddr));
				if (format & PARSE) {
					fprintf(stdout,
						"nuttcp%s%s: multicast_source=%s multicast_group=%s ssm=0\n",
						trans?"-t":"-r", ident,
						multsrc, multaddr);
				}
				else {
					fprintf(stdout,
						"nuttcp%s%s: sending from multicast source %s\n",
						trans?"-t":"-r", ident,
						multsrc);
					fprintf(stdout,
						"nuttcp%s%s: using asm on multicast group %s\n",
						trans?"-t":"-r", ident,
						multaddr);
				}
			}
		    }
#ifdef AF_INET6
		    else if ((af == AF_INET6) && !ssm) { /* IPv6 ASM */
			/* The multicast transmitter just sends to mc group */
			if (client) {
				bcopy((char *)&me6.sin6_addr,
				      (char *)&sinhim6[1].sin6_addr,
				      sizeof(struct in6_addr));
			}
			else {
				bcopy((char *)&peer6.sin6_addr,
				      (char *)&sinhim6[1].sin6_addr,
				      sizeof(struct in6_addr));
			}
			bcopy((char *)&hi_mc6, (char *)&sinhim6[1].sin6_addr,
			      HI_MC6_LEN);
			if (setsockopt(fd[1], IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
				       (void *)&multicast,
				       sizeof(multicast)) < 0)
				err("setsockopt: IPV6_MULTICAST_HOPS");
			if (brief <= 0) {
				inet_ntop(af, &me6.sin6_addr.s6_addr,
					  multsrc, sizeof(multsrc));
				inet_ntop(af, &sinhim6[1].sin6_addr.s6_addr,
					  multaddr, sizeof(multaddr));
				if (format & PARSE) {
					fprintf(stdout,
						"nuttcp%s%s: multicast_source=%s multicast_group=%s ssm=0\n",
						trans?"-t":"-r", ident,
						multsrc, multaddr);
				}
				else {
					fprintf(stdout,
						"nuttcp%s%s: sending from multicast source %s\n",
						trans?"-t":"-r", ident,
						multsrc);
					fprintf(stdout,
						"nuttcp%s%s: using asm on multicast group %s\n",
						trans?"-t":"-r", ident,
						multaddr);
				}
			}
		    }
#endif /* AF_INET6 */
#ifdef MCAST_JOIN_SOURCE_GROUP
		    else if ((af == AF_INET) && ssm) { /* IPv4 SSM */
			/* The multicast transmitter just sends to mc group */
			struct sockaddr_in *group;
			struct sockaddr_in *source;
			group =
			    (struct sockaddr_in*)&group_source_req.gsr_group;
			source =
			    (struct sockaddr_in*)&group_source_req.gsr_source;
			group_source_req.gsr_interface = 0;  /* any interface */
			if (client)
				bcopy((char *)&me, (char *)group,
				      sizeof(struct sockaddr_in));
			else
				bcopy((char *)&peer, (char *)group,
				      sizeof(struct sockaddr_in));
			bcopy((char *)&me, (char *)source,
			      sizeof(struct sockaddr_in));
			group->sin_addr.s_addr &= htonl(0xFFFFFF);
			group->sin_addr.s_addr |= htonl(HI_MC_SSM << 24);
			bcopy((char *)&group->sin_addr.s_addr,
			      (char *)&sinhim[1].sin_addr.s_addr,
			      sizeof(struct in_addr));
			if (setsockopt(fd[1], IPPROTO_IP, IP_MULTICAST_TTL,
				       (void *)&multicast,
				       sizeof(multicast)) < 0)
				err("setsockopt: IP_MULTICAST_TTL");
			if (brief <= 0) {
				inet_ntop(af, &source->sin_addr.s_addr,
					  multsrc, sizeof(multsrc));
				inet_ntop(af, &group->sin_addr.s_addr,
					  multaddr, sizeof(multaddr));
				if (format & PARSE)
					fprintf(stdout,
						"nuttcp%s%s: multicast_source=%s multicast_group=%s ssm=1\n",
						trans?"-t":"-r", ident,
						multsrc, multaddr);
				else
					fprintf(stdout,
						"nuttcp%s%s: sending from multicast source %s\n",
						trans?"-t":"-r", ident,
						multsrc);
					fprintf(stdout,
						"nuttcp%s%s: using ssm on multicast group %s\n",
						trans?"-t":"-r", ident,
						multaddr);
			}
		    }
#ifdef AF_INET6
		    else if ((af == AF_INET6) && ssm) { /* IPv6 SSM */
			/* The multicast transmitter just sends to mc group */
			struct sockaddr_in6 *group;
			struct sockaddr_in6 *source;
			group =
			    (struct sockaddr_in6*)&group_source_req.gsr_group;
			source =
			    (struct sockaddr_in6*)&group_source_req.gsr_source;
			group_source_req.gsr_interface = 0;  /* any interface */
			if (client)
				bcopy((char *)&me6, (char *)group,
				      sizeof(struct sockaddr_in6));
			else
				bcopy((char *)&peer6, (char *)group,
				      sizeof(struct sockaddr_in6));
			bcopy((char *)&me6, (char *)source,
			      sizeof(struct sockaddr_in6));
			bcopy((char *)&hi_mc6, (char *)&group->sin6_addr,
			      HI_MC6_LEN);
			bcopy((char *)&group->sin6_addr.s6_addr,
			      (char *)&sinhim6[1].sin6_addr.s6_addr,
			      sizeof(struct in6_addr));
			if (setsockopt(fd[1], IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
				       (void *)&multicast,
				       sizeof(multicast)) < 0)
				err("setsockopt: IPV6_MULTICAST_HOPS");
			if (brief <= 0) {
				inet_ntop(af, &source->sin6_addr.s6_addr,
					  multsrc, sizeof(multsrc));
				inet_ntop(af, &group->sin6_addr.s6_addr,
					  multaddr, sizeof(multaddr));
				if (format & PARSE)
					fprintf(stdout,
						"nuttcp%s%s: multicast_source=%s multicast_group=%s ssm=1\n",
						trans?"-t":"-r", ident,
						multsrc, multaddr);
				else
					fprintf(stdout,
						"nuttcp%s%s: sending from multicast source %s\n",
						trans?"-t":"-r", ident,
						multsrc);
					fprintf(stdout,
						"nuttcp%s%s: using ssm on multicast group %s\n",
						trans?"-t":"-r", ident,
						multaddr);
			}
		    }
#endif /* AF_INET6 */
#endif /* MCAST_JOIN_SOURCE_GROUP */
		    else {
			err("unsupported AF");
		    }
		}
	}

	if (trans && timeout) {
		itimer.it_value.tv_sec = timeout;
		itimer.it_value.tv_usec =
			(timeout - itimer.it_value.tv_sec)*1000000;
		itimer.it_interval.tv_sec = 0;
		itimer.it_interval.tv_usec = 0;
		signal(SIGALRM, sigalarm);
		if (!udp)
			setitimer(ITIMER_REAL, &itimer, 0);
	}
	else if (!trans && interval) {
		sigact.sa_handler = &sigalarm;
		sigemptyset(&sigact.sa_mask);
		sigact.sa_flags = SA_RESTART;
		sigaction(SIGALRM, &sigact, 0);
		itimer.it_value.tv_sec = interval;
		itimer.it_value.tv_usec =
			(interval - itimer.it_value.tv_sec)*1000000;
		itimer.it_interval.tv_sec = interval;
		itimer.it_interval.tv_usec =
			(interval - itimer.it_interval.tv_sec)*1000000;
		setitimer(ITIMER_REAL, &itimer, 0);
		if (clientserver) {
			chk_idle_data = (interval < idle_data_min) ?
						idle_data_min : interval;
			chk_idle_data = (chk_idle_data > idle_data_max) ?
						idle_data_max : chk_idle_data;
		}
	}
	else if (clientserver && !trans) {
		sigact.sa_handler = &sigalarm;
		sigemptyset(&sigact.sa_mask);
		sigact.sa_flags = SA_RESTART;
		sigaction(SIGALRM, &sigact, 0);
		if (timeout) {
			chk_idle_data = timeout/2;
		}
		else {
			if (rate != MAXRATE)
				chk_idle_data = (double)(nbuf*buflen)
							    /rate/125/2;
			else
				chk_idle_data = default_idle_data;
		}
		chk_idle_data = (chk_idle_data < idle_data_min) ?
					idle_data_min : chk_idle_data;
		chk_idle_data = (chk_idle_data > idle_data_max) ?
					idle_data_max : chk_idle_data;
		itimer.it_value.tv_sec = chk_idle_data;
		itimer.it_value.tv_usec =
			(chk_idle_data - itimer.it_value.tv_sec)
				*1000000;
		itimer.it_interval.tv_sec = chk_idle_data;
		itimer.it_interval.tv_usec =
			(chk_idle_data - itimer.it_interval.tv_sec)
				*1000000;
		setitimer(ITIMER_REAL, &itimer, 0);
	}

	if (interval && clientserver && client && trans)
		do_poll = 1;

	if (irate) {
		pkt_time = (double)buflen/rate/125;
		irate_pk_usec = pkt_time*1000000;
		irate_pk_nsec = (pkt_time*1000000 - irate_pk_usec)*1000;
		pkt_time_ms = pkt_time*1000;
	}
	prep_timer();
	errno = 0;
	stream_idx = 0;
	ocorrection = 0;
	correction = 0.0;
	if (do_poll) {
		long flags;

		pollfds[0].fd = fileno(ctlconn);
		pollfds[0].events = POLLIN | POLLPRI;
		pollfds[0].revents = 0;
		for ( i = 1; i <= nstream; i++ ) {
			pollfds[i].fd = fd[i];
			pollfds[i].events = POLLOUT;
			pollfds[i].revents = 0;
		}
		flags = fcntl(0, F_GETFL, 0);
		if (flags < 0)
			err("fcntl 1");
		flags |= O_NONBLOCK;
		if (fcntl(0, F_SETFL, flags) < 0)
			err("fcntl 2");
	}
	if (sinkmode) {
		register int cnt = 0;
		if (trans)  {
			if(udp) {
				strcpy(buf, "BOD0");
				if (multicast) {
				    bcopy((char *)&sinhim[1].sin_addr.s_addr,
					  (char *)&save_mc.sin_addr.s_addr,
					  sizeof(struct in_addr));
				    bcopy((char *)&save_sinhim.sin_addr.s_addr,
					  (char *)&sinhim[1].sin_addr.s_addr,
					  sizeof(struct in_addr));
				}
				(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr start */
				if (two_bod) {
					usleep(250000);
					strcpy(buf, "BOD1");
					(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr start */
				}
				if (multicast) {
				    bcopy((char *)&save_mc.sin_addr.s_addr,
					  (char *)&sinhim[1].sin_addr.s_addr,
					  sizeof(struct in_addr));
				}
				if (timeout)
					setitimer(ITIMER_REAL, &itimer, 0);
				prep_timer();
			}
			bzero(buf + 8, 8);	/* zero out timestamp */
			nbytes += buflen;
			if (do_poll && (format & DEBUGPOLL)) {
				fprintf(stdout, "do_poll is set\n");
				fflush(stdout);
			}
			if (udplossinfo)
				bcopy(&nbytes, buf + 24, 8);
			if (!udp && interval && !(format & NORETRANS) &&
			    (nstream == 1) &&
			    ((retransinfo == 1) ||
			     ((retransinfo >= 2) &&
			      (force_retrans >= retransinfo)))) {
				uint32_t tmp;

				if (client) {
					if (send_retrans)
						do_retrans = 1;
					send_retrans = 0;
					if (!udp)
						bzero(buf + 24, 8);
				}
				else {
					if (retransinfo == 1)
						tmp = 0x5254524Eu;  /* "RTRN" */
					else
						tmp = 0x48525452u;  /* "HRTR" */
					bcopy(&nretrans[1], buf + 24, 4);
					bcopy(&tmp, buf + 28, 4);
					do_retrans = 0;
				}
			}
			else {
				send_retrans = 0;
				do_retrans = 0;
				if (!udp)
					bzero(buf + 24, 8);
			}
			if (nbuf == INT_MAX)
				nbuf = ULLONG_MAX;
			if (!client) {
				/* check if client went away */
				pollfds[0].fd = fileno(ctlconn);
				save_events = pollfds[0].events;
				pollfds[0].events = POLLIN | POLLPRI;
				pollfds[0].revents = 0;
				if ((poll(pollfds, 1, 0) > 0) &&
				    (pollfds[0].revents & (POLLIN | POLLPRI))) {
					nbuf = 0;
					intr = 1;
				}
				pollfds[0].events = save_events;
			}
			while (nbuf-- && ((cnt = Nwrite(fd[stream_idx + 1],buf,buflen)) == buflen) && !intr) {
				if (clientserver && ((nbuf & 0x3FF) == 0)) {
				    if (!client) {
					/* check if client went away */
					pollfds[0].fd = fileno(ctlconn);
					save_events = pollfds[0].events;
					pollfds[0].events = POLLIN | POLLPRI;
					pollfds[0].revents = 0;
					if ((poll(pollfds, 1, 0) > 0)
						&& (pollfds[0].revents &
							(POLLIN | POLLPRI)))
						intr = 1;
					pollfds[0].events = save_events;
				    }
				    else if (handle_urg) {
					/* check for urgent TCP data
					 * on control connection */
					pollfds[0].fd = fileno(ctlconn);
					save_events = pollfds[0].events;
					pollfds[0].events = POLLPRI;
					pollfds[0].revents = 0;
					if ((poll(pollfds, 1, 0) > 0)
						&& (pollfds[0].revents &
							POLLPRI)) {
						tmpbuf[0] = '\0';
						if ((recv(fd[0], tmpbuf, 1,
							  MSG_OOB) == -1) &&
						    (errno == EINVAL))
							recv(fd[0], tmpbuf,
							     1, 0);
						if (tmpbuf[0] == 'A')
							intr = 1;
						else
							err("recv urgent data");
					}
					pollfds[0].events = save_events;
				    }
				}
				nbytes += buflen;
				cnt = 0;
				if (udplossinfo)
					bcopy(&nbytes, buf + 24, 8);
				if (send_retrans) {
					nretrans[1] = get_retrans(
							fd[stream_idx + 1]);
					nretrans[1] -= iretrans[1];
					bcopy(&nretrans[1], buf + 24, 4);
				}
				stream_idx++;
				stream_idx = stream_idx % nstream;
				if (do_poll &&
				       ((pollst = poll(pollfds, nstream + 1, 0))
						> 0) &&
				       (pollfds[0].revents & (POLLIN | POLLPRI)) && !intr) {
					/* check for server output */
#ifdef DEBUG
					if (format & DEBUGPOLL) {
						fprintf(stdout, "got something %d: ", i);
				    for ( i = 0; i < nstream + 1; i++ ) {
					if (pollfds[i].revents & POLLIN) {
						fprintf(stdout, " rfd %d",
							pollfds[i].fd);
					}
					if (pollfds[i].revents & POLLPRI) {
						fprintf(stdout, " pfd %d",
							pollfds[i].fd);
					}
					if (pollfds[i].revents & POLLOUT) {
						fprintf(stdout, " wfd %d",
							pollfds[i].fd);
					}
					if (pollfds[i].revents & POLLERR) {
						fprintf(stdout, " xfd %d",
							pollfds[i].fd);
					}
					if (pollfds[i].revents & POLLHUP) {
						fprintf(stdout, " hfd %d",
							pollfds[i].fd);
					}
					if (pollfds[i].revents & POLLNVAL) {
						fprintf(stdout, " nfd %d",
							pollfds[i].fd);
					}
				    }
						fprintf(stdout, "\n");
						fflush(stdout);
				    }
					if (format & DEBUGPOLL) {
						fprintf(stdout, "got server output: %s", intervalbuf);
						fflush(stdout);
					}
#endif
					while (fgets(intervalbuf, sizeof(intervalbuf), stdin))
					{
					if (strncmp(intervalbuf, "DONE", 4) == 0) {
						if (format & DEBUGPOLL) {
							fprintf(stdout, "got DONE\n");
							fflush(stdout);
						}
						got_done = 1;
						intr = 1;
						do_poll = 0;
						break;
					}
					else if (strncmp(intervalbuf, "nuttcp-r", 8) == 0) {
						if ((brief <= 0) ||
						    strstr(intervalbuf,
							    "Warning") ||
						    strstr(intervalbuf,
							    "Error") ||
						    strstr(intervalbuf,
							    "Debug")) {
							if (*ident) {
								fputs("nuttcp-r", stdout);
								fputs(ident, stdout);
								fputs(intervalbuf + 8, stdout);
							}
							else
								fputs(intervalbuf, stdout);
							fflush(stdout);
						}
					}
					else {
						if (*ident)
							fprintf(stdout, "%s: ", ident + 1);
						cp1 = intervalbuf +
							strlen(intervalbuf) - 1;
						/* ugly kludge to get rid of
						 * server "0 retrans" info at
						 * start of transfer for small
						 * interval reports -
						 * hopefully it won't be
						 * necessary to also check
						 * at end of transfer when
						 * processing server output */
						if (!got_0retrans) {
						    if (format & PARSE) {
							if ((cp2 = strstr(
								    intervalbuf,
								    "host-"
								    "retrans")))
							    *(cp2 - 1) = '\0';
							else if ((cp2 = strstr(
								    intervalbuf,
								    "retrans")))
							    *(cp2 - 1) = '\0';
							else {
							    *cp1 = '\0';
							    got_0retrans = 1;
							}
						    }
						    else {
							if (strstr(intervalbuf,
								"host-retrans"))
							    *(cp1 - 19) = '\0';
							else if (strstr(
								    intervalbuf,
								    "retrans"))
							    *(cp1 - 14) = '\0';
							else {
							    *cp1 = '\0';
							    got_0retrans = 1;
							}
						    }
						}
						else {
						    *cp1 = '\0';
						}
						if (do_retrans) {
						    cp1 = strstr(intervalbuf,
								 "Mbps") + 4;
						    ch = '\0';
						    if (cp1) {
							if (format & PARSE) {
							    cp1 = strchr(cp1,
									 '.');
							    if (cp1)
								cp1 += 5;
							}
							ch = *cp1;
						    }
						    if (ch)
							*cp1 = '\0';
						}
						fputs(intervalbuf, stdout);
						if (do_retrans && sinkmode) {
						    nretrans[1] =
						      get_retrans(fd[stream_idx
									+ 1]);
						    nretrans[1] -= iretrans[1];
						    if (format & PARSE)
							fprintf(stdout,
							 P_RETRANS_FMT_INTERVAL,
							   (retransinfo == 1) ?
								"" : "host-",
							   (nretrans[1] -
								pretrans));
						    else
							fprintf(stdout,
							 RETRANS_FMT_INTERVAL,
							   (nretrans[1] -
								pretrans),
							   (retransinfo == 1) ?
								"" : "host-");
						    pretrans = nretrans[1];
						}
						if (do_retrans && cp1 && ch) {
						    *cp1 = ch;
						    fputs(cp1, stdout);
						}
						fprintf(stdout, "\n");
						fflush(stdout);
					}
					}
				}
				if (do_poll && (pollst < 0)) {
					if (errno == EINTR)
						break;
					err("poll");
				}
			}
			nbytes -= buflen;
			if (intr && (cnt > 0))
				nbytes += cnt;
			if(udp) {
				if (multicast)
				    bcopy((char *)&save_sinhim.sin_addr.s_addr,
					  (char *)&sinhim[1].sin_addr.s_addr,
					  sizeof(struct in_addr));
				strcpy(buf, "EOD0");
				(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr end */
			}
		} else {
			first_read = 1;
			first_jitter = 1;
			first_jitteri = 1;
			nowd = 0;
			nowdi = 0;
			owd_min = 1000000.0;
			owd_mini = 1000000.0;
			owd_max = -1000000.0;
			owd_maxi = -1000000.0;
			owd_avg = 0.0;
			owd_avgi = 0.0;
			need_swap = 0;
			bzero(buf + 24, 8);
			if (udp) {
			    ntbytesc = 0;
			    got_eod0 = 0;
			    while (((cnt=Nread(fd[stream_idx + 1],buf,buflen)) > 0) && !intr)  {
				    if( cnt <= 4 ) {
					    if (strncmp(buf, "EOD0", 4) == 0) {
						    gettimeofday(&time_eod0,
							(struct timezone *)0);
						    got_eod0 = 1;
						    done = 1;
						    continue;
					    }
					    if (strncmp(buf, "EOD", 3) == 0) {
						    ocorrection = buf[3] - '0';
						    gettimeofday(&time_eod,
							(struct timezone *)0);
						    done = 1;
						    break;	/* "EOF" */
					    }
					    if (strncmp(buf, "BOD", 3) == 0) {
						    if (two_bod &&
							(buf[3] == '0'))
							    continue;
						    if (interval)
							setitimer(ITIMER_REAL,
								  &itimer, 0);
						    prep_timer();
						    got_begin = 1;
						    continue;
					    }
					    break;
				    }
				    else if (!got_begin) {
					    if (interval)
						    setitimer(ITIMER_REAL,
							      &itimer, 0);
					    prep_timer();
					    got_begin = 1;
				    }
				    else if (got_eod0) {
					    /* got data after EOD0, so
					     * extend EOD0 time */
					    gettimeofday(&time_eod0,
							 (struct timezone *)0);
				    }
				    if (!got_begin)
					    continue;
				    nbytes += cnt;
				    cnt = 0;
				    /* problematic if the interval timer
				     * goes off right here */
				    if (udplossinfo) {
					    if (first_read) {
						    bcopy(buf + 24, &ntbytesc,
								8);
						    first_read = 0;
						    if (ntbytesc > 0x100000000ull)
							    need_swap = 1;
						    if (!need_swap) {
							    stream_idx++;
							    stream_idx =
								stream_idx
								    % nstream;
							    continue;
						    }
					    }
					    if (!need_swap)
						    bcopy(buf + 24, &ntbytesc,
								8);
					    else {
						    cp1 = (char *)&ntbytesc;
						    cp2 = buf + 31;
						    for ( i = 0; i < 8; i++ )
							    *cp1++ = *cp2--;
					    }
				    }
				    if (do_jitter) {
					    if (first_jitter) {
						    gettimeofday(
							&timepkr,
							(struct timezone *)0);
						    timepkri = timepkr;
						    first_jitter = 0;
						    first_jitteri = 0;
						    ntbytescp = ntbytesc;
						    ntbytescpi = ntbytesc;
						    jitter = 0.0;
						    jitteri = 0.0;
						    njitter = 0;
						    njitteri = 0;
						    jitter_min = 1000000.0;
						    jitter_mini = 1000000.0;
						    jitter_max = -1000000.0;
						    jitter_maxi = -1000000.0;
						    jitter_avg = 0.0;
						    jitter_avgi = 0.0;
#ifdef DEBUG
						    if (clientserver &&
							client &&
							(format & DEBUGJITTER))
							fprintf(stdout,
							    "pkt_time_ms"
							    " = %6.3f ms\n",
							    pkt_time_ms);
#endif
						    stream_idx++;
						    stream_idx =
							stream_idx % nstream;
						    continue;
					    }
					    /* formula for jitter is from
					     * RFC1889 - note synchronized
					     * clocks are not required since
					     * source packet delta time is
					     * known (pkt_time_ms)
					     *
					     * D(i,j)=(Rj-Ri)-(Sj-Si)
					     *       =(Rj-Sj)-(Ri-Si)
					     * J=J+(|D(i-1,i)|-J)/16
					     *
					     * for nuttcp we just use the raw
					     * absolute value of the delta
					     *
					     * J=|D(i-1,i)|
					     */

					    if (!do_owd) {
						gettimeofday(&timerx,
							(struct timezone *)0);
					    }
					    if (do_jitter & JITTER_IGNORE_OOO) {
						/* first check that packet
						 * is next in sequence */
						if (udplossinfo &&
						    (ntbytescp + buflen)
							!= ntbytesc) {
						    ntbytescp = ntbytesc;
						    ntbytescpi = ntbytesc;
						    timepkr = timerx;
						    timepkri = timerx;
						    stream_idx++;
						    stream_idx =
							stream_idx % nstream;
						    continue;
						}
					    }

					    tvsub( &timed, &timerx, &timepkr );
					    pkt_delta =
						timed.tv_sec*1000
						    + ((double)timed.tv_usec)
								/ 1000;
					    pkt_delta -= pkt_time_ms;
					    if (pkt_delta >= 0)
						jitter = pkt_delta;
					    else
						jitter = -pkt_delta;
					    njitter++;
					    if (jitter < jitter_min)
						jitter_min = jitter;
					    if (jitter > jitter_max)
						jitter_max = jitter;
					    jitter_avg += jitter;
#ifdef DEBUG
					    if (clientserver && client &&
						(format & DEBUGJITTER))
						fprintf(stdout,
						    "pkt_delta = %6.3f ms, "
						    "jitter = %9.6f ms\n",
						    pkt_delta, jitter);
#endif
					    timepkr = timerx;
					    ntbytescp = ntbytesc;
					    if (!interval) {
						    stream_idx++;
						    stream_idx =
							stream_idx % nstream;
						    continue;
					    }
					    if (first_jitteri) {
						    gettimeofday(
							&timepkri,
							(struct timezone *)0);
						    first_jitteri = 0;
						    ntbytescpi = ntbytesc;
						    jitteri = 0.0;
						    njitteri = 0;
						    jitter_mini = 1000000.0;
						    jitter_maxi = -1000000.0;
						    jitter_avgi = 0.0;
						    stream_idx++;
						    stream_idx =
							stream_idx % nstream;
						    continue;
					    }
					    tvsub( &timed, &timerx, &timepkri );
					    pkt_delta =
						timed.tv_sec*1000
						    + ((double)timed.tv_usec)
								/ 1000;
					    pkt_delta -= pkt_time_ms;
					    if (pkt_delta >= 0)
						jitteri = pkt_delta;
					    else
						jitteri = -pkt_delta;
					    njitteri++;
					    if (jitteri < jitter_mini)
						jitter_mini = jitteri;
					    if (jitteri > jitter_maxi)
						jitter_maxi = jitteri;
					    jitter_avgi += jitteri;
					    timepkri = timerx;
					    ntbytescpi = ntbytesc;
				    }
				    stream_idx++;
				    stream_idx = stream_idx % nstream;
			    }
			    if (intr && (cnt > 0))
				    nbytes += cnt;
			    if (got_eod0) {
				    tvsub( &timed, &time_eod, &time_eod0 );
				    correction = timed.tv_sec +
						    ((double)timed.tv_usec)
								/ 1000000;
			    }
			} else {
			    while (((cnt=Nread(fd[stream_idx + 1],buf,buflen)) > 0) && !intr)  {
				    nbytes += cnt;
				    cnt = 0;
				    if (first_read) {
					if (interval && !(format & NORETRANS)) {
					    uint32_t tmp;

					    first_read = 0;
					    bcopy(buf + 24, &nretrans[1], 4);
					    bcopy(buf + 28, &tmp, 4);
					    if (tmp == 0x5254524Eu) {
						    /* "RTRN" */
						    retransinfo = 1;
						    b_flag = 1;
					    }
					    else if (tmp == 0x48525452u) {
						    /* "HRTR" */
						    retransinfo = 2;
						    b_flag = 1;
					    }
					    else if (tmp == 0x4E525452u) {
						    /* "NRTR" */
						    need_swap = 1;
						    retransinfo = 1;
						    b_flag = 1;
					    }
					    else if (tmp == 0x52545248u) {
						    /* "RTRH" */
						    need_swap = 1;
						    retransinfo = 2;
						    b_flag = 1;
					    }
					    else {
						    retransinfo = -1;
						    read_retrans = 0;
					    }
					}
					else
					    read_retrans = 0;
				    }
				    if (read_retrans) {
					    if (!need_swap)
						    bcopy(buf + 24,
							  &nretrans[1], 4);
					    else {
						    cp1 = (char *)&nretrans[1];
						    cp2 = buf + 27;
						    for ( i = 0; i < 4; i++ )
							    *cp1++ = *cp2--;
					    }
				    }
				    stream_idx++;
				    stream_idx = stream_idx % nstream;
			    }
			    if (intr && (cnt > 0))
				    nbytes += cnt;
			}
		}
	} else {
		register int cnt;
		if (trans)  {
			while((cnt=read(savestdin,buf,buflen)) > 0 &&
			    Nwrite(fd[stream_idx + 1],buf,cnt) == cnt) {
				nbytes += cnt;
				cnt = 0;
				stream_idx++;
				stream_idx = stream_idx % nstream;
			}
		}  else  {
			while((cnt=Nread(fd[stream_idx + 1],buf,buflen)) > 0 &&
			    write(savestdout,buf,cnt) == cnt) {
				nbytes += cnt;
				cnt = 0;
				stream_idx++;
				stream_idx = stream_idx % nstream;
			}
		}
	}
	if (errno && (errno != EAGAIN)) {
		if ((errno != EINTR) && (!clientserver || client)) err("IO");
	}
	itimer.it_value.tv_sec = 0;
	itimer.it_value.tv_usec = 0;
	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &itimer, 0);
	done = 1;
	(void)read_timer(stats,sizeof(stats));
	if(udp&&trans)  {
		usleep(500000);
		strcpy(buf, "EOD1");
		(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr end */
		stream_idx++;
		stream_idx = stream_idx % nstream;
		usleep(500000);
		strcpy(buf, "EOD2");
		(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr end */
		stream_idx++;
		stream_idx = stream_idx % nstream;
		usleep(500000);
		strcpy(buf, "EOD3");
		(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr end */
		stream_idx++;
		stream_idx = stream_idx % nstream;
		usleep(500000);
		strcpy(buf, "EOD4");
		(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr end */
		stream_idx++;
		stream_idx = stream_idx % nstream;
	}

	if (!udp && trans && (format & DEBUGRETRANS)) {
		sretrans = get_retrans(-1);
		fprintf(stdout, "before closing system retrans = %d\n",
			sretrans);
	}

#ifdef DEBUG
	if (clientserver && client && !trans && do_jitter && njitter &&
	    (format & DEBUGJITTER)) {
		fprintf(stdout, "njitter = %lld\n", njitter);
		fprintf(stdout, "jitter_min = %9.6f ms\n", jitter_min);
		fprintf(stdout, "jitter_max = %9.6f ms\n", jitter_max);
		fprintf(stdout, "jitter_avg = %9.6f ms\n", jitter_avg/njitter);
	}
#endif

	for ( stream_idx = 1; stream_idx <= nstream; stream_idx++ ) {
		if (!udp && trans) {
#if defined(linux) && defined(TCPI_OPT_TIMESTAMPS) && !defined(BROKEN_UNACKED)
			/* if -DBROKEN_UNACKED skip check for unACKed data
			 * (workaround motivated by possible bug encountered
			 * on a Suse Linux 10.1 system)
			 */
			struct timeval timeunack, timec, timed;
			long flags;

			optlen = sizeof(tcpinf);
			if (getsockopt(fd[stream_idx], SOL_TCP, TCP_INFO,
				       (void *)&tcpinf, &optlen) < 0) {
				mes("couldn't collect TCP info\n");
				retransinfo = -1;
			}
			gettimeofday(&timeunack, (struct timezone *)0);
			realtd = 0.0;
			if (clientserver && client) {
				reading_srvr_info = 1;
				pollfds[0].fd = fileno(ctlconn);
				pollfds[0].events = POLLIN | POLLPRI;
				pollfds[0].revents = 0;
				flags = fcntl(0, F_GETFL, 0);
				if (flags < 0)
					err("fcntl 1");
				flags |= O_NONBLOCK;
				if (fcntl(0, F_SETFL, flags) < 0)
					err("fcntl 2");
				itimer.it_value.tv_sec = MAX_EOT_WAIT_SEC + 10;
				itimer.it_value.tv_usec = 0;
				itimer.it_interval.tv_sec = 0;
				itimer.it_interval.tv_usec = 0;
				setitimer(ITIMER_REAL, &itimer, 0);
			}
			while ((tcpinf.tcpinfo_unacked) &&
			       (realtd < MAX_EOT_WAIT_SEC)) {
				if (clientserver && client &&
				    ((pollst = poll(pollfds, 1, 0)) > 0) &&
				    (pollfds[0].revents & (POLLIN | POLLPRI)) &&
				    !got_done) {
					/* check for server output */
					while (fgets(intervalbuf,
					       sizeof(intervalbuf), stdin))
					{
					setitimer(ITIMER_REAL, &itimer, 0);
					gettimeofday(&timeunack,
						     (struct timezone *)0);
					if (strncmp(intervalbuf, "DONE", 4)
							== 0) {
						if (format & DEBUGPOLL) {
							fprintf(stdout,
								"got DONE\n");
							fflush(stdout);
						}
						got_done = 1;
						intr = 1;
						break;
					}
					else if (strncmp(intervalbuf,
							 "nuttcp-", 7) == 0) {
						if ((brief <= 0) ||
						    strstr(intervalbuf,
							    "Warning") ||
						    strstr(intervalbuf,
							    "Error") ||
						    strstr(intervalbuf,
							    "Debug")) {
							if (*ident) {
							    fputs("nuttcp",
								  stdout);
							    fputs(trans ?
								    "-r" : "-t",
								  stdout);
							    fputs(ident,
								  stdout);
							    fputs(intervalbuf
								    + 8,
								  stdout);
							}
							else
							    fputs(intervalbuf,
								  stdout);
							fflush(stdout);
						}
						if (strstr(intervalbuf,
							   "Error"))
							exit(1);
					}
					else {
						if (*ident)
							fprintf(stdout, "%s: ",
								ident + 1);
						intervalbuf[strlen(intervalbuf)
								- 1] = '\0';
						if (do_retrans) {
						    cp1 = strstr(intervalbuf,
								 "Mbps") + 4;
						    ch = '\0';
						    if (cp1) {
							if (format & PARSE) {
							    cp1 = strchr(cp1,
									 '.');
							    if (cp1)
								cp1 += 5;
							}
							ch = *cp1;
						    }
						    if (ch)
							*cp1 = '\0';
						}
						fputs(intervalbuf, stdout);
						if (do_retrans && sinkmode) {
						    nretrans[stream_idx] =
						      get_retrans(
							    fd[stream_idx]);
						    nretrans[stream_idx] -=
							iretrans[stream_idx];
						    if (format & PARSE)
							fprintf(stdout,
							 P_RETRANS_FMT_INTERVAL,
							   (retransinfo == 1) ?
								"" : "host-",
							   (nretrans[stream_idx]
								- pretrans));
						    else
							fprintf(stdout,
							 RETRANS_FMT_INTERVAL,
							   (nretrans[stream_idx]
								- pretrans),
							   (retransinfo == 1) ?
								"" : "host-");
						    pretrans =
							nretrans[stream_idx];
						}
						if (do_retrans && cp1 && ch) {
						    *cp1 = ch;
						    fputs(cp1, stdout);
						}
						fprintf(stdout, "\n");
						fflush(stdout);
					}
					}
				}
				if (format & DEBUGRETRANS)
					print_tcpinfo();
				if (format & DEBUGRETRANS)
					usleep(100000);
				else
					usleep(1000);
				optlen = sizeof(tcpinf);
				if (getsockopt(fd[stream_idx],
					       SOL_TCP, TCP_INFO,
					       (void *)&tcpinf, &optlen) < 0) {
					mes("couldn't collect TCP info\n");
					retransinfo = -1;
				}
				gettimeofday(&timec, (struct timezone *)0);
				tvsub(&timed, &timec, &timeunack);
				realtd = timed.tv_sec
					    + ((double)timed.tv_usec) / 1000000;
			}
			if (clientserver && client) {
				reading_srvr_info = 0;
				flags = fcntl(0, F_GETFL, 0);
				if (flags < 0)
					err("fcntl 1");
				flags &= ~O_NONBLOCK;
				if (fcntl(0, F_SETFL, flags) < 0)
					err("fcntl 2");
				itimer.it_value.tv_sec = 0;
				itimer.it_value.tv_usec = 0;
				setitimer(ITIMER_REAL, &itimer, 0);
			}

			if (tcpinf.tcpinfo_unacked) {
				/* assume receiver went away */
				if (clientserver && client) {
					mes("Error: receiver not ACKing data");
					exit(1);
				}
				goto cleanup;
			}

			if (format & DEBUGRETRANS)
				print_tcpinfo();
#endif
			if (retransinfo > 0) {
				nretrans[stream_idx] =
					get_retrans(fd[stream_idx]);
				nretrans[stream_idx] -= iretrans[stream_idx];
			}
		}
	}
	if (!udp && trans && (format & DEBUGRETRANS)) {
		sretrans = get_retrans(-1);
		fprintf(stdout, "after closing system retrans = %d\n",
			sretrans);
	}

	if (interval && clientserver && client && do_retrans && !got_done) {
		/* don't fully close data channels yet since there
		 * may be some straggler interval reports to which we
		 * will need to append retrans info, so just shutdown()
		 * for writing for now */
		for ( stream_idx = 1; stream_idx <= nstream; stream_idx++ )
			shutdown(fd[stream_idx], SHUT_WR);
	}
	else
		close_data_channels();

	if (interval && clientserver && !client && !trans) {
		fprintf(stdout, "DONE\n");
		fflush(stdout);
	}

	if( cput <= 0.0 )  cput = 0.000001;
	if( realt <= 0.0 )  realt = 0.000001;

	if (udp && !trans) {
		if (got_eod0)
			realt -= correction;
		else
			realt -= ocorrection * 0.5;
	}

	sprintf(srvrbuf, "%.4f", (double)nbytes/1024/1024);
	sscanf(srvrbuf, "%lf", &MB);

	if (clientserver && client)
		reading_srvr_info = 1;

	if (interval && clientserver && client && trans && !got_done) {
		long flags;

		if (format & DEBUGPOLL) {
			fprintf(stdout, "getting rest of server output\n");
			fflush(stdout);
		}
		flags = fcntl(0, F_GETFL, 0);
		if (flags < 0)
			err("fcntl 3");
		flags &= ~O_NONBLOCK;
		if (fcntl(0, F_SETFL, flags) < 0)
			err("fcntl 4");
		itimer.it_value.tv_sec = SRVR_INFO_TIMEOUT;
		itimer.it_value.tv_usec = 0;
		itimer.it_interval.tv_sec = 0;
		itimer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &itimer, 0);
		while (fgets(intervalbuf, sizeof(intervalbuf), stdin)) {
			setitimer(ITIMER_REAL, &itimer, 0);
			if (strncmp(intervalbuf, "DONE", 4) == 0) {
				if (format & DEBUGPOLL) {
					fprintf(stdout, "got DONE 2\n");
					fflush(stdout);
				}
				break;
			}
			if ((!strstr(intervalbuf, " MB / ") ||
			     !strstr(intervalbuf, " sec = ")) && (brief > 0))
				continue;
			if (*ident)
				fprintf(stdout, "%s: ", ident + 1);
			intervalbuf[strlen(intervalbuf) - 1] = '\0';
			if (do_retrans) {
				cp1 = strstr(intervalbuf, "Mbps") + 4;
				ch = '\0';
				if (cp1) {
					if (format & PARSE) {
						cp1 = strchr(cp1, '.');
						if (cp1)
							cp1 += 5;
					}
					ch = *cp1;
				}
				if (ch)
					*cp1 = '\0';
			}
			fputs(intervalbuf, stdout);
			if (do_retrans && sinkmode) {
				nretrans[1] = get_retrans(fd[1]);
				nretrans[1] -= iretrans[1];
				if (format & PARSE)
					fprintf(stdout, P_RETRANS_FMT_INTERVAL,
						(retransinfo == 1) ?
							"" : "host-",
						(nretrans[1] - pretrans));
				else
					fprintf(stdout, RETRANS_FMT_INTERVAL,
						(nretrans[1] - pretrans),
						(retransinfo == 1) ?
							"" : "host-");
				pretrans = nretrans[1];
			}
			if (do_retrans && cp1 && ch) {
				*cp1 = ch;
				fputs(cp1, stdout);
			}
			fprintf(stdout, "\n");
			fflush(stdout);
		}
		itimer.it_value.tv_sec = 0;
		itimer.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &itimer, 0);
	}

	if (interval && clientserver && client && do_retrans) {
		/* it's OK to fully close the data channels now */
		close_data_channels();
	}

	if (clientserver && client) {
		itimer.it_value.tv_sec = SRVR_INFO_TIMEOUT;
		itimer.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &itimer, 0);
		cp1 = srvrbuf;
		got_srvr_retrans = 0;
		while (fgets(cp1, sizeof(srvrbuf) - (cp1 - srvrbuf), stdin)) {
			setitimer(ITIMER_REAL, &itimer, 0);
			if (*(cp1 + strlen(cp1) - 1) != '\n') {
				*cp1 = '\0';
				break;
			}
			if (strstr(cp1, "real") && strstr(cp1, "seconds")) {
				strcpy(fmt, "nuttcp-%*c: ");
				if (format & PARSE)
					strcat(fmt, P_PERF_FMT_IN);
				else
					strcat(fmt, PERF_FMT_IN);
				sscanf(cp1, fmt,
				       &srvr_MB, &srvr_realt, &srvr_KBps,
				       &srvr_Mbps);
				if (trans && udp) {
					strncpy(tmpbuf, cp1, 256);
					*(tmpbuf + 256) = '\0';
					if (strncmp(tmpbuf,
						    "nuttcp-r", 8) == 0)
						sprintf(cp1, "nuttcp-r%s%s",
							ident, tmpbuf + 8);
					cp1 += strlen(cp1);
					cp2 = cp1;
					sprintf(cp2, "nuttcp-r:");
					cp2 += 9;
					if (format & PARSE)
						strcpy(fmt, P_DROP_FMT);
					else
						strcpy(fmt, DROP_FMT);
					sprintf(cp2, fmt,
						(int64_t)(((MB - srvr_MB)
							*1024*1024)
								/buflen + 0.5),
						(uint64_t)((MB*1024*1024)
							/buflen + 0.5));
					cp2 += strlen(cp2);
					fractloss = ((MB != 0.0) ?
						1 - srvr_MB/MB : 0.0);
					if (format & PARSE)
						strcpy(fmt, P_LOSS_FMT);
					else if ((fractloss != 0.0) &&
						 (fractloss < 0.001))
						strcpy(fmt, LOSS_FMT5);
					else
						strcpy(fmt, LOSS_FMT);
					sprintf(cp2, fmt, fractloss * 100);
					cp2 += strlen(cp2);
					sprintf(cp2, "\n");
				}
			}
			else if (strstr(cp1, "sys")) {
				strcpy(fmt, "nuttcp-%*c: ");
				if (format & PARSE) {
					strcat(fmt, "stats=cpu ");
					strcat(fmt, P_CPU_STATS_FMT_IN2);
				}
				else
					strcat(fmt, CPU_STATS_FMT_IN2);
				if (sscanf(cp1, fmt,
					   &srvr_cpu_util) != 7) {
					strcpy(fmt, "nuttcp-%*c: ");
					if (format & PARSE) {
						strcat(fmt, "stats=cpu ");
						strcat(fmt,
						       P_CPU_STATS_FMT_IN);
					}
					else
						strcat(fmt, CPU_STATS_FMT_IN);
					sscanf(cp1, fmt,
					       &srvr_cpu_util);
				}
			}
			else if ((cp2 = strstr(cp1, "retrans"))) {
				got_srvr_retrans = 1;
				retransinfo = 1;
				if (strstr(cp1, "host-retrans"))
					retransinfo = 2;
				if (format & PARSE)
					sscanf(cp2, P_RETRANS_FMT_IN,
					       &total_retrans);
				else
					sscanf(cp2, RETRANS_FMT_IN,
					       &total_retrans);
				if (format & PARSE) {
					if ((cp2 = strstr(cp2,
							"retrans_by_stream=")))
						cp2 += 18;
					else
						cp2 = NULL;
				}
				else {
					if ((cp2 = strstr(cp2, "( ")))
						cp2 += 2;
					else
						cp2 = NULL;
				}
				if (cp2) {
					sscanf(cp2, "%d", &nretrans[1]);
					stream_idx = 2;
					while ((stream_idx <= nstream) &&
					       (cp2 = strchr(cp2, '+'))) {
						cp2++;
						sscanf(cp2, "%d",
						       &nretrans[stream_idx]);
						stream_idx++;
					}
				}
				/* below is for compatibility with 6.0.x beta */
				if ((cp2 = strstr(cp1, "RTT"))) {
					if (format & PARSE)
						sscanf(cp2, P_RTT_FMT_IN, &rtt);
					else
						sscanf(cp2, RTT_FMT_INB, &rtt);
				}
			}
			else if ((cp2 = strstr(cp1, "RTT")) ||
				 (cp2 = strstr(cp1, "rtt"))) {
				if (format & PARSE)
					sscanf(cp2, P_RTT_FMT_IN, &rtt);
				else
					sscanf(cp2, RTT_FMT_IN, &rtt);
			}
			else if ((cp2 = strstr(cp1, "jitter"))) {
				if (format & PARSE)
					sscanf(cp2, P_JITTER_FMT_IN,
					       &jitter_min, &jitter_avg,
					       &jitter_max);
				else
					sscanf(cp2, JITTER_FMT_IN,
					       &jitter_min, &jitter_avg,
					       &jitter_max);
				njitter = 1;
			}
			else if ((cp2 = strstr(cp1, "OWD"))) {
				if (format & PARSE)
					sscanf(cp2, P_OWD_FMT_IN,
					       &owd_min, &owd_avg, &owd_max);
				else
					sscanf(cp2, OWD_FMT_IN,
					       &owd_min, &owd_avg, &owd_max);
				nowd = 1;
			}
			else if ((strstr(cp1, "KB/cpu")) && !verbose)
				continue;
			strncpy(tmpbuf, cp1, 256);
			*(tmpbuf + 256) = '\0';
			if (strncmp(tmpbuf, "nuttcp-", 7) == 0)
				sprintf(cp1, "nuttcp-%c%s%s",
					tmpbuf[7], ident, tmpbuf + 8);
			if ((strstr(cp1, "Warning") ||
			     strstr(cp1, "Error") ||
			     strstr(cp1, "Debug"))
					&& (brief > 0)) {
				fputs(cp1, stdout);
				fflush(stdout);
			}
			cp1 += strlen(cp1);
		}
		itimer.it_value.tv_sec = 0;
		itimer.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &itimer, 0);
		got_srvr_output = 1;
		if (!udp && !trans && !got_srvr_retrans)
			retransinfo = -1;
	}

	if (!udp && trans && (retransinfo > 0)) {
		total_retrans = 0;
		for ( stream_idx = 1; stream_idx <= nstream; stream_idx++ ) {
			total_retrans += nretrans[stream_idx];
		}
	}

	if (brief <= 0) {
		strcpy(fmt, "nuttcp%s%s: ");
		if (format & PARSE)
			strcat(fmt, P_PERF_FMT_OUT);
		else
			strcat(fmt, PERF_FMT_OUT);
		fprintf(stdout, fmt, trans?"-t":"-r", ident,
			(double)nbytes/(1024*1024), realt,
			(double)nbytes/realt/1024,
			(double)nbytes/realt/125000 );
		if (clientserver && client && !trans && udp) {
			fprintf(stdout, "nuttcp-r%s:", ident);
			if (format & PARSE)
				strcpy(fmt, P_DROP_FMT);
			else
				strcpy(fmt, DROP_FMT);
			fprintf(stdout, fmt,
				(int64_t)(((srvr_MB - MB)*1024*1024)
					/buflen + 0.5),
				(uint64_t)((srvr_MB*1024*1024)/buflen + 0.5));
			fractloss = ((srvr_MB != 0.0) ? 1 - MB/srvr_MB : 0.0);
			if (format & PARSE)
				strcpy(fmt, P_LOSS_FMT);
			else if ((fractloss != 0.0) && (fractloss < 0.001))
				strcpy(fmt, LOSS_FMT5);
			else
				strcpy(fmt, LOSS_FMT);
			fprintf(stdout, fmt, fractloss * 100);
			fprintf(stdout, "\n");
		}
		if (clientserver && udp && !trans && do_jitter && njitter) {
			strcpy(fmt, "nuttcp%s%s: ");
			if (format & PARSE)
				strcat(fmt, P_JITTER_FMT);
			else
				strcat(fmt, JITTER_FMT);
			fprintf(stdout, fmt, trans?"-t":"-r", ident,
				jitter_min, jitter_avg/njitter, jitter_max);
			fprintf(stdout, "\n");
		}
		if (clientserver && !trans && do_owd && nowd) {
			strcpy(fmt, "nuttcp%s%s: ");
			if (format & PARSE)
				strcat(fmt, P_OWD_FMT);
			else
				strcat(fmt, OWD_FMT);
			fprintf(stdout, fmt, trans?"-t":"-r", ident,
				owd_min, owd_avg/nowd, owd_max);
			fprintf(stdout, "\n");
		}
		if (verbose) {
			strcpy(fmt, "nuttcp%s%s: ");
			if (format & PARSE)
				strcat(fmt, "megabytes=%.4f cpu_seconds=%.2f KB_per_cpu_second=%.2f\n");
			else
				strcat(fmt, "%.4f MB in %.2f CPU seconds = %.2f KB/cpu sec\n");
			fprintf(stdout, fmt,
				trans?"-t":"-r", ident,
				(double)nbytes/(1024*1024), cput,
				(double)nbytes/cput/1024 );
		}
		if (!udp && trans && (retransinfo > 0)) {
			fprintf(stdout, "nuttcp%s%s: ",
				trans ? "-t" : "-r", ident);
			if (format & PARSE)
				strcpy(fmt, P_RETRANS_FMT);
			else
				strcpy(fmt, RETRANS_FMT);
			fprintf(stdout, fmt,
				retransinfo == 1 ? "" : "host-", total_retrans);
			if ((nstream > 1) && (retransinfo == 1) &&
			    total_retrans) {
				if (format & PARSE)
					fprintf(stdout, P_RETRANS_FMT_STREAMS,
						nretrans[1]);
				else
					fprintf(stdout, " ( %d", nretrans[1]);
				for ( stream_idx = 2; stream_idx <= nstream;
				      stream_idx++ ) {
					fprintf(stdout, "+%d",
						nretrans[stream_idx]);
				}
				if (!(format & PARSE))
					fprintf(stdout, " )");
			}
			fprintf(stdout, "\n");
		}

		strcpy(fmt, "nuttcp%s%s: ");
		if (format & PARSE)
			strcat(fmt, "io_calls=%d msec_per_call=%.2f calls_per_sec=%.2f\n");
		else
			strcat(fmt, "%d I/O calls, msec/call = %.2f, calls/sec = %.2f\n");
		fprintf(stdout, fmt,
			trans?"-t":"-r", ident,
			numCalls,
			1024.0 * realt/((double)numCalls),
			((double)numCalls)/realt);

		strcpy(fmt, "nuttcp%s%s: ");
		if (format & PARSE)
			strcat(fmt, "stats=cpu %s\n");
		else
			strcat(fmt, "%s\n");
		fprintf(stdout, fmt, trans?"-t":"-r", ident, stats);
	}

	if (format & PARSE)
		strcpy(fmt, P_CPU_STATS_FMT_IN2);
	else
		strcpy(fmt, CPU_STATS_FMT_IN2);
	if (sscanf(stats, fmt, &cpu_util) != 6) {
		if (format & PARSE)
			strcpy(fmt, P_CPU_STATS_FMT_IN);
		else
			strcpy(fmt, CPU_STATS_FMT_IN);
		sscanf(stats, fmt, &cpu_util);
	}

	if (brief && clientserver && client) {
		if ((brief < 0) || interval)
			fprintf(stdout, "\n");
		if (udp) {
			if (trans) {
				if (*ident)
					fprintf(stdout, "%s: ", ident + 1);
				if (format & PARSE)
					strcpy(fmt, P_PERF_FMT_BRIEF);
				else
					strcpy(fmt, PERF_FMT_BRIEF);
				fprintf(stdout, fmt,
					srvr_MB, srvr_realt, srvr_Mbps,
					cpu_util, srvr_cpu_util);
				if (!(format & NODROPS)) {
					if (format & PARSE)
						strcpy(fmt, P_DROP_FMT_BRIEF);
					else
						strcpy(fmt, DROP_FMT_BRIEF);
					fprintf(stdout, fmt,
						(int64_t)(((MB - srvr_MB)
							*1024*1024)
								/buflen + 0.5),
						(uint64_t)((MB*1024*1024)
							/buflen + 0.5));
				}
				if (!(format & NOPERCENTLOSS)) {
					fractloss = ((MB != 0.0) ?
						1 - srvr_MB/MB : 0.0);
					if (format & PARSE)
						strcpy(fmt, P_LOSS_FMT_BRIEF);
					else if ((fractloss != 0.0) &&
						 (fractloss < 0.001))
						strcpy(fmt, LOSS_FMT_BRIEF5);
					else
						strcpy(fmt, LOSS_FMT_BRIEF);
					fprintf(stdout, fmt, fractloss * 100);
				}
				if (format & XMITSTATS) {
					if (format & PARSE)
						strcpy(fmt, P_PERF_FMT_BRIEF3);
					else
						strcpy(fmt, PERF_FMT_BRIEF3);
					fprintf(stdout, fmt, MB);
				}
				if ((do_jitter & JITTER_MIN) && njitter) {
					if (format & PARSE)
						strcpy(fmt,
						       P_JITTER_MIN_FMT_BRIEF);
					else
						strcpy(fmt,
						       JITTER_MIN_FMT_BRIEF);
					fprintf(stdout, fmt, jitter_min);
				}
				if ((do_jitter & JITTER_AVG) && njitter) {
					if (format & PARSE)
						strcpy(fmt,
						       P_JITTER_AVG_FMT_BRIEF);
					else
						strcpy(fmt,
						       JITTER_AVG_FMT_BRIEF);
					fprintf(stdout, fmt,
						jitter_avg/njitter);
				}
				if ((do_jitter & JITTER_MAX) && njitter) {
					if (format & PARSE)
						strcpy(fmt,
						       P_JITTER_MAX_FMT_BRIEF);
					else
						strcpy(fmt,
						       JITTER_MAX_FMT_BRIEF);
					fprintf(stdout, fmt, jitter_max);
				}
				if ((do_owd & OWD_MIN) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_MIN_FMT_BRIEF);
					else
						strcpy(fmt, OWD_MIN_FMT_BRIEF);
					fprintf(stdout, fmt, owd_min);
				}
				if ((do_owd & OWD_AVG) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_AVG_FMT_BRIEF);
					else
						strcpy(fmt, OWD_AVG_FMT_BRIEF);
					fprintf(stdout, fmt, owd_avg/nowd);
				}
				if ((do_owd & OWD_MAX) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_MAX_FMT_BRIEF);
					else
						strcpy(fmt, OWD_MAX_FMT_BRIEF);
					fprintf(stdout, fmt, owd_max);
				}
			}
			else {
				if (*ident)
					fprintf(stdout, "%s: ", ident + 1);
				if (format & PARSE)
					strcpy(fmt, P_PERF_FMT_BRIEF);
				else
					strcpy(fmt, PERF_FMT_BRIEF);
				fprintf(stdout, fmt,
					MB, realt, (double)nbytes/realt/125000,
					srvr_cpu_util, cpu_util);
				if (!(format & NODROPS)) {
					if (format & PARSE)
						strcpy(fmt, P_DROP_FMT_BRIEF);
					else
						strcpy(fmt, DROP_FMT_BRIEF);
					fprintf(stdout, fmt,
						(int64_t)(((srvr_MB - MB)
							*1024*1024)
								/buflen + 0.5),
						(uint64_t)((srvr_MB*1024*1024)
							/buflen + 0.5));
				}
				if (!(format & NOPERCENTLOSS)) {
					fractloss = ((srvr_MB != 0.0) ?
						1 - MB/srvr_MB : 0.0);
					if (format & PARSE)
						strcpy(fmt, P_LOSS_FMT_BRIEF);
					else if ((fractloss != 0.0) &&
						 (fractloss < 0.001))
						strcpy(fmt, LOSS_FMT_BRIEF5);
					else
						strcpy(fmt, LOSS_FMT_BRIEF);
					fprintf(stdout, fmt, fractloss * 100);
				}
				if (format & XMITSTATS) {
					if (format & PARSE)
						strcpy(fmt, P_PERF_FMT_BRIEF3);
					else
						strcpy(fmt, PERF_FMT_BRIEF3);
					fprintf(stdout, fmt, srvr_MB);
				}
				if ((do_jitter & JITTER_MIN) && njitter) {
					if (format & PARSE)
						strcpy(fmt,
						       P_JITTER_MIN_FMT_BRIEF);
					else
						strcpy(fmt,
						       JITTER_MIN_FMT_BRIEF);
					fprintf(stdout, fmt, jitter_min);
				}
				if ((do_jitter & JITTER_AVG) && njitter) {
					if (format & PARSE)
						strcpy(fmt,
						       P_JITTER_AVG_FMT_BRIEF);
					else
						strcpy(fmt,
						       JITTER_AVG_FMT_BRIEF);
					fprintf(stdout, fmt,
						jitter_avg/njitter);
				}
				if ((do_jitter & JITTER_MAX) && njitter) {
					if (format & PARSE)
						strcpy(fmt,
						       P_JITTER_MAX_FMT_BRIEF);
					else
						strcpy(fmt,
						       JITTER_MAX_FMT_BRIEF);
					fprintf(stdout, fmt, jitter_max);
				}
				if ((do_owd & OWD_MIN) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_MIN_FMT_BRIEF);
					else
						strcpy(fmt, OWD_MIN_FMT_BRIEF);
					fprintf(stdout, fmt, owd_min);
				}
				if ((do_owd & OWD_AVG) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_AVG_FMT_BRIEF);
					else
						strcpy(fmt, OWD_AVG_FMT_BRIEF);
					fprintf(stdout, fmt, owd_avg/nowd);
				}
				if ((do_owd & OWD_MAX) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_MAX_FMT_BRIEF);
					else
						strcpy(fmt, OWD_MAX_FMT_BRIEF);
					fprintf(stdout, fmt, owd_max);
				}
			}
			fprintf(stdout, "\n");
		}
		else
			if (trans) {
				if ((retransinfo > 0) &&
				    (!(format & NORETRANS))) {
					if (format & DEBUGRETRANS) {
					    sretrans = get_retrans(-1);
					    fprintf(stdout,
						"report system retrans = %d\n",
						sretrans);
					}
				}
				if (*ident)
					fprintf(stdout, "%s: ", ident + 1);
				if (format & PARSE)
					strcpy(fmt, P_PERF_FMT_BRIEF);
				else
					strcpy(fmt, PERF_FMT_BRIEF);
				fprintf(stdout, fmt,
					srvr_MB, srvr_realt, srvr_Mbps,
					cpu_util, srvr_cpu_util );
				if ((nstream > 1) && (retransinfo == 1) &&
				    total_retrans && !(format & NORETRANS) &&
				    (brief & BRIEF_RETRANS_STREAMS)) {
					if (format & PARSE) {
					    fprintf(stdout, P_RETRANS_FMT_BRIEF,
						    "", total_retrans);
					    fprintf(stdout,
						    P_RETRANS_FMT_STREAMS,
						    nretrans[1]);
					}
					else {
					    fprintf(stdout,
						    RETRANS_FMT_BRIEF_STR1,
						    total_retrans,
						    nretrans[1]);
					}
					for ( stream_idx = 2;
					      stream_idx <= nstream;
					      stream_idx++ ) {
					    fprintf(stdout, "+%d",
						    nretrans[stream_idx]);
					}
					if (!(format & PARSE))
					    fprintf(stdout,
						    RETRANS_FMT_BRIEF_STR2);
				}
				else if ((retransinfo > 0) &&
				    (!(format & NORETRANS))) {
					if (format & PARSE)
						fprintf(stdout,
							P_RETRANS_FMT_BRIEF,
							retransinfo == 1 ?
								"" : "host-",
							total_retrans);
					else
						fprintf(stdout,
							RETRANS_FMT_BRIEF,
							total_retrans,
							retransinfo == 1 ?
								"" : "host-");
				}
				if (rtt && (format & WANTRTT)) {
					if (format & PARSE)
						strcpy(fmt, P_RTT_FMT_BRIEF);
					else
						strcpy(fmt, RTT_FMT_BRIEF);
					fprintf(stdout, fmt, rtt);
				}
				if ((do_owd & OWD_MIN) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_MIN_FMT_BRIEF);
					else
						strcpy(fmt, OWD_MIN_FMT_BRIEF);
					fprintf(stdout, fmt, owd_min);
				}
				if ((do_owd & OWD_AVG) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_AVG_FMT_BRIEF);
					else
						strcpy(fmt, OWD_AVG_FMT_BRIEF);
					fprintf(stdout, fmt, owd_avg/nowd);
				}
				if ((do_owd & OWD_MAX) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_MAX_FMT_BRIEF);
					else
						strcpy(fmt, OWD_MAX_FMT_BRIEF);
					fprintf(stdout, fmt, owd_max);
				}
				fprintf(stdout, "\n");
			}
			else {
				if (*ident)
					fprintf(stdout, "%s: ", ident + 1);
				if (format & PARSE)
					strcpy(fmt, P_PERF_FMT_BRIEF);
				else
					strcpy(fmt, PERF_FMT_BRIEF);
				fprintf(stdout, fmt,
					MB, realt, (double)nbytes/realt/125000,
					srvr_cpu_util, cpu_util );
				if ((nstream > 1) && (retransinfo == 1) &&
				    total_retrans && !(format & NORETRANS) &&
				    (brief & BRIEF_RETRANS_STREAMS) &&
				    (irvers >= 70101)) {
					if (format & PARSE) {
					    fprintf(stdout, P_RETRANS_FMT_BRIEF,
						    "", total_retrans);
					    fprintf(stdout,
						    P_RETRANS_FMT_STREAMS,
						    nretrans[1]);
					}
					else {
					    fprintf(stdout,
						    RETRANS_FMT_BRIEF_STR1,
						    total_retrans,
						    nretrans[1]);
					}
					for ( stream_idx = 2;
					      stream_idx <= nstream;
					      stream_idx++ ) {
					    fprintf(stdout, "+%d",
						    nretrans[stream_idx]);
					}
					if (!(format & PARSE))
					    fprintf(stdout,
						    RETRANS_FMT_BRIEF_STR2);
				}
				else if ((retransinfo > 0) &&
				    (!(format & NORETRANS))) {
					if (format & PARSE)
						fprintf(stdout,
							P_RETRANS_FMT_BRIEF,
							retransinfo == 1 ?
								"" : "host-",
							total_retrans);
					else
						fprintf(stdout,
							RETRANS_FMT_BRIEF,
							total_retrans,
							retransinfo == 1 ?
								"" : "host-");
				}
				if (rtt && (format & WANTRTT)) {
					if (format & PARSE)
						strcpy(fmt, P_RTT_FMT_BRIEF);
					else
						strcpy(fmt, RTT_FMT_BRIEF);
					fprintf(stdout, fmt, rtt);
				}
				if ((do_owd & OWD_MIN) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_MIN_FMT_BRIEF);
					else
						strcpy(fmt, OWD_MIN_FMT_BRIEF);
					fprintf(stdout, fmt, owd_min);
				}
				if ((do_owd & OWD_AVG) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_AVG_FMT_BRIEF);
					else
						strcpy(fmt, OWD_AVG_FMT_BRIEF);
					fprintf(stdout, fmt, owd_avg/nowd);
				}
				if ((do_owd & OWD_MAX) && nowd) {
					if (format & PARSE)
						strcpy(fmt,
						       P_OWD_MAX_FMT_BRIEF);
					else
						strcpy(fmt, OWD_MAX_FMT_BRIEF);
					fprintf(stdout, fmt, owd_max);
				}
				fprintf(stdout, "\n");
			}
	}
	else {
		if (brief && !clientserver) {
			if (brief < 0)
				fprintf(stdout, "\n");
			if (*ident)
				fprintf(stdout, "%s: ", ident + 1);
			fprintf(stdout, PERF_FMT_BRIEF2 "\n", MB,
				realt, (double)nbytes/realt/125000, cpu_util,
				trans?"TX":"RX" );
		}
	}

cleanup:
	if (clientserver) {
		if (client) {
			itimer.it_value.tv_sec = SRVR_INFO_TIMEOUT;
			itimer.it_value.tv_usec = 0;
			setitimer(ITIMER_REAL, &itimer, 0);
			if (brief <= 0)
				fputs("\n", stdout);
			if (brief <= 0) {
				if (got_srvr_output) {
					fputs(srvrbuf, stdout);
				}
			}
			else {
				while (fgets(buf, mallocsize, stdin)) {
					setitimer(ITIMER_REAL, &itimer, 0);
					fputs(buf, stdout);
				}
			}
			itimer.it_value.tv_sec = 0;
			itimer.it_value.tv_usec = 0;
			setitimer(ITIMER_REAL, &itimer, 0);
			fflush(stdout);
			close(0);
		}
		else {
			fflush(stdout);
			close(1);
			if (!inetd) {
				dup(savestdout);
				close(savestdout);
				fflush(stderr);
				if (!nofork) {
					close(2);
					dup(1);
				}
			}
		}
		fclose(ctlconn);
		if (!inetd)
			close(fd[0]);
		if (!udp && trans && (retransinfo > 0)) {
			if (format & DEBUGRETRANS) {
				sretrans = get_retrans(-1);
				fprintf(stdout, "final system retrans = %d\n",
					sretrans);
			}
		}
	}
	if (clientserver && !client) {
		for ( stream_idx = 1; stream_idx <= nstream; stream_idx++ ) {
			fd[stream_idx] = -1;
		}
		itimer.it_value.tv_sec = 0;
		itimer.it_value.tv_usec = 0;
		itimer.it_interval.tv_sec = 0;
		itimer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &itimer, 0);
		signal(SIGALRM, SIG_DFL);
		bzero((char *)&frominet, sizeof(frominet));
		bzero((char *)&clientaddr, sizeof(clientaddr));
#ifdef AF_INET6
		bzero((char *)&clientaddr6, sizeof(clientaddr));
		clientscope6 = 0;
#endif
		cput = 0.000001;
		realt = 0.000001;
		nbytes = 0;
		ntbytes = 0;
		ntbytesc = 0;
		chk_nbytes = 0;
		numCalls = 0;
/*		Don't re-initialize buflen since it's used to		*/
/*		determine if we need to change the buffer memory	*/
/*		allocation for the next client data stream request	*/
/*		buflen = 64 * 1024;					*/
/*		if (udp) buflen = DEFAULTUDPBUFLEN;			*/
		nbuf = 0;
		sendwin = origsendwin;
		rcvwin = origrcvwin;
		b_flag = 1;
		rate = MAXRATE;
		maxburst = 1;
		nburst = 1;
		irate = 0;
		irate_cum_nsec = 0.0;
		timeout = 0.0;
		interval = 0.0;
		chk_interval = 0.0;
		chk_idle_data = 0.0;
		datamss = 0;
		tos = 0;
		nodelay = 0;
		do_poll = 0;
		pbytes = 0;
		ptbytes = 0;
		ident[0] = '\0';
		intr = 0;
		abortconn = 0;
		ipad_stride.ip32 = 0;
		port = 5101;
		trans = 0;
		braindead = 0;
		udp = 0;
		udplossinfo = 0;
		do_jitter = 0;
		do_owd = 0;
		retransinfo = 0;
		force_retrans = 0;
		rtt = 0.0;
		pretrans = 0;
		sretrans = 0;
		got_srvr_output = 0;
		reading_srvr_info = 0;
		reverse = 0;
		format = 0;
		traceroute = 0;
		multicast = 0;
		ssm = -1;
		skip_data = 0;
		host3 = NULL;
		thirdparty = 0;
		ctlport3 = 0;
		nbuf_bytes = 0;
		rate_pps = 0;
		brief = 0;
		done = 0;
		got_begin = 0;
		two_bod = 0;
		handle_urg = 0;
		for ( stream_idx = 0; stream_idx <= nstream; stream_idx++ ) {
			if (res[stream_idx] &&
			    !(multilink && (stream_idx > 1))) {
				freeaddrinfo(res[stream_idx]);
				res[stream_idx] = NULL;
			}
			nretrans[stream_idx] = 0;
			iretrans[stream_idx] = 0;
		}
		nstream = 1;
		multilink = 0;
		if (!oneshot)
			goto doit;
	}

	for ( stream_idx = 0; stream_idx <= nstream; stream_idx++ ) {
		if (res[stream_idx] && !(multilink && (stream_idx > 1))) {
			freeaddrinfo(res[stream_idx]);
			res[stream_idx] = NULL;
		}
	}

	exit(0);

usage:
	fprintf(stdout,Usage);
	exit(1);
}

static void
err( char *s )
{
	long flags, saveflags;

	fprintf(stderr,"nuttcp%s%s: v%d.%d.%d%s: Error: ", trans?"-t":"-r",
			ident, vers_major, vers_minor, vers_delta,
			beta ? BETA_STR : "");
	perror(s);
	fprintf(stderr,"errno=%d\n",errno);
	fflush(stderr);
	if ((stream_idx > 0) && !done &&
	    clientserver && !client && !trans && handle_urg) {
		/* send 'A' for ABORT as urgent TCP data
		 * on control connection (don't block) */
		saveflags = fcntl(fd[0], F_GETFL, 0);
		if (saveflags != -1) {
			flags = saveflags | O_NONBLOCK;
			fcntl(fd[0], F_SETFL, flags);
		}
		send(fd[0], "A", 1, MSG_OOB);
		if (saveflags != -1) {
			flags = saveflags;
			fcntl(fd[0], F_SETFL, flags);
		}
	}
	exit(1);
}

static void
mes( char *s )
{
	fprintf(stdout,"nuttcp%s%s: v%d.%d.%d%s: %s\n", trans?"-t":"-r", ident,
			vers_major, vers_minor, vers_delta,
			beta ? BETA_STR : "", s);
}

static void
errmes( char *s )
{
	fprintf(stderr,"nuttcp%s%s: v%d.%d.%d%s: Error: ", trans?"-t":"-r",
			ident, vers_major, vers_minor, vers_delta,
			beta ? BETA_STR : "");
	perror(s);
	fprintf(stderr,"errno=%d\n",errno);
	fflush(stderr);
}

void
pattern( register char *cp, register int cnt )
{
	register char c;
	c = 0;
	while( cnt-- > 0 )  {
		while( !isprint((c&0x7F)) )  c++;
		*cp++ = (c++&0x7F);
	}
}

/*
 *			P R E P _ T I M E R
 */
void
prep_timer()
{
	gettimeofday(&time0, (struct timezone *)0);
	timep.tv_sec = time0.tv_sec;
	timep.tv_usec = time0.tv_usec;
	timepk.tv_sec = time0.tv_sec;
	timepk.tv_usec = time0.tv_usec;
	getrusage(RUSAGE_SELF, &ru0);
}

/*
 *			R E A D _ T I M E R
 *
 */
double
read_timer( char *str, int len )
{
	struct timeval timedol;
	struct rusage ru1;
	struct timeval td;
	struct timeval tend, tstart;
	char line[132];

	getrusage(RUSAGE_SELF, &ru1);
	gettimeofday(&timedol, (struct timezone *)0);
	prusage(&ru0, &ru1, &timedol, &time0, line);
	(void)strncpy( str, line, len );

	/* Get real time */
	tvsub( &td, &timedol, &time0 );
	realt = td.tv_sec + ((double)td.tv_usec) / 1000000;

	/* Get CPU time (user+sys) */
	tvadd( &tend, &ru1.ru_utime, &ru1.ru_stime );
	tvadd( &tstart, &ru0.ru_utime, &ru0.ru_stime );
	tvsub( &td, &tend, &tstart );
	cput = td.tv_sec + ((double)td.tv_usec) / 1000000;
	if( cput < 0.00001 )  cput = 0.00001;
	return( cput );
}

static void
prusage( register struct rusage *r0, register struct rusage *r1, struct timeval *e, struct timeval *b, char *outp )
{
	struct timeval tdiff;
	register time_t t;
	register char *cp;
	register int i;
	int ms;

	t = (r1->ru_utime.tv_sec-r0->ru_utime.tv_sec)*100+
	    (r1->ru_utime.tv_usec-r0->ru_utime.tv_usec)/10000+
	    (r1->ru_stime.tv_sec-r0->ru_stime.tv_sec)*100+
	    (r1->ru_stime.tv_usec-r0->ru_stime.tv_usec)/10000;
	ms =  (e->tv_sec-b->tv_sec)*100 + (e->tv_usec-b->tv_usec)/10000;

#define END(x)	{while(*x) x++;}

	if (format & PARSE)
		cp = "user=%U system=%S elapsed=%E cpu=%P memory=%Xi+%Dd-%Mmaxrss io=%F+%Rpf swaps=%Ccsw";
	else
		cp = "%Uuser %Ssys %Ereal %P %Xi+%Dd %Mmaxrss %F+%Rpf %Ccsw";

	for (; *cp; cp++)  {
		if (*cp != '%')
			*outp++ = *cp;
		else if (cp[1]) switch(*++cp) {

		case 'U':
			tvsub(&tdiff, &r1->ru_utime, &r0->ru_utime);
			sprintf(outp,"%ld.%01ld", (long)tdiff.tv_sec, (long)tdiff.tv_usec/100000);
			END(outp);
			break;

		case 'S':
			tvsub(&tdiff, &r1->ru_stime, &r0->ru_stime);
			sprintf(outp,"%ld.%01ld", (long)tdiff.tv_sec, (long)tdiff.tv_usec/100000);
			END(outp);
			break;

		case 'E':
			psecs(ms / 100, outp);
			END(outp);
			break;

		case 'P':
			sprintf(outp,"%d%%", (int) (t*100 / ((ms ? ms : 1))));
			END(outp);
			break;

		case 'W':
			i = r1->ru_nswap - r0->ru_nswap;
			sprintf(outp,"%d", i);
			END(outp);
			break;

		case 'X':
			sprintf(outp,"%ld", t == 0 ? 0 : (r1->ru_ixrss-r0->ru_ixrss)/t);
			END(outp);
			break;

		case 'D':
			sprintf(outp,"%ld", t == 0 ? 0 :
			    (r1->ru_idrss+r1->ru_isrss-(r0->ru_idrss+r0->ru_isrss))/t);
			END(outp);
			break;

		case 'K':
			sprintf(outp,"%ld", t == 0 ? 0 :
			    ((r1->ru_ixrss+r1->ru_isrss+r1->ru_idrss) -
			    (r0->ru_ixrss+r0->ru_idrss+r0->ru_isrss))/t);
			END(outp);
			break;

		case 'M':
			sprintf(outp,"%ld", r1->ru_maxrss/2);
			END(outp);
			break;

		case 'F':
			sprintf(outp,"%ld", r1->ru_majflt-r0->ru_majflt);
			END(outp);
			break;

		case 'R':
			sprintf(outp,"%ld", r1->ru_minflt-r0->ru_minflt);
			END(outp);
			break;

		case 'I':
			sprintf(outp,"%ld", r1->ru_inblock-r0->ru_inblock);
			END(outp);
			break;

		case 'O':
			sprintf(outp,"%ld", r1->ru_oublock-r0->ru_oublock);
			END(outp);
			break;
		case 'C':
			sprintf(outp,"%ld+%ld", r1->ru_nvcsw-r0->ru_nvcsw,
				r1->ru_nivcsw-r0->ru_nivcsw );
			END(outp);
			break;
		}
	}
	*outp = '\0';
}

static void
tvadd( struct timeval *tsum, struct timeval *t0, struct timeval *t1 )
{

	tsum->tv_sec = t0->tv_sec + t1->tv_sec;
	tsum->tv_usec = t0->tv_usec + t1->tv_usec;
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
}

static void
tvsub( struct timeval *tdiff, struct timeval *t1, struct timeval *t0 )
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

static void
psecs( long l, register char *cp )
{
	register int i;

	i = l / 3600;
	if (i) {
		sprintf(cp,"%d:", i);
		END(cp);
		i = l % 3600;
		sprintf(cp,"%d%d", (i/60) / 10, (i/60) % 10);
		END(cp);
	} else {
		i = l;
		sprintf(cp,"%d", i / 60);
		END(cp);
	}
	i %= 60;
	*cp++ = ':';
	sprintf(cp,"%d%d", i / 10, i % 10);
}

/*
 *			N R E A D
 */
int
Nread( int fd, char *buf, int count )
{
	struct sockaddr_storage from;
	socklen_t len = sizeof(from);
	struct timeval timed;	/* time delta */
	register int cnt;
	if( udp )  {
		cnt = recvfrom( fd, buf, count, 0, (struct sockaddr *)&from, &len );
		numCalls++;
	} else {
		if( b_flag )
			cnt = mread( fd, buf, count );	/* fill buf */
		else {
			cnt = read( fd, buf, count );
			numCalls++;
		}
	}
	if (do_owd && (cnt > 4)) {
		uint32_t secs, usecs;

		/* get transmitter timestamp */
		bcopy(buf + 8, &secs, 4);
		bcopy(buf + 12, &usecs, 4);
		timetx.tv_sec = ntohl(secs);
		timetx.tv_usec = ntohl(usecs);
		gettimeofday(&timerx, (struct timezone *)0);
		tvsub( &timed, &timerx, &timetx );
		owd = timed.tv_sec*1000 + ((double)timed.tv_usec)/1000;
		nowd++;
		if (owd < owd_min)
			owd_min = owd;
		if (owd > owd_max)
			owd_max = owd;
		owd_avg += owd;
		nowdi++;
		if (owd < owd_mini)
			owd_mini = owd;
		if (owd > owd_maxi)
			owd_maxi = owd;
		owd_avgi += owd;
	}
	return(cnt);
}

/*
 *			N W R I T E
 */
int
Nwrite( int fd, char *buf, int count )
{
	struct timeval timedol;
	struct timeval td;
	register int cnt = 0;
	double deltat;

	if (irate) {
		/* Get real time */
		gettimeofday(&timedol, (struct timezone *)0);
		tvsub( &td, &timedol, &timepk );
		deltat = td.tv_sec + ((double)td.tv_usec) / 1000000;

		if (deltat >= (1 + maxburst)*pkt_time) {
			timepk.tv_sec = timedol.tv_sec;
			timepk.tv_usec = timedol.tv_usec;
			irate_cum_nsec = 0;
			deltat = 0.0;
			nburst = 1;
		}

		if (nburst++ >= maxburst) {
			while ((maxburst*(double)count/rate/125 > deltat)
			       && !intr) {
				/* Get real time */
				gettimeofday(&timedol, (struct timezone *)0);
				tvsub( &td, &timedol, &timepk );
				deltat = td.tv_sec + ((double)td.tv_usec)
							/ 1000000;
			}
		}

		if (nburst > maxburst) {
			irate_cum_nsec += maxburst*irate_pk_nsec;
			while (irate_cum_nsec >= 1000.0) {
				irate_cum_nsec -= 1000.0;
				timepk.tv_usec++;
			}
			timepk.tv_usec += maxburst*irate_pk_usec;
			while (timepk.tv_usec >= 1000000) {
				timepk.tv_usec -= 1000000;
				timepk.tv_sec++;
			}
			nburst = 1;
		}
		if (intr && (!udp || (count != 4))) return(0);
	}
	else {
		while ((double)nbytes/realt/125 > rate) {
			/* Get real time */
			gettimeofday(&timedol, (struct timezone *)0);
			tvsub( &td, &timedol, &time0 );
			realt = td.tv_sec + ((double)td.tv_usec) / 1000000;
			if( realt <= 0.0 )  realt = 0.000001;
		}
	}
	if (do_owd && (count > 4)) {
		uint32_t secs, usecs;

		/* record transmitter timestamp in packet */
		gettimeofday(&timedol, (struct timezone *)0);
		secs = htonl(timedol.tv_sec);
		usecs = htonl(timedol.tv_usec);
		bcopy(&secs, buf + 8, 4);
		bcopy(&usecs, buf + 12, 4);
	}
	if( udp )  {
again:
		if (af == AF_INET) {
			cnt = sendto( fd, buf, count, 0, (struct sockaddr *)&sinhim[stream_idx + 1], sizeof(sinhim[stream_idx + 1]) );
		}
#ifdef AF_INET6
		else if (af == AF_INET6) {
			cnt = sendto( fd, buf, count, 0, (struct sockaddr *)&sinhim6[stream_idx + 1], sizeof(sinhim6[stream_idx + 1]) );
		}
#endif
		else {
			err("unsupported AF");
		}
		numCalls++;
		if( cnt<0 && errno == ENOBUFS )  {
			delay(18000);
			errno = 0;
			goto again;
		}
	} else {
		cnt = write( fd, buf, count );
		numCalls++;
	}
	return(cnt);
}

int
delay( int us )
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = us;
	(void)select( 1, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tv );
	return(1);
}

/*
 *			M R E A D
 *
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This can be necessary because
 * network connections don't deliver data with the same
 * grouping as it is written with.  Written by Robert S. Miles, BRL.
 */
int
mread( int fd, register char *bufp, unsigned n )
{
	register unsigned	count = 0;
	register int		nread;

	do {
		nread = read(fd, bufp, n-count);
		numCalls++;
		if(nread < 0)  {
			if (errno != EINTR)
				perror("nuttcp_mread");
			return(-1);
		}
		if(nread == 0)
			return((int)count);
		count += (unsigned)nread;
		bufp += nread;
	 } while(count < n);

	return((int)count);
}

/*
 *			G E T O P T V A L P
 *
 * This function returns a character pointer to the option value
 * pointed at by argv and sets skiparg to 1 if the option and its
 * value were passed as separate arguments (otherwise it sets
 * skiparg to 0).  index is the position within argv where the
 * option value resides if the option was specified as a single
 * argument.  reqval indicates whether or not the option requires
 * a value
 */
char *
getoptvalp( char **argv, int index, int reqval, int *skiparg )
{
	struct sockaddr_storage dummy;
	char **nextarg;

	*skiparg = 0;
	nextarg = argv + 1;

	/* if there is a value in the current arg return it */
	if (argv[0][index])
		return(&argv[0][index]);

	/* if there isn't a next arg return a pointer to the
	   current arg value (which will be an empty string) */
	if (*nextarg == NULL)
		return(&argv[0][index]);

	/* if the next arg is another option, and a value isn't
	 * required, return a pointer to the current arg value
	 * (which will be an empty string) */
	if ((**nextarg == '-') && !reqval)
		return(&argv[0][index]);

	/* if there is an arg after the next arg and it is another
	   option, return the next arg as the option value */
	if (*(nextarg + 1) && (**(nextarg + 1) == '-')) {
		*skiparg = 1;
		return(*nextarg);
	}

	/* if the option requires a value, return the next arg
	   as the option value */
	if (reqval) {
		*skiparg = 1;
		return(*nextarg);
	}

	/* if the next arg is an Ipv4 address, return a pointer to the
	   current arg value (which will be an empty string) */
	if (inet_pton(AF_INET, *nextarg, &dummy) > 0)
		return(&argv[0][index]);

#ifdef AF_INET6
	/* if the next arg is an Ipv6 address, return a pointer to the
	   current arg value (which will be an empty string) */
	if (inet_pton(AF_INET6, *nextarg, &dummy) > 0)
		return(&argv[0][index]);
#endif

	/* if the next arg begins with an alphabetic character,
	   assume it is a hostname and thus return a pointer to the
	   current arg value (which will be an empty string).
	   note all current options which don't require a value
	   have numeric values (start with a digit) */
	if (isalpha((int)(**nextarg)))
		return(&argv[0][index]);

	/* assume the next arg is the option value */
	*skiparg = 1;

	return(*nextarg);
}

#define PROC_SNMP		"/proc/net/snmp"
#define PROC_BUF_LEN		256
#define PROC_BUF_LEN2		128
#define NETSTAT			"netstat"

#if defined(linux)
#define RETRANS			"segments retransmited"
#define NETSTAT_DIR		"/bin/"
#define NRETRANS_BEFORE
#elif defined(__FreeBSD__)
#define RETRANS			"retransmitted"
#define NETSTAT_DIR		"/usr/bin/"
#define NRETRANS_BEFORE
#elif defined(__APPLE__) && defined(__MACH__)
#define RETRANS			"retransmitted"
#define NETSTAT_DIR		"/usr/sbin/"
#define NRETRANS_BEFORE
#elif defined(sparc)
#define RETRANS			"tcpRetransSegs"
#define NETSTAT_DIR		"/usr/bin/"
#elif defined(sgi)
#define RETRANS			"retransmitted"
#define NETSTAT_DIR		"/usr/etc/"
#define NRETRANS_BEFORE
#elif defined(__CYGWIN__) || defined(_WIN32)
#define RETRANS			"Segments Retransmitted"
#define NETSTAT_DIR		""
#else
#define RETRANS			"retransmitted"
#define NETSTAT_DIR		"/usr/bin/"
#define NRETRANS_BEFORE
#endif

char	proc_buf[PROC_BUF_LEN];
char	proc_buf2[PROC_BUF_LEN2];

int get_retrans( int sockfd )
{
	FILE	*proc_snmp;
	char	*cp, *cp2;
	int	num_retrans;
	int	pipefd[2];
	int	pidstat;
	pid_t	pid = 0;
	pid_t	wait_pid;

	if (retransinfo < 0)
		return(0);

#if defined(linux) && defined(TCPI_OPT_TIMESTAMPS)
	if ((retransinfo <= 1) && (sockfd >= 0)) {
		optlen = sizeof(tcpinf);
		if (getsockopt(sockfd, SOL_TCP, TCP_INFO, (void *)&tcpinf,
			       &optlen) == 0) {
			if (optlen >= SIZEOF_TCP_INFO_RETRANS) {
				retransinfo = 1;
				b_flag = 1;
				return(tcpinf.tcpi_total_retrans);
			}
		}
		if (retransinfo == 1) {
			retransinfo = -1;
			return(0);
		}
		retransinfo = 2;
	}
#endif

	if ((retransinfo == 3) || (!(proc_snmp = fopen(PROC_SNMP, "r")))) {
		retransinfo = 3;
		if (pipe(pipefd) != 0) {
			retransinfo = -1;
			return(0);
		}
		if ((pid = fork()) == (pid_t)-1) {
			perror("can't fork");
			close(pipefd[0]);
			close(pipefd[1]);
			retransinfo = -1;
			return(0);
		}
		if (pid == 0) {
			signal(SIGINT, SIG_DFL);
			close(1);
			close(2);
			dup(pipefd[1]);
			dup(pipefd[1]);
			close(pipefd[0]);
			close(pipefd[1]);
			execl(NETSTAT_DIR NETSTAT, NETSTAT, "-s", NULL);
			perror("execl failed");
			fprintf(stderr, "failed to execute %s%s -s\n",
				NETSTAT_DIR, NETSTAT);
			fflush(stdout);
			fflush(stderr);
			exit(0);
		}
		close(pipefd[1]);
		if (!(proc_snmp = fdopen(pipefd[0], "r"))) {
			close(pipefd[0]);
			retransinfo = -1;
			return(0);
		}
	}

	errno = 0;
	num_retrans = -1;
	while (fgets(proc_buf, sizeof(proc_buf), proc_snmp)) {
		if (retransinfo == 2) {
			if (strncmp(proc_buf, "Tcp:", 4) != 0)
				continue;
			if ((!fgets(proc_buf2, sizeof(proc_buf2), proc_snmp))
				|| (strncmp(proc_buf2, "Tcp:", 4) != 0))
				break;
			cp = proc_buf;
			cp2 = proc_buf2;
			while ((cp = strchr(cp, ' '))) {
				while (*++cp == ' ')
					;
				if (!(*cp))
					goto close;
				if (!(cp2 = strchr(cp2, ' ')))
					goto close;
				while (*++cp2 == ' ')
					;
				if (!(*cp2))
					goto close;
				if (strncmp(cp, "RetransSegs", 11) == 0) {
					if (!isdigit((int)(*cp2)))
						goto close;
					num_retrans = atoi(cp2);
					goto close;
				}
				else
					continue;
			}
		}
		else {
			if ((cp = strstr(proc_buf, RETRANS))) {
#ifdef NRETRANS_BEFORE
				num_retrans = atoi(proc_buf);
#else
				cp2 = strchr(cp, '=');
				cp2++;
				num_retrans = atoi(cp2);
#endif
				break;
			}
		}
	}

close:
	fclose(proc_snmp);
	if (retransinfo == 3) {
		while ((wait_pid = wait(&pidstat)) != pid) {
			if (wait_pid == (pid_t)-1) {
				if (errno == ECHILD)
					break;
				err("wait failed");
			}
		}
	}

	if (num_retrans < 0) {
		retransinfo = -1;
		return(0);
	}

	return(num_retrans);
}

#if defined(linux) && defined(TCPI_OPT_TIMESTAMPS)

void
print_tcpinfo()
{
	fprintf(stdout, "state = %d, ca_state = %d, retransmits = %d, "
			"unacked = %d, sacked = %d\n",
		tcpinf.tcpinfo_state, tcpinf.tcpinfo_ca_state,
		tcpinf.tcpinfo_retransmits, tcpinf.tcpinfo_unacked,
		tcpinf.tcpinfo_sacked);
	fprintf(stdout, "           lost = %d, retrans = %d, fackets = %d, "
			"rtt = %d, rttvar = %d\n",
		tcpinf.tcpinfo_lost, tcpinf.tcpinfo_retrans,
		tcpinf.tcpinfo_fackets, tcpinf.tcpinfo_rtt,
		tcpinf.tcpinfo_rttvar);
	fprintf(stdout, "           snd_ssthresh = %d, snd_cwnd = %d, "
			"total_retrans = %d\n",
		tcpinf.tcpinfo_snd_ssthresh, tcpinf.tcpinfo_snd_cwnd,
		tcpinf.tcpi_total_retrans);
	return;
}

#endif

