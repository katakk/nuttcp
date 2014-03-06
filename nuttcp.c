/*
 *	N U T T C P . C						v3.6.2
 *
 * Copyright(c) 2000 - 2003 Bill Fink.  All rights reserved.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Based on nttcp
 * Developed by Bill Fink, billfink@mindspring.com
 * Latest version available at:
 *	ftp://ftp.lcp.nrl.navy.mil/u/bill/beta/nuttcp/
 *
 * Test TCP connection.  Makes a connection on port 5001
 * and transfers fabricated buffers or data copied from stdin.
 *
 * Run nuttcp with no arguments to get a usage statement
 *
 * SYSV sections are here but intended to be removed
 *
 * Modified for operation under 4.2BSD, 18 Dec 84
 *      T.C. Slattery, USNA
 * Minor improvements, Mike Muuss and Terry Slattery, 16-Oct-85.
 *
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
 *	Handle <control-C> for UDP receiver
 *	Multicast support
 *	Drain UDP receive buffer (wait for fragmentation reassembly)
 *	Add "-ut" option to do both UDP and TCP simultaneously
 *	Default rate limit UDP if too much loss
 *	Watchdog for server to check if client has gone away
 *	QOS support
 *	Ping option
 *	Other brief output formats
 *	Linux window size bug/feature note
 *	Retransmission/timeout info
 *	Network interface interrupts (for Linux only)
 *	netstat -i info
 *	Man page
 *	Forking for multiple streams (or using select)
 *	Bidirectional option
 *	Graphical interface
 *	3rd party nuttcp
 *	IPv6 support
 *	OWD info
 *	RTT info
 *	Jitter info
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
 *      Public Domain.  Distribution Unlimited.
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/time.h>		/* struct timeval */
#include <limits.h>
#include <string.h>

#if defined(SYSV)
#include <sys/times.h>
#include <sys/param.h>
struct rusage {
    struct timeval ru_utime, ru_stime;
};
#define RUSAGE_SELF 0
#else
#include <sys/resource.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define uint64_t u_int64_t
#endif

static struct	timeval time0;	/* Time at which timing started */
static struct	timeval timepk;	/* Time at which last packet sent */
static struct	timeval timep;	/* Previous time - for interval reporting */
static struct	rusage ru0;	/* Resource utilization at the start */

static struct	sigaction sigact;	/* signal handler for alarm */

static void prusage();
static void tvadd();
static void tvsub();
static void psecs();

#define PERF_FMT_OUT	  "%.3f MB in %.2f real seconds = %.2f KB/sec" \
			  " = %.4f Mb/s\n"
#define PERF_FMT_BRIEF	  "%.3f MB / %.2f sec = %.4f Mbps %d %%TX %d %%RX"
#define PERF_FMT_BRIEF2	  "%.3f MB / %.2f sec = %.4f Mbps %d %%%s"
#define PERF_FMT_BRIEF3	  " Trans: %.3f MB"
#define PERF_FMT_INTERVAL  "%.4f MB / %5.2f sec = %9.4f Mbps"
#define PERF_FMT_INTERVAL2 " Tot: %.4f MB / %5.2f sec = %9.4f Mbps"
#define PERF_FMT_INTERVAL3 " Trans: %.4f MB"
#define PERF_FMT_INTERVAL4 " Tot: %.4f MB"
#define PERF_FMT_IN	  "%lf MB in %lf real seconds = %lf KB/sec = %lf Mb/s\n"
#define CPU_STATS_FMT_IN  "%*fuser %*fsys %*d:%*dreal %d%%"
#define CPU_STATS_FMT_IN2 "%*fuser %*fsys %*d:%*d:%*dreal %d%%"

#define LOSS_FMT	" %.2f%% data loss"
#define LOSS_FMT_BRIEF	" %.2f %%loss"
#define LOSS_FMT_INTERVAL " %5.2f ~%%loss"
#define DROP_FMT	" %lld / %lld drop/pkt"
#define DROP_FMT_BRIEF	" %lld / %lld drop/pkt"
#define DROP_FMT_INTERVAL " %5lld / %5lld ~drop/pkt"

#define HELO_FMT	"HELO nuttcp v%d.%d.%d\n"

#ifndef MAXSTREAM
#define MAXSTREAM		128
#endif
#define DEFAULT_NBUF		2048
#define DEFAULTUDPBUFLEN	8192
#define MAXUDPBUFLEN		65507
#define MINMALLOC		1024

#define XMITSTATS		0x1	/* also give transmitter stats (MB) */
#define DEBUGINTERVAL		0x2	/* add info to assist with
					 * debugging interval reports */
#define	RUNNINGTOTAL		0x4	/* give cumulative stats for "-i" */
#define	NODROPS			0x8	/* give packet drop stats for "-i" */
#define	NOPERCENTLOSS		0x10	/* don't give percent loss for "-i" */

void mes(char *s);
void err(char *s);
void pattern(char *cp, int cnt);
#if defined(SYSV)
static void getrusage(int ignored, struct rusage *ru);
#endif

int vers_major = 3;
int vers_minor = 6;
int vers_delta = 2;
int ivers;
int rvers_major = 0;
int rvers_minor = 0;
int rvers_delta = 0;
int irvers;

struct sockaddr_in sinme[MAXSTREAM + 1];
struct sockaddr_in sinhim[MAXSTREAM + 1];
struct sockaddr_in frominet;

int domain, fromlen;
int fd[MAXSTREAM + 1];		/* fd array of network sockets */
int nfd;			/* fd for accept call */

int buflen = 64 * 1024;		/* length of buffer */
int nbuflen;
int mallocsize;
char *buf;			/* ptr to dynamic buffer */
int nbuf = 0;			/* number of buffers to send in sinkmode */

/*  nick code  */
int sendwin, sendwinval, origsendwin, optlen;
int rcvwin, rcvwinval, origrcvwin, maxseg;
int srvrwin;
/*  end nick code  */

int udp = 0;			/* 0 = tcp, !0 = udp */
int udplossinfo = 0;		/* set to 1 to give UDP loss info for
				 * interval reporting */
int need_swap;			/* client and server are different endian */
int options = 0;		/* socket options */
int one = 1;                    /* for 4.3 BSD style setsockopt() */
unsigned short port = 5001;	/* TCP port number */
unsigned short ctlport = 5000;	/* control port for server connection */
int tmpport;
char *host;			/* ptr to name of host */
int trans = 0;			/* 0=receive, !0=transmit mode */
int sinkmode = 1;		/* 0=normal I/O, !0=sink/source mode */
int verbose = 0;		/* 0=print basic info, 1=print cpu rate, proc
				 * resource usage. */
