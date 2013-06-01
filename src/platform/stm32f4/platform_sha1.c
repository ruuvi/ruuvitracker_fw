/*
 *  SHA1 functions for LUA.
 *
 * @author: Seppo Takalo
 */

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "lrotable.h"
#include "platform_conf.h"
#include "auxmods.h"
#include "sha1.h"
#include <string.h>

#define SHA1_STR_LEN	40
#define SHA1_LEN	20
#define BLOCKSIZE	64

static int sha1_gen( lua_State *L )
{
	SHA1Context sha;

	const char *str = luaL_checkstring(L,1);

	SHA1Reset(&sha);
	SHA1Input(&sha, (const unsigned char *)str, strlen(str));

	if (!SHA1Result(&sha)) {
		return 0;
	} else {
		char s[SHA1_STR_LEN];
		int i;
		for(i = 0; i < 5 ; i++) {
			sprintf(s+i*8,"%.8x", sha.Message_Digest[i]);
		}
		lua_pushlstring(L,s,SHA1_STR_LEN);
		return 1;
	}

}

/* Helper to get hash from library */
/* TODO: High&Low endianess */
static void get_hash(SHA1Context *sha, char *p)
{
	int i;
	for(i=0; i<5; i++) {
		*p++ = (sha->Message_Digest[i]>>24)&0xff;
		*p++ = (sha->Message_Digest[i]>>16)&0xff;
		*p++ = (sha->Message_Digest[i]>>8)&0xff;
		*p++ = (sha->Message_Digest[i]>>0)&0xff;
	}
}

#define IPAD 0x36
#define OPAD 0x5C

static int sha1_hmac( lua_State *L )
{
	SHA1Context sha1,sha2;
	char key[BLOCKSIZE];
	char hash[SHA1_LEN];
	int i;
	size_t len;

	const char *secret,*msg;

	luaL_checkstring(L,1);
	msg = luaL_checkstring(L,2);

	//Get length from LUA (may contain zeroes)
	secret = lua_tolstring(L,1,&len);

	//Fill with zeroes
	memset(key, 0, BLOCKSIZE);

	if (len > BLOCKSIZE) {
		//Too long key, shorten with hash
		SHA1Reset(&sha1);
		SHA1Input(&sha1, (const unsigned char *)secret, len);
		SHA1Result(&sha1);
		get_hash(&sha1, key);
	} else {
		memcpy(key, secret, len);
	}

	//XOR key with IPAD
	for (i=0; i<BLOCKSIZE; i++) {
		key[i] ^= IPAD;
	}

	//First SHA hash
	SHA1Reset(&sha1);
	SHA1Input(&sha1, (const unsigned char *)key, BLOCKSIZE);
	SHA1Input(&sha1, (const unsigned char *)msg, strlen(msg));
	SHA1Result(&sha1);
	get_hash(&sha1, hash);

	//XOR key with OPAD
	for (i=0; i<BLOCKSIZE; i++) {
		key[i] ^= IPAD ^ OPAD;
	}

	//Second hash
	SHA1Reset(&sha2);
	SHA1Input(&sha2, (const unsigned char *)key, BLOCKSIZE);
	SHA1Input(&sha2, (const unsigned char *)hash, SHA1_LEN);
	SHA1Result(&sha2);

	char s[SHA1_STR_LEN];
	for(i = 0; i < 5 ; i++) {
		sprintf(s+i*8,"%.8x", sha2.Message_Digest[i]);
	}
	lua_pushlstring(L,s,SHA1_STR_LEN);
	return 1;
}

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"

// Module function map
const LUA_REG_TYPE sha1_map[] = {
	{ LSTRKEY("gen") , LFUNCVAL(sha1_gen) },
	{ LSTRKEY("hmac"), LFUNCVAL(sha1_hmac) },
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_sha1( lua_State *L )
{
	LREGISTER( L, "sha1", sha1_map );
}
