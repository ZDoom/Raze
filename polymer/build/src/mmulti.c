#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock.h>
#else
#include <unistd.h>
#include <netinet/in.h>
#ifndef __BEOS__
#include <arpa/inet.h>
#endif
#ifdef __sun
#include <sys/filio.h>
#endif
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#define SOCKET int
#define INVALID_HANDLE_VALUE (-1)
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket close
#define ioctlsocket ioctl
#define LPHOSTENT struct hostent *

#include <sys/time.h>
static long GetTickCount(void)
{
	struct timeval tv;
	long ti;
	if (gettimeofday(&tv,NULL) < 0) return 0;
	// tv is sec.usec, GTC gives msec
	ti = tv.tv_sec * 1000;
	ti += tv.tv_usec / 1000;
	return ti;
}
#endif

#ifdef KSFORBUILD
# include "baselayer.h"
# define printf initprintf
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#define MAXPLAYERS 16
#define MAXPAKSIZ 256 //576


#define PAKRATE 40   //Packet rate/sec limit ... necessary?
#define SIMMIS 0     //Release:0  Test:100 Packets per 256 missed.
#define SIMLAG 0     //Release:0  Test: 10 Packets to delay receipt
static long simlagcnt[MAXPLAYERS];
static char simlagfif[MAXPLAYERS][SIMLAG+1][MAXPAKSIZ+2];
#if ((SIMMIS != 0) || (SIMLAG != 0))
#pragma message("\n\nWARNING! INTENTIONAL PACKET LOSS SIMULATION IS ENABLED!\nREMEMBER TO CHANGE SIMMIS&SIMLAG to 0 before RELEASE!\n\n")
#endif

long myconnectindex, numplayers;
long connecthead, connectpoint2[MAXPLAYERS];

static long tims, lastsendtims[MAXPLAYERS];
static char pakbuf[MAXPAKSIZ];

#define FIFSIZ 512 //16384/40 = 6min:49sec
static long ipak[MAXPLAYERS][FIFSIZ], icnt0[MAXPLAYERS];
static long opak[MAXPLAYERS][FIFSIZ], ocnt0[MAXPLAYERS], ocnt1[MAXPLAYERS];
static char pakmem[4194304]; static long pakmemi = 1;

#define NETPORT 0x5bd9
static SOCKET mysock;
static long myip, myport = NETPORT, otherip[MAXPLAYERS], otherport[MAXPLAYERS];
static long snatchip = 0, snatchport = 0, danetmode = 255, netready = 0;

void netuninit ()
{
	if (mysock != (SOCKET)INVALID_HANDLE_VALUE) closesocket(mysock);
#ifdef _WIN32
	WSACleanup();
#endif
}

long netinit (long portnum)
{
	LPHOSTENT lpHostEnt;
	char hostnam[256];
	struct sockaddr_in ip;
	long i;

#ifdef _WIN32
	WSADATA ws;

	if (WSAStartup(0x101,&ws) == SOCKET_ERROR) return(0);
#endif

	mysock = socket(AF_INET,SOCK_DGRAM,0); if (mysock == INVALID_SOCKET) return(0);
#ifdef __BEOS__
	i = 1; if (setsockopt(mysock,SOL_SOCKET,SO_NONBLOCK,&i,sizeof(i)) < 0) return(0);
#else
	i = 1; if (ioctlsocket(mysock,FIONBIO,(unsigned long *)&i) == SOCKET_ERROR) return(0);
#endif

	ip.sin_family = AF_INET;
	ip.sin_addr.s_addr = INADDR_ANY;
	ip.sin_port = htons(portnum);
	if (bind(mysock,(struct sockaddr *)&ip,sizeof(ip)) != SOCKET_ERROR)
	{
		myport = portnum;
		if (gethostname(hostnam,sizeof(hostnam)) != SOCKET_ERROR)
			if ((lpHostEnt = gethostbyname(hostnam)))
			{
				myip = ip.sin_addr.s_addr = *(long *)lpHostEnt->h_addr;
				printf("mmulti: This machine's IP is %s\n", inet_ntoa(ip.sin_addr));
			}
		return(1);
	}
	return(0);
}

