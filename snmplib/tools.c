/*
 * tools.c
 */

#include <config.h>

#include <ctype.h>
#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "asn1.h" 
#include "snmp_api.h"
#include "snmp_debug.h"
#include "tools.h" 
#include "scapi.h" 


/*******************************************************************-o-******
 * free_zero
 *
 * Parameters:
 *	*buf	Pointer at bytes to free.
 *	size	Number of bytes in buf.
 */
void
free_zero(void *buf, size_t size)
{
	if (buf) {
		memset(buf, 0, size);
		free(buf);
	}

}  /* end free_zero() */




/*******************************************************************-o-******
 * malloc_random
 *
 * Parameters:
 *	size	Number of bytes to malloc() and fill with random bytes.
 *      
 * Returns pointer to allocaed & set buffer on success, size contains
 * number of random bytes filled.
 *
 * buf is NULL and *size set to KMT error value upon failure.
 *
 * (Degenerates to malloc_zero if HAVE_LIBKMT is not defined.)
 */
u_char *
malloc_random(size_t *size)
{
	int	 rval	= SNMPERR_SUCCESS;
	u_char	*buf	= (u_char *) malloc_zero(*size);

#if defined(HAVE_LIBKMT) || defined(USE_INTERNAL_MD5)
	if (buf) {
		rval = sc_random(buf, size);

		if (rval < 0) {
			free_zero(buf, *size);
			buf = NULL;
		} else {
			*size = rval;
		}
	}
#endif	/* HAVE_LIBKMT || USE_INTERNAL_MD5 */

	return buf;

}  /* end malloc_random() */




/*******************************************************************-o-******
 * malloc_zero
 *
 * Parameters:
 *	size	Number of bytes to malloc().
 *      
 * Returns pointer to allocaed & zeroed buffer on success.
 */
u_char *
malloc_zero(size_t size)
{
	u_char	*buf = (u_char *) malloc(size);

	if (buf) {
		memset(buf, 0, size);
	}

	return buf;

}  /* end malloc_zero() */



/*******************************************************************-o-******
 * memdup
 *
 * Parameters:
 *	to       Pointer to allocate and copy memory to.
 *      from     Pointer to copy memory from.
 *      size     Size of the data to be copied.
 *      
 * Returns
 *	SNMPERR_SUCCESS	On success.
 *      SNMPERR_GENERR	On failure.
 */
int
memdup(u_char **to, const u_char *from, size_t size)
{
  if (to == NULL)
    return SNMPERR_GENERR;
  if (from == NULL) {
    *to = NULL;
    return SNMPERR_SUCCESS;
  }
  if ((*to = malloc(size)) == NULL)
    return SNMPERR_GENERR;
  memcpy(*to, from, size);
  return SNMPERR_SUCCESS;

}  /* end memdup() */




/*******************************************************************-o-******
 * binary_to_hex
 *
 * Parameters:
 *	*input		Binary data.
 *	len		Length of binary data.
 *	**output	NULL terminated string equivalent in hex.
 *      
 * Returns:
 *	olen	Length of output string not including NULL terminator.
 *
 * FIX	Is there already one of these in the UCD SNMP codebase?
 *	The old one should be used, or this one should be moved to
 *	snmplib/snmp_api.c.
 */
u_int
binary_to_hex(const u_char *input, size_t len, char **output)
{
	u_int	olen	= (len * 2) + 1;
	char	*s	= (char *) SNMP_MALLOC(olen),
		*op	= s;
	const u_char *ip	= input;


	while (ip-input < (int)len) {
		*op++ = VAL2HEX( (*ip >> 4) & 0xf );
		*op++ = VAL2HEX( *ip & 0xf );
		ip++;
	}
	*op = '\0';
	
	*output = s;
	return olen;

}  /* end binary_to_hex() */




/*******************************************************************-o-******
 * hex_to_binary2
 *
 * Parameters:
 *	*input		Printable data in base16.
 *	len		Length in bytes of data.
 *	**output	Binary data equivalent to input.
 *      
 * Returns:
 *	SNMPERR_GENERR	Failure.
 *	<len>		Otherwise, Length of allocated string.
 *
 *
 * Input of an odd length is right aligned.
 *
 * FIX	Another version of "hex-to-binary" which takes odd length input
 *	strings.  It also allocates the memory to hold the binary data.
 *	Should be integrated with the official hex_to_binary() function.
 */