int nodelay = 0;		/* set TCP_NODELAY socket option */
unsigned long rate = ~0;	/* transmit rate limit in Kbps */
int irate = 0;			/* instantaneous rate limit if set */
double timeout = 0.0;		/* timeout interval in seconds */
double interval = 0.0;		/* interval timer in seconds */
char intervalbuf[256+2];	/* buf for interval reporting */
int reverse = 0;		/* reverse direction of data connection open */
int format = 0;			/* controls formatting of output */
int traceroute = 0;		/* do traceroute back to client if set */
int skip_data = 0;		/* skip opening of data channel */
int priority = 0;		/* nuttcp process priority */
long timeout_sec = 0;
struct itimerval itimer;	/* for setitimer */
int srvr_helo = 1;		/* set to 0 if server doesn't send HELO */
char *ident = "";		/* identifier for nuttcp output */
int intr = 0;
int abortconn = 0;
int braindead = 0;		/* for braindead Solaris 2.8 systems */
int brief = 0;			/* set for brief output */
int done = 0;			/* don't output interval report if done */
int got_begin = 0;		/* don't output interval report if not begun */
int buflenopt = 0;		/* whether or not user specified buflen */
int clientserver = 0;		/* client server mode (use control channel) */
int client = 0;			/* 0=server side, 1=client (initiator) side */
int oneshot = 0;		/* 1=run server only once */
int inetd = 0;			/* set to 1 if server run from inetd */
pid_t pid;			/* process id when forking server process */
pid_t wait_pid;			/* return of wait system call */
int pidstat;			/* status of forked process */
FILE *ctlconn;			/* uses fd[0] for control channel */
int savestdout;			/* used to save real standard out */
int realstdout;			/* used for "-s" output to real standard out */
int firsttime = 1;		/* flag for first pass through server */
struct in_addr clientaddr;	/* IP address of client connecting to server */

struct hostent *addr;
extern int errno;

char Usage[] = "\
Usage: nuttcp or nuttcp -h	prints this usage info\n\
Usage: nuttcp -V		prints version info\n\
Usage: nuttcp -xt		do traceroute back to client\n\
Usage (transmitter): nuttcp -t [-options] host [ <in ]\n\
	-l##	length of network write buf (default 8192/udp, 65536/tcp)\n\
	-s	don't source a pattern to network, use stdin\n\
	-n##	number of source bufs written to network (default 2048)\n\
	-w##	transmitter window size in KB\n\
	-ws##	server receive window size in KB\n\
	-wb	braindead Solaris 2.8 (sets both xmit and rcv windows)\n\
	-p##	port number to send to (default 5001)\n\
	-P##	port number for control connection (default 5000)\n\
	-u	use UDP instead of TCP\n\
	-D	don't buffer TCP writes (sets TCP_NODELAY socket option)\n\
	-N##	number of streams (starting at port number)\n\
	-R##	transmit rate limit in Kbps (or (m|M)bps or (g|G)bps)\n\
	-T##	transmit timeout interval in seconds (or (m|M)inutes)\n\
	-i##	server interval reporting in seconds (or (m|M)inutes)\n\
	-Ixxx	identifier for nuttcp output (max of 40 characters)\n\
	-F	flip option to reverse direction of data connection open\n\
	-xP##	set nuttcp process priority (must be root)\n\
	-d	set TCP SO_DEBUG option on data socket\n\
	-v	verbose output\n\
	-b	brief output\n\
Usage (receiver): nuttcp -r [-options] [host] [ >out]\n\
	-l##	length of network read buf (default 8192/udp, 65536/tcp)\n\
	-s	don't sink (discard): prints all data from network to stdout\n\
	-n##	number of bufs for server to write to network (default 2048)\n\
	-w##	receiver window size in KB\n\
	-ws##	server transmit window size in KB\n\
	-wb	braindead Solaris 2.8 (sets both xmit and rcv windows)\n\
	-p##	port number to listen at (default 5001)\n\
	-P##	port number for control connection (default 5000)\n\
	-B	Only output full blocks, as specified in -l## (for TAR)\n\
	-u	use UDP instead of TCP\n\
	-N##	number of streams (starting at port number), implies -B\n\
	-R##	server transmit rate limit in Kbps (or (m|M)bps or (g|G)bps)\n\
	-T##	server transmit timeout interval in seconds (or (m|M)inutes)\n\
	-i##	client interval reporting in seconds (or (m|M)inutes)\n\
	-Ixxx	identifier for nuttcp output (max of 40 characters)\n\
	-F	flip option to reverse direction of data connection open\n\
	-xP##	set nuttcp process priority (must be root)\n\
	-d	set TCP SO_DEBUG option on data socket\n\
	-v	verbose output\n\
	-b	brief output\n\
Usage (server): nuttcp -S [-options]\n\
		note server mode excludes use of -s\n\
	-1	oneshot server mode (implied with inetd/xinetd), implies -S\n\
	-P##	port number for server connection (default 5000)\n\
		note don't use with inetd/xinetd (use services file instead)\n\
	-xP##	set nuttcp process priority (must be root)\n\
Format options:\n\
	-fxmitstats	also give transmitter stats (MB) with -i (UDP only)\n\
	-frunningtotal	also give cumulative stats on interval reports\n\
	-f-drops	don't give packet drop info on brief output (UDP)\n\
	-f-percentloss	don't give %%loss info on brief output (UDP)\n\
";	

char stats[128];
char srvrbuf[4096];
char tmpbuf[257];
uint64_t nbytes = 0;		/* bytes on net */
uint64_t pbytes = 0;		/* previous bytes - for interval reporting */
uint64_t ntbytes = 0;		/* bytes sent by transmitter */
uint64_t ptbytes = 0;		/* previous bytes sent by transmitter */
uint64_t ntbytesc = 0;		/* bytes sent by transmitter that have
				 * been counted */
int numCalls = 0;		/* # of NRead/NWrite calls. */
int nstream = 1;		/* number of streams */
int stream_idx = 0;		/* current stream */
int start_idx = 1;		/* set to use or bypass control channel */
int b_flag = 0;			/* use mread() */
int got_srvr_output = 0;	/* set when server output has been read */
double srvr_MB;
double srvr_realt;
double srvr_KBps;
double srvr_Mbps;
int srvr_cpu_util;

void prep_timer();
double read_timer();
double cput = 0.000001, realt = 0.000001;	/* user, real time (seconds) */
double realtd = 0.000001;	/* real time delta - for interval reporting */

void
sigpipe()
{
	signal(SIGPIPE, sigpipe);
}

void
sigint()
{
	signal(SIGINT, SIG_DFL);
	fputs("\n*** transfer interrupted ***\n", stdout);
	intr = 1;
	done++;
	return;
}