long netsend (long other, char *dabuf, long bufsiz) //0:buffer full... can't send
{
	struct sockaddr_in ip;

	if (!otherip[other]) return(0);
	ip.sin_family = AF_INET;
	ip.sin_addr.s_addr = otherip[other];
	ip.sin_port = otherport[other];
	return(sendto(mysock,dabuf,bufsiz,0,(struct sockaddr *)&ip,sizeof(struct sockaddr_in)) != SOCKET_ERROR);
}

long netread (long *other, char *dabuf, long bufsiz) //0:no packets in buffer
{
	struct sockaddr_in ip;
	long i;

	i = sizeof(ip);
	if (recvfrom(mysock,dabuf,bufsiz,0,(struct sockaddr *)&ip,(int *)&i) == -1) return(0);
#if (SIMMIS > 0)
	if ((rand()&255) < SIMMIS) return(0);
#endif

	snatchip = (long)ip.sin_addr.s_addr; snatchport = (long)ip.sin_port;

	(*other) = myconnectindex;
	for(i=0;i<MAXPLAYERS;i++)
		if ((otherip[i] == snatchip) && (otherport[i] == snatchport))
			{ (*other) = i; break; }
#if (SIMLAG > 1)
	i = simlagcnt[*other]%(SIMLAG+1);
	*(short *)&simlagfif[*other][i][0] = bufsiz; memcpy(&simlagfif[*other][i][2],dabuf,bufsiz);
	simlagcnt[*other]++; if (simlagcnt[*other] < SIMLAG+1) return(0);
	i = simlagcnt[*other]%(SIMLAG+1);
	bufsiz = *(short *)&simlagfif[*other][i][0]; memcpy(dabuf,&simlagfif[*other][i][2],bufsiz);
#endif

	return(1);
}

long isvalidipaddress (char *st)
{
	long i, bcnt, num;

	bcnt = 0; num = 0;
	for(i=0;st[i];i++)
	{
		if (st[i] == '.') { bcnt++; num = 0; continue; }
		if (st[i] == ':')
		{
			if (bcnt != 3) return(0);
			num = 0;
			for(i++;st[i];i++)
			{
				if ((st[i] >= '0') && (st[i] <= '9'))
					{ num = num*10+st[i]-'0'; if (num >= 65536) return(0); }
				else return(0);
			}
			return(1);
		}
		if ((st[i] >= '0') && (st[i] <= '9'))
			{ num = num*10+st[i]-'0'; if (num >= 256) return(0); }

	}
	return(bcnt == 3);
}

//---------------------------------- Obsolete variables&functions ----------------------------------
char syncstate = 0;
void setpackettimeout (long datimeoutcount, long daresendagaincount) {}
void genericmultifunction (long other, char *bufptr, long messleng, long command) {}
long getoutputcirclesize () { return(0); }
void setsocket (long newsocket) { }
void flushpackets () {}
void sendlogon () {}
void sendlogoff () {}
//--------------------------------------------------------------------------------------------------

static long crctab16[256];
static void initcrc16 ()
{
	long i, j, k, a;
	for(j=0;j<256;j++)
	{
		for(i=7,k=(j<<8),a=0;i>=0;i--,k=((k<<1)&65535))
		{
			if ((k^a)&0x8000) a = ((a<<1)&65535)^0x1021;
							 else a = ((a<<1)&65535);
		}
		crctab16[j] = (a&65535);
	}
}
#define updatecrc16(crc,dat) crc = (((crc<<8)&65535)^crctab16[((((unsigned short)crc)>>8)&65535)^dat])
static unsigned short getcrc16 (char *buffer, long bufleng)
{
	long i, j;

	j = 0;
	for(i=bufleng-1;i>=0;i--) updatecrc16(j,buffer[i]);
	return((unsigned short)(j&65535));
}

void uninitmultiplayers () { netuninit(); }

