// eLua shell

#ifndef __SHELL_H__
#define __SHELL_H__

#define SHELL_WELCOMEMSG    "\nRuuviTracker: http://ruuvitracker.fi\n"
#define SHELL_PROMPT        "tracker $ "
#define SHELL_ERRMSG        "That's not going to do it. See `help`.\n"
#define SHELL_MAXSIZE       50
#define SHELL_MAX_LUA_ARGS  8

int shell_init();
void shell_start();

#endif // #ifndef __SHELL_H__