int
hex_to_binary2(const u_char *input, size_t len, char **output)
{
	u_int	olen	= (len/2) + (len%2);
	char	*s	= (char *) malloc_zero(olen),
		*op	= s;
	const u_char *ip	= input;


	*output = NULL;
	*op = 0;
	if (len%2) {
		if(!isxdigit(*ip)) goto hex_to_binary2_quit;
		*op++ = HEX2VAL( *ip );		ip++;
	}

	while (ip-input < (int)len) {
		if(!isxdigit(*ip)) goto hex_to_binary2_quit;
		*op = HEX2VAL( *ip ) << 4;	ip++;

		if(!isxdigit(*ip)) goto hex_to_binary2_quit;
		*op++ += HEX2VAL( *ip );	ip++;
	}

	*output = s;	
	return olen;

hex_to_binary2_quit:
	free_zero(s, olen);
	return -1;

}  /* end hex_to_binary2() */




/*******************************************************************-o-******
 * dump_chunk
 *
 * Parameters:
 *	*title	(May be NULL.)
 *	*buf
 *	 size
 */
void
dump_chunk(const char *debugtoken, const char *title, const u_char *buf, int size)
{
	u_int		printunit = 64;		/* XXX  Make global. */
	char		chunk[SNMP_MAXBUF],
			*s, *sp;

	if ( title && (*title != '\0') ) {
          DEBUGMSGTL((debugtoken, "%s\n", title));
	}


	memset(chunk, 0, SNMP_MAXBUF);
	size = binary_to_hex(buf, size, &s);
	sp   = s;

	while (size > 0)
	{
		if (size > printunit) {
			strncpy(chunk, sp, printunit);	
			chunk[printunit] = '\0';
			DEBUGMSGTL((debugtoken, "\t%s\n", chunk));
		} else {
			DEBUGMSGTL((debugtoken, "\t%s\n", sp));
		}

		sp	+= printunit;
		size	-= printunit;
	}


	SNMP_FREE(s);

}  /* end dump_chunk() */




/*******************************************************************-o-******
 * dump_snmpEngineID
 *
 * Parameters:
 *	*estring
 *	*estring_len
 *      
 * Returns:
 *	Allocated memory pointing to a string of buflen char representing
 *	a printf'able form of the snmpEngineID.
 *
 *	-OR- NULL on error.
 *
 *
 * Translates the snmpEngineID TC into a printable string.  From RFC 2271,
 * Section 5 (pp. 36-37):
 *
 * First bit:	0	Bit string structured by means non-SNMPv3.
 *  		1	Structure described by SNMPv3 SnmpEngineID TC.
 *  
 * Bytes 1-4:		Enterprise ID.  (High bit of first byte is ignored.)
 *  
 * Byte 5:	0	(RESERVED by IANA.)
 *  		1	IPv4 address.		(   4 octets)
 *  		2	IPv6 address.		(  16 octets)
 *  		3	MAC address.		(   6 octets)
 *  		4	Locally defined text.	(0-27 octets)
 *  		5	Locally defined octets.	(0-27 octets)
 *  		6-127	(RESERVED for enterprise.)
 *  
 * Bytes 6-32:		(Determined by byte 5.)
 *  
 *
 * Non-printable characters are given in hex.  Text is given in quotes.
 * IP and MAC addresses are given in standard (UN*X) conventions.  Sections
 * are comma separated.
 *
 * esp, remaining_len and s trace the state of the constructed buffer.
 * s will be defined if there is something to return, and it will point
 * to the end of the constructed buffer.
 *
 *
 * ASSUME  "Text" means printable characters.
 *
 * XXX	Must the snmpEngineID always have a minimum length of 12?
 *	(Cf. part 2 of the TC definition.)
 * XXX	Does not enforce upper-bound of 32 bytes.
 * XXX	Need a switch to decide whether to use DNS name instead of a simple
 *	IP address.
 *
 * FIX	Use something other than sprint_hexstring which doesn't add 
 *	trailing spaces and (sometimes embedded) newlines...
 */