long getpacket(long *, char *);
static void initmultiplayers_reset(void)
{
	long i;

	initcrc16();
	memset(icnt0,0,sizeof(icnt0));
	memset(ocnt0,0,sizeof(ocnt0));
	memset(ocnt1,0,sizeof(ocnt1));
	memset(ipak,0,sizeof(ipak));
	//memset(opak,0,sizeof(opak)); //Don't need to init opak
	//memset(pakmem,0,sizeof(pakmem)); //Don't need to init pakmem
#if (SIMLAG > 1)
	memset(simlagcnt,0,sizeof(simlagcnt));
#endif

	lastsendtims[0] = GetTickCount();
	for(i=1;i<MAXPLAYERS;i++) lastsendtims[i] = lastsendtims[0];
	numplayers = 1; myconnectindex = 0;

	memset(otherip,0,sizeof(otherip));
	for(i=0;i<MAXPLAYERS;i++) otherport[i] = htons(NETPORT);
}

	// Multiplayer command line summary. Assume myconnectindex always = 0 for 192.168.1.2
	//
	// /n0 (mast/slav) 2 player:               3 player:
	// 192.168.1.2     game /n0                game /n0:3
	// 192.168.1.100   game /n0 192.168.1.2    game /n0 192.168.1.2
	// 192.168.1.4                             game /n0 192.168.1.2
	//
	// /n1 (peer-peer) 2 player:               3 player:
	// 192.168.1.2     game /n1 192.168.1.100  game /n1 192.168.1.100 192.168.1.4
	// 192.168.1.100   game 192.168.1.2 /n1    game 192.168.1.2 /n1 192.168.1.4
	// 192.168.1.4                             game 192.168.1.2 192.168.1.100 /n1
long initmultiplayersparms(long argc, char **argv)
{
	long i, j, daindex, portnum = NETPORT;
	char *st;

	initmultiplayers_reset();
	danetmode = 255; daindex = 0;

	// go looking for the port, if specified
	for (i=0;i<argc;i++) {
		if (argv[i][0] != '-' && argv[i][0] != '/') continue;
		if ((argv[i][1] == 'p' || argv[i][1] == 'P') && argv[i][2]) {
			char *p;
			j = strtol(argv[i]+2, &p, 10);
			if (!(*p) && j > 0 && j<65535) portnum = j;

			printf("mmulti: Using port %ld\n", portnum);
		}
	}

	netinit(portnum);

	for(i=0;i<argc;i++)
	{
		//if (((argv[i][0] == '/') || (argv[i][0] == '-')) &&
		//    ((argv[i][1] == 'N') || (argv[i][1] == 'n')) &&
		//    ((argv[i][2] == 'E') || (argv[i][2] == 'e')) &&
		//    ((argv[i][3] == 'T') || (argv[i][3] == 't')) &&
		//     (!argv[i][4]))
		//   { foundnet = 1; continue; }
		//if (!foundnet) continue;

		if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
			if ((argv[i][1] == 'N') || (argv[i][1] == 'n') || (argv[i][1] == 'I') || (argv[i][1] == 'i'))
			{
				numplayers = 2;
				if (argv[i][2] == '0')
				{
					danetmode = 0;
					if ((argv[i][3] == ':') && (argv[i][4] >= '0') && (argv[i][4] <= '9'))
					{
						numplayers = (argv[i][4]-'0');
						if ((argv[i][5] >= '0') && (argv[i][5] <= '9')) numplayers = numplayers*10+(argv[i][5]-'0');
						printf("mmulti: %ld-player game\n", numplayers);
					}
					printf("mmulti: Master-slave mode\n");
				}
				else if (argv[i][2] == '1')
				{
					danetmode = 1;
					myconnectindex = daindex; daindex++;
					printf("mmulti: Peer-to-peer mode\n");
				}
				continue;
			}
			else if ((argv[i][1] == 'P') || (argv[i][1] == 'p')) continue;
		}

		st = strdup(argv[i]); if (!st) break;
		if (isvalidipaddress(st))
		{
			if ((danetmode == 1) && (daindex == myconnectindex)) daindex++;
			for(j=0;st[j];j++) {
				if (st[j] == ':')
					{ otherport[daindex] = htons((unsigned short)atol(&st[j+1])); st[j] = 0; break; }
			}
			otherip[daindex] = inet_addr(st);
			printf("mmulti: Player %ld at %s:%d\n",daindex,st,ntohs(otherport[daindex]));
			daindex++;
		}
		else
		{
			LPHOSTENT lph;
			unsigned short pt = htons(NETPORT);

			for(j=0;st[j];j++)
				if (st[j] == ':')
					{ pt = htons((unsigned short)atol(&st[j+1])); st[j] = 0; break; }
			if ((lph = gethostbyname(st)))
			{
				if ((danetmode == 1) && (daindex == myconnectindex)) daindex++;
				otherip[daindex] = *(long *)lph->h_addr;
				otherport[daindex] = pt;
				printf("mmulti: Player %ld at %s:%d (%s)\n",daindex,
						inet_ntoa(*(struct in_addr *)lph->h_addr),ntohs(pt),argv[i]);
				daindex++;
			} else printf("mmulti: Failed resolving %s\n",argv[i]);
		}
		free(st);
	}
	if ((danetmode == 255) && (daindex)) { numplayers = 2; danetmode = 0; } //an IP w/o /n# defaults to /n0
	if ((numplayers >= 2) && (daindex) && (!danetmode)) myconnectindex = 1;
	if (daindex > numplayers) numplayers = daindex;

		//for(i=0;i<numplayers;i++)
		  //   printf("Player %d: %d.%d.%d.%d:%d\n",i,otherip[i]&255,(otherip[i]>>8)&255,(otherip[i]>>16)&255,((unsigned long)otherip[i])>>24,ntohs(otherport[i]));

	connecthead = 0;
	for(i=0;i<numplayers-1;i++) connectpoint2[i] = i+1;
	connectpoint2[numplayers-1] = -1;

	return (((!danetmode) && (numplayers >= 2)) || (numplayers == 2));
}

