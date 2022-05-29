#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <time.h>
#include <sys/time.h>

#include <debug.h>
#include <math.h>

#include <libgctools.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static void *xfb = NULL;

// #define BBA_DEBUG
#define TRACE_PORT 10000
#define TRACE_IP "192.168.1.53"

extern const uint8_t script_lua[];

// Callbacks
static void return_to_loader (void) {
	printf("Return to loader\n");
	close_bba_logging();

  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

static void reset_cb(u32 irq, void* ctx) {
	printf("Reset button pushed with IRQ %d\n!", irq);
	return_to_loader();
}

lua_State * LUA_Init() {

	lua_State * L;
	L = luaL_newstate();

	// Init Lua state with default libraries
	luaL_openlibs(L);

	return L;
}

static int average(lua_State *L)
{
	/* get number of arguments */
	int n = lua_gettop(L);
	double sum = 0;
	int i;

	/* loop through each argument */
	for (i = 1; i <= n; i++)
	{
		/* total the arguments */
		sum += lua_tonumber(L, i);
	}

	/* push the average */
	lua_pushnumber(L, sum / n);

	/* push the sum */
	lua_pushnumber(L, sum);

	/* return the number of results */
	return 2;
}

int main(int argc, char *argv[]) {

	int is_connected = 0;
	lua_State * L;
	GXRModeObj *rmode;

	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);

	PAD_Init();

	L = LUA_Init();

	// be sure we're going back to the gclink loader
	SYS_SetResetCallback(reset_cb);

	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	console_init(xfb, 20, 20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*2);
	printf("\n\nTesting Lua JIT API\n");

#ifdef BBA_DEBUG
	if(argc != 3) {
		printf("No IP configuration received as arg, using DHCP");
		argv = NULL;
	}

	is_connected = setup_bba_logging(TRACE_PORT, TRACE_IP, KPRINTF, true, argv);
	if(is_connected)
		kprintf("BBA Traces enabled\n");
#endif

	while(1) {

		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);

		if (buttonsDown & PAD_BUTTON_A) {

			double sum;

			// load script from buffer
			int status = luaL_loadstring(L, (const char *)script_lua);
			if(status) {
				kprintf("Cannot load script from buffer: %s\n", lua_tostring(L, -1));
				break;
			}

			/* register our function */
			lua_register(L, "average", average);

			lua_newtable(L);
			for (int i = 1; i <= 5; i++) {
				lua_pushnumber(L, i);   /* Push the table index */
				lua_pushnumber(L, i*2); /* Push the cell value */
				lua_rawset(L, -3);      /* Stores the pair in the table */
			}
			lua_setglobal(L, "foo");

			int result = lua_pcall(L, 0, LUA_MULTRET, 0);
			if (result) {
				kprintf("Failed to run script: %s\n", lua_tostring(L, -1));
				break;
			}

			sum = lua_tonumber(L, -1);
			printf("Script returned: %.0f\n", sum);

			lua_pop(L, 1);  /* Take the returned value out of the stack */
		}

		if (buttonsDown & PAD_BUTTON_START) {
			lua_close(L);   /* Cya, Lua */
			exit(0);
		}

	}
	printf("Bye bye!)\n");

	return 0;
}