void
sigalarm()
{
	struct	timeval timec;	/* Current time */
	struct	timeval timed;	/* Delta time */
	struct	timeval timet;	/* Transmitter time */
	uint64_t nrbytes;
	int i;
	char *cp1, *cp2;

	if (interval && !trans) {
		if ((udp && !got_begin) || done)
			return;
		if (clientserver && !trans) {
			/* Get real time */
			gettimeofday(&timec, (struct timezone *)0);
			tvsub( &timed, &timec, &timep );
			realtd = timed.tv_sec + ((double)timed.tv_usec)
								/ 1000000;
			if( realtd <= 0.0 )  realtd = 0.000001;
			tvsub( &timed, &timec, &time0 );
			realt = timed.tv_sec + ((double)timed.tv_usec)
							    / 1000000;
			if( realt <= 0.0 )  realt = 0.000001;
			nrbytes = nbytes;
			if (udplossinfo) {
				ntbytes = *(unsigned long long *)(buf + 24);
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
			if (*ident)
				fprintf(stdout, "%s: ", ident + 1);
			fprintf(stdout, PERF_FMT_INTERVAL,
				(double)(nrbytes - pbytes)/(1024*1024), realtd,
				(double)(nrbytes - pbytes)/realtd/125000);
			if (udplossinfo) {
				if (!(format & NODROPS))
					fprintf(stdout, DROP_FMT_INTERVAL,
						((ntbytes - ptbytes)
							- (nrbytes - pbytes))
								/buflen,
						(ntbytes - ptbytes)/buflen);
				if (!(format & NOPERCENTLOSS))
					fprintf(stdout, LOSS_FMT_INTERVAL,
						ntbytes == ptbytes ? 0.0 :
						((1 - (double)(nrbytes - pbytes)
						  /(double)(ntbytes - ptbytes))
							*100));
			}
			if (format & RUNNINGTOTAL) {
				fprintf(stdout, PERF_FMT_INTERVAL2,
					(double)nrbytes/(1024*1024), realt,
					(double)nrbytes/realt/125000 );
				if (udplossinfo) {
					if (!(format & NODROPS))
						fprintf(stdout,
							DROP_FMT_INTERVAL,
							(ntbytes - nrbytes)
								/buflen,
							ntbytes/buflen);
					if (!(format & NOPERCENTLOSS))
						fprintf(stdout,
							LOSS_FMT_INTERVAL,
							ntbytes == 0 ? 0.0 :
							((1 - (double)nrbytes
							  /(double)ntbytes)
								*100));
				}
			}
			if (udplossinfo && (format & XMITSTATS)) {
				fprintf(stdout, PERF_FMT_INTERVAL3,
					(double)(ntbytes - ptbytes)/1024/1024);
				if (format & RUNNINGTOTAL) {
					fprintf(stdout, PERF_FMT_INTERVAL4,
						(double)ntbytes/1024/1024);
					if (format & DEBUGINTERVAL)
						fprintf(stdout, " Pre: %.4f MB",
							(double)ntbytesc
								  /1024/1024);
				}
			}
			fprintf(stdout, "\n");
			fflush(stdout);
/*			beginnings of timestamps - not ready for prime time */
/*			bcopy(buf + 8, &timet.tv_sec, 4);		*/
/*			bcopy(buf + 12, &timet.tv_usec, 4);		*/
/*			tvsub( &timed, &timec, &timet );		*/
/*			realt = timed.tv_sec + ((double)timed.tv_usec)	*/
/*							    / 1000000;	*/
/*			if( realt <= 0.0 )  realt = 0.000001;		*/
/*			fprintf(stdout, "%.3f ms-OWD timet = %08X/%08X timec = %08X/%08X\n", */
/*				realt*1000, timet.tv_sec, timet.tv_usec, */
/*				timec.tv_sec, timec.tv_usec);		*/
/*			fprintf(stdout, "%.3f ms-OWD\n", realt*1000);	*/
/*			fflush(stdout);					*/
			timep.tv_sec = timec.tv_sec;
			timep.tv_usec = timec.tv_usec;
			pbytes = nrbytes;
			ptbytes = ntbytes;
		}
	}
	else
		intr = 1;
	return;
}

void
sigterm()
{
	return;
}

main(argc,argv)
int argc;
char **argv;

{
	unsigned long addr_tmp;
	double MB;
	int cpu_util;
	int first_read;
	int correction;
	int i;
	char *cp1, *cp2;
	char ch;

/*  nick code  */
optlen = sizeof(maxseg);
/* end of nick code  */

	sendwin = 0;
	rcvwin = 0;
	srvrwin = -1;

	if (argc < 2) goto usage;

	argv++; argc--;
	while( argc>0 && argv[0][0] == '-' )  {
		switch (argv[0][1]) {

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
			nbuf = atoi(&argv[0][2]);
			if (nbuf < 0) {
				fprintf(stderr, "invalid nbuf = %d\n", nbuf);
				fflush(stderr);
				exit(1);
			}
			break;
		case 'l':
			buflen = atoi(&argv[0][2]);
			buflenopt = 1;
			if (buflen < 1) {
				fprintf(stderr, "invalid buflen = %d\n", buflen);
				fflush(stderr);
				exit(1);
			}
			break;
		case 'w':
			if (argv[0][2] == 's') {
				srvrwin = 1024 * atoi(&argv[0][3]);
				if (srvrwin < 0) {
					fprintf(stderr, "invalid srvrwin = %d\n", srvrwin);
					fflush(stderr);
					exit(1);
				}
			}
			else {
				if (argv[0][2] == 'b') {
					braindead = 1;
					if (argv[0][3])
						sendwin = 1024 *
							  atoi(&argv[0][3]);
					else
						break;
				}
				else
					sendwin = 1024 * atoi(&argv[0][2]);

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
			tmpport = atoi(&argv[0][2]);
			if ((tmpport < 5001) || (tmpport > 65535)) {
				fprintf(stderr, "invalid port = %d\n", tmpport);
				fflush(stderr);
				exit(1);
			}
			port = tmpport;
			break;
		case 'P':
			tmpport = atoi(&argv[0][2]);
			if ((tmpport < 5000) || (tmpport > 65535)) {
				fprintf(stderr, "invalid ctlport = %d\n", tmpport);
				fflush(stderr);
				exit(1);
			}
			ctlport = tmpport;
			break;
		case 'u':
			udp = 1;
			if (!buflenopt) buflen = DEFAULTUDPBUFLEN;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'N':
			nstream = atoi(&argv[0][2]);
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
			if (nstream > 1) b_flag = 1;
			break;
		case 'R':
			if (argv[0][2] == 'i') {
				rate = atoi(&argv[0][3]);
				irate = 1;
			}
			else
				rate = atoi(&argv[0][2]);
			if (rate == 0)
				rate = ~0;
			ch = argv[0][strlen(argv[0]) - 1];
			if ((ch == 'm') || (ch == 'M'))
				rate *= 1000;
			else if ((ch == 'g') || (ch == 'G'))
				rate *= 1000000;
			break;
		case 'T':
			sscanf(&argv[0][2], "%lf", &timeout);
			if (timeout < 0) {
				fprintf(stderr, "invalid timeout = %f\n", timeout);
				fflush(stderr);
				exit(1);
			}
			ch = argv[0][strlen(argv[0]) - 1];
			if ((ch == 'm') || (ch == 'M'))
				timeout *= 60;
			itimer.it_value.tv_sec = timeout;
			itimer.it_value.tv_usec =
				(timeout - itimer.it_value.tv_sec)*1000000;
			if (timeout && !nbuf)
				nbuf = INT_MAX;
			break;
		case 'i':
			sscanf(&argv[0][2], "%lf", &interval);
			if (interval < 0) {
				fprintf(stderr, "invalid interval = %f\n", interval);
				fflush(stderr);
				exit(1);
			}
			ch = argv[0][strlen(argv[0]) - 1];
			if ((ch == 'm') || (ch == 'M'))
				interval *= 60;
			break;
		case 'I':
			ident = &argv[0][1];
			*ident = '-';
			if (strlen(ident) > 41)
				*(ident + 41) = '\0';
			break;
		case 'F':
			reverse = 1;
			break;
		case 'b':
			if (argv[0][2])
				brief = atoi(&argv[0][2]);
			else
				brief = 1;
			break;
		case 'S':
			clientserver = 1;
			verbose = 1;
			break;
		case '1':
			oneshot = 1;
			clientserver = 1;
			verbose = 1;
			break;
		case 'V':
			fprintf(stdout, "nuttcp v%d.%d.%d\n", vers_major,
					vers_minor, vers_delta);
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
			if (argv[0][2] == 't') {
				traceroute = 1;
				brief = 1;
			}
			else if (argv[0][2] == 'P')
				priority = atoi(&argv[0][3]);
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
		case 'h':
		default:
			goto usage;
		}
		argv++; argc--;
	}

	if (argc > 1) goto usage;
	if (trans && (argc != 1)) goto usage;
	if (clientserver && (argc != 0)) goto usage;

	bzero((char *)&frominet, sizeof(frominet));
	bzero((char *)&clientaddr, sizeof(clientaddr));

	if (!nbuf)
		nbuf = DEFAULT_NBUF;

	if ((port < 5000) || ((port + nstream - 1) > 65535)) {
		fprintf(stderr, "invalid port/nstream = %d/%d\n", port, nstream);
		fflush(stderr);
		exit(1);
	}

	if ((ctlport >= port) && (ctlport <= (port + nstream - 1))) {
		fprintf(stderr, "ctlport = %d overlaps port/nstream = %d/%d\n", ctlport, port, nstream);
		fflush(stderr);
		exit(1);
	}

	if (srvrwin == -1) {
		srvrwin = sendwin;
	}

	if (clientserver) {
		if (trans) {
			fprintf(stderr, "server mode only allowed for receiver\n");
			goto usage;
		}
		udp = 0;
		sinkmode = 1;
		start_idx = 0;
		ident = "";
		{ struct sockaddr_in peer;
		  int peerlen = sizeof(peer);
		  if (getpeername(0, (struct sockaddr *) &peer, 
				&peerlen) == 0) {
			clientaddr = peer.sin_addr;
			inetd = 1;
			oneshot = 1;
			start_idx = 1;
		  }
		}
	}

	if (clientserver && !inetd) {
		if ((pid = fork()) == (pid_t)-1)
			err("can't fork");
		if (pid != 0)
			exit(0);
	}

	if (priority) {
		if (setpriority(PRIO_PROCESS, 0, priority) != 0)
			err("couldn't change priority");
	}

	if (argc == 1) {
		start_idx = 0;
		client = 1;
		clientserver = 1;
	}

	if (argc == 1) host = argv[0];

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

	if (udp && (buflen < 5)) {
	    fprintf(stderr, "UDP buflen = %d < 5, set to 5\n", buflen);
	    buflen = 5;		/* send more than the sentinel size */
	}

	if (udp && (buflen > MAXUDPBUFLEN)) {
	    fprintf(stderr, "UDP buflen = %d > MAXUDPBUFLEN, set to %d\n",
		buflen, MAXUDPBUFLEN);
	    buflen = MAXUDPBUFLEN;
	}

	if (udp && interval && (buflen >= 32))
		udplossinfo = 1;

	ivers = vers_major*10000 + vers_minor*100 + vers_delta;

	mallocsize = buflen;
	if (mallocsize < MINMALLOC) mallocsize = MINMALLOC;
	if( (buf = (char *)malloc(mallocsize)) == (char *)NULL)
		err("malloc");

	pattern( buf, buflen );

	signal(SIGPIPE, sigpipe);
	signal(SIGINT, sigint);

doit:
	for ( stream_idx = start_idx; stream_idx <= nstream; stream_idx++ ) {
		if (clientserver && (stream_idx == 1)) {
			if (client) {
				if (!(ctlconn = fdopen(fd[0], "w")))
					err("fdopen: ctlconn for writing");
				close(0);
				dup(fd[0]);
				if (srvr_helo) {
					fprintf(ctlconn,
						HELO_FMT, vers_major,
						vers_minor, vers_delta);
					fflush(ctlconn);
					if (!fgets(buf, mallocsize, stdin)) {
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
				fprintf(ctlconn, "buflen = %d, nbuf = %d, win = %d, nstream = %d, rate = %lu, port = %hu, trans = %d, braindead = %d", buflen, nbuf, srvrwin, nstream, rate, port, trans, braindead);
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
				fprintf(ctlconn, "\n");
				fflush(ctlconn);
				if (abortconn) {
					brief = 1;
					if (irvers >= 30601)
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
					savestdout=dup(1);
					close(1);
					dup(fd[0]);
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
				if (sscanf(buf, "buflen = %d, nbuf = %d, win = %d, nstream = %d, rate = %lu, port = %hu, trans = %d, braindead = %d, timeout = %lf, udp = %d, vers = %d.%d.%d", &nbuflen, &nbuf, &sendwin, &nstream, &rate, &port, &trans, &braindead, &timeout, &udp, &rvers_major, &rvers_minor, &rvers_delta) < 13) {
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
				trans = !trans;
				if (nbuflen != buflen) {
					buflen = nbuflen;
					if (buflen < 1) {
						fputs("KO\n", stdout);
						mes("invalid buflen");
						fprintf(stdout, "buflen = %d\n", buflen);
						fputs("KO\n", stdout);
						goto cleanup;
					}
					free(buf);
					mallocsize = buflen;
					if (mallocsize < MINMALLOC) mallocsize = MINMALLOC;
					if( (buf = (char *)malloc(mallocsize)) == (char *)NULL)
						err("malloc");
					pattern( buf, buflen );
				}
				if (nbuf < 1) {
					fputs("KO\n", stdout);
					mes("invalid nbuf");
					fprintf(stdout, "nbuf = %d\n", nbuf);
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
				if (nstream > 1) b_flag = 1;
				if (rate == 0)
					rate = ~0;
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
				if ((port < 5000) || ((port + nstream - 1) > 65535)) {
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
				if (interval < 0) {
					fputs("KO\n", stdout);
					mes("invalid interval");
					fprintf(stdout, "interval = %f\n", interval);
					fputs("KO\n", stdout);
					goto cleanup;
				}
				fprintf(stdout, "OK v%d.%d.%d\n", vers_major,
						vers_minor, vers_delta);
				fflush(stdout);
				if (udp && interval && (buflen >= 32) &&
					(irvers >= 30403))
					udplossinfo = 1;
				if ((trans && !reverse) || (!trans && reverse))
					usleep(50000);
			}
		}

		if (!client)
			host = (char *)inet_ntoa(clientaddr);

		if ((stream_idx > 0) && skip_data)
			break;

		bzero((char *)&sinme[stream_idx], sizeof(sinme[stream_idx]));
		bzero((char *)&sinhim[stream_idx], sizeof(sinhim[stream_idx]));
		if (((trans && !reverse) && (stream_idx > 0)) ||
		    ((!trans && reverse) && (stream_idx > 0)) ||
		    (client && (stream_idx == 0))) {
			/* xmitr initiates connections (unless reversed) */
			if (client) {
				if (atoi(host) > 0 )  {
					/* Numeric */
					sinhim[stream_idx].sin_family = AF_INET;
#ifdef cray
					addr_tmp = inet_addr(host);
					sinhim[stream_idx].sin_addr = addr_tmp;
#else
					sinhim[stream_idx].sin_addr.s_addr = inet_addr(host);
#endif
				} else {
					if ((addr=gethostbyname(host)) == NULL)
						err("bad hostname");
					sinhim[stream_idx].sin_family = addr->h_addrtype;
					bcopy(addr->h_addr,(char*)&addr_tmp, addr->h_length);
#ifdef cray
					sinhim[stream_idx].sin_addr = addr_tmp;
#else
#if _MIPS_SZLONG==64
					bcopy((char *)&addr_tmp,(char *)&sinhim[stream_idx].sin_addr.s_addr,
						sizeof(sinhim[stream_idx].sin_addr.s_addr));
#else
					sinhim[stream_idx].sin_addr.s_addr = addr_tmp;
#endif /* _MIPS_SZ_LONG==64 */
#endif /* cray */
				}
			}
			else {
				sinhim[stream_idx].sin_family = AF_INET;
				sinhim[stream_idx].sin_addr = clientaddr;
			}
			if (stream_idx == 0)
				sinhim[stream_idx].sin_port = htons(ctlport);
			else
				sinhim[stream_idx].sin_port = htons(port + stream_idx - 1);
			sinme[stream_idx].sin_port = 0;		/* free choice */
		} else {
			/* rcvr listens for connections (unless reversed) */
			if (stream_idx == 0)
				sinme[stream_idx].sin_port =  htons(ctlport);
			else
				sinme[stream_idx].sin_port =  htons(port + stream_idx - 1);
		}
		sinme[stream_idx].sin_family = AF_INET;

		if ((fd[stream_idx] = socket(AF_INET, (udp && (stream_idx != 0))?SOCK_DGRAM:SOCK_STREAM, 0)) < 0)
			err("socket");

		if (stream_idx == nstream) {
			if (!sinkmode && !trans) {
				realstdout = dup(1);
				close(1);
				dup(2);
			}
			if (brief <= 0)
				mes("socket");
			if (priority && (brief <= 0)) {
				errno = 0;
				priority = getpriority(PRIO_PROCESS, 0);
				if (errno)
					mes("couldn't get priority");
				else
					fprintf(stdout,
						"nuttcp%s%s: priority = %d\n",
						trans ? "-t" : "-r", ident,
						priority);
			}
			if (trans) {
			    if (brief <= 0) {
				fprintf(stdout,"nuttcp-t%s: buflen=%d, ",
					ident, buflen);
				if (nbuf != INT_MAX)
				    fprintf(stdout,"nbuf=%d, ", nbuf);
				fprintf(stdout,"nstream=%d, port=%d %s -> %s\n",
				    nstream, port,
				    udp?"udp":"tcp",
				    host);
				if (timeout)
				    fprintf(stdout,"nuttcp-t%s: time limit = %.2f second%s\n",
					ident, timeout,
					(timeout == 1.0)?"":"s");
				if (rate != ~0)
				    fprintf(stdout,"nuttcp-t%s: rate limit = %.3f Mb/s (%s)\n",
					ident, (double)rate/1000,
					irate ? "instantaneous" : "aggregate");
			    }
			} else {
			    if (brief <= 0) {
				fprintf(stdout,"nuttcp-r%s: buflen=%d, ",
					ident, buflen);
				if (nbuf != INT_MAX)
				    fprintf(stdout,"nbuf=%d, ", nbuf);
				fprintf(stdout,"nstream=%d, port=%d %s\n",
				    nstream, port,
				    udp?"udp":"tcp");
				if (interval)
				    fprintf(stdout,"nuttcp-r%s: interval reporting every %.2f second%s\n",
					ident, interval,
					(interval == 1.0)?"":"s");
			    }
			}
		}

		if( setsockopt(fd[stream_idx], SOL_SOCKET, SO_REUSEADDR,
			(void *)&one, sizeof(one)) < 0)
				err("setsockopt: so_reuseaddr");

		if (bind(fd[stream_idx], (struct sockaddr *)&sinme[stream_idx], sizeof(sinme[stream_idx])) < 0)
			err("bind");

		if (stream_idx > 0)  {
		    if (trans) {
			/* Set the transmitter options */
			if (sendwin) {
				if( setsockopt(fd[stream_idx], SOL_SOCKET, SO_SNDBUF,
					(void *)&sendwin, sizeof(sendwin)) < 0)
					err("setsockopt");
				if (braindead && (setsockopt(fd[stream_idx], SOL_SOCKET, SO_RCVBUF,
					(void *)&rcvwin, sizeof(rcvwin)) < 0))
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
					err("setsockopt");
				if (braindead && (setsockopt(fd[stream_idx], SOL_SOCKET, SO_SNDBUF,
					(void *)&sendwin, sizeof(sendwin)) < 0))
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
				if( setsockopt(fd[stream_idx], SOL_SOCKET, options, &one, sizeof(one)) < 0)
					err("setsockopt");
			}
			usleep(20000);
			if(connect(fd[stream_idx], (struct sockaddr *)&sinhim[stream_idx], sizeof(sinhim[stream_idx]) ) < 0) {
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
					if (interval) {
						fprintf(stderr, "interval option only supported for client/server mode\n");
						fflush(stderr);
						exit(1);
					}
					if (reverse) {
						fprintf(stderr, "flip option only supported for client/server mode\n");
						fflush(stderr);
						exit(1);
					}
					if (traceroute) {
						fprintf(stderr, "traceroute option only supported for client/server mode\n");
						fflush(stderr);
						exit(1);
					}
				}
			}
			if ((stream_idx == nstream) && (brief <= 0)) {
				fprintf(stdout,"nuttcp%s%s: connect to %s\n", 
					trans?"-t":"-r", ident,
					(char *)inet_ntoa(sinhim[stream_idx].sin_addr));
			}
		    } else {
			/* The receiver listens for the connection
			 * (unless reversed by the flip option)
			 */
			listen(fd[stream_idx],1);   /* allow a queue of 1 */
			if (options && (stream_idx > 0))  {
				if( setsockopt(fd[stream_idx], SOL_SOCKET, options, (void *)&one, sizeof(one)) < 0)
					err("setsockopt");
			}
			fromlen = sizeof(frominet);
			domain = AF_INET;
			if((nfd=accept(fd[stream_idx], (struct sockaddr *)&frominet, &fromlen) ) < 0)
				err("accept");
			close(fd[stream_idx]);
			fd[stream_idx]=nfd;
			{ struct sockaddr_in peer;
			  int peerlen = sizeof(peer);
			  if (getpeername(fd[stream_idx], (struct sockaddr *) &peer, 
					&peerlen) < 0) {
				err("getpeername");
			  }
			if ((stream_idx == nstream) && (brief <= 0))
				fprintf(stdout,"nuttcp%s%s: accept from %s\n", 
					trans?"-t":"-r", ident,
					(char *)inet_ntoa(peer.sin_addr));
			if (stream_idx == 0) clientaddr = peer.sin_addr;
			}
		    }
		}
		if (getsockopt(fd[stream_idx], SOL_SOCKET, SO_SNDBUF,  (void *)&sendwinval,
			 &optlen) < 0)
				err("get send window size didn't work\n");
		if (getsockopt(fd[stream_idx], SOL_SOCKET, SO_RCVBUF,  (void *)&rcvwinval,
			 &optlen) < 0)
				err("Get recv. window size didn't work\n");
		if ((stream_idx == nstream) && (brief <= 0))
			fprintf(stdout,"nuttcp%s%s: send window size = %d, receive window size = %d\n", trans?"-t":"-r", ident, sendwinval, rcvwinval);
		if (firsttime) {
			firsttime = 0;
			origsendwin = sendwinval;
			origrcvwin = rcvwinval;
		}
	}
	if (abortconn)
		exit(1);
	if (trans && timeout) {
		itimer.it_value.tv_sec = timeout;
		itimer.it_value.tv_usec =
			(timeout - itimer.it_value.tv_sec)*1000000;
		signal(SIGALRM, sigalarm);
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
	}

	if (interval && clientserver && client && trans) {
		if ((pid = fork()) == (pid_t)-1)
			err("can't fork");
		if (pid == 0) {
			signal(SIGALRM, SIG_IGN);
			signal(SIGINT, SIG_DFL);
			signal(SIGTERM, sigterm);

			/* don't gobble up parent's input too
			 * set stdin unbuffered */
			setbuf(stdin, NULL);

			for ( stream_idx = 1; stream_idx <= nstream;
					      stream_idx++ )
				close(fd[stream_idx]);

			*srvrbuf = '\0';
			while (fgets(intervalbuf, sizeof(intervalbuf),
				     stdin)) {
				if (strncmp(intervalbuf, "nuttcp-r", 8) == 0) {
					if (brief <= 0) {
						if (strlen(intervalbuf) +
							strlen(ident) >
							  sizeof(srvrbuf) -
							    strlen(srvrbuf) - 1)
							break;
						if (*ident) {
							cp1 = srvrbuf;
							strcat(cp1, "nuttcp-r");
							cp1 += 8;
							strcat(cp1, ident);
							cp1 += strlen(ident);
							strcat(cp1,
							       intervalbuf + 8);
						}
						else
							strcat(srvrbuf,
							       intervalbuf);
					}
					continue;
				}
				if (strncmp(intervalbuf, "DONE", 4) == 0)
					break;
				if (*ident)
					fprintf(stdout, "%s: ", ident + 1);
				fputs(intervalbuf, stdout);
				fflush(stdout);
			}
			sleep(INT_MAX);
			if (brief <= 0) {
				fputs(srvrbuf, stdout);
			}
			exit(0);
		}
	}

	if (traceroute && clientserver) {
		fflush(stdout);
		fflush(stderr);
		if (!skip_data) {
			for ( stream_idx = 1; stream_idx <= nstream;
					      stream_idx++ )
				close(fd[stream_idx]);
		}
		if (client) {
			if ((pid = fork()) == (pid_t)-1)
				err("can't fork");
			if (pid == 0) {
				while (fgets(intervalbuf, sizeof(intervalbuf),
					     stdin) && !intr) {
					if (strncmp(intervalbuf, "DONE", 4)
							== 0)
						exit(0);
					fputs(intervalbuf, stdout);
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
			execlp("/usr/local/bin/traceroute", "traceroute",
				host, NULL);
			if (errno == ENOENT)
				execlp("traceroute", "traceroute", host, NULL);
			perror("failed to execute traceroute");
			fflush(stdout);
			fflush(stderr);
			if (!inetd)
				exit(0);
			goto cleanup;
		}
	}

	prep_timer();
	errno = 0;
	stream_idx = 0;
	if (sinkmode) {      
		register int cnt;
		if (trans)  {
			if(udp) {
				strcpy(buf, "BOD0");
				(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr start */
				prep_timer();
			}
/*			beginnings of timestamps - not ready for prime time */
/*			bzero(buf + 8, 4);				*/
/*			bzero(buf + 12, 4);				*/
			nbytes += buflen;
			if (udplossinfo)
				bcopy(&nbytes, buf + 24, 8);
			while (nbuf-- && ((cnt = Nwrite(fd[stream_idx + 1],buf,buflen)) == buflen) && !intr) {
				nbytes += buflen;
				if (udplossinfo)
					bcopy(&nbytes, buf + 24, 8);
				stream_idx = ++stream_idx % nstream;
			}
			nbytes -= buflen;
			if (intr)
				nbytes += cnt;
			if(udp) {
				strcpy(buf, "EOD0");
				(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr end */
			}
		} else {
			if (udp) {
			    bzero(buf + 24, 8);
			    ntbytesc = 0;
			    first_read = 1;
			    need_swap = 0;
			    while (((cnt=Nread(fd[stream_idx + 1],buf,buflen)) > 0) && !intr)  {
				    if( cnt <= 4 ) {
					    if (strncmp(buf, "EOD", 3) == 0) {
						    correction = buf[3] - '0';
						    break;	/* "EOF" */
					    }
					    if (strncmp(buf, "BOD", 3) == 0) {
						    if (interval)
							setitimer(ITIMER_REAL,
								  &itimer, 0);
						    prep_timer();
						    got_begin = 1;
						    continue;
					    }
					    break;
				    }
				    if (!got_begin)
					    continue;
				    nbytes += cnt;
				    /* problematic if the interval timer
				     * goes off right here */
				    if (udplossinfo) {
					    if (first_read) {
						    bcopy(buf + 24, &ntbytesc,
								8);
						    first_read = 0;
						    if (ntbytesc > 0x100000000)
							    need_swap = 1;
						    if (!need_swap)
							    continue;
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
				    stream_idx = ++stream_idx % nstream;
			    }
			    if (intr)
				    nbytes += cnt;
			} else {
			    while (((cnt=Nread(fd[stream_idx + 1],buf,buflen)) > 0) && !intr)  {
				    nbytes += cnt;
				    stream_idx = ++stream_idx % nstream;
			    }
			    if (intr)
				    nbytes += cnt;
			}
		}
	} else {
		register int cnt;
		if (trans)  {
			while((cnt=read(0,buf,buflen)) > 0 &&
			    Nwrite(fd[stream_idx + 1],buf,cnt) == cnt) {
				nbytes += cnt;
				stream_idx = ++stream_idx % nstream;
			}
		}  else  {
			while((cnt=Nread(fd[stream_idx + 1],buf,buflen)) > 0 &&
			    write(realstdout,buf,cnt) == cnt) {
				nbytes += cnt;
				stream_idx = ++stream_idx % nstream;
			}
		}
	}
	if (errno) {
		if ((errno != EINTR) && (!clientserver || client)) err("IO");
	}
	itimer.it_value.tv_sec = 0;
	itimer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &itimer, 0);
	done++;
	(void)read_timer(stats,sizeof(stats));
	if(udp&&trans)  {
		usleep(500000);
		strcpy(buf, "EOD1");
		(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr end */
		stream_idx = ++stream_idx % nstream;
		usleep(500000);
		strcpy(buf, "EOD2");
		(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr end */
		stream_idx = ++stream_idx % nstream;
		usleep(500000);
		strcpy(buf, "EOD3");
		(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr end */
		stream_idx = ++stream_idx % nstream;
		usleep(500000);
		strcpy(buf, "EOD4");
		(void)Nwrite( fd[stream_idx + 1], buf, 4 ); /* rcvr end */
		stream_idx = ++stream_idx % nstream;
	}
	for ( stream_idx = 1; stream_idx <= nstream; stream_idx++ )
		close(fd[stream_idx]);

	if (interval && clientserver && !client && !trans) {
		fprintf(stdout, "DONE\n");
		fflush(stdout);
	}

	if( cput <= 0.0 )  cput = 0.000001;
	if( realt <= 0.0 )  realt = 0.000001;

	if (udp && !trans)
		realt -= correction * 0.5;

	sprintf(srvrbuf, "%.3f", (double)nbytes/1024/1024);
	sscanf(srvrbuf, "%lf", &MB);

	if (clientserver && client) {
		/* set stdin unbuffered just to be safe */
		setbuf(stdin, NULL);
		cp1 = srvrbuf;
		while (fgets(cp1, sizeof(srvrbuf) - (cp1 - srvrbuf), stdin)) {
			if (*(cp1 + strlen(cp1) - 1) != '\n') {
				*cp1 = '\0';
				break;
			}
			if (strstr(cp1, "real seconds")) {
				sscanf(cp1, "nuttcp-%*c: " PERF_FMT_IN,
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
					sprintf(cp2, DROP_FMT,
						(uint64_t)(((MB - srvr_MB)
							*1024*1024)
								/buflen + 0.5),
						(uint64_t)((MB*1024*1024)
							/buflen + 0.5));
					cp2 += strlen(cp2);
					sprintf(cp2, LOSS_FMT,
						((1 - srvr_MB/MB)*100));
					cp2 += strlen(cp2);
					sprintf(cp2, "\n");
				}
			}
			else if (strstr(cp1, "sys")) {
				if (sscanf(cp1,
					   "nuttcp-%*c: " CPU_STATS_FMT_IN2,
					   &srvr_cpu_util) != 7)
					sscanf(cp1,
					       "nuttcp-%*c: " CPU_STATS_FMT_IN,
					       &srvr_cpu_util);
			}
			else if ((strstr(cp1, "KB/cpu")) && !verbose)
				continue;
			strncpy(tmpbuf, cp1, 256);
			*(tmpbuf + 256) = '\0';
			if (strncmp(tmpbuf, "nuttcp-", 7) == 0)
				sprintf(cp1, "nuttcp-%c%s%s",
					tmpbuf[7], ident, tmpbuf + 8);
			cp1 += strlen(cp1);
		}
		got_srvr_output = 1;
	}

	if (interval && clientserver && !client && !trans) {
		if (udp)
			/* sleep longer for weird case where high rate
			 * UDP data effectively prevents interval reports
			 * on TCP control channel from being received
			 * until transmission is complete - gives time
			 * for interval reports to be output before
			 * processing final server output */
			usleep(500000);
		else
			usleep(200000);
	}

	if (brief <= 0) {
		fprintf(stdout, "nuttcp%s%s: " PERF_FMT_OUT,
			trans?"-t":"-r", ident,
			(double)nbytes/(1024*1024), realt,
			(double)nbytes/realt/1024,
			(double)nbytes/realt/125000 );
		if (clientserver && client && !trans && udp) {
			fprintf(stdout, "nuttcp-r%s:", ident);
			fprintf(stdout, DROP_FMT,
				(uint64_t)(((srvr_MB - MB)*1024*1024)
					/buflen + 0.5),
				(uint64_t)((srvr_MB*1024*1024)/buflen + 0.5));
			fprintf(stdout, LOSS_FMT, ((1 - MB/srvr_MB)*100));
			fprintf(stdout, "\n");
		}
		if (verbose) {
			fprintf(stdout,
				"nuttcp%s%s: %.3f MB in %.2f CPU seconds = %.2f KB/cpu sec\n",
				trans?"-t":"-r", ident,
				(double)nbytes/(1024*1024), cput,
				(double)nbytes/cput/1024 );
		}
		fprintf(stdout,
			"nuttcp%s%s: %d I/O calls, msec/call = %.2f, calls/sec = %.2f\n",
			trans?"-t":"-r", ident,
			numCalls,
			1024.0 * realt/((double)numCalls),
			((double)numCalls)/realt);

		fprintf(stdout,"nuttcp%s%s: %s\n", trans?"-t":"-r",
			ident, stats);
	}

	if (sscanf(stats, CPU_STATS_FMT_IN2, &cpu_util) != 6)
		sscanf(stats, CPU_STATS_FMT_IN, &cpu_util);

	if (brief && clientserver && client) {
		if (brief < 0)
			fprintf(stdout, "\n");
		if (udp) {
			if (trans) {
				if (*ident)
					fprintf(stdout, "%s: ", ident + 1);
				fprintf(stdout, PERF_FMT_BRIEF,
					srvr_MB, srvr_realt, srvr_Mbps,
					cpu_util, srvr_cpu_util);
				if (!(format & NODROPS))
					fprintf(stdout, DROP_FMT_BRIEF,
						(uint64_t)(((MB - srvr_MB)
							*1024*1024)
								/buflen + 0.5),
						(uint64_t)((MB*1024*1024)
							/buflen + 0.5));
				if (!(format & NOPERCENTLOSS))
					fprintf(stdout, LOSS_FMT_BRIEF,
						(1 - srvr_MB/MB)*100);
				if (format & XMITSTATS)
					fprintf(stdout, PERF_FMT_BRIEF3, MB);
			}
			else {
				if (*ident)
					fprintf(stdout, "%s: ", ident + 1);
				fprintf(stdout, PERF_FMT_BRIEF,
					MB, realt, (double)nbytes/realt/125000,
					srvr_cpu_util, cpu_util);
				if (!(format & NODROPS))
					fprintf(stdout, DROP_FMT_BRIEF,
						(uint64_t)(((srvr_MB - MB)
							*1024*1024)
								/buflen + 0.5),
						(uint64_t)((srvr_MB*1024*1024)
							/buflen + 0.5));
				if (!(format & NOPERCENTLOSS))
					fprintf(stdout, LOSS_FMT_BRIEF,
						(1 - MB/srvr_MB)*100);
				if (format & XMITSTATS)
					fprintf(stdout, PERF_FMT_BRIEF3,
						srvr_MB);
			}
			fprintf(stdout, "\n");
		}
		else
			if (trans) {
				if (*ident)
					fprintf(stdout, "%s: ", ident + 1);
				fprintf(stdout, PERF_FMT_BRIEF "\n",
					srvr_MB, srvr_realt, srvr_Mbps,
					cpu_util, srvr_cpu_util );
			}
			else {
				if (*ident)
					fprintf(stdout, "%s: ", ident + 1);
				fprintf(stdout, PERF_FMT_BRIEF "\n",
					MB, realt, (double)nbytes/realt/125000,
					srvr_cpu_util, cpu_util );
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
			if (brief <= 0)
				fputs("\n", stdout);
			if (interval && trans) {
				kill(pid, SIGTERM);
				while ((wait_pid = wait(&pidstat)) != pid) {
					if (wait_pid == (pid_t)-1) {
						if (errno == ECHILD)
							break;
						err("wait failed");
					}
				}
			}
			if (brief <= 0) {
				if (got_srvr_output) {
					fputs(srvrbuf, stdout);
				}
			}
			else {
				while (fgets(buf, mallocsize, stdin))
					fputs(buf, stdout);
			}
			fflush(stdout);
			close(0);
		}
		else {
			fflush(stdout);
			close(1);
			if (!inetd) {
				dup(savestdout);
				close(savestdout);
			}
		}
		fclose(ctlconn);
		if (!inetd)
			close(fd[0]);
	}
	if (clientserver && !client) {
		itimer.it_value.tv_sec = 0;
		itimer.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &itimer, 0);
		signal(SIGALRM, SIG_DFL);
		bzero((char *)&frominet, sizeof(frominet));
		bzero((char *)&clientaddr, sizeof(clientaddr));
		cput = 0.000001;
		realt = 0.000001;
		nbytes = 0;
		ntbytes = 0;
		ntbytesc = 0;
		numCalls = 0;
		buflen = 64 * 1024;
		if (udp) buflen = DEFAULTUDPBUFLEN;
		nbuf = 0;
		sendwin = origsendwin;
		rcvwin = origrcvwin;
		nstream = 1;
		b_flag = 0;
		rate = ~0;
		irate = 0;
		timeout = 0.0;
		interval = 0.0;
		pbytes = 0;
		ptbytes = 0;
		ident = "";
		intr = 0;
		abortconn = 0;
		port = 5001;
		trans = 0;
		braindead = 0;
		udp = 0;
		udplossinfo = 0;
		got_srvr_output = 0;
		reverse = 0;
		format = 0;
		traceroute = 0;
		skip_data = 0;
		priority = 0;
		brief = 0;
		done = 0;
		got_begin = 0;
		if (!oneshot)
			goto doit;
	}
	if (!sinkmode && !trans)
		close(realstdout);
	exit(0);

usage:
	fprintf(stderr,Usage);
	exit(1);
}

void
err(s)
char *s;
{
	fprintf(stderr,"nuttcp%s%s: ", trans?"-t":"-r", ident);
	perror(s);
	fprintf(stderr,"errno=%d\n",errno);
	exit(1);
}

void
mes(s)
char *s;
{
	fprintf(stdout,"nuttcp%s%s: v%d.%d.%d: %s\n", trans?"-t":"-r", ident,
			vers_major, vers_minor, vers_delta, s);
}

void
pattern( cp, cnt )
register char *cp;
register int cnt;
{
	register char c;
	c = 0;
	while( cnt-- > 0 )  {
		while( !isprint((c&0x7F)) )  c++;
		*cp++ = (c++&0x7F);
	}
}


#if defined(SYSV)
static void
getrusage(ignored, ru)
    int ignored;
    register struct rusage *ru;
{
    struct tms buf;

    times(&buf);

    ru->ru_stime.tv_sec  = buf.tms_stime / HZ;
    ru->ru_stime.tv_usec = (buf.tms_stime % HZ) * (1000000/HZ);
    ru->ru_utime.tv_sec  = buf.tms_utime / HZ;
    ru->ru_utime.tv_usec = (buf.tms_utime % HZ) * (1000000/HZ);
}

#ifndef sgi	/* it's a real system call */
/*ARGSUSED*/
static 
gettimeofday(tp, zp)
    struct timeval *tp;
    struct timezone *zp;
{
    tp->tv_sec = time(0);
    tp->tv_usec = 0;
}
#endif
#endif /* SYSV */

/*
 *			P R E P _ T I M E R
 */
void
prep_timer()
{
	gettimeofday(&time0, (struct timezone *)0);
	timep.tv_sec = time0.tv_sec;
	timep.tv_usec = time0.tv_usec;
	getrusage(RUSAGE_SELF, &ru0);
}

/*
 *			R E A D _ T I M E R
 * 
 */
double
read_timer(str,len)
char *str;
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
prusage(r0, r1, e, b, outp)
	register struct rusage *r0, *r1;
	struct timeval *e, *b;
	char *outp;
{
	struct timeval tdiff;
	register time_t t;
	register char *cp;
#if !defined(SYSV)
	register int i;
#endif
	int ms;

	t = (r1->ru_utime.tv_sec-r0->ru_utime.tv_sec)*100+
	    (r1->ru_utime.tv_usec-r0->ru_utime.tv_usec)/10000+
	    (r1->ru_stime.tv_sec-r0->ru_stime.tv_sec)*100+
	    (r1->ru_stime.tv_usec-r0->ru_stime.tv_usec)/10000;
	ms =  (e->tv_sec-b->tv_sec)*100 + (e->tv_usec-b->tv_usec)/10000;

#define END(x)	{while(*x) x++;}
#if defined(SYSV)
	cp = "%Uuser %Ssys %Ereal %P";
#else
	cp = "%Uuser %Ssys %Ereal %P %Xi+%Dd %Mmaxrss %F+%Rpf %Ccsw";
#endif
	for (; *cp; cp++)  {
		if (*cp != '%')
			*outp++ = *cp;
		else if (cp[1]) switch(*++cp) {

		case 'U':
			tvsub(&tdiff, &r1->ru_utime, &r0->ru_utime);
			sprintf(outp,"%d.%01d", tdiff.tv_sec, tdiff.tv_usec/100000);
			END(outp);
			break;

		case 'S':
			tvsub(&tdiff, &r1->ru_stime, &r0->ru_stime);
			sprintf(outp,"%d.%01d", tdiff.tv_sec, tdiff.tv_usec/100000);
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

#if !defined(SYSV)
		case 'W':
			i = r1->ru_nswap - r0->ru_nswap;
			sprintf(outp,"%d", i);
			END(outp);
			break;

		case 'X':
			sprintf(outp,"%d", t == 0 ? 0 : (r1->ru_ixrss-r0->ru_ixrss)/t);
			END(outp);
			break;

		case 'D':
			sprintf(outp,"%d", t == 0 ? 0 :
			    (r1->ru_idrss+r1->ru_isrss-(r0->ru_idrss+r0->ru_isrss))/t);
			END(outp);
			break;

		case 'K':
			sprintf(outp,"%d", t == 0 ? 0 :
			    ((r1->ru_ixrss+r1->ru_isrss+r1->ru_idrss) -
			    (r0->ru_ixrss+r0->ru_idrss+r0->ru_isrss))/t);
			END(outp);
			break;

		case 'M':
			sprintf(outp,"%d", r1->ru_maxrss/2);
			END(outp);
			break;

		case 'F':
			sprintf(outp,"%d", r1->ru_majflt-r0->ru_majflt);
			END(outp);
			break;

		case 'R':
			sprintf(outp,"%d", r1->ru_minflt-r0->ru_minflt);
			END(outp);
			break;

		case 'I':
			sprintf(outp,"%d", r1->ru_inblock-r0->ru_inblock);
			END(outp);
			break;

		case 'O':
			sprintf(outp,"%d", r1->ru_oublock-r0->ru_oublock);
			END(outp);
			break;
		case 'C':
			sprintf(outp,"%d+%d", r1->ru_nvcsw-r0->ru_nvcsw,
				r1->ru_nivcsw-r0->ru_nivcsw );
			END(outp);
			break;
#endif /* !SYSV */
		}
	}
	*outp = '\0';
}

static void
tvadd(tsum, t0, t1)
	struct timeval *tsum, *t0, *t1;
{

	tsum->tv_sec = t0->tv_sec + t1->tv_sec;
	tsum->tv_usec = t0->tv_usec + t1->tv_usec;
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
}

static void
tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

static void
psecs(l,cp)
long l;
register char *cp;
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
Nread( fd, buf, count )
	char* buf;
{
	struct sockaddr_in from;
	int len = sizeof(from);
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
	return(cnt);
}

/*
 *			N W R I T E
 */
Nwrite( fd, buf, count )
	char* buf;
{
	struct timeval timedol;
	struct timeval td;
	register int cnt;

	if (irate) {
		while (((double)count/realt/125 > rate) && !intr) {
			/* Get real time */
			gettimeofday(&timedol, (struct timezone *)0);
			tvsub( &td, &timedol, &timepk );
			realt = td.tv_sec + ((double)td.tv_usec) / 1000000;
			if( realt <= 0.0 )  realt = 0.000001;
		}
		if (intr) return(0);
		gettimeofday(&timepk, (struct timezone *)0);
		realt = 0.000001;
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
/*	beginnings of timestamps - not ready for prime time		*/
/*	gettimeofday(&timedol, (struct timezone *)0);			*/
/*	bcopy(&timedol.tv_sec, buf + 8, 4);				*/
/*	bcopy(&timedol.tv_usec, buf + 12, 4);				*/
	if( udp )  {
again:
		cnt = sendto( fd, buf, count, 0, (struct sockaddr *)&sinhim[stream_idx + 1], sizeof(sinhim[stream_idx + 1]) );
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

delay(us)
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
mread(fd, bufp, n)
int fd;
register char	*bufp;
unsigned	n;
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