long initmultiplayerscycle(void)
{
	long i, k;

	getpacket(&i,0);

	tims = GetTickCount();
	if (myconnectindex == connecthead)
	{
		for(i=numplayers-1;i>0;i--)
			if (!otherip[i]) break;
		if (!i) {
			netready = 1;
			return 0;
		}
	}
	else
	{
		if (netready) return 0;
		if (tims < lastsendtims[connecthead]) lastsendtims[connecthead] = tims;
		if (tims >= lastsendtims[connecthead]+250) //1000/PAKRATE)
		{
			lastsendtims[connecthead] = tims;

				//   short crc16ofs;       //offset of crc16
				//   long icnt0;           //-1 (special packet for MMULTI.C's player collection)
				//   ...
				//   unsigned short crc16; //CRC16 of everything except crc16
			k = 2;
			*(long *)&pakbuf[k] = -1; k += 4;
			pakbuf[k++] = 0xaa;
			*(unsigned short *)&pakbuf[0] = (unsigned short)k;
			*(unsigned short *)&pakbuf[k] = getcrc16(pakbuf,k); k += 2;
			netsend(connecthead,pakbuf,k);
		}
	}

	return 1;
}

void initmultiplayers (long argc, char **argv, char damultioption, char dacomrateoption, char dapriority)
{
	long i, j, k, otims;

	if (initmultiplayersparms(argc,argv))
	{
#if 0
			//Console code seems to crash Win98 upon quitting game
			//it's not necessary and it's not portable anyway
		char tbuf[1024];
		unsigned long u;
		HANDLE hconsout;
		AllocConsole();
		SetConsoleTitle("Multiplayer status...");
		hconsout = GetStdHandle(STD_OUTPUT_HANDLE);
		otims = 0;
#endif
		while (initmultiplayerscycle())
		{
#if 0
			if ((tims < otims) || (tims > otims+100))
			{
				otims = tims;
				sprintf(tbuf,"\rWait for players (%d/%d): ",myconnectindex,numplayers);
				for(i=0;i<numplayers;i++)
				{
					if (i == myconnectindex) { strcat(tbuf,"<me> "); continue; }
					if (!otherip[i]) { strcat(tbuf,"?.?.?.?:? "); continue; }
					sprintf(&tbuf[strlen(tbuf)],"%d.%d.%d.%d:%04x ",otherip[i]&255,(otherip[i]>>8)&255,(otherip[i]>>16)&255,(((unsigned long)otherip[i])>>24),otherport[i]);
				}
				WriteConsole(hconsout,tbuf,strlen(tbuf),&u,0);
			}
		}
		FreeConsole();
#else
		}
#endif
	}
	netready = 1;
}