#ifdef SNMP_TESTING_CODE
char *
dump_snmpEngineID(u_char *estring, size_t *estring_len)
{
#define eb(b)	( *(esp+b) & 0xff )

	int		 rval		= SNMPERR_SUCCESS,
                         gotviolation	= 0,
                         slen           = 0;
	u_int	 	 remaining_len;

	char	 	 buf[SNMP_MAXBUF],
			*s = NULL,
			*t,
			*esp = estring;

	struct	in_addr	 iaddr;



	/*
	 * Sanity check.
	 */
	if ( !estring || (*estring_len <= 0) ) {
		QUITFUN(SNMPERR_GENERR, dump_snmpEngineID_quit);
	}
	remaining_len = *estring_len;
	memset(buf, 0, SNMP_MAXBUF);



	/*
	 * Test first bit.  Return immediately with a hex string, or
	 * begin by formatting the enterprise ID.
	 */
	if ( !(*esp & 0x80) ) {
		sprint_hexstring(buf, esp, remaining_len);
		s  = strchr(buf, '\0');
		s -= 1;
		goto dump_snmpEngineID_quit;
	}

	s = buf;
	s += sprintf(s, "enterprise %d, ",	((*(esp+0)&0x7f) << 24) |
						((*(esp+1)&0xff) << 16) |
						((*(esp+2)&0xff) <<  8) |
						((*(esp+3)&0xff)) );
						/* XXX  Ick. */

	if (remaining_len < 5) {	/* XXX	Violating string. */
		goto dump_snmpEngineID_quit;
	}

	esp += 4;		/* Incremented one more in the switch below. */
	remaining_len -= 5;



	/*
	 * Act on the fifth byte.
	 */
	switch ((int) *esp++) {
	case 1:					/* IPv4 address. */

		if (remaining_len < 4) goto dump_snmpEngineID_violation;
		memcpy(&iaddr.s_addr, esp, 4);

		if ( !(t = inet_ntoa(iaddr)) ) goto dump_snmpEngineID_violation;
		s += sprintf(s, "%s", t);

		esp += 4;
		remaining_len -= 4;
		break;

	case 2:					/* IPv6 address. */

		if (remaining_len < 16) goto dump_snmpEngineID_violation;

		s += sprintf(	s,
				"%02X%02X %02X%02X %02X%02X %02X%02X::"
				"%02X%02X %02X%02X %02X%02X %02X%02X",
					eb(0),  eb(1),  eb(2),  eb(3),
					eb(4),  eb(5),  eb(6),  eb(7),
					eb(8),  eb(9),  eb(10), eb(11),
					eb(12), eb(13), eb(14), eb(15) );

		esp += 16;
		remaining_len -= 16;
		break;

	case 3:					/* MAC address. */

		if (remaining_len < 6) goto dump_snmpEngineID_violation;

		s += sprintf( s, "%02X:%02X:%02X:%02X:%02X:%02X",
			eb(0), eb(1), eb(2), eb(3), eb(4), eb(5), eb(6) );

		esp += 6;
		remaining_len -= 6;
		break;

	case 4:					/* Text. */

                /* Doesn't exist on all (many) architectures */
                /* s += snprintf(s, remaining_len+3, "\"%s\"", esp); */
		s += sprintf(s, "\"%s\"", esp);
		goto dump_snmpEngineID_quit;
		break;	/*NOTREACHED*/

	case 5:					/* Octets. */

		sprint_hexstring(s, esp, remaining_len);
		s  = strchr(buf, '\0');
		s -= 1;
		goto dump_snmpEngineID_quit;
		break;	/*NOTREACHED*/


dump_snmpEngineID_violation:
	case 0:					/* Violation of RESERVED, 
						 *   -OR- of expected length.
						 */
		gotviolation = 1;
		s += sprintf(s, "!!! ");

	default:				/* Unknown encoding. */

		if ( !gotviolation ) {
			s += sprintf(s, "??? ");
		}
		sprint_hexstring(s, esp, remaining_len);
		s  = strchr(buf, '\0');
		s -= 1;

		goto dump_snmpEngineID_quit;

	}  /* endswitch */



	/*
	 * Cases 1-3 (IP and MAC addresses) should not have trailing
	 * octets, but perhaps they do.  Throw them in too.  XXX
	 */
	if (remaining_len > 0) {
		s += sprintf(s, " (??? ");

		sprint_hexstring(s, esp, remaining_len);
		s  = strchr(buf, '\0');
		s -= 1;

		s += sprintf(s, ")");
	}



dump_snmpEngineID_quit:
	if (s) {
                slen = s-buf+1;
		s = SNMP_MALLOC(slen);
		memcpy(s, buf, (slen)-1);
	}

	memset(buf, 0, SNMP_MAXBUF);	/* XXX -- Overkill? XXX: Yes! */

	return s;

#undef eb
}  /* end dump_snmpEngineID() */
#endif /* SNMP_TESTING_CODE */