void dosendpackets (long other)
{
	long i, j, k;

	if (!otherip[other]) return;

		//Packet format:
		//   short crc16ofs;       //offset of crc16
		//   long icnt0;           //earliest unacked packet
		//   char ibits[32];       //ack status of packets icnt0<=i<icnt0+256
		//   while (short leng)    //leng: !=0 for packet, 0 for no more packets
		//   {
		//      long ocnt;         //index of following packet data
		//      char pak[leng];    //actual packet data :)
		//   }
		//   unsigned short crc16; //CRC16 of everything except crc16


	tims = GetTickCount();
	if (tims < lastsendtims[other]) lastsendtims[other] = tims;
	if (tims < lastsendtims[other]+1000/PAKRATE) return;
	lastsendtims[other] = tims;

	k = 2;
	*(long *)&pakbuf[k] = icnt0[other]; k += 4;
	memset(&pakbuf[k],0,32);
	for(i=icnt0[other];i<icnt0[other]+256;i++)
		if (ipak[other][i&(FIFSIZ-1)])
			pakbuf[((i-icnt0[other])>>3)+k] |= (1<<((i-icnt0[other])&7));
	k += 32;

	while ((ocnt0[other] < ocnt1[other]) && (!opak[other][ocnt0[other]&(FIFSIZ-1)])) ocnt0[other]++;
	for(i=ocnt0[other];i<ocnt1[other];i++)
	{
		j = *(short *)&pakmem[opak[other][i&(FIFSIZ-1)]]; if (!j) continue; //packet already acked
		if (k+6+j+4 > (long)sizeof(pakbuf)) break;

		*(unsigned short *)&pakbuf[k] = (unsigned short)j; k += 2;
		*(long *)&pakbuf[k] = i; k += 4;
		memcpy(&pakbuf[k],&pakmem[opak[other][i&(FIFSIZ-1)]+2],j); k += j;
	}
	*(unsigned short *)&pakbuf[k] = 0; k += 2;
	*(unsigned short *)&pakbuf[0] = (unsigned short)k;
	*(unsigned short *)&pakbuf[k] = getcrc16(pakbuf,k); k += 2;

	//printf("Send: "); for(i=0;i<k;i++) printf("%02x ",pakbuf[i]); printf("\n");
	netsend(other,pakbuf,k);
}

void sendpacket (long other, char *bufptr, long messleng)
{
	long i, j;

	if (numplayers < 2) return;

	if (pakmemi+messleng+2 > (long)sizeof(pakmem)) pakmemi = 1;
	opak[other][ocnt1[other]&(FIFSIZ-1)] = pakmemi;
	*(short *)&pakmem[pakmemi] = messleng;
	memcpy(&pakmem[pakmemi+2],bufptr,messleng); pakmemi += messleng+2;
	ocnt1[other]++;

	//printf("Send: "); for(i=0;i<messleng;i++) printf("%02x ",bufptr[i]); printf("\n");

	dosendpackets(other);
}

	//passing bufptr == 0 enables receive&sending raw packets but does not return any received packets
	//(used as hack for player collection)
long getpacket (long *retother, char *bufptr)
{
	long i, j, k, ic0, crc16ofs, messleng, other;

	if (numplayers < 2) return(0);

	if (netready)
	{
		for(i=connecthead;i>=0;i=connectpoint2[i])
		{
			if (i != myconnectindex) dosendpackets(i);
			if ((!danetmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
		}
	}

	while (netread(&other,pakbuf,sizeof(pakbuf)))
	{
			//Packet format:
			//   short crc16ofs;       //offset of crc16
			//   long icnt0;           //earliest unacked packet
			//   char ibits[32];       //ack status of packets icnt0<=i<icnt0+256
			//   while (short leng)    //leng: !=0 for packet, 0 for no more packets
			//   {
			//      long ocnt;         //index of following packet data
			//      char pak[leng];    //actual packet data :)
			//   }
			//   unsigned short crc16; //CRC16 of everything except crc16
		k = 0;
		crc16ofs = (long)(*(unsigned short *)&pakbuf[k]); k += 2;

		//printf("Recv: "); for(i=0;i<crc16ofs+2;i++) printf("%02x ",pakbuf[i]); printf("\n");

		if ((crc16ofs+2 <= (long)sizeof(pakbuf)) && (getcrc16(pakbuf,crc16ofs) == (*(unsigned short *)&pakbuf[crc16ofs])))
		{
			ic0 = *(long *)&pakbuf[k]; k += 4;
			if (ic0 == -1)
			{
					 //Slave sends 0xaa to Master at initmultiplayers() and waits for 0xab response
					 //Master responds to slave with 0xab whenever it receives a 0xaa - even if during game!
				if ((pakbuf[k] == 0xaa) && (myconnectindex == connecthead))
				{
					for(other=1;other<numplayers;other++)
					{
							//Only send to others asking for a response
						if ((otherip[other]) && ((otherip[other] != snatchip) || (otherport[other] != snatchport))) continue;
						otherip[other] = snatchip;
						otherport[other] = snatchport;

							//   short crc16ofs;       //offset of crc16
							//   long icnt0;           //-1 (special packet for MMULTI.C's player collection)
							//   ...
							//   unsigned short crc16; //CRC16 of everything except crc16
						k = 2;
						*(long *)&pakbuf[k] = -1; k += 4;
						pakbuf[k++] = 0xab;
						pakbuf[k++] = (char)other;
						pakbuf[k++] = (char)numplayers;
						*(unsigned short *)&pakbuf[0] = (unsigned short)k;
						*(unsigned short *)&pakbuf[k] = getcrc16(pakbuf,k); k += 2;
						netsend(other,pakbuf,k);
						break;
					}
				}
				else if ((pakbuf[k] == 0xab) && (myconnectindex != connecthead))
				{
					if (((unsigned long)pakbuf[k+1] < (unsigned long)pakbuf[k+2]) &&
						 ((unsigned long)pakbuf[k+2] < (unsigned long)MAXPLAYERS))
					{
						myconnectindex = (long)pakbuf[k+1];
						numplayers = (long)pakbuf[k+2];

						connecthead = 0;
						for(i=0;i<numplayers-1;i++) connectpoint2[i] = i+1;
						connectpoint2[numplayers-1] = -1;

						otherip[connecthead] = snatchip;
						otherport[connecthead] = snatchport;
						netready = 1;
					}
				}
			}
			else
			{
				if (ocnt0[other] < ic0) ocnt0[other] = ic0;
				for(i=ic0;i<min(ic0+256,ocnt1[other]);i++)
					if (pakbuf[((i-ic0)>>3)+k]&(1<<((i-ic0)&7)))
						opak[other][i&(FIFSIZ-1)] = 0;
				k += 32;

				messleng = (long)(*(unsigned short *)&pakbuf[k]); k += 2;
				while (messleng)
				{
					j = *(long *)&pakbuf[k]; k += 4;
					if ((j >= icnt0[other]) && (!ipak[other][j&(FIFSIZ-1)]))
					{
						if (pakmemi+messleng+2 > (long)sizeof(pakmem)) pakmemi = 1;
						ipak[other][j&(FIFSIZ-1)] = pakmemi;
						*(short *)&pakmem[pakmemi] = messleng;
						memcpy(&pakmem[pakmemi+2],&pakbuf[k],messleng); pakmemi += messleng+2;
					}
					k += messleng;
					messleng = (long)(*(unsigned short *)&pakbuf[k]); k += 2;
				}
			}
		}
	}

		//Return next valid packet from any player
	if (!bufptr) return(0);
	for(i=connecthead;i>=0;i=connectpoint2[i])
	{
		if (i != myconnectindex)
		{
			j = ipak[i][icnt0[i]&(FIFSIZ-1)];
			if (j)
			{
				messleng = *(short *)&pakmem[j]; memcpy(bufptr,&pakmem[j+2],messleng);
				*retother = i; ipak[i][icnt0[i]&(FIFSIZ-1)] = 0; icnt0[i]++;
				//printf("Recv: "); for(i=0;i<messleng;i++) printf("%02x ",bufptr[i]); printf("\n");
				return(messleng);
			}
		}
		if ((!danetmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
	}

	return(0);
}
